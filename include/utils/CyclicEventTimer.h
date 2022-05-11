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
#include <chrono>
#include <atomic>
#include <thread>
#include <condition_variable>

/**
 * This class implements logic for cyclic invocation of a
 * closure in a separate thread.
 *
 * The closure itself is provided as argument to the constructor
 * and must return a boolean. When the return value is false
 * the cyclic invocation will continue, when the return value
 * is true the cyclic invocation will stop.
 *
 * Cyclic invocation can be (re)started (again) by calling the
 * start method and likewise stopped by calling the stop method.
 *
 * The period must be specified in milliseconds and set via
 * the setPeriod method.
 */
class CyclicEventTimer
{
public:
    CyclicEventTimer(std::function<bool(void)> func)
        : mStop(true)
        , mExit(false)
        , mPeriodMilliseconds(0)
        , mFunc(func) {
        mThread = std::shared_ptr<std::thread>(
                new std::thread(&CyclicEventTimer::threadLoop, this));
    }

    ~CyclicEventTimer() {
        mExit = true;
        stop();
        if(mThread->joinable()) {
            mThread->join();
        }
    }

    void setPeriodInMilliseconds(uint16_t time) {
        mPeriodMilliseconds = time;
    }

    bool start() {
        std::unique_lock<std::mutex> lk(mMutex);
        if (mFunc && mStop && mPeriodMilliseconds) {
            mStop = false;
            mCv.notify_one();
            return true;
        } else {
            return false;
        }
    }

    void stop() {
        std::unique_lock<std::mutex> lk(mMutex);
        mStop = true;
        mCv.notify_one();
    }

private:
    CyclicEventTimer(const CyclicEventTimer&) = delete;
    CyclicEventTimer& operator=(const CyclicEventTimer&) = delete;

    std::atomic<bool> mStop;
    std::atomic<bool> mExit;
    std::atomic<uint16_t> mPeriodMilliseconds;
    std::function<bool(void)> mFunc;
    std::shared_ptr<std::thread> mThread;
    std::condition_variable mCv;
    std::mutex mMutex;

    void threadLoop() {
        do {
            {
                std::unique_lock<std::mutex> lk(mMutex);
                mCv.wait(lk);
            }
            while (!mExit && !mStop) {
                if (mFunc()) {
                    mStop = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(mPeriodMilliseconds));
            }
        } while (!mExit);
    }
};
