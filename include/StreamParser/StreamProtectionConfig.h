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

#include <string>
#include <utility>
#include <vector>
#include "config_fcc.h"

class StreamProtectionConfig {
public:

    enum ConfidenceTypes {
        RESET = 0,
        LOW   = 1,
        MID   = 2,
        HIGH  = 3
    };


    ConfidenceTypes confidence() const { return mConfidence; }
    std::string channelInfo() const { return mChannelInfo; }
    ByteVectorType ecm() const { return mEcm; }
    ByteVectorType pat() const { return mPat; }
    ByteVectorType pmt() const { return mPmt; }
    bool isClearStream() const { return mIsClear; }

    StreamProtectionConfig(ConfidenceTypes confidence,
                           std::string channelInfo,
                           ByteVectorType ecm,
                           ByteVectorType pat,
                           ByteVectorType pmt,
                           bool isClearStream)
            : mConfidence(confidence),
              mChannelInfo(std::move(channelInfo)),
              mEcm(std::move(ecm)),
              mPat(std::move(pat)),
              mPmt(std::move(pmt)),
              mIsClear(isClearStream){}

    StreamProtectionConfig(const std::string &channelInfo)
            : mConfidence(RESET),
              mChannelInfo(channelInfo) {}

    StreamProtectionConfig() {}

private:
    ConfidenceTypes mConfidence{RESET};
    std::string mChannelInfo;
    ByteVectorType mEcm;
    ByteVectorType mPat;
    ByteVectorType mPmt;
    bool mIsClear{true};
};
