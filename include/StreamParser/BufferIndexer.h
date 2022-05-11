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
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <boost/circular_buffer.hpp>
#include <mutex>
#include <atomic>

namespace StreamParser {

enum BuffErr {
    BUF_OK,
    BUF_OUT_OF_RANGE,
    BUF_EMPTY
};

typedef std::pair<uint64_t, uint64_t> index_pair; // <time in us, accumulated buffer size>

class BufferIndexer {

public:
    /**
     * Buffer indexer constructor
     *
     * @param tsbSize        - the TSB size. Must match the size of TSB BufferPool.
     * @param tailSize       - the tail size of the TSB size, not to be used but required to
     *                         eliminate potential out-of-bound indexing when reading from
     *                         the oldest position in the TSB (i.e. in the scenario where the
     *                         player unexpected reduces the rate at which one or more buffer
     *                         are read).
     * @param samplingRatio  - integer defining the buffer register down-sampling ratio.
     */
    explicit BufferIndexer(uint64_t tsbSize, uint64_t tailSize, uint8_t samplingRatio);

    ~BufferIndexer();

    /**
     * Register the timestamp for a particular buffer count.
     *
     * @param size - the accumulated buffer count in bytes.
     * @return     - pair where the first value is a boolean that will be
     *               true if the accumulated buffer count is registered or
     *               false if it was discarded (as a result of down-sampling).
     *               The value will always be true if the samplingRatio is set
     *               to 1 in the constructor, meaning that the accumulated
     *               buffer count will always be registered. The second value
     *               is the current size of the circular buffer, after the
     *               accumulated buffer count was added.
     */
    std::pair<bool, uint64_t> registerBufferCount(uint64_t bufferCount);

    /**
     * Get the interpolated byte offset for a certain seek time, in microseconds.
     *
     * @param time       - seek time in us
     * @param byteOffset - byte offset associated with the seek time
     * @return           - error
     */
    BuffErr getByteOffsetFromTimeUs(uint64_t time, uint64_t& byteOffset);

    /**
     * Get the interpolated seek time, in microseconds, for a certain byte offset.
     *
     * @param byteOffset - byte offset
     * @param timeS      - seek time in us associated with the byte offset
     * @return           - error
     */
    BuffErr getTimeUsFromByteOffset(uint64_t byteOffset, uint64_t& time);

    /**
     * Get interpolated EPOC sample timestamp, in microseconds, for a certain
     * absolute byte index.
     *
     * @param byteIndex - byte index
     * @param timeS      - EPOC timestamp in us
     * @return           - error
     */
    BuffErr getTimestampUsForByteIndex(uint64_t byteIndex, uint64_t& time);

    /**
     * Get the current buffer size in microseconds.
     *
     * @return - size in seconds
     */
    uint64_t getIndexSizeInTimeUs();

    /**
     * Get the current buffer size in bytes.
     *
     * @return - size in bytes
     */
    uint64_t getIndexSizeInBytes();

    /**
     * Get the buffer capacity in number of chunks.
     *
     * @return - buffer capacity
     */
    uint64_t getBufferCapacity();

    /**
     * Clear the buffer indexer and associated member variables.
     *
     * @return      - void
     */
    void clear();

private:
    uint8_t mSamplingRatio;
    uint64_t mBufferCount;
    uint64_t mTsbSize;
    boost::circular_buffer<index_pair> mBufInd;
    index_pair mLastByteOffsetFromTimeUs;    // used in getByteOffsetFromTimeUs
    index_pair mLastTimeUsFromByteOffset;    // used in getTimeUsFromByteOffset
    std::mutex mIndexMutex;


    /**
     * Returns the size of the index buffer excluding the
     * the tail size, if the actual size is above mTsbSize.
     *
     * @return - size of TSB (excluding tail size).
     */
    inline size_t getIndexSize() const;

    /**
     * Return the front index buffer index indicating the
     * oldest TSB position. Values less than this are tail
     * TSB positions out side the actual exposed TSB range.
     *
     * @return - index defining the front of the index buffer.
     */
    inline size_t getFrontIndex() const;

    /**
     * Returns the oldest index_pair from the index buffer.
     *
     * @return - oldest index_pair.
     */
    inline index_pair getFront() const;

};

}

