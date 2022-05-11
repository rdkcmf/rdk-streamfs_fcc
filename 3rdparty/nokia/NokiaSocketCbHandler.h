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


#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include "DemuxerStatusCallback.h"
#include "DemuxerCallbackHandler.h"
#include "MediaSourceHandler.h"
#include "confighandler/ConfigHandlerMVarCb.h"
#include "utils/MonitoredVariable.h"
#include "StreamParser/StreamProtectionConfig.h"

// Defined by PAT/PMT Parsing section of the Nokia documentation

#define NOKIA_SECTION_LENGHT_TYPE_SIZE  4

typedef MVar<ByteVectorType> MonitoredCharArray;

class NokiaSocketCbHandler : public DemuxerCallbackHandler {
    enum NokiaMessageType {
        PAT_PMT_MESSAGE = 0,
        ECM_MESSAGE = 1
    };

public:
    explicit NokiaSocketCbHandler();

    ~NokiaSocketCbHandler();

    void notifyStreamSwitched(const std::string &channelIp) override;

private:
    int openLoop();

    /**
     * Parse PAT/PMT/ECM sections.
     *
     * @param inBuf  - input buffer
     * @param inBufSize  - size of the input buffer
     * @param msgType  - callback message type (i.e. ECM or PAT/PMT)
     * @param feipId  - frontend identifier
     * @return - 0 on success, negative on failure
     */
    static int parseSections(const char *inBuf, unsigned int inBufSize, NokiaMessageType msgType,
                             int32_t &feipId);
    static bool isCaDescriptorPresentInPMT();

private:
    static int byteBufToInt(const char *buffer);

    std::mutex mStreamStatusMtx;

    struct sockaddr_in mServerAddr{}, mClientAddr{};
    int mSocketFd;
    bool mExitRequested;
    std::shared_ptr<std::thread> mNetworkThread;
    bool mIsPatPmtReceived = true;
    bool mIsEcmReceived = true;
    std::string mChannelIp;

    MVar<StreamProtectionConfig> *mDrm;

    static MonitoredCharArray *mPat;
    static MonitoredCharArray *mPmt;
    static MonitoredCharArray *mEcm;
};
