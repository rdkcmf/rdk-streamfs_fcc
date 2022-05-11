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

#include "gtest/gtest.h"
#include <boost/algorithm/string.hpp>
#include "unit_tests.h"
#include "StreamProtectionConfigExport.h"
#include "confighandler/ProtectionInfoRequestHandler.h"

class ProtectionInfoRequestHandlerTests : public ::testing::Test {
public:

    ProtectionInfoRequestHandlerTests() {
        mMockPluginInterface = new MockPluginCallbackInterface();
        mHandler = new fcc::ProtectionInfoRequestHandler(mMockPluginInterface);
        mDrm = &MVar<StreamProtectionConfig>::getVariable(kDrm0);
    }

    ~ProtectionInfoRequestHandlerTests() override {
        delete mHandler;
        delete mMockPluginInterface;
    }

public:
    MockPluginCallbackInterface *mMockPluginInterface;
    fcc::ProtectionInfoRequestHandler *mHandler;
    MVar<StreamProtectionConfig> *mDrm;

protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();

    }
};

// Helper function to trim and sort (in alphabetical order) a JSON formatted string
// outputted from StreamProtectionConfigExport
inline std::string trimAndSort(const std::string& input) {
    std::string trimmed = input.substr(1, input.size()-2);
    std::vector<std::string> result;
    boost::split(result, trimmed, boost::is_any_of(","));
    std::sort(result.begin(), result.end());
    return boost::join(result,",");
}

// Test vectors to StreamProtectionConfig input and StreamProtectionConfigExport output
struct {
    struct {
        // Input to StreamProtectionConfig
        const std::string channel{"239.100.0.1:8433"};
        // Expected and alphabetical sorted KV output from StreamProtectionConfigExport
        const std::string kvRef{"\"channel\":\"239.100.0.1:8433\",\"clear\":true,\"ecm\":\"\",\"pat\":\"\",\"pmt\":\"\""};
    } init;

    struct {
        // Input to StreamProtectionConfig
        const std::string channel{"239.100.0.2:8433"};
        const ByteVectorType ecm{10,'0','a'};
        const ByteVectorType pat{11,'1','b'};
        const ByteVectorType pmt{12,'2','c'};
        const bool clear = false;
        // Expected and alphabetical sorted KV output from StreamProtectionConfigExport
        const std::string kvRef{"\"channel\":\"239.100.0.2:8433\",\"clear\":false,\"ecm\":\"0A3061\",\"pat\":\"0B3162\",\"pmt\":\"0C3263\""};
    } setA;

    struct {
        // Input to StreamProtectionConfig
        const std::string channel{"239.100.0.3:8433"};
        const ByteVectorType ecm{13,'3','d'};
        const ByteVectorType pat{14,'4','e'};
        const ByteVectorType pmt{15,'5','f'};
        const bool clear = false;
        // Expected and alphabetical sorted KV output from StreamProtectionConfigExport
        const std::string kvRef{"\"channel\":\"239.100.0.3:8433\",\"clear\":false,\"ecm\":\"0D3364\",\"pat\":\"0E3465\",\"pmt\":\"0F3566\""};
    } setB;
} static testVectors;

TEST(StreamProtectionConfig, InitConfigAndExport) {
    // Create initial configuration with no ECM/PAT/PMT values
    auto config = StreamProtectionConfig(testVectors.init.channel);

    // Test that confidence level is correctly set to RESET
    ASSERT_EQ(config.confidence(), StreamProtectionConfig::ConfidenceTypes::RESET);
    StreamProtectionConfigExport configExport = StreamProtectionConfigExport(config);
    // Test that the exporter provides the expected JSON string
    ASSERT_EQ(trimAndSort(configExport.getJsonAsString()), testVectors.init.kvRef);
};

TEST(StreamProtectionConfig, ConfigAndExport) {
    // Create configuration with ECM/PAT/PMT values and HIGH confidence level
    auto config = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::HIGH,
                                         testVectors.setA.channel,
                                         testVectors.setA.ecm,
                                         testVectors.setA.pat,
                                         testVectors.setA.pmt,
                                         testVectors.setA.clear);

    // Test that confidence level is correctly set to HIGH
    ASSERT_EQ(config.confidence(), StreamProtectionConfig::ConfidenceTypes::HIGH);
    auto configExport = StreamProtectionConfigExport(config);
    // Test that the exporter provides the expected JSON string
    ASSERT_EQ(trimAndSort(configExport.getJsonAsString()), testVectors.setA.kvRef);
};

TEST_F(ProtectionInfoRequestHandlerTests, ReadAndConfidenceUpdate) {

    EXPECT_CALL(*mMockPluginInterface, notifyUpdate(testing::_))
            .Times(5);

    // Create configuration with ECM/PAT/PMT setA test vector values and MID confidence level
    auto configA = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::MID,
                                          testVectors.setA.channel,
                                          testVectors.setA.ecm,
                                          testVectors.setA.pat,
                                          testVectors.setA.pmt,
                                          testVectors.setA.clear);

    // Assign configuration with MID confidence level to the associated monitored variable
    *mDrm = configA;
    // Test that the protection request config handler provides the expected JSON string
    // for the assigned configuration with the MID confidence level
    ASSERT_EQ(trimAndSort(mHandler->readConfig("")), testVectors.setA.kvRef);

    // Create configuration with ECM/PAT/PMT setB test vector values and HIGH confidence level
    auto configB = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::HIGH,
                                          testVectors.setB.channel,
                                          testVectors.setB.ecm,
                                          testVectors.setB.pat,
                                          testVectors.setB.pmt,
                                          testVectors.setB.clear);

    // Assign configuration with HIGH confidence level to the associated monitored variable
    *mDrm = configB;
    // Test that the protection request config handler provides the expected JSON string
    // for the configuration with the HIGH confidence level
    ASSERT_EQ(trimAndSort(mHandler->readConfig("")), testVectors.setB.kvRef);

    // Assign configuration with MID confidence level to the associated monitored variable
    *mDrm = configA;
    // Test that the protection request config handler provides the expected JSON string
    // for the configuration with the HIGH confidence level, meaning that the
    // configuration with the MID confidence value is ignored
    ASSERT_EQ(trimAndSort(mHandler->readConfig("")), testVectors.setB.kvRef);

    // Create and assign initial configuration with no ECM/PAT/PMT values and
    *mDrm = StreamProtectionConfig(testVectors.init.channel);
    // Test that the protection request config handler provides the expected JSON string
    // for the initial configuration
    ASSERT_EQ(trimAndSort(mHandler->readConfig("")), testVectors.init.kvRef);

    // Create configuration with ECM/PAT/PMT setA test vector values and HIGH confidence level
    configA = StreamProtectionConfig(StreamProtectionConfig::ConfidenceTypes::HIGH,
                                     testVectors.setA.channel,
                                     testVectors.setA.ecm,
                                     testVectors.setA.pat,
                                     testVectors.setA.pmt,
                                     testVectors.setA.clear);
    // Assign configuration with HIGH confidence to the associated monitored variable
    *mDrm = configA;
    // Test that the protection request config handler provides the expected JSON string
    // for the configuration based on setA test vector values and HIGH confidence level
    ASSERT_EQ(trimAndSort(mHandler->readConfig("")), testVectors.setA.kvRef);

    // Assign configuration with HIGH confidence to the associated monitored variable
    *mDrm = configB;
    // Test that the protection request config handler provides the expected JSON string
    // for the configuration based on setB test vector values and HIGH confidence level
    ASSERT_EQ(trimAndSort(mHandler->readConfig("")), testVectors.setB.kvRef);
}
