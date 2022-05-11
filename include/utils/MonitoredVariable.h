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

#include <functional>
#include <set>
#include <glog/logging.h>

#include "config_fcc.h"
#include "confighandler/ConfigHandler.h"
#include "VariableMonitorDispatcher.h"
#include "MonVarObserver.h"
#include <chrono>
#include <condition_variable>
#include "tracing.h"

/**
 * Monitored variable type. Observer can subscribe to change of
 * a global MVar variable. MVars are global variables with thread
 * safe access.
 *
 * For example:
 * Class A getting variable:
 *
 *    MVar<std::string>& x = MVar<std::string>::getVariable(someId);
 *
 * Class B registering a watcher function:
 *
 *    MVar<std::vector<char>>::watch(someId, mCbFunc);
 *
 * Class A changing the variable will notify Class B:
 *
 *    x = "new_value";
 *
 * @tparam T
 */


template<typename T>
class MVar final : public VariableMonitorDispatcher::VBase {
public:
    CLASS_NO_COPY_OR_ASSIGN(MVar);

    typedef std::function<void(const T &, const T &, const ConfigVariableId &id)> watcher_function;

    typedef std::shared_ptr<watcher_function> watcher_sp_t;
    typedef std::weak_ptr<watcher_function> watcher_wp_t;

    /**
     * Get variable from variable database.
     * @param id
     * @return
     */
    static MVar<T> &getVariable(ConfigVariableId &id) {
        auto &vd = VariableMonitorDispatcher::getInstance();
        std::lock_guard<std::mutex> mLock(vd.getLock());

        auto *val = vd.find(id);
        if (val == nullptr) {
            auto *var = new MVar();
            var->mId = id;
            if (vd.registerVariable(id, var) == 0) {
                return *var;
            } else {
                throw;
            }
        } else {
            return *static_cast<MVar *>(val);
        }
    }

    template<typename TDuration>
    bool waitForValue(T value, TDuration duration) {
        std::unique_lock<std::mutex> lock(mCvMutex);
        return mCvVariableUpdated.wait_for(lock, duration, [=]{
            std::lock_guard<std::mutex> mLock(vd.getLock());
            return mValue == value;
        });
    }

    bool waitForValue(T value) {
        std::unique_lock<std::mutex> lock(mCvMutex);

        mCvVariableUpdated.wait(lock, [=]{
            std::lock_guard<std::mutex> mLock(vd.getLock());
            return mValue == value;
        });
        return true;
    }

    /**
     * Watch variable
     * @param id
     */
    static void watch(ConfigVariableId id, watcher_sp_t &watcher) {
        auto &var = getVariable(id);
        var.registerWatcher(watcher);
    }

    /**
     * Obtain value of variable
     * @return
     */
    const T &getValue() {
        std::lock_guard<std::mutex> mLock(vd.getLock());
        return mValue;
    }

    MVar<T> &operator=(T const &rhs) {
        std::lock_guard<std::mutex> mLock(vd.getLock());

        auto it = this->mWatchers.begin();
        while (it != this->mWatchers.end()) {
            if (it->expired())
                continue;

            auto p = *it->lock();
            if (!p) {
                this->mWatchers.erase(it++);
            } else {
                p(mValue, rhs, mId);
                it++;
            }
        }
        mValue = rhs;
        mCvVariableUpdated.notify_all();

        return (*this);
    }

    T &operator*() {
        return mValue;
    }

    T *operator->() {
        return &mValue;
    }

    friend bool operator==(const MVar<T> &lhs, const T &rhs) {
        return lhs.mValue == rhs;
    }

    friend bool operator!=(const MVar<T> &lhs, const T &rhs) {
        return lhs != rhs;
    }

    MVar() = default;

    ~MVar() final = default;
    static std::shared_ptr<typename MVar<T>::watcher_function> getWatcher(MonVarObserver<T>* configHandler,
                                                                          const MapConfigToFSPathType& configMap);
private:
    std::condition_variable mCvVariableUpdated;
    std::mutex mCvMutex;

    void cleanExpiredCallbacks() {
        auto it = mWatchers.begin();
        while(it != mWatchers.end() ) {
            if( it->expired() )
            {
                it = mWatchers.erase(it );
                continue;
            }
            ++it;
        }
    }
    void registerWatcher(watcher_sp_t &watcher) {
        watcher_wp_t wp(watcher);
        cleanExpiredCallbacks();
        mWatchers.emplace_back(wp);
    }


private:
    T mValue{};
    ConfigVariableId mId{};
    std::vector<watcher_wp_t> mWatchers;
    VariableMonitorDispatcher &vd = VariableMonitorDispatcher::getInstance();
};

template<typename T>
std::shared_ptr<typename MVar<T>::watcher_function> MVar<T>::getWatcher(
        MonVarObserver<T>* varObserver,
        const MapConfigToFSPathType& configMap) {

    auto watcherFunc =  std::make_shared<MVar<T>::watcher_function>(
            [varObserver, configMap](T oldVal, T newVal, const ConfigVariableId &id) {
                UNUSED(oldVal);
                auto it = configMap.find(id);
                if (it != configMap.end()) {
                    TRACE_EVENT(TR_FCC_CORE, "Var changed", "name", id.c_str());
                    varObserver->notifyConfigurationChanged(it->second, newVal);
                }
            }
    );

    // Register variables with watcher
    for (const auto& it : configMap) {
        MVar<T>::watch(it.first, watcherFunc);
    }

    return watcherFunc;
}
