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

#include <thread>
#include <condition_variable>
#include <atomic>

namespace StreamParser {
class ScheduledTask {
public:
    ScheduledTask() : mRunning(false), mRequestExit(false) {}
    /**
     * Schedule function call
     * @tparam T
     * @param function
     * @param delay
     */
    template<typename T>
    void schedule(T function, std::chrono::milliseconds delay);

    /**
     * Schedule call at fixed rate
     * @tparam T
     * @param func     - function to be called
     * @param initialDelay - initial delay in milliseconds
     * @param interval     - schedule interval in milliseconds
     */
    template<typename T>
    void scheduleFixedRate(T func,
                           std::chrono::milliseconds initialDelay,
                           std::chrono::milliseconds interval);
    /**
     * Stop scheduled task
     * @param waitExit - lock until time thread exit
     */
    void stop(bool waitExit);

private:
    std::mutex mWaitMutex;
    std::condition_variable mCv;
    std::atomic_bool mRunning;
    bool mRequestExit;

};
template<typename T>
void ScheduledTask::schedule(T function, std::chrono::milliseconds delay) {
    this->mRequestExit = false;
    std::thread t([=]() {
        if(this->mRequestExit)
            return;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        if(this->mRequestExit)
            return;
        function();
    });
    t.detach();
}

template<typename T>
void ScheduledTask::scheduleFixedRate(T func,
                                      std::chrono::milliseconds initialDelay,
                                      std::chrono::milliseconds interval) {
    this->mRequestExit = false;

    std::thread t([=]() {
        mRunning = true;
        if (initialDelay >  std::chrono::milliseconds (0)) {
            std::this_thread::sleep_for(initialDelay);
            if(this->mRequestExit) {
                mCv.notify_all();
                return;
            }
            func();
        }

        while(!this->mRequestExit) {
            std::this_thread::sleep_for(interval);
            if(this->mRequestExit)
                break;
            func();
        }
        mRunning = false;
        mCv.notify_all();
    });
    t.detach();
}

}
