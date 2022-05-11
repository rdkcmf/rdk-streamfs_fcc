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

#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

/**
 * This helper class implements logic for converting a PlayerStateEnum::StateType
 * enum to a string and vise versa.
 *
 * An object instance is created by providing an enumerated value represented as
 * either a string or enum to the constructor. The associated enum and string
 * representation can be obtained afterwards by calling enumValue() and
 * stringValue() respectively.
 *
 * The method isValid() returns a boolean if the enum value is valid (true) or
 * not (false).
 */
class PlayerStateEnum
{
public:
    enum StateType {
        UNDEF,
        READY,
        PLAYING,
        PAUSED
    };

    explicit PlayerStateEnum(const std::string& enumString) : mIsValid(false) {
        init([enumString](const auto& obj) { return obj.second == boost::to_upper_copy<std::string>(boost::trim_copy(enumString)); });
    }

    explicit PlayerStateEnum(StateType enumValue) : mIsValid(false) {
        init([enumValue](const auto& obj) { return obj.first == enumValue; });
    }

    bool isValid() const { return mIsValid; };
    StateType enumValue() const { return mEnumValue; };
    std::string stringValue() const { return mStringValue; };

private:
    const std::map<StateType, const std::string> map {
        { StateType::UNDEF   , "UNDEF" },
        { StateType::READY   , "READY" },
        { StateType::PLAYING , "PLAYING" },
        { StateType::PAUSED  , "PAUSED"}
    };

    bool mIsValid;
    StateType mEnumValue;
    std::string mStringValue;

    template<typename predicate_type>
    void init(predicate_type pred) {
        auto it = std::find_if(
                map.begin(),
                map.end(),
                [&pred](const auto& obj) { return pred(obj); });

        if ( it != map.end() ) {
            mEnumValue = it->first;
            mStringValue = it->second;
            mIsValid = true;
        }
    }
};
