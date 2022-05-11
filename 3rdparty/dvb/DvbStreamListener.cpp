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
#include "DvbStreamListener.h"
#include <glog/logging.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <thread>
#include <algorithm>

#define BUF_SIZE FEIP_BUFFER_SIZE

namespace dvb {

DvbStreamListener::DvbStreamListener() : mFd(0), mExitRequested(false) {

    mReaderThread = std::shared_ptr<std::thread>(
            new std::thread(&DvbStreamListener::readLoop, this));
}

int DvbStreamListener::setup(const char *group, int port, MediaSourceHandler **pHandler) {
    UNUSED(group);
    UNUSED(port);
    UNUSED(pHandler);
    return 0;
}

int DvbStreamListener::readLoop() {
    while (!mExitRequested) {
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            LOG(INFO) << "TODO: implement DVB read";
        }
    }

    return 0;
}

DvbStreamListener::~DvbStreamListener() {

    mExitRequested = true;

    if (mFd != 0) {
        close(mFd.load());
    }

    if (mReaderThread->joinable()) {
        mReaderThread->join();
    }
}

}
