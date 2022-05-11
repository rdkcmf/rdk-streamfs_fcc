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

#include "NetworkRouteMonitor.h"
#include <streamfs/MessageQueue.h>

class NetworkMessage {
public:
    enum MessageType {
        UNDEFINED,
        NO_GATEWAY,
        NEW_GATEWAY,
        NO_MULTICAST,
    };

    explicit NetworkMessage() = default;

    explicit NetworkMessage(MessageType _what) : what(_what) {}

    NetworkMessage(MessageType _what, const std::string &_interface)
            : what(_what), interface(_interface) {};

    MessageType what = UNDEFINED; // Message identifier (mandatory field)
    std::string interface; // network interface
};

typedef MessageQueue<NetworkMessage> msg_queue_t;
typedef std::shared_ptr<msg_queue_t> shared_msg_queue_t;

class NetworkRouteObserver : public NetworkRouteMonitor::NetworkRouteCallback {
public:

    explicit NetworkRouteObserver(msg_queue_t *msgQueue) : mMsgQueue(msgQueue) {};

    ~NetworkRouteObserver() {}

    void defaultRouteChanged(NetworkRouteMonitor::routeInfo route) override {
        LOG(INFO) << "Default route changed to: " << route.gateway << " " << route.interface << " " << route.metric;
        if (route.interface.empty()) {
            mMsgQueue->pushMessage(NetworkMessage(NetworkMessage::NO_GATEWAY));
        } else {
            mMsgQueue->pushMessage(NetworkMessage(NetworkMessage::NEW_GATEWAY, route.interface));
        }
    }

private:
    msg_queue_t *mMsgQueue;
};
