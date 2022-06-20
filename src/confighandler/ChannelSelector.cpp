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

#include "confighandler/ChannelSelector.h"
#include "MediaSourceHandler.h"
#include "config_options.h"

int fcc::ChannelSelector::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &mLastRequestTime)) {
        LOG(INFO) << "Error on getting monotonic time << " << buf;
    }

    std::string tmp(buf);
    tmp.erase(std::remove(tmp.begin(), tmp.end(), '\n'), tmp.end());

    if (fileName == CONFIG_F_CHANNEL_SELECT) {
        if (mMSrcHandler->open(tmp, 0, 0) == 0) {
            ByteVectorType chan(buf.begin(), buf.end());
            notifyConfigurationChanged(fileName, chan);
            // Interrupt all reads on TSB when setting empty channel
            if (!mMSrcHandler->isChannelActive()) {
                mTsb->interruptReads();
            }
            return size;
        }

    }
    /** ERROR. Configuration not handled **/
    notifyConfigurationChanged(fileName, ByteVectorType(buf.begin(), buf.end()));
    return -1;
}


std::string fcc::ChannelSelector::readConfig(const std::string &fileName) {
    return getConfig(fileName);
}

uint64_t fcc::ChannelSelector::getSize(const std::string &fileName) {
    return getConfig(fileName).size();
}

std::string fcc::ChannelSelector::getConfig(const std::string &fileName) {
    std::lock_guard<std::mutex> lockGuard(mStateMutex);

    std::string result;

    if (fileName == CONFIG_F_CHANNEL_SELECT_TIMESTAMP) {
        uint64_t timeStamp = mLastRequestTime.tv_sec * 1000 + mLastRequestTime.tv_nsec / 1000000;
        result = std::to_string(timeStamp);
    } else {
        result = mMSrcHandler->getCurrentUri();
    }

    return result;
}
