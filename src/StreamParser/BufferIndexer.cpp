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


#include "StreamParser/BufferIndexer.h"
#include "StreamParser/ScheduledTask.h"
#include <iostream>
#include <glog/logging.h>

namespace StreamParser {

// bisection search predicate functions declared in anonymous namespace
namespace {
    bool byteOffsetSearch (uint64_t target,index_pair current) { return target < current.first; }
    bool timeSearch (uint64_t target,index_pair current) { return target < current.second; }
}

BufferIndexer::BufferIndexer(uint64_t tsbSize, uint64_t  tailSize, uint8_t samplingRatio)
    : mSamplingRatio(samplingRatio)
    , mBufferCount(0)
    , mTsbSize((tsbSize-tailSize) / samplingRatio)
    , mBufInd(tsbSize / samplingRatio)
    , mLastByteOffsetFromTimeUs(std::make_pair(0,0))
    , mLastTimeUsFromByteOffset(std::make_pair(0,0))
{
}

BufferIndexer::~BufferIndexer() {
}

std::pair<bool, uint64_t> BufferIndexer::registerBufferCount(uint64_t bufferCount) {
    std::lock_guard<std::mutex> mLock(mIndexMutex);

    uint64_t currentTime = std::chrono::duration_cast< std::chrono::microseconds >(
            std::chrono::high_resolution_clock::now().time_since_epoch()
    ).count();

    if (mBufferCount++ % mSamplingRatio == 0) {
        mBufInd.push_back(std::make_pair(currentTime, bufferCount));
        return std::make_pair(true, mBufInd.size());
    }

    return std::make_pair(false, mBufInd.size());
}

BuffErr BufferIndexer::getByteOffsetFromTimeUs(uint64_t time, uint64_t& byteOffset) {
    std::lock_guard<std::mutex> mLock(mIndexMutex);

    // Bail out with BUF_EMPTY if the buffer is empty
    if (mBufInd.empty()) {
        return BUF_EMPTY;
    }

    // Return previous derived byte offset if the current time equals the previous time
    if (time == mLastByteOffsetFromTimeUs.first) {
        byteOffset = mLastByteOffsetFromTimeUs.second;
        return BUF_OK;
    }

    // If seek time is zero (i.e. live point), set the byte offset to zero.
    if (time == 0) {
        byteOffset = 0;
        mLastByteOffsetFromTimeUs = std::make_pair(0,0);
        return BUF_OK;
    }

    // If seek time exceed the capture range, set the byte offset to the
    // maximum byte offset, captured in the buffer.
    if (getFront().first + time > mBufInd.back().first) {
        byteOffset = mBufInd.back().second - getFront().second;
        mLastByteOffsetFromTimeUs = std::make_pair(time, byteOffset);
        return BUF_OUT_OF_RANGE;
    }

    LOG(INFO) << "::getBufferByteOffset - find byte offset for " << time << "us";

    auto begin = mBufInd.begin() + getFrontIndex();
    uint64_t timeTarget = mBufInd.back().first - time;
    auto it=std::upper_bound (begin, mBufInd.end(), timeTarget, byteOffsetSearch);

    if (it == mBufInd.end()) {
        return BUF_OUT_OF_RANGE;
    }

    std::size_t i = std::distance(mBufInd.begin(), it);

    // Calculate the index using interpolation
    float gradient = 1.0 / (int64_t)(mBufInd[i - 1].first - mBufInd[i].first);
    uint64_t index = i + gradient * (float)(time - (mBufInd.back().first - mBufInd[i].first));

    // Calculate and set the byte offset associated with the seek time
    byteOffset = mBufInd.back().second - mBufInd[index].second;
    mLastByteOffsetFromTimeUs = std::make_pair(time, byteOffset);

    LOG(INFO) << "Completed: byteOffset=" << byteOffset;

    return BUF_OK;
}

BuffErr BufferIndexer::getTimeUsFromByteOffset(uint64_t byteOffset, uint64_t& time) {
    std::lock_guard<std::mutex> mLock(mIndexMutex);

    // Bail out with BUF_EMPTY if the buffer is empty
    if (mBufInd.empty()) {
        return BUF_EMPTY;
    }

    // Return previous derived time if the current byte offset equals the previous byte offset
    if (byteOffset == mLastTimeUsFromByteOffset.first) {
        time = mLastTimeUsFromByteOffset.second;
        return BUF_OK;
    }

    // If byte offset is zero (i.e. live point), set offset time to zero as well.
    if (byteOffset == 0) {
        time = 0;
        mLastTimeUsFromByteOffset = std::make_pair(0, 0);
        return BUF_OK;
    }

    // If byte offset exceed the capture range, set the time to the
    // maximum seek time, captured in the buffer.
    if (getFront().second + byteOffset > mBufInd.back().second) {
        time = mBufInd.back().first - getFront().first;
        mLastTimeUsFromByteOffset = std::make_pair(byteOffset, time);
        return BUF_OUT_OF_RANGE;
    }

    LOG(INFO) << "::getTimeUsFromByteOffset - find seek time for " << byteOffset;

    auto begin = mBufInd.begin() + getFrontIndex();
    uint64_t targetOffset = mBufInd.back().second - byteOffset;
    auto it=std::upper_bound (begin, mBufInd.end(), targetOffset, timeSearch);

    if (it == mBufInd.end()) {
        return BUF_OUT_OF_RANGE;
    }

    std::size_t i = std::distance(mBufInd.begin(), it);

    // Calculate the absolute seek time using linear interpolation
    float gradient = (float)(mBufInd[i].first - mBufInd[i-1].first) / (mBufInd[i].second - mBufInd[i - 1].second);
    uint64_t absTime = mBufInd[i - 1].first + (uint64_t)(gradient * (float)(targetOffset - mBufInd[i - 1].second));

    // Calculate relative seek time and convert to seconds
    time = mBufInd.back().first - absTime;
    mLastTimeUsFromByteOffset = std::make_pair(byteOffset, time);

    LOG(INFO) << "Completed: time=" << time << "us";

    return BUF_OK;
}

BuffErr BufferIndexer::getTimestampUsForByteIndex(uint64_t byteIndex, uint64_t& time) {
    std::lock_guard<std::mutex> mLock(mIndexMutex);

    // Bail out with BUF_EMPTY if the buffer is empty
    if (mBufInd.empty()) {
        return BUF_EMPTY;
    }

    // If the byte index is no longer in the buffer, set the time to the
    // oldest timestamp available.
    if (mBufInd.front().second >= byteIndex) {
        time = mBufInd.front().first;
        return mBufInd.front().second > byteIndex ? BUF_OUT_OF_RANGE : BUF_OK;
    }

    // If the byte index is larger that the total bytecount, set the time
    // to the newest (live-point) timestamp.
    if (mBufInd.back().second <= byteIndex) {
        time = mBufInd.back().first;
        return mBufInd.back().second < byteIndex ? BUF_OUT_OF_RANGE : BUF_OK;
    }

    LOG(INFO) << "::getTimestampUsForByteIndex - find timestamp for " << byteIndex;

    auto it=std::upper_bound (mBufInd.begin(), mBufInd.end(), byteIndex, timeSearch);

    if (it == mBufInd.end()) {
        return BUF_OUT_OF_RANGE;
    }

    std::size_t i = std::distance(mBufInd.begin(), it);

    // Calculate the EPOC timestamp for the byteIndex using linear interpolation
    float gradient = (float)(mBufInd[i].first - mBufInd[i-1].first) / (mBufInd[i].second - mBufInd[i - 1].second);
    time = mBufInd[i - 1].first + (uint64_t)(gradient * (float)(byteIndex - mBufInd[i - 1].second));

    LOG(INFO) << "Completed: timestamp=" << time << "us";

    return BUF_OK;
}

uint64_t BufferIndexer::getIndexSizeInTimeUs() {
    std::lock_guard<std::mutex> lockGuard(mIndexMutex);

    if (mBufInd.empty()) {
        return 0;
    }
    return (mBufInd.back().first - getFront().first);
}

uint64_t BufferIndexer::getIndexSizeInBytes() {
    std::lock_guard<std::mutex> lockGuard(mIndexMutex);

    if (mBufInd.empty()) {
        return 0;
    }

    return mBufInd.back().second - getFront().second;
}

uint64_t BufferIndexer::getBufferCapacity() {
    std::lock_guard<std::mutex> lockGuard(mIndexMutex);
    return mTsbSize;
}

void BufferIndexer::clear() {
    std::lock_guard<std::mutex> lockGuard(mIndexMutex);
    mBufferCount = 0;
    mBufInd.clear();
}

inline size_t BufferIndexer::getIndexSize() const{
    uint64_t actualSize = mBufInd.size();
    return actualSize > mTsbSize ? mTsbSize : actualSize;
}

inline size_t BufferIndexer::getFrontIndex() const{
    return mBufInd.size() - getIndexSize();
}

inline index_pair BufferIndexer::getFront() const{
    uint64_t index = mBufInd.size() - getIndexSize();
    return mBufInd[index];
}

}
