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

#include <string.h>
#include "unit_tests.h"
#include "gtest/gtest.h"
#include "FCCPlugin.h"
#include "glog/logging.h"
#include "utils/ConstDelayDefHandler.h"
#include <thread>
#include <streamfs/ByteBufferPool.h>
#include "utils/MonitoredVariable.h"
#include "utils/TimeIntervalMonitor.h"

debug_options_t dDebugOptions{0xff,0};

class TestCore : public ::testing::Test {
};

class SeekTests : public TestCore {
public:

    SeekTests() {
        mTestCbHandler = std::shared_ptr<TestCBHandler>(new TestCBHandler());
        mMockDemuxer = new MockDemuxer(demuxerId);
        mMockPluginInterface = new MockPluginCallbackInterface();

        EXPECT_CALL(*mMockPluginInterface, setAvailableStreams(testing::_))
                .Times(testing::AtLeast(1));

        EXPECT_CALL(*mMockDemuxer, init())
                .Times(testing::AtLeast(1));

        mPlugin = new streamfs::FCCPlugin(mMockPluginInterface, mMockDemuxer, mTestCbHandler, &dDebugOptions);
    }

    ~SeekTests() override {
        delete mPlugin;
        delete mMockDemuxer;
        delete mMockPluginInterface;
    }

public:
    std::shared_ptr<TestCBHandler> mTestCbHandler;
    MockPluginCallbackInterface *mMockPluginInterface;
    const int demuxerId = 0;
    MockDemuxer *mMockDemuxer;
    streamfs::FCCPlugin *mPlugin;

    const std::vector<std::string> EXPECTED_STREAM_IDS
        = {"chan_select0", "seek0", "stream0.ts"};

protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();

    }
};

TEST_F(SeekTests, DISABLED_VerifyPluginSetup) {
    // Validate demuxer ID was set
    ASSERT_EQ(mTestCbHandler->getDemuxerId(), demuxerId);
}

TEST_F(SeekTests, DISABLED_PuhsSingleBuffer) {
    // Validate demuxer ID was set
    EXPECT_CALL(*mMockDemuxer, informFirstFrameReceived())
            .Times(testing::AtLeast(1));

    auto sH = mPlugin->getSourceHandler();
    auto buffer = sH->acquireBuffer(0);
    sH->pushBuffertoBQ(buffer, demuxerId);
}

TEST_F(SeekTests, DISABLED_GetSingleBuffer) {
    // Validate demuxer ID was set

    EXPECT_CALL(*mMockDemuxer, informFirstFrameReceived())
            .Times(testing::AtLeast(1));
    std::string path("stream0.ts");

    auto sH = mPlugin->getSourceHandler();
    auto buffer = sH->acquireBuffer(0);
    buffer->size = FEIP_BUFFER_SIZE;
    memset(buffer->buffer, 1, FEIP_BUFFER_SIZE);

    sH->pushBuffertoBQ(buffer, demuxerId);

    // Producer always writes in BUFFER_CHUNK_SIZE sizes.
    // readSize is the max number of bytes we may have
    // in the buffer pool
    int readSize = FEIP_BUFFER_SIZE - FEIP_BUFFER_SIZE % BUFFER_CHUNK_SIZE;
    char outBuffer[readSize];

    mPlugin->read(0, path, &outBuffer[0], readSize, 0);

    EXPECT_EQ(memcmp(&outBuffer[0], buffer->buffer, readSize), 0);
}

TEST_F(SeekTests, DISABLED_VerifySeekStatus) {

    // Validate demuxer ID was set

    EXPECT_CALL(*mMockDemuxer, informFirstFrameReceived())
            .Times(testing::AtLeast(1));
    std::string path("stream0.ts");
    int seek_count = 10;
    int totalWriteBytes = seek_count * FEIP_BUFFER_SIZE;
    char buffCopy[seek_count][FEIP_BUFFER_SIZE];

    for (int i = 0; i < seek_count; i++) {
        auto sH = mPlugin->getSourceHandler();
        auto buffer = sH->acquireBuffer(0);
        buffer->size = FEIP_BUFFER_SIZE;
        memset(&buffCopy[i], i, FEIP_BUFFER_SIZE);
        memcpy(buffer->buffer, &buffCopy[i], FEIP_BUFFER_SIZE);
        sH->pushBuffertoBQ(buffer, demuxerId);
    }

    // Producer always writes in BUFFER_CHUNK_SIZE sizes.
    // readSize is the max number of bytes we may have
    // in the buffer pool
    int readSize = FEIP_BUFFER_SIZE - FEIP_BUFFER_SIZE % BUFFER_CHUNK_SIZE;
    char outBuffer[readSize];
    mPlugin->read(0, path, &outBuffer[0], readSize, 0);
    EXPECT_EQ(memcmp(&outBuffer[0], buffCopy[0], readSize), 0);

    // Wait for the ring buffer for populating the the last buffers
    // TODO: add sync point
    sleep(1);

    char seek_info[256];
    auto outReadSize = mPlugin->read(0, CONFIG_F_SEEK_CONTROL, seek_info, 256, 0);
    ASSERT_GE(outReadSize, 0);
    seek_info[outReadSize] = '\0';
    std::string seekExpected = "0,3"; //All buffers are pushed in one second. The size should be the minimum seek time.
    ASSERT_STREQ(seek_info, seekExpected.c_str());
}

/**
 * Test seek back and read buffers.
 *
 * Test steps:
 *
 * 1.) Queue multiple buffers
 * 2.) Read and verify first chunk
 * 3.) Seek to first chunk and verify read
 * 4.) Switch channel and write new chunnk
 * 5.) Increment offset of read, read data and verify it equals written in (4)
 */
TEST_F(SeekTests, DISABLED_SeekBack) {
    // Validate demuxer ID was set

    EXPECT_CALL(*mMockDemuxer, informFirstFrameReceived())
            .Times(testing::AtLeast(1));
    std::string path("stream0.ts");
    int initialFeipBufCount = 10;
    int totalWriteBytes = initialFeipBufCount * FEIP_BUFFER_SIZE;
    char buffCopy[initialFeipBufCount][FEIP_BUFFER_SIZE];

    for (int i = 0; i < initialFeipBufCount; i++) {
        auto sH = mPlugin->getSourceHandler();
        auto buffer = sH->acquireBuffer(0);
        buffer->size = FEIP_BUFFER_SIZE;
        memset(&buffCopy[i], i + 1, FEIP_BUFFER_SIZE);
        memcpy(buffer->buffer, &buffCopy[i], FEIP_BUFFER_SIZE);
        std::this_thread::sleep_for (std::chrono::seconds(1));
        sH->pushBuffertoBQ(buffer, demuxerId);
    }

    int readSize = FEIP_BUFFER_SIZE - FEIP_BUFFER_SIZE % BUFFER_CHUNK_SIZE;
    char outBuffer[readSize];
    mPlugin->read(0, path, &outBuffer[0], readSize, 0);
    EXPECT_EQ(memcmp(&outBuffer[0], buffCopy[0], readSize), 0);

    sleep(1);

    // Verify Seek data and read back
    char seek_info[256];
    auto outReadSize = mPlugin->read(0, CONFIG_F_SEEK_CONTROL, seek_info, 256, 0);
    ASSERT_GE(outReadSize, 0);
    seek_info[outReadSize] = '\0';
    auto totalBytesInBuffer = totalWriteBytes - totalWriteBytes % BUFFER_CHUNK_SIZE;

    std::string seekExpStr = "0," + std::to_string(initialFeipBufCount - 1);
    ASSERT_STREQ(seek_info, seekExpStr.c_str());

    //Write seek data expect that read location will be adjusted

    const auto seekBack = 7;

    std::string seekBackStr = std::to_string(seekBack);
    mPlugin->write(CONFIG_F_SEEK_CONTROL, seekBackStr.c_str(), seekBackStr.size(), 0);
    mPlugin->read(0, path, &outBuffer[0], readSize, 0);

    int o =  (outBuffer[0]);
    int inval  = buffCopy [0][0];

    EXPECT_EQ(memcmp(&outBuffer[0], buffCopy[0], readSize), 0);

    // Change channel expect seek info has zero size
    //Writing channel should trigger open on Demuxer and connect

    EXPECT_CALL(*mMockDemuxer, open("01",""))
            .Times(testing::AtLeast(1));

    EXPECT_CALL(*mMockDemuxer, connect())
            .Times(testing::AtLeast(1));

    EXPECT_CALL(*mTestCbHandler.get(), notifyStreamSwitched("")).Times(testing::Exactly(1));

    mPlugin->write(CONFIG_F_CHANNEL_SELECT, "01", 2, 0);
    outReadSize = mPlugin->read(0, CONFIG_F_SEEK_CONTROL, seek_info, 256, 0);
    ASSERT_GE(outReadSize, 0);
    seek_info[outReadSize] = '\0';
    // Seek info should be empty
    seekExpStr = "0,0";
    ASSERT_STREQ(seek_info, seekExpStr.c_str());

    //Write new data and verify read back
    int BUF_IDX = 9;
    auto buffer = mPlugin->getSourceHandler()->acquireBuffer(0);
    buffer->size = FEIP_BUFFER_SIZE;
    memcpy(buffer->buffer, &buffCopy[BUF_IDX], FEIP_BUFFER_SIZE);
    mPlugin->getSourceHandler()->pushBuffertoBQ(buffer, demuxerId);

    readSize = FEIP_BUFFER_SIZE - FEIP_BUFFER_SIZE % BUFFER_CHUNK_SIZE;
    auto readCount = mPlugin->read(0, path, &outBuffer[0], readSize, readSize);

    ASSERT_EQ(readCount, readSize);

    EXPECT_EQ(memcmp(&outBuffer[0], buffCopy[BUF_IDX], readSize), 0);
}

static void send_response(
        ConstDelayDefHandler &handler,
        ReadDefferHandler::DefferSignalTypes &sig,
        uint32_t &sleepMs
) {

    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    handler.signal(sig);
}

TEST(ConstDefferTest, SuccesFullRead) {
    ConstDelayDefHandler defHandler(milliseconds(500));
    uint32_t delayMs = 200;
    auto msg = ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED;

    auto thr1 = std::thread(send_response,
                            std::ref(defHandler),
                            std::ref(msg),
                            std::ref(delayMs));

    ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED);
    thr1.join();
}

TEST(ConstDefferTest, ReadSingleThread) {
    auto delayTime = milliseconds(500);
    ConstDelayDefHandler defHandler(delayTime);

    auto msg = ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED;
    defHandler.signal(msg);
    auto t1 = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );

    ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED);

    auto t2 = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()
    );

    ASSERT_GE(delayTime, t2 - t1);
}

TEST(ConstDefferTest, ReadSingleThreadFailure) {
    ConstDelayDefHandler defHandler(milliseconds(500));

    auto msg = ReadDefferHandler::DefferSignalTypes::NETWORK_FAILURE;
    defHandler.signal(msg);
    ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::NETWORK_FAILURE);
}


TEST(ConstDefferTest, FailedRead) {
    ConstDelayDefHandler defHandler(milliseconds(500));
    uint32_t delayMs = 501;
    auto msg = ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED;

    auto thr1 = std::thread(send_response,
                            std::ref(defHandler),
                            std::ref(msg),
                            std::ref(delayMs));


    ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::GENERIC_FAILURE);
    thr1.join();
}

TEST(ConstDefferTest, NetworkFailure) {
    ConstDelayDefHandler defHandler(milliseconds(500));
    uint32_t delayMs = 100;
    auto msg = ReadDefferHandler::DefferSignalTypes::NETWORK_FAILURE;

    auto thr1 = std::thread(send_response,
                            std::ref(defHandler),
                            std::ref(msg),
                            std::ref(delayMs));

    ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::NETWORK_FAILURE);
    thr1.join();
}

TEST(ConstDefferTest, MultipleRead) {
    ConstDelayDefHandler defHandler(milliseconds(1000));
    uint32_t delayMs = 200;
    auto msg = ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED;

    auto thr1 = std::thread(send_response,
                            std::ref(defHandler),
                            std::ref(msg),
                            std::ref(delayMs));

    std::thread test1([&defHandler]() {
        ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED);
    });

    std::thread test2([&defHandler]() {
        ASSERT_EQ(defHandler.waitSignal(), ReadDefferHandler::DefferSignalTypes::DATA_RECEIVED);
    });

    test1.join();
    test2.join();
    thr1.join();
}

TEST(VariableWatcher, set_var) {
    ConfigVariableId testId("testId");

    MVar<std::string> &x = MVar<std::string>::getVariable(testId);
    int count = 0;
    auto func = std::make_shared<MVar<std::string>::watcher_function>(
            [&count](std::string oldVal, std::string newVal, const ConfigVariableId &id) {
                count++;
            }
    );

    MVar<std::string>::watch(testId,func);

    x = "one";
    x = "two";
    ASSERT_EQ(x, std::string("two"));
    ASSERT_EQ(count, 2);
}

/*
 * This test case validates that the bisection search implementation,
 * for finding the buffer offset for a certain seek time, is within
 * an accuracy of 1000ms (specified in the constructor).
 *
 * This is done the following way:
 *
 * - A range of buffers are registered in the BufferIndexer each having 
 *   a buffer size, defined by buffer_size. The registration rate is
 *   defined by period_us and the total duration is defined by duration_us.
 *
 * - When all the buffers are registered, the byte offset is read from
 *   the BufferIndexer using seek times in intervals of a second
 *   covering the specified duration interval. For each seek time, the
 *   byte offset is validated up against the expected byte offset
 *   accuracy range for that seek time, which is constrained by and
 *   derived from the seek accuracy.
 *
 * The outer loop controls the down-sampling ratio and must not exceed
 * buffer chunk registration frequency when using an accuracy of 1000ms.
 */
TEST(BufferIndexer, bisectionSearchTest) {
    // Buffer chunk registration rate (i.e. when a new buffer chunk is received)
    const uint64_t period_us     = 200e3;
    // Buffer chunk registration frequency
    const uint64_t frequency     = 1e6 / period_us;
    // Buffer chunk byte size
    const uint64_t buffer_size   = 1024;
    // Total buffer chunk registration duration
    const uint64_t duration_us   = 5e6;
    // Total number of buffer to inject
    const uint64_t samples_count = duration_us / period_us;

    // Down-sampling ratio loop
    for (uint8_t sampling_ratio = 1; sampling_ratio <= 4; ++sampling_ratio) {
        StreamParser::BufferIndexer bIdx(200, 0, sampling_ratio);

        // Structure containing reference data for validation
        struct refData {
            uint64_t timeUs;      // seek time [us] reference value
            uint64_t bufferCount; // buffer count reference value
            uint64_t timestampUs; // EPOC timestamp [us] reference value
        };
        // Reference vector containing the refData objects
        std::vector<refData> refVector;
        uint64_t currentTimeUs = 0;
        uint64_t currentBufferCount = 0;

        // Register buffers using a fixed buffer size and rate
        for (uint16_t n = 0; n < samples_count; ++n) {
            currentBufferCount += buffer_size;
            currentTimeUs += period_us;
            uint64_t timestampUs = std::chrono::duration_cast< std::chrono::microseconds >(
                    std::chrono::high_resolution_clock::now().time_since_epoch()
            ).count();
            refVector.push_back({currentTimeUs, currentBufferCount, timestampUs});
            bIdx.registerBufferCount(currentBufferCount);
            usleep(period_us);
        }

        // Validate the total buffer size
        ASSERT_EQ((uint64_t)(bIdx.getIndexSizeInTimeUs() / 1e6), ((samples_count-1)/frequency));

        // Iterate over the total duration
        for (uint64_t seek_time_s = 0; seek_time_s < (duration_us / 1e6); ++seek_time_s) {
            uint64_t offset_range_start = buffer_size * (frequency * (seek_time_s + 1) - 1);
            uint64_t offset_range_end = buffer_size * frequency * seek_time_s;

            uint64_t found_offset;
            // Get the byte offset for the particular seek time
            bIdx.getByteOffsetFromTimeUs(seek_time_s*1e6, found_offset);
            // Validate that the obtained offset is within the accuracy range
            ASSERT_NEAR(offset_range_end, found_offset, offset_range_start - offset_range_end);
        }

        // Iterate over the total number of samples
        for (uint16_t n = 0; n < samples_count - 1; ++n) {
            uint64_t time;
            // Get the seek time for a particular byte offset being in-between two samples
            bIdx.getTimeUsFromByteOffset((refVector[n].bufferCount + refVector[n + 1].bufferCount) / 2, time);
            // Get the reference seek time in seconds
            time /= 1e6;
            uint64_t refTime = refVector[n].timeUs / 1e6;
            // Validate the returned seek time up against the associated reference seek time
            ASSERT_EQ(time, refTime);
        }

        // Iterate over the total number of samples
        for (uint16_t n = 0; n < samples_count; ++n) {
            uint64_t timeUs;
            // Get the absolute EPOC timestamp for a particular buffer count / byte index
            bIdx.getTimestampUsForByteIndex(refVector[n].bufferCount, timeUs);
            // Validate the returned timestamp up against the associated reference timestamp using 300us allowed tolerance
            ASSERT_NEAR(refVector[n].timestampUs, timeUs, 300);
        }

    }
}

TEST(BufferIndexer, indexRangeAndSizeTest) {
    // Create a BufferIndexer having a capacity of 10 samples
    StreamParser::BufferIndexer bIdx(10, 0, 1);
    uint64_t buf;

    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(0, buf), StreamParser::BUF_EMPTY);

    // Fill up half of the BufferIndexer's capacity using 500ms registration rate
    for (uint16_t n = 0; n < 5; ++n) {
        bIdx.registerBufferCount(1);
        usleep(500e3);
    }

    ASSERT_EQ((uint64_t)(bIdx.getIndexSizeInTimeUs() / 1e6), 2);
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(2e6, buf), StreamParser::BUF_OK);
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(3e6, buf), StreamParser::BUF_OUT_OF_RANGE);

    // Fill up the remaining part the BufferIndexer's capacity using 750ms registration rate
    for (uint16_t n = 5; n < 10; ++n) {
        bIdx.registerBufferCount(1);
        usleep(750e3);
    }

    ASSERT_EQ((uint64_t)(bIdx.getIndexSizeInTimeUs() / 1e6), 5);
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(5e6, buf), StreamParser::BUF_OK);
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(6e6, buf), StreamParser::BUF_OUT_OF_RANGE);
}

TEST(BufferIndexer, rotationTest) {
    // Create a BufferIndexer having a capacity of 10 samples
    StreamParser::BufferIndexer bIdx(50, 0, 1);
    uint64_t buf;

    for (uint16_t n = 1; n < 100; ++n) {
        bIdx.registerBufferCount(n);
        usleep(100e3);
    }

    // Test for zero seek time (i.e. live)
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(0, buf), StreamParser::BUF_OK);
    ASSERT_EQ(buf, 0);

    // Test for out-of-range seek time (i.e. live)
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(100e6, buf), StreamParser::BUF_OUT_OF_RANGE);
    ASSERT_EQ(buf, 49);

    // Test for in-range seek time
    ASSERT_EQ(bIdx.getByteOffsetFromTimeUs(1e6, buf), StreamParser::BUF_OK);
    ASSERT_NEAR(10, buf, 19);
}

TEST(TimeIntervalMonitor, unitTest) {
    const uint64_t tolerance_us = 10e3;
    TimeIntervalMonitor timer;
    // Test that the initial time is zero after object construction
    ASSERT_EQ(timer.getAccumulatedTimeInMicroSeconds(), 0);
    // Initiate time registration
    timer.updateTimeInterval();
    // Sleep for 500ms and register time afterward
    std::this_thread::sleep_for(500ms);
    timer.updateTimeInterval();
    // Test that the accumulated time since initialization is ~500ms
    ASSERT_NEAR(500e3, timer.getAccumulatedTimeInMicroSeconds(), tolerance_us);
    // Sleep for additional 200ms and register time afterward
    std::this_thread::sleep_for(200ms);
    timer.updateTimeInterval();
    // Test that the accumulated time since initialization is now ~700ms
    ASSERT_NEAR(700e3, timer.getAccumulatedTimeInMicroSeconds(), tolerance_us);

    // Stop the time registration
    timer.stopTimeInterval();
    // Sleep for 500ms and initiate a new time registration
    std::this_thread::sleep_for(500ms);
    timer.updateTimeInterval();
    // Test that the accumulated time since initialization is still ~700ms and that
    // 500ms sleep after stopping time registration has been ignored
    ASSERT_NEAR(700e3, timer.getAccumulatedTimeInMicroSeconds(), tolerance_us);
    // Now sleep for 300ms and register time afterward
    std::this_thread::sleep_for(300ms);
    timer.updateTimeInterval();
    // Test that the accumulated time has increased 300ms to ~1000ms
    ASSERT_NEAR(1000e3, timer.getAccumulatedTimeInMicroSeconds(), tolerance_us);

    // Reset the duration timer
    timer.reset();
    // Test that the initial time after reset is zero
    ASSERT_EQ(timer.getAccumulatedTimeInMicroSeconds(), 0);
    // Initiate time registration
    timer.updateTimeInterval();
    // Sleep for 400ms and register time afterward
    std::this_thread::sleep_for(400ms);
    timer.updateTimeInterval();
    // Test that the accumulated time since initialization is ~400ms
    ASSERT_NEAR(400e3, timer.getAccumulatedTimeInMicroSeconds(), tolerance_us);
    // Sleep for additional 450ms and register time afterward
    std::this_thread::sleep_for(450ms);
    timer.updateTimeInterval();
    // Test that the accumulated time is now ~850ms
    ASSERT_NEAR(850e3, timer.getAccumulatedTimeInMicroSeconds(), tolerance_us);
}
