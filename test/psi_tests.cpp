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

#include <fstream>
#include <boost/algorithm/hex.hpp>
#include "psi_tests.h"
#include "StreamParser/PSIParser.h"

// clear samples
#define CLEAR1_TS "clear1.ts"
#define CLEAR2_TS "clear2.ts"

// encrypted samples
#define ENCRYPTED1_TS "encrypted1.ts"
#define ENCRYPTED2_TS "encrypted2.ts"

void fillChunk(uint32_t startValue, buffer_chunk& chunk) {
    for (uint32_t i = 0; i < chunk.size(); i ++ ) {
        chunk[i] = (startValue + i) % 256;
    }
}

// Fill dummy TS header data
void fillDummyPackageHeader(buffer_chunk& chunk, int headerStartPos) {
    int pos = headerStartPos;
    while (pos < chunk.size()) {
        chunk[pos] = 0x47; // Set header
        pos += TS_PACKAGE_SIZE;
    }
}

std::string byteArrayAsHex(const ByteVectorType &bArray) {
    try {
        return boost::algorithm::hex(std::string({ bArray.begin(), bArray.end() }));
    } catch (std::exception &e) {
        LOG(ERROR) << "Could convert byte array to hex string";
        return std::string();
    }
}

class PSIParserTests : public ::testing::Test {
public:

    PSIParserTests() {
        mDrm = &MVar<StreamProtectionConfig>::getVariable(kDrm0);
    }

    void openStream(const std::string &fileName) {
        buffer_chunk buffer;
        std::basic_ifstream<char> fileIn(fileName, std::ifstream::binary);

        ASSERT_TRUE(fileIn.is_open());
        mParser.onOpen(fileName.c_str());
        unsigned int headerOffs = 0;
        ByteVectorType ecm;
        mParser.notifyConfigurationChanged(CONFIG_F_FCC_STREAM_INFO_ECM, ecm);

        while(!fileIn.eof()) {
            fileIn.read((char*) buffer.data(), buffer.size());
            if (fileIn.gcount() != buffer.size())
                break;

            // Validate header
            while(headerOffs + TS_PACKAGE_SIZE < buffer.size()) {
                if (buffer.data()[headerOffs] != 0x47)
                    break;
                headerOffs += TS_PACKAGE_SIZE;
            }

            ASSERT_LE((int) (buffer.size()) - headerOffs, (int) TS_PACKAGE_SIZE);

            // Start of header in the next chunk
            headerOffs =  TS_PACKAGE_SIZE -  (buffer.size() - headerOffs);

            StreamParser::Buffer b {fileName.c_str(), &buffer};
            mParser.post(b);
        }
    }

    std::string getChannelInfo() const {
        return mDrm->getValue().channelInfo();
    }

    bool getIsClearStream() const {
        return mDrm->getValue().isClearStream();
    }

    std::string getPat() const {
        return byteArrayAsHex(mDrm->getValue().pat());
    }

    std::string getPmt() const {
        return byteArrayAsHex(mDrm->getValue().pmt());
    }

    std::string getEcm() const {
        return byteArrayAsHex(mDrm->getValue().ecm());
    }

    void validateCLEAR1() {
        openStream(CLEAR1_TS);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_EQ(getIsClearStream(), true);
        ASSERT_EQ(getPat().substr(0,32), "00B00D00A1C5000005BEE1E053232CEA");
        ASSERT_EQ(getPmt().substr(0,126), "02B03C05BEC50000E3E8F00024E3E8F011380F0160000000B000000000005D9F1F1F0FE7D0F0085006F0000000000006EBB8F007560564616E09007F085E5C");
        ASSERT_EQ(getEcm(), "");
    }

    void validateCLEAR2() {
        openStream(CLEAR2_TS);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_EQ(getIsClearStream(), true);
        ASSERT_EQ(getPat().substr(0,32), "00B00D00BEC500000F3CE83D7EC3B837");
        ASSERT_EQ(getPmt().substr(0,64), "02B01D0F3CC50000E834F0001BE834F00003E835F0060A0464616E00AB75DAF6");
        ASSERT_EQ(getEcm(), "");
    }

    void validateENCRYPTED1() {
        openStream(ENCRYPTED1_TS);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_EQ(getIsClearStream(), false);
        ASSERT_EQ(getPat().substr(0,32), "00B00D00D5C500000898E064BB887179");
        ASSERT_EQ(getPmt().substr(0,124), "02B03B0898C70000E065F0036501011BE065F00B0E03C035B609045601E0660FE0C9F0160A0464616E007C035880030E03C000F009045601E066EACEF414");
        ASSERT_EQ(getEcm(), "80B0695601E50000564D45434D02000200002200212B8D8056516036BA4EFF48C199B948C7F8E0C2A0A4F569F5AD8F58A5223F8DBB84DBCAE438EF2CDCA3DD178CC29D3673F041F2380783208FC6A93D6DF1DE3342EA6F9E415149A5152545894AC934DBD32D5E71413981EB");
    }

    void validateENCRYPTED2() {
        openStream(ENCRYPTED2_TS);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        ASSERT_EQ(getIsClearStream(), false);
        ASSERT_EQ(getPat().substr(0,32), "00B00D00D6C5000009C4E064E901B104");
        ASSERT_EQ(getPmt().substr(0,124), "02B03B09C4C70000E065F0036501011BE065F00B0E03C035B609045601E0660FE0C9F0160A0464616E007C035880030E03C000F009045601E0660F282E10");
        ASSERT_EQ(getEcm(), "80B0695601F90000564D45434D02000200002500212B8D8081813368C1248585157ACE618C57AEC69243BCA7BE8847E22300DE1D36BDBF9A17908361F77D51EA399E4022F378ABB9A3380331E4B1FA07AFA44CC0BC1B28CCB4DF018C14AAD5F3C9C3113DD1B50AAF240843E1");
    }

protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }

private:
    StreamParser::PSIParser mParser;
    MVar<StreamProtectionConfig> *mDrm;
};

TEST(PSIParser, GetPacketCCError)
{
    StreamParser::PSIParser::TSStream stream;

    buffer_chunk chunk;
    fillChunk(0, chunk);

    ASSERT_TRUE(stream.needsNewChunk());

    ASSERT_EQ(stream.insertChunk(chunk), true) ;
    StreamParser::StreamPacketT packet;
    ASSERT_EQ(stream.getNextPacket(packet), StreamParser::PSIParser::TSStream::DATA_CC_ERROR);
}

TEST(PSIParser, GetPacketOk) {

    StreamParser::PSIParser::TSStream stream;

    // Validate the buffer is empty
    ASSERT_TRUE(stream.needsNewChunk());

    // Initialize and insert first chunk
    buffer_chunk chunk;
    fillChunk(0, chunk);
    for (int i =0; i < chunk.size(); i++) {
        chunk[i] = i / TS_PACKAGE_SIZE;
    }

    fillDummyPackageHeader(chunk, 0);
    ASSERT_EQ(stream.insertChunk(chunk), true) ;

    // Initialize and insert second chunk
    for (int i =0; i < chunk.size(); i++) {
        chunk[i] = i / TS_PACKAGE_SIZE + BUFFER_CHUNK_SIZE / TS_PACKAGE_SIZE;
    }
    fillDummyPackageHeader(chunk, 0);
    ASSERT_EQ(stream.insertChunk(chunk), true) ;

    StreamParser::StreamPacketT packet;
    uint32_t  mNumPack = 2 * BUFFER_CHUNK_SIZE / TS_PACKAGE_SIZE;

    // Validate we read back all packages from the two inserted chunks in order by creation / insertion
    for (int i = 0; i < mNumPack; i ++) {
        ASSERT_EQ(stream.getNextPacket(packet), StreamParser::PSIParser::TSStream::OK);
        ASSERT_EQ(packet[0], 0x47);
        ASSERT_EQ(packet[1], i);
    }

    ASSERT_EQ(stream.getNextPacket(packet), StreamParser::PSIParser::TSStream::NOT_ENOUGH_DATA);
}

TEST(PSIParser, RandomPackets) {
    const uint32_t packageCount = BUFFER_CHUNK_SIZE;
    const uint32_t numberOfChunks = TS_PACKAGE_SIZE;
    std::vector<std::array<unsigned char, TS_PACKAGE_SIZE>> tsPackages(packageCount);
    std::vector<buffer_chunk> bufChunks(numberOfChunks);
    StreamParser::PSIParser::TSStream stream;

    srand((unsigned) time(0));

    for (int i = 0; i < packageCount; i ++) {
        tsPackages[i][0] = 0x47;
        for (int k = 1; k < TS_PACKAGE_SIZE; k++) {
            tsPackages[i][k] = rand() % 10;
        }
    }

    // Place buffers to chunks
    int bufId = 0;
    int bufIdx = 0;
    for(int i = 0; i < bufChunks.size(); i ++) {
        for (int k = 0; k < BUFFER_CHUNK_SIZE; k ++) {
            bufChunks[i][k] =  tsPackages[bufId][bufIdx];
            bufIdx ++;
            if (bufIdx == TS_PACKAGE_SIZE) {
                bufIdx = 0;
                bufId += 1;
            }
        }
    }

    int chunkIdx = 0;
    int packetId = 0;
    do {
        if (stream.needsNewChunk()) {
            stream.insertChunk(bufChunks[chunkIdx]);
            chunkIdx ++;
        }
        int err ;
        StreamParser::StreamPacketT packet;

        do {
            err = stream.getNextPacket(packet);
            if (err ==StreamParser::PSIParser::TSStream::OK) {
                ASSERT_EQ(packet[0], 0x47);
                for(int j = 0; j < TS_PACKAGE_SIZE; j ++) {
                    ASSERT_EQ(packet[j], tsPackages[packetId][j]);
                }
                packetId ++;
            }
        } while (err == StreamParser::PSIParser::TSStream::OK);
    } while (chunkIdx != numberOfChunks - 1);
}

TEST(PSIParser, QueueStream) {
    buffer_chunk buffer;
    std::basic_ifstream<char> fileIn(ENCRYPTED1_TS, std::ifstream::binary);

    ASSERT_TRUE(fileIn.is_open());
    ByteVectorType ecm;
    StreamParser::PSIParser::TSStream stream;

    while(!fileIn.eof()) {
        fileIn.read((char *) buffer.data(), buffer.size());

        if (fileIn.gcount() != buffer.size())
            break;

        if (stream.needsNewChunk()) {
            stream.insertChunk(buffer);
        } else {
            LOG(ERROR) << "Failed to insert chunk";
        }
        int err ;
        StreamParser::StreamPacketT packet;

        do {
            err = stream.getNextPacket(packet);
            if (err  ==StreamParser::PSIParser::TSStream::OK) {
                ASSERT_EQ(packet[0], 0x47);
                for(int j = 0; j < TS_PACKAGE_SIZE; j ++) {
                }
            }
        } while (err == StreamParser::PSIParser::TSStream::OK);
    }

}

TEST_F(PSIParserTests, ZapPatternA) {
    validateCLEAR1();     // clear (initial)
    validateCLEAR2();     // clear
    validateENCRYPTED1(); // encrypted
    validateENCRYPTED2(); // encrypted
}

TEST_F(PSIParserTests, ZapPatternB) {
    validateCLEAR1();     // clear (initial)
    validateENCRYPTED1(); // encrypted
    validateCLEAR2();     // clear
    validateENCRYPTED2(); // encrypted
}

TEST_F(PSIParserTests, ZapPatternC) {
    validateENCRYPTED1(); // encrypted (initial)
    validateENCRYPTED2(); // encrypted
    validateCLEAR1();     // clear
    validateCLEAR2();     // clear
}

TEST_F(PSIParserTests, ZapPatternD) {
    validateENCRYPTED1(); // encrypted (initial)
    validateCLEAR1();     // clear
    validateENCRYPTED2(); // encrypted
    validateCLEAR2();     // clear
}
