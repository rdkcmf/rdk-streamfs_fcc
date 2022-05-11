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

#include "utils/TimeoutWatchdog.h"

TimeoutWatchdog::TimeoutWatchdog(uint32_t timeout, std::function<void(bool)> func)
        : mState (CLEAR)
        , mClear(true)
        , mStop(true)
        , mExit(false)
        , mNotified(false)
        , mTimeout(timeout)
        , mFunc(func) {
    mThread = std::make_shared<std::thread>(&TimeoutWatchdog::threadLoop, this);
}

TimeoutWatchdog::~TimeoutWatchdog() {
    mExit = true;
    clear();
    mCvStart.notify_one();
    if(mThread->joinable())
        mThread->join();
}

void TimeoutWatchdog::clear() {
    mState = CLEAR;
    mStop = true;
    mClear = true;
    restart();
}

void TimeoutWatchdog::start() {
    if (mStop) {
        mState = RUNNING;
        mClear = false;
        mStop = false;
        mCvStart.notify_one();
    }
}

void TimeoutWatchdog::stop() {
    if (!mStop) {
        mState = STOPPED;
        mStop = true;
        mFunc(false);
    }
}

void TimeoutWatchdog::restart() {
    mNotified = true;
    mCvStart.notify_one();
}

TimeoutWatchdog::StateType TimeoutWatchdog::getState() const{
    return mState;
}

void TimeoutWatchdog::threadLoop() {
    do {
        boost::mutex mutex;
        boost::unique_lock<boost::mutex> lock(mutex);
        mCvStart.wait(lock, [this](){ return !mStop || mExit; });

        while (!mStop && !mExit) {
            boost::chrono::duration<double> timeout = boost::chrono::milliseconds(mTimeout);

            bool status = mCvStart.wait_for(lock, timeout, [this](){ return mNotified.load(); });

            mNotified = false;
            if (!status && !mStop) {
                mState = EXPIRED;
                mStop = true;
                mFunc(true);
            }
        }
    } while (!mExit);
}
