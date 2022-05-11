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
#include <thread>
#include <boost/thread/condition.hpp>
#include <atomic>
/**
 * This class implements a simple watchdog timer that invokes a callback function
 * when the timer expires. The timeout value (in milliseconds) and callback function
 * are provided as arguments to the constructor.
 *
 * Following methods are implemented for controlling the watchdog:
 *
 * - clear: clearing the watchdog timer.
 * - start: starting the watchdog timer.
 * - restart: restarting the watchdog timer if it is already running / has not expired.
 * - stop: stopping the watchdog timer if it running / has not expired.
 *
 * The callback function will only be invoked when the watchdog timer expires or
 * is stopped if it is already running / has not expired. A boolean is provided as
 * parameter to the callback indicating if the timer has expired (true) or if it
 * was stopped (false) as a result of calling stop or clear.
 *
 * The state of the watchdog timer can be obtained at any time by calling getState.
 * The return value will by a StateType enum.
 */
class TimeoutWatchdog {
public:

    enum StateType {
        CLEAR,
        RUNNING,
        STOPPED,
        EXPIRED
    };

    TimeoutWatchdog(uint32_t timeout, std::function<void(bool)> func);
    ~TimeoutWatchdog();
    
    void clear();
    void start();
    void stop();
    void restart();
    StateType getState() const;

private:
    TimeoutWatchdog(const TimeoutWatchdog&) = delete;
    TimeoutWatchdog& operator=(const TimeoutWatchdog&) = delete;

    void threadLoop();

    std::atomic<StateType> mState;
    std::atomic<bool> mClear;
    std::atomic<bool> mStop;
    std::atomic<bool> mExit;
    std::atomic<bool> mNotified;
    uint32_t mTimeout;
    std::function<void(bool)> mFunc;

    std::shared_ptr<std::thread> mThread;

    boost::condition_variable mCvStart;
};
