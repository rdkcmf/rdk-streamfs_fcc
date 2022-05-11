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

#include "SocketServer.h"

#include <glog/logging.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <signal.h>

// Anonymous namespace for class helper functions
namespace {

inline int isFdValid(int fd)
{
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

}

SocketServer::SocketServer()
    : mServerFd(-1)
    , mIsRunning(false) {
}

SocketServer::~SocketServer() {
    stop();
}

bool SocketServer::start(uint16_t port)
{
    std::lock_guard <std::mutex> lock(mStartStopMtx);

    if (mIsRunning) {
        return false;
    }

    // Ignore SIGPIPE signals on the socket
    signal(SIGPIPE, SIG_IGN);

    // Create TCP stream socket
    mServerFd = socket(AF_INET, SOCK_STREAM, 0);
    if (mServerFd < 0) {
        LOG(ERROR) << "Could not open socket: " << mServerFd;
        return false;
    }

    // Set option to reuse the port to avoid bind from failing for the scenario,
    // where the server has been shut down and then restarted right away.
    int enable = 1;
    if (setsockopt(mServerFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        LOG(ERROR) << "setsockopt(SO_REUSEADDR) failed";
        return false;
    }

    // Bind socket to the address
    struct sockaddr_in address;
    bzero((char *) &address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(mServerFd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        LOG(ERROR) << "Error on binding";
        return false;
    }

    // Listen for a connection on the socket
    if (listen(mServerFd,5) < 0) {
        LOG(ERROR) << "Error on listen";
        return false;
    }

    mIsRunning = true;

    // Create thread for handling client connection acccept
    mAcceptThread = std::shared_ptr<std::thread>(
            new std::thread(&SocketServer::acceptLoop, this));

    // Create thread for writing buffer to the clients
    mWriteThread = std::shared_ptr<std::thread>(
            new std::thread(&SocketServer::writeLoop, this));

    return true;
}

bool SocketServer::stop() {
    std::lock_guard <std::mutex> lock(mStartStopMtx);

    if (!mIsRunning) {
        return false;
    }

    mIsRunning = false;

    // Push an empty buffer to the write queue to unblock the wait in the writeLoop thread
    pushBuffer(std::string());

    // apparently the only way to get the blocking accept request to terminate (with -1)
    // sigaction failed doing so.
    if (shutdown(mServerFd, SHUT_RD) < 0) {
        LOG(ERROR) << "Error on shutdown";
    }

    for (auto fd : mClientFd)
    {
        close(fd);
    }
    close(mServerFd);

    if (mAcceptThread->joinable()) {
        mAcceptThread->join();
    }

    if (mWriteThread->joinable()) {
        mWriteThread->join();
    }

    return true;
}

bool SocketServer::send(const void *buf, size_t size) {
    if (!mIsRunning) {
        return false;
    }

    pushBuffer(std::string(static_cast<const char*>(buf), size));
    return true;
}

void SocketServer::pushBuffer(const std::string &buf) {
    std::unique_lock<std::mutex> lock(mQueueMtx);
    mQueue.push(buf);
    mCond.notify_one();
}

std::string SocketServer::popBuffer() {
    std::unique_lock<std::mutex> lock(mQueueMtx);
    this->mCond.wait(lock, [=]{ return !mQueue.empty(); });
    std::string res = mQueue.front();
    mQueue.pop();
    return res;
}

void SocketServer::writeLoop() {
    while (mIsRunning) {
        std::string buf = popBuffer();
        if (mIsRunning && buf.size()) {
            // iterate over client file descriptors
            for(auto itr = mClientFd.cbegin(); itr != mClientFd.cend();) {
                if (isFdValid(*itr)) {
                    // if the client file descriptor is valid, send the buffer data over the socket
                    auto status = write(*itr, buf.c_str(), buf.size());
                    if ( status < 0) {
                        LOG(ERROR) << "Error writing to file descriptor: " << *itr << " status=" << status << std::endl;
                        close(*itr);
                        itr = mClientFd.erase(itr);
                    } else {
                        ++itr;
                    }

                } else {
                    // if the client file descriptor is invalid, remove it from the set of descriptors
                    LOG(INFO) << "Found invalid file descriptor " << *itr << "\n";
                    close(*itr);
                    itr = mClientFd.erase(itr);
                }
            }
        }

    }
    LOG(INFO) << "writeLoop stopped";
}

void SocketServer::acceptLoop() {
    // Accept new socket connection

    while (mIsRunning) {
        struct sockaddr_in address;
        socklen_t length = sizeof(address);
        int fd = accept(mServerFd, (struct sockaddr *) &address, &length);
        if (fd < 0) {
            LOG(INFO) << "Error on accept" << std::endl;
        } else {
            mClientFd.insert(fd);
        }
    }

    LOG(INFO) << "acceptLoop stopped";
}
