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

#include <glog/logging.h>
#include <boost/algorithm/hex.hpp>
#include "utils/json.hpp"
#include "StreamProtectionConfigExport.h"

// JSON key definitions
const std::string kCHANNEL = "channel";
const std::string kECM = "ecm";
const std::string kPAT = "pat";
const std::string kPMT = "pmt";
const std::string kCLEAR = "clear";

namespace {
inline std::string byteArrayAsHex(const ByteVectorType &bArray) {
    try {
        return boost::algorithm::hex(std::string({bArray.begin(), bArray.end()}));
    } catch (std::exception &e) {
        LOG(ERROR) << "Could convert byte array to hex string";
        return std::string();
    }
}
}

StreamProtectionConfigExport::StreamProtectionConfigExport(const StreamProtectionConfig &protectionConfig) {
    mChannelInfo = protectionConfig.channelInfo();
    mEcm = byteArrayAsHex(protectionConfig.ecm());
    mPat = byteArrayAsHex(protectionConfig.pat());
    mPmt = byteArrayAsHex(protectionConfig.pmt());
    mClear = protectionConfig.isClearStream();
}

std::string StreamProtectionConfigExport::getJsonAsString() const {
    json::JSON obj;
    obj[kCHANNEL] = mChannelInfo;
    obj[kECM] = mEcm;
    obj[kPAT] = mPat;
    obj[kPMT] = mPmt;
    obj[kCLEAR] = mClear;
    std::string jsonStr = obj.dump();
    jsonStr.erase(std::remove_if(jsonStr.begin(), jsonStr.end(), ::isspace), jsonStr.end()); // trim whitespaces
    return jsonStr;
}

ByteVectorType StreamProtectionConfigExport::getJsonAsByteArray() const {
    std::string jsonStr = getJsonAsString();
    return ByteVectorType(jsonStr.begin(), jsonStr.end());
}
