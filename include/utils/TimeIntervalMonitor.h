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

#include <chrono>
#include <mutex>

/**
 * This class implements logic for simple time interval measurement
 * and accumulation. It provides the necessary methods for
 * initialization, updating current time interval and for getting the
 * accumulated time for current and previous time intervals.
 */
class TimeIntervalMonitor {
public:

    TimeIntervalMonitor();

    /**
     * Reset the class variables and initializes the interval and
     * accumulation time to zero.
     */
    void reset();

    /**
     * Stop current time interval registration. The accumulated
     * interval time will remain unchanged.
     */
    void stopTimeInterval();

    /**
     * Update current time interval, defined as the delta time
     * between the current time and the time when the interval
     * was initialized (which happens if reset or
     * stopTimeInterval is called).
     */
    void updateTimeInterval();

    /**
     * Returns the accumulated time of previous time intervals
     * (since reset) and the current time interval.
     * The resolution is in microseconds.
     *
     * @return accumulated time in microseconds
     */
    uint64_t getAccumulatedTimeInMicroSeconds();

private:
    bool mRunning;
    std::chrono::high_resolution_clock::time_point mInitTime;
    std::chrono::high_resolution_clock::time_point mLastTime;
    std::chrono::microseconds mDeltaTime;
    std::mutex mMutex;
};
