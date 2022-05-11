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

#include <map>

namespace yousee {
enum SourceType {
    MULTICAST,
    MSTV
};

enum ServiceType {
    MPEG,
    HEVC_HD,
    MPEG4_HD,
    RADIO
};

enum ChanIds {
    CHAN_DR1,
    CHAN_DR1_SYNSTOLKNING,
    CHAN_TV_2_ZULU,
    CHAN_TV_2_FRI,
    CHAN_NATIONAL_GEOGRAPHIC,
    CHAN_BBC_WORLD_NEWS,
    CHAN_DISNEY_XD,
    CHAN_TV_2_NEWS,
    CHAN_CNN,
    CHAN_DR_P1,
    CHAN_DR_P3,
    CHAN_RADIO4,
    CHAN_NOVA_FM,
    CHAN_THE_VOICE,
    CHAN_DR_P5,
    CHAN_RADIO_100,
    CHAN_DR_P4_NORDJYLLAND,
    CHAN_TV3,
    CHAN_DR2,
    CHAN_DR2_SYNSTOLKNING,
    CHAN_TV_2_DANMARK,
    CHAN_NORD,
    CHAN_ARD,
    CHAN_SVT2,
    CHAN_DISNEY_JUNIOR,
    CHAN_HBO_NORDIC_TV,
    CHAN_C_MORE_HITS,
    CHAN_DANSK_FILMSKAT_TV,
    CHAN_TV_2_SPORT,
    CHAN_NORDISK_FILM_TV,
    CHAN_TV_2_CHARLIE,
    CHAN_EVENTKANALEN,
    CHAN_C_MORE_STARS,
    CHAN_C_MORE_SERIES,
    CHAN_TV2NOR,
    CHAN_TV_MID,
    CHAN_ZDF,
    CHAN_NDR,
    CHAN_RTL,
    CHAN_DK4,
    CHAN_XEE,
    CHAN_SVT1,
    CHAN_TV4,
    CHAN_NRK1,
    CHAN_FOLKETINGET,
    CHAN_PARAMOUNT_NETWORK,
    CHAN_DR_RAMASJANG,
    CHAN_INFOKANALEN,
    CHAN_EKSTRAKANALEN,
    CHAN_TRAILERKANALEN,
    CHAN_NATIONAL_GEOGRAPHIC_WILD,
    CHAN_C_MORE_FIRST,
    CHAN_TV_2_SPORT_X,
    CHAN_DISNEY_CHANNEL
};


struct ChannelDescriptor {
    ChanIds id;
    const char *name;
    const char *uri;
    SourceType sourceType;
    ServiceType serviceType;
};

const ChannelDescriptor channelList[] = {
        {
                .id = CHAN_DR1,
                .name = "DR1",
                .uri =  "234.80.160.1:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DR1_SYNSTOLKNING,
                .name = "DR1_SYNSTOLKNING",
                .uri =  "234.80.160.169:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_ZULU,
                .name = "TV_2_ZULU",
                .uri =  "234.80.160.13:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_TV_2_FRI,
                .name = "TV_2_FRI",
                .uri =  "234.80.160.27:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_NATIONAL_GEOGRAPHIC,
                .name = "NATIONAL_GEOGRAPHIC",
                .uri =  "234.80.160.29:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_BBC_WORLD_NEWS,
                .name = "BBC_WORLD_NEWS",
                .uri =  "234.80.160.197:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DISNEY_XD,
                .name = "DISNEY_XD",
                .uri =  "234.80.160.130:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_NEWS,
                .name = "TV_2_NEWS",
                .uri =  "234.80.160.15:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_CNN,
                .name = "CNN",
                .uri =  "234.80.160.207:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_DR_P1,
                .name = "DR_P1",
                .uri =  "234.80.161.1:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_DR_P3,
                .name = "DR_P3",
                .uri =  "234.80.161.2:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_RADIO4,
                .name = "RADIO4",
                .uri =  "234.80.161.15:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_NOVA_FM,
                .name = "NOVA_FM",
                .uri =  "234.80.161.16:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_THE_VOICE,
                .name = "THE_VOICE",
                .uri =  "234.80.161.18:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_DR_P5,
                .name = "DR_P5",
                .uri =  "234.80.161.14:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_RADIO_100,
                .name = "RADIO_100",
                .uri =  "234.80.161.17:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_DR_P4_NORDJYLLAND,
                .name = "DR_P4_NORDJYLLAND",
                .uri =  "234.80.161.8:5900",
                .sourceType = MULTICAST,
                .serviceType = RADIO,
        },
        {
                .id = CHAN_TV3,
                .name = "TV3",
                .uri =  "234.80.160.10:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DR2,
                .name = "DR2",
                .uri =  "234.80.160.24:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DR2_SYNSTOLKNING,
                .name = "DR2_SYNSTOLKNING",
                .uri =  "234.80.160.170:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_DANMARK,
                .name = "TV_2_DANMARK",
                .uri =  "234.80.160.8:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_NORD,
                .name = "NORD",
                .uri =  "234.80.160.179:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_ARD,
                .name = "ARD",
                .uri =  "234.80.160.157:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_SVT2,
                .name = "SVT2",
                .uri =  "234.80.160.154:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DISNEY_JUNIOR,
                .name = "DISNEY_JUNIOR",
                .uri =  "234.80.160.131:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_HBO_NORDIC_TV,
                .name = "HBO_NORDIC_TV",
                .uri =  "234.80.160.212:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_C_MORE_HITS,
                .name = "C_MORE_HITS",
                .uri =  "234.80.160.209:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_DANSK_FILMSKAT_TV,
                .name = "DANSK_FILMSKAT_TV",
                .uri =  "234.80.160.214:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_TV_2_SPORT,
                .name = "TV_2_SPORT",
                .uri =  "234.80.160.36:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_NORDISK_FILM_TV,
                .name = "NORDISK_FILM+_TV",
                .uri =  "234.80.160.215:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_TV_2_CHARLIE,
                .name = "TV_2_CHARLIE",
                .uri =  "234.80.160.14:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_EVENTKANALEN,
                .name = "EVENTKANALEN",
                .uri =  "234.80.160.162:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_C_MORE_STARS,
                .name = "C_MORE_STARS",
                .uri =  "234.80.160.196:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_C_MORE_SERIES,
                .name = "C_MORE_SERIES",
                .uri =  "234.80.160.210:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_TV2NOR,
                .name = "TV2NOR",
                .uri =  "234.80.160.173:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_MID,
                .name = "TV_MID",
                .uri =  "234.80.160.174:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_ZDF,
                .name = "ZDF",
                .uri =  "234.80.160.158:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_NDR,
                .name = "NDR",
                .uri =  "234.80.160.159:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_RTL,
                .name = "RTL",
                .uri =  "234.80.160.160:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DK4,
                .name = "DK4",
                .uri =  "234.80.160.192:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_XEE,
                .name = "XEE",
                .uri =  "234.80.160.11:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_SVT1,
                .name = "SVT1",
                .uri =  "234.80.160.153:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV4,
                .name = "TV4",
                .uri =  "234.80.160.155:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_NRK1,
                .name = "NRK1",
                .uri =  "234.80.160.156:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_FOLKETINGET,
                .name = "FOLKETINGET",
                .uri =  "234.80.160.208:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_PARAMOUNT_NETWORK,
                .name = "PARAMOUNT_NETWORK",
                .uri =  "234.80.160.18:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_DR_RAMASJANG,
                .name = "DR_RAMASJANG",
                .uri =  "234.80.160.200:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_INFOKANALEN,
                .name = "INFOKANALEN",
                .uri =  "234.80.160.190:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_EKSTRAKANALEN,
                .name = "EKSTRAKANALEN",
                .uri =  "234.80.160.194:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_TRAILERKANALEN,
                .name = "TRAILERKANALEN",
                .uri =  "234.80.160.146:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_NATIONAL_GEOGRAPHIC_WILD,
                .name = "NATIONAL_GEOGRAPHIC_WILD",
                .uri =  "234.80.160.204:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_C_MORE_FIRST,
                .name = "C_MORE_FIRST",
                .uri =  "234.80.160.195:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_SPORT_X,
                .name = "TV_2_SPORT_X",
                .uri =  "234.80.160.37:5900",
                .sourceType = MULTICAST,
                .serviceType = MPEG4_HD,
        },
        {
                .id = CHAN_DISNEY_CHANNEL,
                .name = "DISNEY_CHANNEL",
                .uri =  "234.80.160.129:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DR1,
                .name = "DR1",
                .uri =  "234.80.160.50:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_ZULU,
                .name = "TV_2_ZULU",
                .uri =  "234.80.160.62:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_FRI,
                .name = "TV_2_FRI",
                .uri =  "234.80.160.72:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_NEWS,
                .name = "TV_2_NEWS",
                .uri =  "234.80.160.64:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV3,
                .name = "TV3",
                .uri =  "234.80.160.59:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_DR2,
                .name = "DR2",
                .uri =  "234.80.160.69:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_DANMARK,
                .name = "TV_2_DANMARK",
                .uri =  "234.80.160.57:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_SPORT,
                .name = "TV_2_SPORT",
                .uri =  "234.80.160.78:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_CHARLIE,
                .name = "TV_2_CHARLIE",
                .uri =  "234.80.160.63:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_XEE,
                .name = "XEE",
                .uri =  "234.80.160.60:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_PARAMOUNT_NETWORK,
                .name = "PARAMOUNT_NETWORK",
                .uri =  "234.80.160.66:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        },
        {
                .id = CHAN_TV_2_SPORT_X,
                .name = "TV_2_SPORT_X",
                .uri =  "234.80.160.79:5900",
                .sourceType = MULTICAST,
                .serviceType = HEVC_HD,
        }
};

class YouseeChannels {
public:

    static YouseeChannels& getInstance()
    {
        static YouseeChannels instance;

        return instance;
    }

    std::map<ChanIds, ChannelDescriptor> & getChannels() { return mChanMap; }
private:
    YouseeChannels();

    std::map<ChanIds, ChannelDescriptor> mChanMap;
    size_t mChannelCount;
};

}

