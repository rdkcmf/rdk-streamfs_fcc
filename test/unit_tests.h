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
#include <Demuxer.h>
#include <streamfs/PluginCallbackInterface.h>
#include "gmock/gmock.h"
#include "gtest/gtest.h"


class MockDemuxer : public Demuxer {

public:
    MockDemuxer(int demuxId) : Demuxer(demuxId) {};

    MOCK_METHOD0(init, void());

    MOCK_METHOD0(connect, int());

    MOCK_METHOD1(start, int(int identifier));

    MOCK_METHOD0(informFirstFrameReceived, void());

    MOCK_METHOD1(setDemuxerParameters, int(const std::string &params));

    MOCK_METHOD1(disconnect, int(int connectionId));

    MOCK_METHOD2(open, int(const std::string &uri, const std::string &hwInterface));

    MOCK_METHOD0(getGlobalStats, std::string());

    MOCK_METHOD1(getChannelStats, std::string(bool resetCounters));

    std::shared_ptr<DemuxerCallbackHandler> getCallbackHandler() override {
        return std::shared_ptr<DemuxerCallbackHandler>();
    };
};

class MockPluginCallbackInterface : public streamfs::PluginCallbackInterface {
public:
    MOCK_METHOD1(setAvailableStreams, void(const std::vector<std::string> &streamIds));

    MOCK_METHOD1(updateConfig, void(const PluginConfig &config));

    MOCK_METHOD0(getAvailableStreams, const std::vector<std::string> & ());

    MOCK_METHOD1(notifyUpdate, void(const std::string &path));
};

class TestCBHandler : public DemuxerCallbackHandler {
public:
    MOCK_METHOD1(notifyStreamSwitched, void(const std::string &channelIp));
};
