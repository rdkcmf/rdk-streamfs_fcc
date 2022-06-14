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
#include "UdpStreamListener.h"
#include <glog/logging.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <thread>
#include <algorithm>

#define BUF_SIZE FEIP_BUFFER_SIZE

namespace multicast {

UdpStreamListener::UdpStreamListener() : mFd(0), mExitRequested(false) {

    mReaderThread = std::shared_ptr<std::thread>(
            new std::thread(&UdpStreamListener::readLoop, this));
}

int UdpStreamListener::setup(const char *group, int port, MediaSourceHandler **pHandler) {
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

int UdpStreamListener::readLoop() {
    int n;
    char buf[BUF_SIZE];
    socklen_t addrlen = sizeof(mAddr);
    //FILE* f = fopen("/tmp/dump.mov", "w");

    while (mFd == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        LOG(INFO) << "Waiting for UDP open";
    }
    bq_buffer *cBuf;
    while (!mExitRequested) {
        {
            std::lock_guard<std::mutex> lockGuard(mLock);
            n = recvfrom(mFd.load(), buf, BUF_SIZE,0,(struct sockaddr *) &mAddr, &addrlen);
            //fwrite(buf, n, 1, f);
            if (mPHandler != nullptr ) {
                cBuf = (*mPHandler)->acquireBuffer(0);
                memcpy(cBuf->buffer, buf, BUF_SIZE);
                cBuf->size = n;
                (*mPHandler)->pushBuffertoBQ(cBuf, 0);
                (*mPHandler)->reportTSBufferQueued();
            }
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

UdpStreamListener::~UdpStreamListener() {

    mExitRequested = true;

    if (mFd != 0) {
        close(mFd.load());
    }

    if (mReaderThread->joinable()) {
        mReaderThread->join();
    }
}

}
