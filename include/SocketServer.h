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

#include <set>
#include <thread>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <atomic>

/**
 * This class implement a TCP stream socket server for providing
 * buffer data to one or more clients.
 *
 * It provides methods for start and stop the server and for
 * sending buffer data. See method descriptions
 *
 * The class also implements two threads:
 *
 * - acceptLoop: responsible for accepting new client connections
 *   and storing the file descriptors in mClientFd (std::set)
 *
 * - writeLoop: responsible for sending buffer data over the socket to
 *   the clients. Buffer data is read from a queue (mQueue) where new
 *   buffer data is queued using the send-method. Any invalid file
 *   descriptors will also b closed and removed in the writeLoop.
 */
class SocketServer {
public:

    SocketServer();
    ~SocketServer();

    /**
     * Start TCP stream socket server.
     *
     * @param port - The port to use.
     * @return boolean indicating if the socket was opened (true) or not (false)
     */
    bool start(uint16_t port);

    /**
     * Stop TCP stream socket server and closes any client connections.
     *
     * @return boolean indicating if the socket was closed (true) or not (false)
     */
    bool stop();

    /**
     * Send data to the socket clients. The data will be buffered in mQueue
     * and send in the writeLoop thread.
     *
     * @param buf - pointer to buffer containing the data.
     * @param size - size of buffer in number of bytes.
     * @return boolean indicating if the buffer was send (true) or not (false)
     */
    bool send(const void *buf, size_t size);

private:

    void acceptLoop();
    void writeLoop();

    void pushBuffer(const std::string &buf);
    std::string popBuffer();

    int mServerFd;
    std::set<int> mClientFd;

    std::atomic<bool> mIsRunning;
    std::mutex mStartStopMtx;

    std::queue <std::string> mQueue;
    mutable std::mutex mQueueMtx;
    std::condition_variable mCond;

    std::shared_ptr<std::thread> mAcceptThread;
    std::shared_ptr<std::thread> mWriteThread;
};
