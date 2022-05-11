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
#include <cstring>
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>
#include <signal.h>

#include "network/NetworkRouteMonitor.h"

#define NL_BUFFER_HEADER_SIZE 16*4096 // TODO: find out what the buffer size should be in order not to run into buffer overflow issues in recv. Right now it is too large.

static void sighand(int) {}

void NetworkRouteMonitor::registerCallback(const std::shared_ptr<NetworkRouteCallback> &cb) {
    std::lock_guard<std::mutex> guard(mMutex);
    std::weak_ptr<NetworkRouteCallback> wp = cb;
    nStateCbs.emplace_back(std::move(wp));
}

void NetworkRouteMonitor::stop() {
    mRequestExit = true;

    if (mRMonThread && mRMonThread->joinable()) {
        if(!sendRouteDumpRequest()) {
            LOG(ERROR) << "Failed to send network state update request. Potential deadlock.";
        }
        mRMonThread->join();
    }
}

void NetworkRouteMonitor::routePollThread() {

    sockaddr_nl addr;
    struct nlmsghdr *nlMsgHeader;
    int length;
    char buf[NL_BUFFER_HEADER_SIZE];

    bzero(buf, sizeof(buf));

    struct ifaddrmsg *ifa;
    struct rtattr *rth;
    int rtl;

    LOG(INFO) << "Starting network route monitor";

    if ((mSocketId = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1) {
        LOG(ERROR) << "Couldn't create NETLINK_ROUTE socket. Not running with root permissions?";
        return;
    }

    bzero(&addr, sizeof(addr));

    addr.nl_family = AF_NETLINK;
    addr.nl_groups = RTMGRP_IPV4_ROUTE;

    if (bind(mSocketId, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        LOG(ERROR) << "Could not bind network socket. Network detect will fail";
        return;
    }

    do {

        if (!sendRouteDumpRequest()) {
            break;
        }

        while ((length = recv(mSocketId, buf, NL_BUFFER_HEADER_SIZE, 0)) > 0 && !mRequestExit) {
            nlMsgHeader = (struct nlmsghdr *) buf;

            if (mRequestExit) {
                continue;
            }

            bool gatewayUpdated = false;

            for ( ; NLMSG_OK(nlMsgHeader, length); nlMsgHeader = NLMSG_NEXT(nlMsgHeader, length)) {
                ifa = (struct ifaddrmsg *) NLMSG_DATA(nlMsgHeader);
                rtl = RTM_PAYLOAD(nlMsgHeader);
                rth = RTM_RTA(ifa);

                switch (nlMsgHeader->nlmsg_type) {
                    case RTM_NEWROUTE: {
                        routeInfo route = getRouteInfo(rth, rtl);
                        if (route.gateway.size()) {
                            gatewayUpdated = true;
                            mDefaultRoutes.emplace_back(route);
                            std::sort(mDefaultRoutes.begin(), mDefaultRoutes.end(), [](routeInfo a, routeInfo b) {
                                return a.metric < b.metric;
                            });
                        }
                        break;
                    }

                    case RTM_DELROUTE: {
                        routeInfo route = getRouteInfo(rth, rtl);

                        if (route.interface.size()) {
                            gatewayUpdated = true;
                            mDefaultRoutes.erase(
                                    std::remove_if(
                                            mDefaultRoutes.begin(),
                                            mDefaultRoutes.end(),
                                            [&]( const routeInfo& vw ) {
                                                // Delete route with specific priority or if the metric is 0 meaning the interface has been taken down
                                                return (vw.interface == route.interface && (vw.metric == route.metric || route.metric == 0));
                                            }
                                    ),
                                    mDefaultRoutes.end()
                            );
                        }
                        break;
                    }

                    default:
                        break;
                }
            }

            if (gatewayUpdated) {
                dispatchDefaultRouteChange();
            }

        }

    } while (!mRequestExit);

    close(mSocketId);
}

bool NetworkRouteMonitor::sendRouteDumpRequest() {
    char msgBuf[NL_BUFFER_HEADER_SIZE];
    bzero(msgBuf, sizeof(msgBuf));

    struct nlmsghdr *nlMsg;
    static uint32_t msgSeq = 0;
    nlMsg = (struct nlmsghdr *) msgBuf;

    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));  // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE;   // Get the routes from kernel routing table .

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;    // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++;    // Sequence of the message packet.
    nlMsg->nlmsg_pid = pthread_self() << 16 | getpid();    // PID of process sending the request.
    if (send(mSocketId, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        LOG(ERROR) << "Route update request failed";
        return false;
    }
    return true;
}

NetworkRouteMonitor::routeInfo NetworkRouteMonitor::getRouteInfo(struct rtattr *rth, int rtl) const {
    char gateway_address[INET_ADDRSTRLEN];
    char interface[IF_NAMESIZE];
    bzero(gateway_address, sizeof(gateway_address));
    bzero(interface, sizeof(interface));

    routeInfo route{"","",0};

    for ( ; rtl && RTA_OK(rth, rtl); rth = RTA_NEXT(rth, rtl)) {
        switch(rth->rta_type) {
            case RTA_GATEWAY:
                inet_ntop(AF_INET, RTA_DATA(rth),
                          gateway_address, sizeof(gateway_address));
                route.gateway = gateway_address;
                break;
            case RTA_OIF:
                if_indextoname(*(int *) RTA_DATA(rth), interface);
                route.interface = interface;
                break;
            case RTA_PRIORITY:
                route.metric = *(uint32_t *) (RTA_DATA(rth));
                break;
            default:
                break;
        }
    }

    return route;
}

void NetworkRouteMonitor::dispatchDefaultRouteChange() {
    std::lock_guard<std::mutex> guard(mMutex);

    LOG(INFO) << "------------ default routes changed -----------";
    for(const auto& value: mDefaultRoutes) {
        LOG(INFO) << "   " << value.metric << " " << value.gateway << " " << value.interface;
    }
    LOG(INFO) << "-----------------------------------------------";
    LOG(INFO) << "";

    routeInfo route{"","",UINT16_MAX};
    if (mDefaultRoutes.size()) {
        route = mDefaultRoutes.front();
    }

    if (route.interface != mRoute.interface) {
        mRoute = route;
        for (auto it = nStateCbs.begin(); it != nStateCbs.end(); ) {

            if (it->expired())
                continue;

            auto p = it->lock();
            if (p) {
                p->defaultRouteChanged(mRoute);
                it++;
            } else {
                nStateCbs.erase(it++);
            }
        }

    }
}

NetworkRouteMonitor::~NetworkRouteMonitor() {
    stop();
}

NetworkRouteMonitor::NetworkRouteMonitor() {
    struct sigaction actions{};
    sigemptyset(&actions.sa_mask);
    actions.sa_flags = 0;
    actions.sa_handler = sighand;
    mDefaultRoutes.clear();

    if (sigaction(SIGUSR1, &actions, nullptr) != 0) {
        LOG(ERROR) << "Failed to install SIGUSR1 handler. Network monitoring will not work";
    }
}

NetworkRouteMonitor::NetworkRouteCallback::NetworkRouteCallback() {
}
