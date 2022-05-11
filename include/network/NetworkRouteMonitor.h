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
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <mutex>

/**
 * Notifier of network route changes
 */
class NetworkRouteMonitor {
public:

    struct routeInfo {
        std::string interface;
        std::string gateway;
        uint32_t metric;
    };

    class NetworkRouteCallback {
    public:
        CLASS_NO_COPY_OR_ASSIGN(NetworkRouteCallback);

        NetworkRouteCallback();

        virtual void defaultRouteChanged(routeInfo route) = 0;

        virtual ~NetworkRouteCallback() = default;
    };

    CLASS_NO_COPY_OR_ASSIGN(NetworkRouteMonitor);

    static NetworkRouteMonitor &getInstance() {
        static NetworkRouteMonitor instance;
        return instance;
    }

    /**
     * Register NetworkRoute Callback
     * @param cb - callback implementation
     */
    void registerCallback(const std::shared_ptr<NetworkRouteCallback> &cb);

    void start() {
        if (mRMonThread == nullptr) {
            mRMonThread = new std::thread(&NetworkRouteMonitor::routePollThread, this);
        }
    }

    void stop();

    ~NetworkRouteMonitor();

private:

    void routePollThread();
    bool sendRouteDumpRequest();
    routeInfo getRouteInfo(struct rtattr *rth, int rtl) const;
    void dispatchDefaultRouteChange();

private:
    explicit NetworkRouteMonitor();

    std::vector<std::weak_ptr<NetworkRouteCallback>> nStateCbs;
    std::thread *mRMonThread = nullptr;
    std::mutex mMutex;
    bool mRequestExit = false;
    routeInfo mRoute{"","",UINT16_MAX};
    int mSocketId{};
    std::vector<routeInfo> mDefaultRoutes;
};
