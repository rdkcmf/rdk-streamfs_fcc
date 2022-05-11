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

#include <config_fcc.h>
#include <mutex>
#include <map>
#include <config_options.h>
#include <memory>

template<typename T>
class MVar;

class VariableMonitorDispatcher {
public:
    class VBase {
    public:
        VBase() = default;

        virtual ~VBase() = default;

        bool mValid = false;
    };

    CLASS_NO_COPY_OR_ASSIGN(VariableMonitorDispatcher);

    static VariableMonitorDispatcher &getInstance() {
        static VariableMonitorDispatcher instance;
        return instance;
    }

    std::mutex &getLock() { return mLock; }

    VBase *find(ConfigVariableId &id) {
        auto val = mConfigMap.find(id);
        if (val == mConfigMap.end()) {
            return nullptr;
        } else {
            return val->second;
        }
    }

    int registerVariable(ConfigVariableId &basicString, VBase *value);

private:
    std::map<ConfigVariableId, VBase *> mConfigMap;

    VariableMonitorDispatcher() = default;

    std::mutex mLock;
};
