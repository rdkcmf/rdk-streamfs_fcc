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

#include <memory>
#include <boost/lockfree/queue.hpp>
#include "StreamConsumer.h"
#include "utils/MonVarObserver.h"
#include "utils/MonitoredVariable.h"
#include "confighandler/ConfigHandler.h"
#include "StreamProtectionConfig.h"
#include <vector>
#include <confighandler/ConfigHandlerMVarCb.h>
#include <thread>
#include <boost/circular_buffer.hpp>


#define PSI_INVALID_PID  0xFFFF
#define PSI_MAX_PID      0x1FFF

#define GET_PID(b) (((b[1] << 8) + b[2]) & 0x1FFF)
#define TSC_ISSET(p)       (p[3] & 0xC0)
#define IS_TSC_EVEN(p)     ((p[3] & 0xC0) == 0x80)

#define CA_DESC_LEN 6


static std::map<ConfigVariableId, const char *> ConfigMAP_StreamInfoToPath = {
        {kEcm0, CONFIG_F_FCC_STREAM_INFO_ECM},
        {kPmt0, CONFIG_F_FCC_STREAM_INFO_PMT},
        {kPat0, CONFIG_F_FCC_STREAM_INFO_PAT}
};

namespace StreamParser {
typedef std::array<unsigned char, TS_PACKAGE_SIZE> StreamPacketT;

class PidInfo {
private:
    unsigned mPid;
    unsigned mVersion;
    bool mPidValid;
    bool mVersionValid;
    StreamPacketT mTsPacket;
    bool Valid() { return mPidValid; }

public:
    PidInfo(unsigned pid) :
            mPid(pid),
            mVersion(0),
            mPidValid(pid <= PSI_MAX_PID),
            mVersionValid(false),
            mTsPacket {0} {}

    explicit PidInfo() : PidInfo(PSI_INVALID_PID) {}

    virtual ~PidInfo() = default;

    bool setPid(unsigned pid);

    void setPacket(const StreamPacketT& p) {
        mTsPacket = p;
    }

    bool getPmtData(StreamPacketT& p) {
        if (mPid == PSI_INVALID_PID || mTsPacket[0] != 0x2)
            return false;
        p = mTsPacket;
        return true;
    }

    bool getPatData(StreamPacketT& p) {
        if (mPid == PSI_INVALID_PID || mTsPacket[0] != 0x0)
            return false;
        p = mTsPacket;
        return true;
    }

    bool setVer(unsigned ver);

    void reset();

    bool isNew(unsigned v);

    bool isPid(unsigned pid);

    bool processed() const { return mVersionValid; };

    unsigned getPid();
};


/**
 * Parser for ECM/PMT and PAT values
 */
class PSIParser : public StreamConsumer, public MonVarObserver<ByteVectorType> {
public:
    typedef enum {
        ERROR,
        IGNORE,
        DROP,
        DECRYPT_ODD,
        DECRYPT_EVEN,
        NEW_ECM,
        NEW_ECMT,
        NEW_PAT_PMT
    } ParserActionT;

    typedef enum {
        INVALID,
        NEEDS_PAT,
        NEEDS_PMT,
        NEEDS_VMX_PMT,
        NEEDS_ECM,
        GOT_ECM,
        NO_ECM
    } PsiParserState_t;

public:

    PSIParser() : StreamConsumer("PSIParser", true),
                  mCbFunc(MVar<ByteVectorType>::getWatcher(this, ConfigMAP_StreamInfoToPath)),
                  mTsStream(), mParserThread(nullptr) {
        mDrm = &MVar<StreamProtectionConfig>::getVariable(kDrm0);
        mIsCdmSetupDone = &MVar<bool>::getVariable(kCdm0);
        *mIsCdmSetupDone = true;
    };

    virtual ~PSIParser() override;

    void post(const Buffer &buf) override;

    void join() override;
    void joinParserThread();

    void notifyConfigurationChanged(const std::string &var, const ByteVectorType &value) override;

    void onEndOfStream(const char *channelId) override;

    void onOpen(const char* channelId) override;

    class TSStream {
    public:
        enum TSDataError {
            OK = 0,             // All good
            NOT_ENOUGH_DATA,    // Not enough data supplied to parse a package
            DATA_CC_ERROR,      // Data continuity error
        };
    public:
        explicit TSStream() : mCircBuf(2), mPointer(0) {
        };

        /**
         * Read the next TS packet
         * @return -
         */
        TSDataError getNextPacket(StreamPacketT &packet);

        // Insert a new chunk
        // @param chunk - input data
        // @result - insert will fail if tail package is not processed
         bool insertChunk(const buffer_chunk &chunk);

        // Tail chunk is processed, more data is needed
        // Use this method to check when to insert a new
        // chunk
        inline bool needsNewChunk() {
            return (mCircBuf.size() < 2);
        }

        virtual ~TSStream() = default;

    private:
        boost::circular_buffer<buffer_chunk> mCircBuf;
        int mPointer;
    };

private:
    void threadLoop();

private:
    std::shared_ptr<MVar<ByteVectorType>::watcher_function> mCbFunc{};
    std::atomic<bool> mPsiParserRunning{false};
    boost::lockfree::queue<buffer_chunk> mQueue{100};
    std::string mChannel{};
    std::mutex mStreamMtx;
    std::mutex mConsumerMtx;
    std::mutex mPidInfoMtx;

    MVar<StreamProtectionConfig> *mDrm;

private:
    TSStream mTsStream;

private:
    // Clear queue.
    void clearQueue();

    void processChunk(const buffer_chunk &array);

private:
    std::unique_ptr<std::thread> mParserThread;

    PidInfo mPat{0};
    PidInfo mPmt{0};
    PidInfo mEcm{0};
    bool mIsClearStream{false};
    PsiParserState_t mParserState{PSIParser::NEEDS_PAT};
    unsigned char mOpid{0};
    std::atomic<bool> mExitParserThread{false};

private:
    ParserActionT parseTsPacket(const StreamPacketT &packet);

    bool patProcessed() { return mPat.processed(); };

    bool pmtProcessed() { return mPmt.processed(); };

    bool ecmProcessed() { return mEcm.processed(); };

    bool hasEcmPid() { return mEcm.getPid() != PSI_INVALID_PID; }

    bool hasPmtPid() { return mPmt.getPid() != PSI_INVALID_PID; }

    ByteVectorType getCurrentEcm() { return mEcmTable; }
    ByteVectorType getCurrentPmt();
    ByteVectorType getCurrentPat();

    ParserActionT parsePat(const StreamPacketT &packet);

    ParserActionT parsePmt(const StreamPacketT &packet);

    ParserActionT parseEcm(const StreamPacketT &packet);

    ParserActionT ParseOther(const StreamPacketT &packet);

    ParserActionT collectEcm(const StreamPacketT &packet);

    void resetCollectionState();

    ByteVectorType mEcmTable;
    ByteVectorType mTempCollectedEcmData;

    bool mEcmCollectionInProgress{false};
    unsigned int mCollectedEcmDataExpected{0};
    unsigned int mCollectedDataLength{0};
    uint64_t mPacketCounter{0};

    MVar<bool> *mIsCdmSetupDone;

};

} //StreamParser
