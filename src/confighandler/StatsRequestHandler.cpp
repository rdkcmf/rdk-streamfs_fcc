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

#include <MediaSourceHandler.h>
#include <config_options.h>
#include "version.h"
#include "confighandler/StatsRequestHandler.h"
#include <unistd.h>

static std::map<ConfigVariableId, const char *> ConfigMAP_StreamConfigs = {
        {kBufferSrcLost0, CONFIG_F_STREAM_STATUS}
};

int fcc::StatsRequestHandler::writeConfig(const std::string &fileName, const std::string &buf, size_t size) {

    UNUSED(size);
    UNUSED(buf);

    LOG(WARNING) << "TODO. Write not implemented for:" << fileName;

    switch (hashStr(fileName.c_str())) {

        case hashStr(CONFIG_F_STATS_SW_VERSION):
            return -1;
    }
    return -1;

}

std::string fcc::StatsRequestHandler::readConfig(const std::string &fileName) {
    return getConfig(fileName);
}

fcc::StatsRequestHandler::StatsRequestHandler(MediaSourceHandler *mediaSourceHandler,
                                              streamfs::PluginCallbackInterface *cb)
        : ConfigHandlerMVarCb<ByteVectorType>(cb),
          mMSrcHandler(mediaSourceHandler) {
    mCbFunc = MVar<ByteVectorType>::getWatcher(this, ConfigMAP_StreamConfigs);
    mSrcLost0 = &MVar<ByteVectorType>::getVariable(kBufferSrcLost0);
}

std::string fcc::StatsRequestHandler::getConfig(const std::string &fileName) {
    std::lock_guard<std::mutex> lockGuard(mStateMtx);

    switch (hashStr(fileName.c_str())) {

        case hashStr(CONFIG_F_STATS_SW_VERSION):
            return std::to_string(PROJECT_MAJOR_VERSION) + "." + std::to_string(PROJECT_MINOR_VERSION) + "-" +
                   PROJECT_REVISION + "\n";

        case hashStr(CONFIG_F_STREAMFS_PID): {
            pid_t streamfsPid = getpid();
            auto pid = (unsigned long) streamfsPid;
            return std::to_string(pid) + "\n";
        }

        case hashStr(CONFIG_F_STAT_GLOBAL):
            mGlobalStats = mMSrcHandler->getGlobalStats(0);
            return mGlobalStats;

        case hashStr(CONFIG_F_STAT_CHANNEL):
            mChannelStats = mMSrcHandler->getChannelStats(0);
            return mChannelStats;

       case hashStr(CONFIG_F_STREAM_STATUS):
          auto tmp = mSrcLost0->getValue();
          return  std::string(tmp.begin(), tmp.end());
    }

    return "NOT IMPLEMENTED";
}

uint64_t fcc::StatsRequestHandler::getSize(const std::string &fileName) {
    return getConfig(fileName).size();
}
