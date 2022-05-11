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

#include "MediaSourceHandler.h"
#include <boost/filesystem.hpp>
#include <fstream>

#include "gtest/gtest.h"


static const char *MOUNT_DIR = "temp";
static const char *STREAMFS_LOG_FILE = "streamfs_test.log";
#define STREAM_SRC_FILE  "stream0.ts"
#define CHANNEL_SELECT_FILE "chan_select0"

typedef std::string ChannelId;
bool mounted_fs = true;

int setChannel(const std::string &path, const std::string chan) {

    FILE *f = fopen(path.c_str(), "w");

    if (f == nullptr) {
        return -1;
    }

    auto res = fwrite(chan.c_str(), chan.size(), 1, f);

    fclose(f);
    if (res != 1) {
        return -1;
    }

    return 0;
}

int setupMount() {
    int result;
    if (mounted_fs) {
        return 0;
    }
    boost::filesystem::create_directory(MOUNT_DIR);

    std::string umountCmd = "fusermount -u " + std::string(MOUNT_DIR);

    system(umountCmd.c_str());

    std::string cmd =
            "streamfs -o direct_io " + std::string(MOUNT_DIR) + " > " + std::string(STREAMFS_LOG_FILE) + " 2>&1";

    result = system(cmd.c_str());

    if (result != 0) {
        std::cout << "ERROR: Failed to mount steamfs director to: " << MOUNT_DIR;
    } else {
        mounted_fs = true;
    }
    return 0;
}

class Channels {
public:
    virtual std::vector<ChannelId> getChannelsIds() {
        printf("Getting channels\n");
        return std::vector<ChannelId>();
    }

    virtual std::string getMountPath(std::string fileName, std::string mountPoint) {
        auto path = boost::filesystem::path(mountPoint);
        path.append("/fcc");
        path.append(fileName);
        return path.string();
    }
};

class VirtualNetworkChannels : public Channels {
public:
    VirtualNetworkChannels() {
        setupMount();
    }

    std::vector<ChannelId> getChannelsIds() override {
        std::string streamfFile = "/home/root/streamfsvideofile.conf";
        const char *prefix = "http";
        ChannelId streamfsurl;
        std::ifstream file(streamfFile);
        if (file.good()) {
            std::string buf;
            while (file.good()) {
                std::getline(file, buf);
                std::size_t pos = buf.find(prefix);
                if (pos != std::string::npos) {
                    streamfsurl = buf.substr(pos);
                    mChannels.push_back(streamfsurl);
                }
            }
            file.close();
        }

        return mChannels;
    }

private:
    std::vector<ChannelId> mChannels;
};

class YouSeeNetworkChannels : public Channels {
public:
    YouSeeNetworkChannels() {
        setupMount();
    }

    std::vector<ChannelId> getChannelsIds() override {
        return mChannels;
    }

private:
    const std::vector<ChannelId> mChannels = {  /* TBD */ };
};

template<typename T>
struct ChannelTests : public testing::Test {
    using paramType = T;
};

template<>
struct ChannelTests<VirtualNetworkChannels> : public testing::Test {
    static VirtualNetworkChannels channels;
};

template<>
struct ChannelTests<YouSeeNetworkChannels> : public testing::Test {
    static YouSeeNetworkChannels channels;
};

VirtualNetworkChannels ChannelTests<VirtualNetworkChannels>::channels;
YouSeeNetworkChannels ChannelTests<YouSeeNetworkChannels>::channels;

typedef ::testing::Types<YouSeeNetworkChannels,
        VirtualNetworkChannels> Implementations;

TYPED_TEST_CASE(ChannelTests, Implementations);

TYPED_TEST(ChannelTests, OpenChannel) {
    auto channelIds = this->channels.getChannelsIds();
    int readLen = 4 * 1024;
    char buf[readLen];

    if (channelIds.size() == 0) {
        std::cout << "No channels defined. Skipping test" << std::endl;
        return;
    }

    auto firstChannel = channelIds[0];

    auto channelLocation = this->channels.getMountPath(STREAM_SRC_FILE, MOUNT_DIR);
    auto chanSelectFile = this->channels.getMountPath(CHANNEL_SELECT_FILE, MOUNT_DIR);

    ASSERT_EQ(setChannel(chanSelectFile, channelIds[0]), 0) << "Failed to set channel:" << channelIds[0];

    FILE *f = fopen(channelLocation.c_str(), "r");

    ASSERT_NE(f, nullptr) << "Failed to open channel location: " << channelLocation;

    auto chunkReads = fread(buf, readLen, 1, f);

    ASSERT_EQ(chunkReads, 1);
    fclose(f);
}



TYPED_TEST(ChannelTests, OpenChannelMultipleChannels_300X) {
    auto channelIds = this->channels.getChannelsIds();
    int readLen = 4 * 1024;
    char buf[readLen];
    int switchCount = 300;

    if (channelIds.empty()) {
        std::cout << "No channels defined. Skipping test" << std::endl;
        return;
    }

    auto chanSelectFile = this->channels.getMountPath(CHANNEL_SELECT_FILE, MOUNT_DIR);
    auto channelLocation = this->channels.getMountPath(STREAM_SRC_FILE, MOUNT_DIR);
    FILE *f = fopen(channelLocation.c_str(), "r");
    ASSERT_NE(f, nullptr) << "Failed to open channel location: " << channelLocation;

    while (switchCount > 0) {
        for (const auto& channel : channelIds) {
            ASSERT_EQ(setChannel(chanSelectFile, channel), 0) << "Failed to set channel:" << channel;
            auto bytesRead = fread(buf, readLen, 1, f);

            std::this_thread::sleep_for(std::chrono::seconds(1));

            ASSERT_EQ(bytesRead, 1);
            switchCount--;
            if (switchCount == 0)
                break;
        }
    }
    fclose(f);
}
