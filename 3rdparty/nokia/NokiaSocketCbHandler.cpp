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


#include <cstdint>
#include <cstring>
#include <mutex>
#include "config_options.h"

#include <glog/logging.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config_fcc.h"
#include "NokiaSocketCbHandler.h"
#include "DemuxerStatusCallback.h"

MonitoredCharArray *NokiaSocketCbHandler::mEcm;
MonitoredCharArray *NokiaSocketCbHandler::mPmt;
MonitoredCharArray *NokiaSocketCbHandler::mPat;

inline uint32_t parseIPV4string(const char *ipAddress) {
    in_addr addr{};
    inet_aton(ipAddress, &addr);
    return ntohl(addr.s_addr);
}

int NokiaSocketCbHandler::byteBufToInt(const char *buffer) {
    if (buffer == nullptr) {
        LOG(ERROR) << "Got null buffer";
        return 0;
    }
    int feip = buffer[0] + (buffer[1] << 8) + (buffer[2] << 16) + (buffer[3] << 24);
    return feip;
}

int NokiaSocketCbHandler::openLoop() {
    unsigned int len, res;
    char buffer[SOCKET_MAXLINE + 1];

    len = sizeof(mClientAddr);

    LOG(INFO) << "Waiting for socket message. mSocketFd = " << mSocketFd;

    while (!mExitRequested) {
        res = recvfrom(mSocketFd,
                       (char *) buffer,
                       SOCKET_MAXLINE,
                       0,
                       (struct sockaddr *) &mClientAddr,
                       &len);

        {
            std::lock_guard<std::mutex> lock(mStreamStatusMtx);
            NokiaMessageType msgType;
            LOG(INFO) << "** Got message of length = " << res;

            if (res < NOKIA_SECTION_LENGHT_TYPE_SIZE) {
                LOG(WARNING) << "Got invalid socket message with return value: " << res;
                continue;
            }

            /*Ignore dummy buffer if received before PAT*/
            if (!mIsPatPmtReceived && (res == NOKIA_SECTION_LENGHT_TYPE_SIZE)  ) {
                LOG(WARNING) << "Got dummy buffer from socket before PAT/PMT";
                continue;
            }

            // We should not receive any more messages efter the ECM message if the stream was
            // not reset
            if (mIsEcmReceived) {
                LOG(ERROR) << "BUG: New message received after ECM parsing. ";
                continue;
            }


            // Nokia has no message identifier on socket messages. We need
            // to figure out the type from the order of the messages.

            if (mIsPatPmtReceived) {
                msgType = NokiaMessageType::ECM_MESSAGE;
                mIsEcmReceived = true;
            } else {
                msgType = NokiaMessageType::PAT_PMT_MESSAGE;
                mIsPatPmtReceived = true;
            }

            int32_t feipId;
            if (parseSections(buffer, res, msgType, feipId) == 0) {
                if (msgType == ECM_MESSAGE) {
                    // Testing: skip Nokia ECM parsing
                    *mDrm = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::HIGH,
                                                   mChannelIp,
                                                   mEcm->getValue(),
                                                   mPat->getValue(),
                                                   mPmt->getValue(), false);

                    mCb->notify(DemuxerStatusCallback::ECM_UPDATED, feipId, getDemuxerId());
                }

                if (msgType == PAT_PMT_MESSAGE) {
                    mCb->notify(DemuxerStatusCallback::PAT_PMT_UPDATED, feipId, getDemuxerId());
                }
            }
            else {
                LOG(WARNING) << "parseSections failed. msgType : " << msgType;
                if (msgType == ECM_MESSAGE) {
                    /*Check if dummy ECM message*/
                    if ( res == NOKIA_SECTION_LENGHT_TYPE_SIZE) {
                        if (!isCaDescriptorPresentInPMT()) {
                            LOG(INFO) << "dummy ECM and clear channel. : update mDrm";
                            *mDrm = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::HIGH,
                                                           mChannelIp, mEcm->getValue(), mPat->getValue(),
                                    mPmt->getValue(), true);
                        }
                    }
                }
            }

        } //lock
    }

    LOG(INFO) << "Exit socket receiver";

    return 0;
}

NokiaSocketCbHandler::~NokiaSocketCbHandler() {
    mExitRequested = true;
    shutdown(mSocketFd, SHUT_RDWR);
    close(mSocketFd);

    if (mNetworkThread->joinable()) {
        mNetworkThread->join();
    }
}

NokiaSocketCbHandler::NokiaSocketCbHandler() :
        mExitRequested(false) {
    const int optVal = 1;
    const socklen_t optLen = sizeof(optVal);
    memset(&mServerAddr, 0, sizeof(mServerAddr));
    memset(&mClientAddr, 0, sizeof(mClientAddr));

    mDrm = &MVar<StreamProtectionConfig>::getVariable(kDrm0);
    mEcm = &MVar<ByteVectorType>::getVariable(kEcm0);
    mPmt = &MVar<ByteVectorType>::getVariable(kPmt0);
    mPat = &MVar<ByteVectorType>::getVariable(kPat0);

    if ((mSocketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Failed to create UDP socket");
        return;
    }

    mServerAddr.sin_family = AF_INET;
    mServerAddr.sin_addr.s_addr = INADDR_ANY;
    mServerAddr.sin_port = htons(FEIP_UDP_SOCKET_PORT);

    int open(const char *ip,
             uint16_t port,
             uint32_t feipId);

    if (((setsockopt(mSocketFd, SOL_SOCKET, SO_REUSEADDR, (void *) &optVal, optLen)) != 0)) {
        LOG(ERROR) << "setsockopt() failed with errno " << strerror(errno);
        throw;
    }

    if (bind(mSocketFd, (const struct sockaddr *) &mServerAddr,
             sizeof(mServerAddr)) < 0) {
        LOG(ERROR) << "Failed to bind port" << strerror(errno);
        throw;
    }

    mNetworkThread = std::shared_ptr<std::thread>(
            new std::thread(&NokiaSocketCbHandler::openLoop, this));
}

bool NokiaSocketCbHandler::isCaDescriptorPresentInPMT() {
    bool descriptorFound = false;
    do {
        const int programInfoIndex = 12;
        const std::vector<unsigned char> &pmt = mPmt->getValue();
        if (pmt.size() < programInfoIndex) {
            LOG(WARNING) << "isCaDescriptorPresentInPMT() : Invalid size : " << pmt.size();
            break;
        }

        if (0x02 != pmt[0]) {
            LOG(WARNING) << "isCaDescriptorPresentInPMT() : Invalid TableId : 0x" << std::hex << pmt[0];
            break;
        }

        size_t sectionLength = (((pmt[1] & 0x0F) << 8) | pmt[2]) & 0xFFF;

        if (sectionLength > (pmt.size() - 3)) {
            LOG(WARNING) << "isCaDescriptorPresentInPMT() : Invalid size : " << pmt.size() << " sectionLength : "
                    << sectionLength;
            break;
        }
        /*3 bytes Table header + 5 bytes syntax section header + 2 byte pmt prefix */
        size_t descriptorLen = (((pmt[10] & 0x0F) << 8) | pmt[11]) & 0xFFF;

        if (descriptorLen > sectionLength) {
            LOG(ERROR) << "Invalid descriptor field length: " << descriptorLen << " sectionLength = " << sectionLength;
            break;
        }

        for (size_t i = programInfoIndex; i < (programInfoIndex + descriptorLen - 1);) {
            if (pmt[i] == 0x09) {
                descriptorFound = true;
                LOG(WARNING) << "isCaDescriptorPresentInPMT() : Got CA descriptor";
                break;
            }
            LOG(WARNING) << "isCaDescriptorPresentInPMT() : Got descriptor : 0x" << std::hex << pmt[i];
            i += pmt[i + 1] + 2;
        }

        auto remSectBytes = sectionLength - 9;
        remSectBytes -= descriptorLen;

        auto streamLoopOffset = programInfoIndex + descriptorLen;

        unsigned p = 0;

        while (remSectBytes > 4) {

            //skip stream type and pid
            p += 3;

            auto es_info_length = ((pmt[streamLoopOffset + p] << 8) + pmt[streamLoopOffset + p + 1]) & 0xFFF;
            p += 2;

            // 0x09 - CA_descriptor
            // 0x5601 - VMX identifier
            if ((pmt[streamLoopOffset + p] == 0x9) && (pmt[streamLoopOffset + p + 2] == 0x56)
                    && (pmt[streamLoopOffset + p + 3] == 0x01)) {
                LOG(INFO) << "isCaDescriptorPresentInPMT() : Got VMX CA descriptor";
                descriptorFound = true;
                break;
            }
            p += es_info_length;
            remSectBytes -= es_info_length + 5;
        }
    } while (false);
    LOG(INFO) << "isCaDescriptorPresentInPMT() : Exit. descriptorFound : " << descriptorFound;
    return descriptorFound;
}

int NokiaSocketCbHandler::parseSections(const char *inBuf, unsigned int inBufSize, NokiaMessageType msgType,
                                        int32_t &feipId) {

    unsigned int offset = 0;
    int ecmSectionLength, patSectionLength, pmtSectionLength = 0;
    ByteVectorType tmpPat = {};

    if (inBuf == nullptr || inBufSize < NOKIA_SECTION_LENGHT_TYPE_SIZE) {
        LOG(WARNING) << "Received invalid VBO message. Input buffer:" << inBuf << " input buffer size:" << inBufSize;
        return -1;
    }

    feipId = byteBufToInt(inBuf);
    offset += NOKIA_SECTION_LENGHT_TYPE_SIZE;

    if (offset + NOKIA_SECTION_LENGHT_TYPE_SIZE > inBufSize) {
        LOG(WARNING) << "Received dummy VBO message of type:" << msgType << " Likely it is a non-encrypted stream";
    }

    switch (msgType) {
        case NokiaMessageType::ECM_MESSAGE:
            if (offset >= inBufSize) {
                LOG(ERROR) << "Invalid ECM message received"
                           << " input buffer size = " << inBufSize;
                return -1;
            }

            ecmSectionLength = byteBufToInt(inBuf + offset);
            offset += NOKIA_SECTION_LENGHT_TYPE_SIZE;

            if (ecmSectionLength == 0 || ecmSectionLength + offset > inBufSize) {
                LOG(ERROR) << "Invalid ECM message received. ecmSectionLength = " << ecmSectionLength
                           << " input buffer size = " << inBufSize << " Stream may be un-encrypted";
                *mEcm = ByteVectorType(0);
                return -1;
            }

            *mEcm = ByteVectorType(&inBuf[offset],
                                   &inBuf[offset] + ecmSectionLength);

            offset += ecmSectionLength;
            break;
        case NokiaMessageType::PAT_PMT_MESSAGE:
            patSectionLength = byteBufToInt(inBuf + offset);
            offset += NOKIA_SECTION_LENGHT_TYPE_SIZE;

            if (patSectionLength + offset > inBufSize) {
                LOG(ERROR) << "Invalid PAT/PMT message received. patSectionLength = " << patSectionLength
                           << " input buffer size = " << inBufSize;
                *mPat = ByteVectorType(0);
                return -1;
            }

            if (patSectionLength > 0) {
                *mPat = ByteVectorType(&inBuf[offset], &inBuf[offset] + patSectionLength);
            }
            offset += patSectionLength;

            // Parse PMT section
            pmtSectionLength = byteBufToInt(inBuf + offset);
            offset += NOKIA_SECTION_LENGHT_TYPE_SIZE;

            if (pmtSectionLength + offset > inBufSize) {
                LOG(ERROR) << "Invalid PAT/PMT message received. patSectionLength = " << pmtSectionLength
                           << " input buffer size = " << inBufSize;
                *mPmt = ByteVectorType(0);
                return -1;
            }

            if (pmtSectionLength > 0) {
                *mPmt = ByteVectorType(&inBuf[offset],
                                       &inBuf[offset] + pmtSectionLength);

            }
            offset += pmtSectionLength;

            break;
        default:
            LOG(WARNING) << "Invalid message type:" << msgType;
            return -1;
    }

    return 0;
}

void NokiaSocketCbHandler::notifyStreamSwitched(const std::string &channelIp) {
    std::lock_guard<std::mutex> lock(mStreamStatusMtx);
    mChannelIp = channelIp;
    mIsPatPmtReceived = false;
    mIsEcmReceived = false;
    *mEcm = ByteVectorType(0);
    *mPmt = ByteVectorType(0);
    *mPat = ByteVectorType(0);
}
