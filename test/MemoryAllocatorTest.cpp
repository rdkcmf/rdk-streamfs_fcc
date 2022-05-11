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

#include <fcntl.h>
#include <limits.h>
#include <mutex>
#include <string>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <thread>

#include "MemoryAllocatorTest.h"
#include "gtest/gtest.h"

#define TSB_SIZE_MB 1024
#define TSB_SLICE "tsb.slice"

bool memory_pressure_good = true;

typedef void(*boundaryReachedCbType)();

void MemoryCbListener() {
    printf("Reached cb limit\n");
}

/**********************************************
 * Test for validating cgroup memory limits
 **********************************************/
int memory_pressure_listener(boundaryReachedCbType cb) {
    int evFd;
    int cfd;

    int event_control;
    char event_control_path[PATH_MAX];
    char line[LINE_MAX];
    int ret;

    uint64_t result;

    std::string pFile = "/sys/fs/cgroup/memory/" + std::string(TSB_SLICE) + "/memory.pressure_level";
    std::string evControlFile = "/sys/fs/cgroup/memory/" + std::string(TSB_SLICE) + "/cgroup.event_control";

    cfd = open(pFile.c_str(), O_RDONLY);

    if (cfd == -1) {
        std::cerr << "Cannot open " << pFile << std::endl;
        return -1;
    }

    event_control = open(evControlFile.c_str(), O_WRONLY);

    if (event_control == -1) {
        std::cerr << "Cannot open " << evControlFile << std::endl;
        close(cfd);
        return -1;
    }

    evFd = eventfd(0, 0);

    if (evFd == -1) {
        std::cerr << "eventfd() failed \n";
        goto fail;
    }

    ret = snprintf(line, LINE_MAX, "%d %d %s", evFd, cfd, "medium");

    if (ret >= LINE_MAX) {
        std::cerr << "Arguments string is too long\n";
        goto fail;
    }

    ret = write(event_control, line, strlen(line) + 1);

    if (ret == -1) {
        std::cerr << "Cannot write cgroup.event_control\n";
    }
    while (1) {
        std::cout << "Waiting for memory notifier callback\n";
        ret = read(evFd, &result, sizeof(result));
        if (ret == -1) {
            if (errno == EINTR)
                continue;
            std::cerr << "Cannot read from eventfd\n";
        }

        memory_pressure_good = false;

        assert(ret == sizeof(result));

        ret = access(event_control_path, W_OK);

        if ((ret == -1) && (errno == ENOENT)) {
            std::cerr << "Cgroup destroyed\n";
            break;
        }

        if (ret == -1)
            std::cerr << "cgroup destroyed\n";
        cb();
    }

    close(cfd);
    close(event_control);
    return 0;

    fail:
    close(cfd);
    close(event_control);
    return -1;
}

/**
 * Allocator for malloc memory testing
 */
class Allocator {
public:
    Allocator() {
        mfd = open("/dev/urandom", O_RDONLY);
    }

    /**
     * Allocate buffer
     * @param size_kb
     * @return - return allocation time in ns
     */
    __attribute__((optimize("O0")))
    int64_t allocate(uint32_t size_kb) {
        std::lock_guard<std::mutex> lockGuard(mMtx);
        if (!memory_pressure_good) {
            std::cout << "Skipp alloc. Memory pressure medium.\n";
            return -2;
        }
        auto start = std::chrono::high_resolution_clock::now();
        volatile char *addr = (char *) malloc(1024 * size_kb);
        memset((void *) addr, 'x', 1024 * size_kb);
        auto finish = std::chrono::high_resolution_clock::now();
        addr[0] = 1;
        if (addr != nullptr) {
            mBuffers.push_back(addr);
            allocSizeKb += size_kb;
            return std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
        } else {
            return -1;
        }
    }

    void releaseAll() {
        std::lock_guard<std::mutex> lockGuard(mMtx);
        for (auto d : mBuffers) {
            free((void *) d);
        }

        mBuffers.clear();
    }

    uint64_t getAllocSize() {
        return allocSizeKb;
    }

    ~Allocator() {
        int sum = 0;
        for (int i = 0; i < mBuffers.size(); i++) {
            sum = (sum + (int) mBuffers[i][0]) % 1000;
        }
        //Gcc and the Kernel will optimize out the
        //malloc if the memory is not used, even in case of O0.
        // Here we make sure we touch the memory region to avoid
        // any kernel and runtime optimizatins.
        printf("val= %d\n", sum);
        releaseAll();
        close(mfd);
    }

private:
    int mfd;
    std::vector<volatile char *> mBuffers;
    uint64_t allocSizeKb = 0;
    std::mutex mMtx;
};

/**
 * Consider TSB allocation ok if the we are able to allocate
 * at least 90% of the TSB_SIZE_MB;
 */

TEST(AllocatorTest, one_gb_memory_pressure_test) {
    Allocator allocator;
    uint64_t time = 0;
    int allocCount = 0;
    bool allocFailed = false;
    std::thread t1(memory_pressure_listener, MemoryCbListener);

    std::cout << "starting memory allocator\n";
    for (int i = 0; i < TSB_SIZE_MB /*1GB*/; i++) {
        allocCount += 1;
        //std::this_thread::sleep_for(std::chrono::milliseconds(20));
        auto t = allocator.allocate(1024);
        if (t < 0) {
            allocFailed = true;
            break;
        } else {
            time += t;
        }
    }

    if (allocFailed) {
        std::cout << "Alloc failed after number of alloc: " << allocCount << std::endl;
        ASSERT_GE(allocCount, 0.9 * TSB_SIZE_MB);
    } else {
        std::cout << "[-> Total time spent with malloc :" << time / 1000 << "us" << std::endl;
    }
    while (1 != 0) {
        sleep(1);
    }
    t1.join();
};

/**
 * Consider TSB allocation if we are allowed to allocate
 * at more than 110% of the TSB_SIZE_MB;
 */

TEST(AllocatorTest, TSB_size_exceeded) {
    Allocator allocator;
    uint64_t time = 0;
    int allocCount = 0;
    bool allocFailed = false;

    for (int i = 0; i < TSB_SIZE_MB; i++) {
        allocCount += 1;
        auto t = allocator.allocate(1024);
        if (t < 0) {
            allocFailed = true;
            break;
        } else {
            time += t;
        }
    }

    if (!allocFailed) {
        std::cout << "Total time spent with malloc :" << time / 1000 << "us" << std::endl;
        ASSERT_LE(allocCount, TSB_SIZE_MB * 1.10);
    }
}
