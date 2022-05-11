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

#ifndef STREAMFS_BUFFERQUEUE_H
#define STREAMFS_BUFFERQUEUE_H

#include <cstdint>
#include <queue>
#include <condition_variable>

#define QUEUE_SIZE    2

/**
 * Generic producer/consumer queue
 * @tparam T
 * @tparam queueSize
 */

template<typename T, unsigned long queueSize = QUEUE_SIZE>
class BufferQueue {
    std::condition_variable fillBuffCond;
    std::condition_variable emptyBuffCond;
    std::mutex fillM;
    std::mutex emptyM;
    std::queue<T *> consumerQueue;
    std::queue<T *> producerQueue;
    size_t mMaxDepth;
    bool exitPending = false;
public:
    BufferQueue() : mMaxDepth(queueSize) {}

    uint32_t getQueueSize() { return mMaxDepth; }

    void queue(T *req) {
        std::unique_lock<std::mutex> lock(fillM);
        fillBuffCond.wait(lock, [this]() { return !isConsumerQueueFull() || exitPending; });

        if (exitPending)
            return;

        consumerQueue.push(req);
        lock.unlock();
        fillBuffCond.notify_all();
    }

    void acquire(T **req) {
        std::unique_lock<std::mutex> lock(emptyM);
        emptyBuffCond.wait(lock, [this]() { return !isProducerQueueEmpty() || exitPending; });

        if (exitPending)
            return;

        *req = producerQueue.front();
        producerQueue.pop();
        lock.unlock();
        emptyBuffCond.notify_all();
    }

    void consume(T **req) {
        std::unique_lock<std::mutex> lock(fillM);
        fillBuffCond.wait(lock, [this]() { return !isConsumerQueueEmpty() || exitPending; });

        if (exitPending)
            return;

        *req = consumerQueue.front();
        consumerQueue.pop();
        lock.unlock();
        fillBuffCond.notify_all();
    }

    bool consume(T **req, std::chrono::duration<int64_t> timeout) {
        std::unique_lock<std::mutex> lock(fillM);

        bool done = fillBuffCond.wait_for(lock, timeout, [this]() { return !isConsumerQueueEmpty() || exitPending; });

        if (exitPending)
            return false;

        if (!done)
            return false;
        *req = consumerQueue.front();
        consumerQueue.pop();
        lock.unlock();
        fillBuffCond.notify_all();
        return true;
    }


    void release(T *req) {
        std::unique_lock<std::mutex> lock(emptyM);
        emptyBuffCond.wait(lock, [this]() { return !isProducerQueueFull() || exitPending; });
        producerQueue.push(req);
        lock.unlock();
        emptyBuffCond.notify_all();
    }

    bool isConsumerQueueFull() const {
        return consumerQueue.size() >= mMaxDepth;
    }

    bool isConsumerQueueEmpty() const {
        return consumerQueue.size() == 0;
    }

    bool isProducerQueueFull() const {
        return producerQueue.size() >= mMaxDepth;
    }

    bool isProducerQueueEmpty() const {
        return producerQueue.size() == 0;
    }

    void clear() {
        std::unique_lock<std::mutex> lock(fillM);
        while (!isConsumerQueueEmpty()) {
            consumerQueue.pop();
        }
        fillBuffCond.notify_all();
        while (!isProducerQueueEmpty()) {
            producerQueue.pop();
        }
        lock.unlock();
        emptyBuffCond.notify_all();
    }

    ~BufferQueue() {
        clear();
        exitPending = true;
        emptyBuffCond.notify_all();
        fillBuffCond.notify_all();
    }
};

#endif //STREAMFS_BUFFERQUEUE_H
