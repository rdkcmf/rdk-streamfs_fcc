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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#define SOURCE_IP_STR "sourceip"

using namespace boost::algorithm;

/**
 * Channel configuration obtained from channel config file
 */
struct ChannelConfig {
public:
    ChannelConfig() {

    }
    /**
     * @param uri
     * @param defaultPort
     */
    ChannelConfig(const std::string& uri, uint16_t defaultPort = DEFAULT_SOCKET_PORT) {
        int portIn = 0;
        std::string dstIp, dstPort, srcIp;

        extract(uri, dstIp, dstPort, srcIp);

        LOG(INFO) << "Destination IP = " << dstIp
            << " port = " << dstPort
            << " srcIp = " << srcIp;

        if (dstIp.length() == 0) {
            LOG(ERROR) << "Invalid URI:" << uri;
            return;
        }

        if (dstPort.length() > 0 ) {
            try {
                portIn = std::stoi(dstPort);
            } catch (std::exception const & e)
            {
                LOG(WARNING) << "Invalid port: " << dstPort << " using default value";
                portIn = defaultPort;
            }
        } else {
            LOG(WARNING) << "No port specified. Using default port: " << defaultPort;
            portIn = defaultPort;
        }

        if (portIn <= 0 || portIn >= USHRT_MAX) {
            LOG(ERROR) << "Invalid port number: " << port;
            mIsValid = false;
            return;
        }

        port = portIn;
        destinationIp = parseIPV4string(dstIp.c_str());
        destIPDotDecimal = dstIp;

        mIsValid = true;

        if (srcIp.length() > 0) {
            sourceIp = parseIPV4string(srcIp.c_str());
            srcIPDotDecimal = srcIp;
        } else {
            sourceIp = 0x0;
        }

        mIsValid = true;
    }

    bool IsValid() {
        return mIsValid;
    }

private:
    /**
     * Extract IP, port and sourceIp for IGMP3
     * Example inputs:
     *
     *   234.80.160.204:5900/?sourceIp=2.3.4.5
     *   234.80.160.204:5900
     *
     * @param URI     - input URI
     * @param address - IP address
     * @param p       - port
     * @param sIp     - source IP address
     */
    void extract(std::string const& URI, std::string& address, std::string& p, std::string& sIp) {
        boost::regex e("(\\d+\\.\\d+\\.\\d+\\.\\d+)(:[0-9]+)?(/[^\\r\\n]*)?");
        boost::smatch what;
        std::string srcS;

        if (boost::regex_match(URI, what, e, boost::match_extra)) {
            boost::smatch::iterator it = what.begin();
            ++it;
            address = *it;
            ++it;
            // Second group
            std::string tmp = *it;
            if (tmp.length() > 1) {
                // Remove ":" from beginning of the string
                p = tmp.substr(1, tmp.length() - 1);
            }
            ++it;
            srcS = *it;
        }

        boost::regex matchSourceIp("\\/\\?(\\w+)=(\\d+\\.\\d+\\.\\d+\\.\\d+)");

        if (boost::regex_match(srcS, what, matchSourceIp, boost::match_extra)) {
            boost::smatch::iterator it = what.begin();
            ++it;
            std::string sip = *it;
            to_lower(sip);
            if (sip == SOURCE_IP_STR) {
                ++it;
                if (it != what.end()) {
                    sIp = *it;
                }
            }
        }
    }
    static uint32_t parseIPV4string(const char *ipAddress) {
        in_addr addr;
        inet_aton(ipAddress, &addr);
        return ntohl(addr.s_addr);
    }
private:
    bool mIsValid = false;
public:
    /**
     * Destination IP
     */
    uint32_t destinationIp = 0;

    /**
     * Destination IP
     */
    std::string destIPDotDecimal {};
    /**
     * Destination port
     */
    uint16_t port = 0;
    /**
     * Source IP (IGMPv3 only)
     */
    uint32_t sourceIp = 0;

    /**
     * Source IP
     */
    std::string srcIPDotDecimal {};

};

