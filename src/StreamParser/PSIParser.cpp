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


#include "StreamParser/PSIParser.h"
#include "StreamParser/StreamProtectionConfig.h"
#include <glog/logging.h>
#include <confighandler/ConfigHandler.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <boost/algorithm/hex.hpp>
#include <Logging.h>
#include <streamfs/LogLevels.h>

namespace {

std::string toHexString(const ByteVectorType &bArray) {
    try {
        return boost::algorithm::hex(std::string({bArray.begin(), bArray.end()}));
    } catch (std::exception &e) {
        LOG(ERROR) << "Could convert byte array to hex string";
        return std::string();
    }
}

}

namespace StreamParser {

PSIParser::~PSIParser() {
    LOG(INFO) << "Destroy PSI parser";
    mExitParserThread = true;
    joinParserThread();
}

void PSIParser::post(const StreamParser::Buffer &buf) {

    // Delayed start of parser thread.
    if (mParserThread == nullptr) {
        mParserThread = std::make_unique<std::thread>(&PSIParser::threadLoop, this);
    }

    if (buf.channelInfo == nullptr) {
        SLOG(INFO, LOG_DATA_SRC) << " Invalid channel info";
        return;
    }

    std::lock_guard<std::mutex> lockGuard(mStreamMtx);

    if (buf.channelInfo != mChannel) {
        clearQueue();
        mChannel = buf.channelInfo;
    }

    if (mPsiParserRunning) {
        mQueue.push(*buf.chunk);
    }
}

void PSIParser::join() {
}

void PSIParser::notifyConfigurationChanged(const std::string &var, const ByteVectorType &value) {
    std::lock_guard<std::mutex> lockGuard(mStreamMtx);

    if (var == CONFIG_F_FCC_STREAM_INFO_ECM) {
        mPsiParserRunning = value.empty();
        LOG(INFO) << "Setting mPsiParserRunning to " << mPsiParserRunning;
    }
}

void PSIParser::clearQueue() {
    std::lock_guard<std::mutex> lockGuard(mConsumerMtx);
    buffer_chunk chunk;
    unsigned int i = 0;
    while (mQueue.pop(chunk)) {
        i++;
        //Pop all data from the queue and ignore it
    };

    mPacketCounter = 0;
    LOG(INFO) << "Discarded " << i << " chunks";
}

void PSIParser::threadLoop() {
    buffer_chunk chunk;
    while (!mExitParserThread) {

        while (!mQueue.pop(chunk)) {
            //TODO: We are using a non-blocking queue.
            // Investigate if notify would be a better option here.
            // We are not in hurry of processing the data on this thread
            // it doesn't affect the channel switch time and getting an ECM
            // may take seconds anyway.
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (mExitParserThread)
                return;
        }
        if (mExitParserThread)
            return;

        mPacketCounter++;
        processChunk(chunk);
    }
    LOG(INFO) << "Exiting. Processed buffers: " << mPacketCounter << " exit thread: " << mExitParserThread;
}

void PSIParser::processChunk(const buffer_chunk &array) {
    if (mTsStream.needsNewChunk()) {
        mTsStream.insertChunk(array);
    } else {
        LOG(ERROR) << "Continuity error, unable to insert array";
    }

    static StreamPacketT packet;
    TSStream::TSDataError state;

    while ((state = mTsStream.getNextPacket(packet)) != TSStream::TSDataError::NOT_ENOUGH_DATA) {
        if (state == TSStream::TSDataError::DATA_CC_ERROR) {
            LOG(INFO) << "Continuity error";
            break;
        }
        switch (parseTsPacket(packet)) {
            case ERROR:
                break;
            case IGNORE:
                break;
            case DROP:
                break;
            case DECRYPT_ODD:
                break;
            case DECRYPT_EVEN:
                break;
            case NEW_ECM:
            case NEW_ECMT:
            case NEW_PAT_PMT:
                // Set drm0 one time after channel switch if not already set in NokiaSocketCbHandler
                if (mPsiParserRunning) {
                    *mDrm = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::HIGH,
                                                   mChannel,
                                                   getCurrentEcm(),
                                                   getCurrentPat(),
                                                   getCurrentPmt(),
                                                   mIsClearStream);
                    mPsiParserRunning = false;
                }

                LOG(INFO) << "Current ecm: " << toHexString(getCurrentEcm());
                LOG(INFO) << "Current pmt: " << toHexString(getCurrentPmt());
                LOG(INFO) << "Current pat: " << toHexString(getCurrentPat());
                break;
        }
    }
}

bool PSIParser::TSStream::insertChunk(const buffer_chunk &chunk) {
    if (mCircBuf.size() == 2)
        return false;
    else {
        mCircBuf.push_back(chunk);
    }
    return true;
}

PSIParser::TSStream::TSDataError
PSIParser::TSStream::getNextPacket(StreamPacketT &packet) {

    if (mCircBuf.size() == 0 ||
        (mCircBuf.size() == 1 && (BUFFER_CHUNK_SIZE - mPointer) < TS_PACKAGE_SIZE)) {
        return NOT_ENOUGH_DATA;
    }

    if (BUFFER_CHUNK_SIZE - mPointer >= TS_PACKAGE_SIZE) {
        std::copy(mCircBuf[0].begin() + mPointer, mCircBuf[0].begin() + mPointer + TS_PACKAGE_SIZE, packet.begin());
        mPointer += TS_PACKAGE_SIZE;

        if (mPointer == BUFFER_CHUNK_SIZE) {
            mCircBuf.pop_front();
            mPointer = 0;
        }

    } else {
        auto firstSeq = BUFFER_CHUNK_SIZE - mPointer;
        auto secondSeq = TS_PACKAGE_SIZE - firstSeq;
        std::copy(mCircBuf[0].begin() + mPointer, mCircBuf[0].begin() + mPointer + firstSeq, packet.begin());
        std::copy(mCircBuf[1].begin(), mCircBuf[1].begin() + secondSeq, packet.begin() + firstSeq);
        mPointer = secondSeq;
        mCircBuf.pop_front();
    }

    if (packet[0] != 0x47)
        return DATA_CC_ERROR;

    return OK;
}

bool PidInfo::setPid(unsigned pid) {
    if (mPidValid)
        return false;
    mPid = pid;
    mPidValid = pid <= PSI_MAX_PID;
    return mPidValid;
}

bool PidInfo::setVer(unsigned ver) {
    if (!mPidValid) return false;
    mVersion = ver;
    mVersionValid = true;
    return true;
}

void PidInfo::reset() {
    mPid = PSI_INVALID_PID;
    mVersion = 0;
    mVersionValid = false;
    mPidValid = false;
    mTsPacket.fill({0});
}

bool PidInfo::isNew(unsigned v) {
    if (!mPidValid)
        return false;

    if (!mVersionValid)
        return true;

    return mVersion != v;
}

bool PidInfo::isPid(unsigned pid) {
    if (!mPidValid)
        return false;
    return pid == mPid;
}


unsigned PidInfo::getPid() {
    if (!mPidValid)
        return PSI_INVALID_PID;

    return mPid;
}


PSIParser::ParserActionT PSIParser::parseTsPacket(const StreamPacketT &packet) {
    PSIParser::ParserActionT action;
    unsigned pid = GET_PID(packet);

    std::lock_guard<std::mutex> lockGuard(mPidInfoMtx);
    if (mPat.isPid(pid)) {
        action = parsePat(packet);
        if ((mParserState == PSIParser::NEEDS_PAT) && patProcessed() && hasPmtPid())
            mParserState = PSIParser::NEEDS_PMT;
        return action;
    }

    if (mPmt.isPid(pid)) {
        action = parsePmt(packet);
        if (!pmtProcessed())
            return action;

        switch (mParserState) {
            case PSIParser::NEEDS_PMT:
            case PSIParser::NEEDS_VMX_PMT:
            case PSIParser::NO_ECM:
                if (!mIsClearStream) {
                    if (hasEcmPid())
                        mParserState = PSIParser::NEEDS_ECM;
                    else
                        mParserState = PSIParser::NEEDS_VMX_PMT;
                }
                break;
            case PSIParser::NEEDS_ECM:
            case PSIParser::GOT_ECM:
                if (mIsClearStream) {
                    mParserState = PSIParser::NO_ECM;
                    mEcm.reset();
                }
                break;
            case PSIParser::INVALID:
            case PSIParser::NEEDS_PAT:
            default:
                break;
        }
        return action;
    }
    if (mEcm.isPid(pid)) {
        action = parseEcm(packet);
        if ((mParserState == PSIParser::NEEDS_PMT) && ecmProcessed())
            mParserState = PSIParser::GOT_ECM;
        return action;
    }

    action = ParseOther(packet);

    return action;
}

PSIParser::ParserActionT PSIParser::parsePat(const StreamPacketT &packet) {
    PSIParser::ParserActionT rv = PSIParser::ERROR;
    do {
        if (!mPat.isPid(GET_PID(packet))) {
            LOG(INFO) << "Ignore pat";
            break;
        }

        unsigned offs;

        if ((packet[3] & 0x30) == 0x30)
            offs = packet[4] + 1;
        else
            offs = packet[4];

        if (offs > 184) {
            LOG(INFO) << "Invalid TS packet size";
            break;
        }

        if (packet[offs + 5] != 0x0) {
            LOG(INFO) << "Wrong header";
            break;
        }

        unsigned version = (packet[offs + 10] & 0x3E) >> 1;
        if (mPat.isNew(version)) {
            mPmt.reset();
            mEcm.reset();
            mPat.setVer(version);
            int pgm = ((packet[offs + 13] << 8) + packet[offs + 14]) & 0x1FFF;
            unsigned pmtPid;

            if (pgm == 0)
                pmtPid = ((packet[offs + 17] << 8) + packet[offs + 18]) & 0x1FFF;
            else
                pmtPid = ((packet[offs + 15] << 8) + packet[offs + 16]) & 0x1FFF;

            mPmt.setPid(pmtPid);

            StreamPacketT pOut;
            std::copy(packet.data() + 5, packet.data() + packet.size(), pOut.begin());
            mPat.setPacket(pOut);
        }
        rv = PSIParser::IGNORE;

    } while (false);

    return rv;
}

PSIParser::ParserActionT PSIParser::parsePmt(const StreamPacketT &packet) {
    PSIParser::ParserActionT rv = PSIParser::ERROR;
    bool descriptorFound = false;
    do {
        if (!mPmt.isPid(GET_PID(packet))) {
            LOG(INFO) << "Not a PID";
            break;
        }

        unsigned offs = 0;

        if ((packet[3] & 0x30) == 0x30)
            offs = packet[4] + 1;

        if (offs > 184) {
            LOG(INFO) << "Wrong size";
            break;
        }

        if (packet[offs + 5] != 0x2) {
            LOG(INFO) << "Wrong header";
            break;
        }

        unsigned int section_len = ((packet[offs + 6] << 8) + packet[offs + 7]) & 0xFFF;
        unsigned descriptorLen = ((packet[offs + 15] << 8) + packet[offs + 16]) & 0x0FFF;

        unsigned pLen = descriptorLen;

        if (pLen > section_len) {
            LOG(ERROR) << "Invalid descriptor field length: " << pLen << " section size = " << section_len;
            return rv;
        }

        auto remSectBytes = section_len - 9;

        unsigned version = (packet[offs + 10] & 0x3E) >> 1;

        if (mPmt.isNew(version)) {
            mPmt.setVer(version);

            const unsigned char *descP = packet.data() + offs + 17;

            remSectBytes -= pLen;

            while (pLen > 0) {
                if ((descP[1] + 2) > TS_PACKAGE_SIZE) {
                    return PSIParser::IGNORE;
                }

                unsigned dtLen = descP[1] + 2;
                if (descP[0] == 0x09) {
                    if ((descP[2] == 0x56) && (descP[3] == 0x01)) {
                        descriptorFound = true;
                        bool accepted = false;
                        if (dtLen > CA_DESC_LEN) {
                            auto ext_len = dtLen - CA_DESC_LEN;
                            auto *ext = descP + CA_DESC_LEN;
                            if (ext_len >= 2) {
                                accepted = !mOpid || (mOpid == ext[1]);
                            } else {
                                LOG(INFO) << "Missing CA extension";
                            }
                        } else {
                            LOG(INFO) << "Standard CA descriptor. " << " opid = " << (int) mOpid;
                            accepted = true;
                        }

                        if (accepted) {
                            auto ecmPid = ((descP[4] << 8) + descP[5]) & 0x1FFF;
                            if (!mEcm.isPid(ecmPid)) {
                                mEcm.reset();
                                mEcm.setPid(ecmPid);
                                StreamPacketT pOut = packet;
                                std::copy(packet.begin() + 5, packet.end(), pOut.begin());
                                mEcm.setPacket(pOut);
                            }
                            break;
                        }
                    }
                }
                pLen -= dtLen;
                descP += dtLen;
            }

            // Offset of stream_loop section
            const unsigned char *streamLoopOffs = packet.data() + offs + 17 + descriptorLen;

            unsigned p = 0;
            // The ECM PID may be in the stream_type loop
            //
            while (remSectBytes > 4) {

                auto descr = streamLoopOffs[p]; //[= ITU-T Rec. H.222.0 | ISO/IEC 13818-1 reserved]
                UNUSED(descr);
                p++;
                // TODO: Check that we extract the CA info from the video stream
                // Since there is a range of codecs, we need to do multiple checks here.
                // Audio stream may have CA info too.

                // Stream PID
                p += 2;

                auto es_info_length = ((streamLoopOffs[p] << 8) + streamLoopOffs[p + 1]) & 0xFFF;
                p += 2;

                // 0x09 - CA_descriptor
                // 0x5601 - VMX identifier
                auto desc_offset = 0;
                bool ecmPidSet = false;
                while ( desc_offset <= (es_info_length - 6)) {
                    const auto desc_idx = p + desc_offset;
                    if ((streamLoopOffs[desc_idx] == 0x9)
                            && (streamLoopOffs[desc_idx + 2] == 0x56)
                            && (streamLoopOffs[desc_idx + 3] == 0x01)) {
                        descriptorFound = true;

                        auto ecmPid = ((streamLoopOffs[desc_idx + 4] << 8) + streamLoopOffs[desc_idx + 5]) & 0x1FFF;

                        if (!mEcm.isPid(ecmPid)) {
                            mEcm.reset();
                            mEcm.setPid(ecmPid);
                            StreamPacketT pOut;
                            std::copy(packet.data() + 5, packet.data() + packet.size(), pOut.begin());
                            mEcm.setPacket(pOut);
                            ecmPidSet = true;
                            break;
                        }
                    }
                    desc_offset += streamLoopOffs[ desc_idx + 1] + 2;
                }
                if (ecmPidSet) {
                    break;
                }
                p += es_info_length;
                remSectBytes -= es_info_length + 5;
            }

            // Stream Type extractor

            mIsClearStream = !descriptorFound;

            rv = PSIParser::IGNORE;

            if (mIsClearStream) {
                rv = PSIParser::NEW_PAT_PMT;
                StreamPacketT pOut = packet;
                std::copy(packet.begin() + 5, packet.end(), pOut.begin());
                mPmt.setPacket(pOut);
                *mIsCdmSetupDone = true;
            }
        }

    } while (false);

    return rv;
}

PSIParser::ParserActionT PSIParser::parseEcm(const StreamPacketT &packet) {
    PSIParser::ParserActionT rv = PSIParser::ERROR;
    static int ecmCount = 0, dupEcmCount = 0;
    do {
        if (!mEcm.isPid(GET_PID(packet)))
            break;

        if (mEcmCollectionInProgress)
            return collectEcm(packet);

        unsigned a = 0;
        if ((packet[3] & 0x30) == 0x30)
            a = packet[4] + 1;

        if (a > 184)
            break;

        const unsigned char *table = packet.data() + 5 + a;
        unsigned remainingBytes = TS_PACKAGE_SIZE - (5 + a);
        if (remainingBytes < 13) {
            LOG(WARNING) << "Invalid packet size.";
            break;
        }

        unsigned version = (table[5] & 0x3E) >> 1;

        if (!((table[0] == 0x80) || (table[0] == 0x81))) {
            LOG(WARNING) << "Invalid version info " << (int) table[0];
            break;
        }

        if (strncmp((const char *) (table + 8), "VMECM", 5)) {
            LOG(INFO) << "Ignoring ECM. Not a Verimatrix ECM";
            break;
        }

        ecmCount++;

        mTempCollectedEcmData.resize(0);

        mEcmCollectionInProgress = false;
        mCollectedEcmDataExpected = 0;
        mCollectedDataLength = 0;

        unsigned table_len = ((table[1] & 0x0F) << 8) + table[2] + 3;

        mTempCollectedEcmData.resize(table_len);

        remainingBytes = std::min(table_len, remainingBytes);

        memcpy(mTempCollectedEcmData.data(), table, remainingBytes);

        mEcmCollectionInProgress = true;
        mCollectedEcmDataExpected = table_len;
        mCollectedDataLength = remainingBytes;

        if (mCollectedEcmDataExpected > mCollectedDataLength) {
            // Table not complete
            LOG(INFO) << "Collecting ECM version: "
                      << version
                      << " Collected size: " << mCollectedDataLength
                      << " Expected size: " << mCollectedEcmDataExpected;
            rv = PSIParser::IGNORE;
        } else {
            if (mEcm.isNew(version)) {
                mEcm.setVer(version);
                rv = PSIParser::NEW_ECM;
                mEcmTable = mTempCollectedEcmData;
            } else {
                dupEcmCount++;
                rv = PSIParser::IGNORE;
            }
            resetCollectionState();
        }

    } while (false);

    return rv;
}

PSIParser::ParserActionT PSIParser::collectEcm(const StreamPacketT &packet) {
    PSIParser::ParserActionT result = PSIParser::ERROR;

    if (!mEcm.isPid(GET_PID(packet)))
        return result;

    unsigned a = 0;
    if ((packet[3] & 0x30) == 0x30)
        a = packet[4] + 1;

    if (a > 184)
        return result;

    auto remainingBytes = TS_PACKAGE_SIZE - (4 + a);
    auto need = mCollectedEcmDataExpected - mCollectedDataLength;
    unsigned version = (mTempCollectedEcmData.data()[5] & 0x3E) >> 1;

    if (remainingBytes > need)
        remainingBytes = need;

    memcpy(mTempCollectedEcmData.data() + mCollectedDataLength, packet.data() + 4 + a, remainingBytes);

    mCollectedDataLength += remainingBytes;

    if (mCollectedDataLength == mCollectedEcmDataExpected) {
        if (mEcm.isNew(version)) {
            mEcm.setVer(version);
            result = PSIParser::NEW_ECMT;
            mEcmTable = mTempCollectedEcmData;
        } else {
            result = PSIParser::IGNORE;
        }
        resetCollectionState();

    } else {
        LOG(INFO) << "Collecting ECM version:"
                  << version
                  << " collected = " << mCollectedDataLength
                  << " expected = " << mCollectedEcmDataExpected;
        return PSIParser::IGNORE;
    }

    return result;
}

PSIParser::ParserActionT PSIParser::ParseOther(const StreamPacketT &pkt) {
    ParserActionT rv = PSIParser::IGNORE;
    if (TSC_ISSET(pkt)) {
        if (IS_TSC_EVEN(pkt))
            rv = PSIParser::DECRYPT_EVEN;
        else
            rv = PSIParser::DECRYPT_ODD;
    }
    return rv;
}

void PSIParser::resetCollectionState() {
    mCollectedDataLength = 0;
    mCollectedEcmDataExpected = 0;
    mTempCollectedEcmData.resize(0);
    mEcmCollectionInProgress = false;
}

ByteVectorType PSIParser::getCurrentPmt() {
    StreamPacketT packet;
    PidInfo pidInfo = mIsClearStream ? mPmt : mEcm;
    if (pidInfo.getPmtData(packet)) {
        return ByteVectorType(packet.begin(), packet.end());
    } else {
        return ByteVectorType();
    }
}

ByteVectorType PSIParser::getCurrentPat() {
    StreamPacketT packet;
    if (mPat.getPatData(packet)) {
        return ByteVectorType(packet.begin(), packet.end());
    } else {
        return ByteVectorType();
    }
}

void PSIParser::joinParserThread() {
    if (mParserThread != nullptr && mParserThread->joinable())
        mParserThread->join();
}

void PSIParser::onEndOfStream(const char *channelId) {
    StreamConsumer::onEndOfStream(channelId);
    *mIsCdmSetupDone = false;
}

void PSIParser::onOpen(const char *channelId) {
    UNUSED(channelId);
    clearQueue();
    { // lock this
        std::lock_guard<std::mutex> lockGuard(mPidInfoMtx);
        resetCollectionState();
        mEcmTable.resize(0);
        mEcm.reset();
        mPmt.reset();
        mPat.reset();
        mPat.setPid(0);
        mParserState = PSIParser::NEEDS_PAT;
    }
}

} //namespace StreamParser
