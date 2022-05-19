/*
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright (c) 2022 Nuuday.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <sys/socket.h>
#include "RtpStreamListener.h"
#include <glog/logging.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <thread>
#include <algorithm>

#define BUF_SIZE FEIP_BUFFER_SIZE
constexpr static int MAX_RTP_BUFFER_CACHE = 5;
constexpr static int MAX_SEQNUM_DIFF_FOR_DISCONTINUITY = 10;
constexpr static int MID_SEQUENCE_NUMBER = 0x8000;
constexpr static bool disableSequencing = false;

namespace multicast {

RtpStreamListener::RtpStreamListener() : mFd(0), mExitRequested(false),
        mPHandler(nullptr), mNextSequenceNumber (-1) {

    mReaderThread = std::shared_ptr<std::thread>(
            new std::thread(&RtpStreamListener::readLoop, this));
}

int RtpStreamListener::setup(const char *group, int port, MediaSourceHandler **pHandler) {
    struct sockaddr_in addr {};
    mPHandler = pHandler;

    u_int allow = 1;

    struct ip_mreq mreq  {};

    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd < 0) {
        LOG(ERROR) << "Socket creation failed";
        goto exit;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *) &allow, sizeof(allow)) < 0) {
        LOG(ERROR) << "Socket reuse configuration failed";
        goto fail_setup;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOG(ERROR) << "Failed to bind to port:" << port;
        goto fail_setup;
    }
    LOG(INFO) << "Joining group:" << group;
    mreq.imr_multiaddr.s_addr = inet_addr(group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq)) < 0) {
        LOG(ERROR) << "Failed to add membership";
        goto fail_setup;
    }


    {
        std::lock_guard<std::mutex> lockGuard(mLock);

        if (mFd != 0) {
            close(mFd);
        }
        memcpy(&mAddr, &addr, sizeof(addr));

        mFd = fd;

    }
    return 0;

fail_setup:
    close(fd);

exit:
    return 1;
}

int RtpStreamListener::readLoop() {
    int n;
    char buf[BUF_SIZE];
    socklen_t addrlen = sizeof(mAddr);
    //FILE* f = fopen("/tmp/dump.mov", "w");

    while (mFd == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(INFO) << "Waiting for RTP open";
    }
    while (!mExitRequested) {
        {
            std::lock_guard<std::mutex> lockGuard(mLock);
            n = recvfrom(mFd.load(), buf, BUF_SIZE,0,(struct sockaddr *) &mAddr, &addrlen);
            processRtpPacket(buf, n);
        }

        if (n <= 0 ) {
            // Do not exit on error.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
#ifdef DEBUG
        LOG(INFO) << "Got number of bytes " << n;
#endif
    }

    return 0;
}

RtpStreamListener::~RtpStreamListener() {

    mExitRequested = true;

    if (mFd != 0) {
        close(mFd.load());
    }

    if (mReaderThread->joinable()) {
        mReaderThread->join();
    }
}

constexpr static int diffSequenceNumber(uint16_t first, uint16_t second) {
    int diff = 0;
    if (first > second) {
        if ((first - second) > MID_SEQUENCE_NUMBER) {
            diff = -(0x10000 + second - first);
        }
        else {
            diff = first - second;
        }
    }
    else {
        if ((second - first) > MID_SEQUENCE_NUMBER) {
            diff = (0x10000 + second - first);
        }
        else {
            diff = -(second - first);
        }
    }
    return diff;
}

void RtpStreamListener::processRtpPacket(const char *buf, const size_t len) {
    bool padding = buf[0] & 0x20;
    bool extension = buf[0] & 0x10;
    int csrcCount = buf[0] & 0x0F;
    uint16_t sequenceNumber = buf[2] << 8 | buf[3];
    size_t offset = 12 + csrcCount * 4;
    size_t paddingLen = 0;
    size_t extensionLen = 0;
    if (padding) {
        paddingLen = buf[len - 1];
    }
    if (extension) {
        if (offset > len + 4) {
            LOG(ERROR) << "Invalid packet. offset : " << offset << "  paddingLen : " << paddingLen << " len: " << len
                    << " sequenceNumber : " << sequenceNumber;
            return;
        }
        extensionLen = (buf[offset + 2] << 8) + buf[offset + 3];
        offset += 4 * extensionLen + 4;
    }

    if ((offset + paddingLen) > len) {
        LOG(ERROR) << "Invalid packet. offset : " << offset << "  paddingLen : " << paddingLen << " len: " << len
                << " sequenceNumber : " << sequenceNumber << " extensionLen : " << extensionLen;
        return;
    }

    size_t payLoadSize = len - offset - paddingLen;
#if DEBUG
    LOG(INFO) << "New packet. offset : " << offset << " payLoadSize :  " << payLoadSize << "  paddingLen : "
            << paddingLen << " len: " << len << " extensionLen : " << extensionLen << " sequenceNumber : "
            << sequenceNumber;
#endif

    bool cachedBuffersPresent = (mBufferCache.size() > 0);
    if (disableSequencing || (sequenceNumber == mNextSequenceNumber) || (-1 == mNextSequenceNumber)) {
        pushBuffertoBQ(buf + offset, payLoadSize, sequenceNumber);
    }
    else {
        int diff = diffSequenceNumber(mNextSequenceNumber, sequenceNumber);
        LOG(INFO) << "Sequence Number mismatch expected : " << mNextSequenceNumber << " got :  " << sequenceNumber
                << " diff : " << diff << " offset : " << offset << " payLoadSize :  " << payLoadSize
                << "  paddingLen : " << paddingLen << " len: " << len << " extensionLen : " << extensionLen;

        if (abs(diff) > MAX_SEQNUM_DIFF_FOR_DISCONTINUITY) {
            LOG(WARNING) << "Discontinuity detected : inject all cached buffer and reset sequence";
            for (auto it = mBufferCache.begin(); it != mBufferCache.end();) {
                pushBuffertoBQ(it->mBuffer, it->mLen, it->mSequenceNumber);
                it = mBufferCache.erase(it);
            }
            pushBuffertoBQ(buf + offset, payLoadSize, sequenceNumber);
            cachedBuffersPresent = false;
        }
        else if (mBufferCache.size() < MAX_RTP_BUFFER_CACHE) {
            BufferInfo bufferInfo(sequenceNumber, buf + offset, payLoadSize);
            if (mBufferCache.size() == 0) {
                mBufferCache.emplace_back(std::move(bufferInfo));
            }
            else {
                for (auto it = mBufferCache.begin(); it != mBufferCache.end();) {
                    if (diffSequenceNumber(it->mSequenceNumber, sequenceNumber) > 0) {
                        mBufferCache.emplace(it, std::move(bufferInfo));
                        break;
                    }
                    it++;
                    if (it == mBufferCache.end())
                    {
                        mBufferCache.emplace(it, std::move(bufferInfo));
                        break;
                    }
                }
            }
        }
        else {
            LOG(ERROR) << "Cannot cache packet as cache is full ";
            abort();
        }
    }
    if (cachedBuffersPresent) {
#ifdef DEBUG
        LOG(INFO) << "About to process cached buffers size :  " << mBufferCache.size();
        for (auto it = mBufferCache.begin(); it != mBufferCache.end(); it++) {
            LOG(INFO) << "Cache seqNum :  " << it->mSequenceNumber;
        }
#endif
        processCachedBuffers();
    }
}

void RtpStreamListener::pushBuffertoBQ(const char *buf, const size_t len, int sequenceNumber) {
    if (mPHandler != nullptr) {
        bq_buffer *cBuf = (*mPHandler)->acquireBuffer(0);
        memcpy(cBuf->buffer, buf, len);
        cBuf->size = len;
        (*mPHandler)->pushBuffertoBQ(cBuf, 0);
        (*mPHandler)->reportTSBufferQueued();
    }
    mNextSequenceNumber = (sequenceNumber + 1) & 0xFFFF;
}

void RtpStreamListener::processCachedBuffers() {
    bool pushed, continueLoop;
    do {
        pushed = false;
        for (auto it = mBufferCache.begin(); it != mBufferCache.end();) {
            if (it->mSequenceNumber == mNextSequenceNumber) {
                pushBuffertoBQ(it->mBuffer, it->mLen, it->mSequenceNumber);
                it = mBufferCache.erase(it);
                pushed = true;
            }
            else {
                it++;
            }
        }
        if ((mBufferCache.size() > 0) && pushed) {
            continueLoop = true;
        }
        else if (mBufferCache.size() == MAX_RTP_BUFFER_CACHE) {
            LOG(WARNING) << "Cache is full, unavailable seq : " << mNextSequenceNumber;
            mNextSequenceNumber = (mNextSequenceNumber + 1) & 0xFFFF;
            continueLoop = true;
        }
        else {
            continueLoop = false;
        }
    } while (continueLoop);
}

BufferInfo::BufferInfo(BufferInfo &&other) :
        mSequenceNumber(other.mSequenceNumber), mBuffer(other.mBuffer), mLen(other.mLen) {
    other.mBuffer = nullptr;
    other.mLen = 0;
}

BufferInfo::BufferInfo(uint16_t sequenceNumber, const char *buffer, size_t len) :
        mSequenceNumber(sequenceNumber), mLen(len) {
    mBuffer = new char[len];
    memcpy(mBuffer, buffer, len);
}

BufferInfo::~BufferInfo() {
    if (mBuffer) {
        delete[] mBuffer;
    }
}

}
