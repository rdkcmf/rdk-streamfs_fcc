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

#include <curses.h>
#include <thread>
#include "YouseeChannels.h"
#include <mutex>
#include <list>
#include <chrono>

#define DRM0_UPDATE_DISPLAY_COUNT 21

class ChannelSelector {

public:
    void select(const char* fccPath, const char* channel);

    std::chrono::time_point<std::chrono::system_clock> getLastChanSwitchTime() {
        std::lock_guard<std::mutex> lockGuard(mTimeMeasureMtx);
        return mChannelSwitchStartTime;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> mChannelSwitchStartTime;
    std::mutex mTimeMeasureMtx;
};

class StreamFSInteractive {

public:
    explicit StreamFSInteractive(char *string);

    void drm0PollThread();
    void statCollectorThread();

    void showMenu();

    ~StreamFSInteractive();

private:
    // Separator between windows
    int mWinSepY;
    int mWinSepX;

    // path to fcc folder
    char* mFccPath;

    ChannelSelector mChannelSelector;

    // list of channels
    yousee::YouseeChannels& mChannels;

    // Windows
    WINDOW *mLeftTopWin;
    WINDOW *mRightTopWin;

    // path to channel selection
    std::string mChannelSelectorPath;

    std::thread* mInfoThread;

    std::thread* mStatsThread;

    bool mRun = true;

    std::basic_string<char> mDrm;

    std::string mSeekInfo;

    // Protect window updates
    std::mutex mWindowMtx;

    void updateRightWindow();

    // Recent changes in stream
    std::string mDrm0UpdateList;

    // Number of drm0 updates
    int mDrmCount = 0;

    void updateDrm0State(char* cfg);

    std::string mOldEcm;
    std::string mOldPmt;
    std::string mOldPat;
    std::string mChanSwitchTimeMs;
};
