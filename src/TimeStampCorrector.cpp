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


#include "TimeStampCorrector.h"
#include <sys/sysinfo.h>
#include <glog/logging.h>

#define NTP_CORRECTION_THLD 1000000000

static long getUptime()
{
    struct sysinfo s_info{};
    int error = sysinfo(&s_info);
    if(error != 0)
    {
        LOG(ERROR) << "Failed to get system uptime. Error: "  << error;
        return 0;
    }

    return s_info.uptime;
}

TimeStampCorrector::TimeStampCorrector() {
    mTm = (unsigned int)time(nullptr);
}

unsigned int TimeStampCorrector::correctTimeStamp(unsigned int t) {
    auto current_time = (unsigned int)time(nullptr);

    if (current_time - t > NTP_CORRECTION_THLD) {
        auto diff = (current_time - getUptime()) + t;
        return diff;
    }

    return t;
}
