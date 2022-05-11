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
#include <string>

// File system node type
typedef const char* FsNode;

// Base type for dynamic config vars exposed by streamfs
typedef std::string ConfigVariableId;

/**
 * Map configuration value to config path
 *
 * Map internal configuration name to file system node
 */
typedef std::map<ConfigVariableId, FsNode> MapConfigToFSPathType;

// End of Generic Config

/**
 * Size of the buffer pool in seconds.
 */
#define TSB_SIZE_SEC 1

/**
 * Buffer chunk injection period for CBR in usec.
 */
#define BUFFER_CHUNK_CBR_PERIOD_USEC 12655

/**
 * Additional tail buffer size not to be used, but required to eliminate potential
 * out-of-bound indexing when reading from the oldest position in the TSB (i.e.
 * in the scenario where the player unexpected reduces the rate at which one or
 * more buffer are read relative to the rate at which buffers are injected into
 * the TSB from the stream source).
 */
#define BUFFER_POOL_TAIL_SIZE 100
/**
 * The actual buffer pool size derived from TSB_SIZE_SEC and BUFFER_CHUNK_CBR_PERIOD_USEC.
 */
#define BUFFER_POOL_SIZE (((0.5 + TSB_SIZE_SEC) * 1e6 / BUFFER_CHUNK_CBR_PERIOD_USEC) + BUFFER_POOL_TAIL_SIZE)

/**
 * Buffer index sampling ratio
 */
#define BUFFER_SAMPLING_RATIO 4

/**
 * Maximum size of time shift buffer
 */
#define TSB_MAX_DURATION_BYTES (1024*1024*1024)

#define MIN_BITRATE_SUPPORTED (1024*1024/8)

/**
 * Maximum duration of the Time Shift buffer
 */
#define TSB_MAX_DURATION_SECONDS (TSB_MAX_DURATION_BYTES/MIN_BITRATE_SUPPORTED)

#define INDEX_CB_SIZE (TSB_MAX_DURATION_SECONDS/INDEX_SAMPLING_INTERVAL_SECONDS)
// End of Generic Config

#define CONFIG_F_CHANNEL_SELECT    "chan_select0"
#define CONFIG_F_CHANNEL_SELECT_TIMESTAMP    "chan_select_timestamp0"
#define CONFIG_F_PLAYER_STATE "player_state0"
#define CONFIG_F_SEEK_CONTROL "seek0"
#define STREAM_SRC_FILE  "stream0.ts"

// FCC statistics module configs
#define CONFIG_F_STATS_DEVICE_IP "stat_device_ip"
#define CONFIG_F_STATS_SW_VERSION "sw_version"
#define CONFIG_F_MODEL_ID "model_id"

#define CONFIG_F_STAT_DAILY_OFFSET "stat_daily_offset"
#define CONFIG_F_STAT_RESET_STATS "feipmgr_reset_statistics"
#define CONFIG_F_STAT_ONDEMAND_STATISTIC "stat_ondemand_statistics"
#define CONFIG_F_STAT_PERIODIC_STAT_SERVER "stat_periodic_stat_server"
#define CONFIG_F_STAT_ONDEMAND_STAT_SERVER "stat_ondemand_stat_server"
#define CONFIG_F_STAT_INFORM_FIRST_FRAME "stat_inform_first_frame"
#define CONFIG_F_STAT_GLOBAL "stat_global"
#define CONFIG_F_STAT_CHANNEL "stat_channel"
#define CONFIG_F_STAT_PERIOD_FREQ "stat_periodic_freq"
#define CONFIG_F_STREAMFS_PID "pidfile"
#define CONFIG_F_STREAM_STATUS "stream_status"
#define CONFIG_FCC_PLUGIN_ID "fcc"

// Compile time djb2 HASH
// We don't care about possible equal values since the compiler will check
// that at compile time
static constexpr unsigned int hashStr(const char *str, int h = 0) {
    return !str[h] ? 5381 : (hashStr(str, h + 1) * 33) ^ str[h];
}

enum ConfigType {
    CHANNEL_SELECT  = 1,
    PLAYER_STATE    = 2,
    SEEK_CONTROL    = 3,
    STATS_CONTROL   = 4,
    STREAM_INFO     = 5,
    PROTECTION_INFO = 6,
    CDM_STATUS      = 7,
    TRICK_PLAY      = 8,
    UNDEFINED       = 9999
};

struct config_cb_type {
    const char *config_file;
    ConfigType type;
};

static const char *available_channels_g[] __attribute__ ((unused)) = {
        STREAM_SRC_FILE
};

// Internal configuration variables
static ConfigVariableId kDrm0       = "streamfs.fcc.kDrm0";   // NOLINT(cert-err58-cpp)
static ConfigVariableId kEcm0       = "streamfs.fcc.kEcm0";   // NOLINT(cert-err58-cpp)
static ConfigVariableId kPmt0       = "streamfs.fcc.kPmt0";   // NOLINT(cert-err58-cpp)
static ConfigVariableId kPat0       = "streamfs.fcc.kPat0";   // NOLINT(cert-err58-cpp)
static ConfigVariableId kFlush0     = "streamfs.fcc.kFlush0"; // NOLINT(cert-err58-cpp)
static ConfigVariableId kCdm0       = "streamfs.fcc.kCdm0";   // NOLINT(cert-err58-cpp)
static ConfigVariableId kTrickPlay0 = "streamfs.fcc.kTrickPlay0"; // NOLINT(cert-err58-cpp)

static ConfigVariableId kBufferSrcLost0  = "streamfs.fcc.kBufferSrcLost0"; // NOLINT(cert-err58-cpp)

// Stream INFO files

static constexpr FsNode CONFIG_F_PROTECTION_INFO      = "drm0";
static constexpr FsNode CONFIG_F_FCC_STREAM_INFO_ECM  = "ecm0";
static constexpr FsNode CONFIG_F_FCC_STREAM_INFO_PAT  = "pat0";
static constexpr FsNode CONFIG_F_FCC_STREAM_INFO_PMT  = "pmt0";
static constexpr FsNode CONFIG_F_STREAM_INFO_FLUSH    = "flush0";
static constexpr FsNode CONFIG_F_CDM_STATUS           = "cdm_ready0";
static constexpr FsNode CONFIG_F_TRICK_PLAY           = "trick_play0";

// Configuration handlers for different nodes.
static config_cb_type config_handlers_g[]  __attribute__ ((unused)) = {
        {CONFIG_F_CHANNEL_SELECT,            CHANNEL_SELECT},
        {CONFIG_F_CHANNEL_SELECT_TIMESTAMP,  CHANNEL_SELECT},
        {CONFIG_F_PLAYER_STATE,              PLAYER_STATE},
        {CONFIG_F_SEEK_CONTROL,              SEEK_CONTROL},
        {CONFIG_F_STATS_SW_VERSION,          STATS_CONTROL},
        {CONFIG_F_MODEL_ID,                  STATS_CONTROL},
        {CONFIG_F_MODEL_ID,                  STATS_CONTROL},
        {CONFIG_F_STAT_DAILY_OFFSET,         STATS_CONTROL},
        {CONFIG_F_STAT_RESET_STATS,          STATS_CONTROL},
        {CONFIG_F_STAT_ONDEMAND_STATISTIC,   STATS_CONTROL},
        {CONFIG_F_STAT_PERIODIC_STAT_SERVER, STATS_CONTROL},
        {CONFIG_F_STAT_ONDEMAND_STAT_SERVER, STATS_CONTROL},
        {CONFIG_F_STAT_INFORM_FIRST_FRAME,   STATS_CONTROL},
        {CONFIG_F_STAT_GLOBAL,               STATS_CONTROL},
        {CONFIG_F_STAT_CHANNEL,              STATS_CONTROL},
        {CONFIG_F_STATS_DEVICE_IP,           STATS_CONTROL},
        {CONFIG_F_STAT_PERIOD_FREQ,          STATS_CONTROL},
        {CONFIG_F_PROTECTION_INFO,           PROTECTION_INFO},
        {CONFIG_F_CDM_STATUS,                CDM_STATUS},
        {CONFIG_F_FCC_STREAM_INFO_ECM,       STREAM_INFO},
        {CONFIG_F_FCC_STREAM_INFO_PAT,       STREAM_INFO},
        {CONFIG_F_FCC_STREAM_INFO_PMT,       STREAM_INFO},
        {CONFIG_F_STREAMFS_PID,              STATS_CONTROL},
        {CONFIG_F_STREAM_INFO_FLUSH,         SEEK_CONTROL},
        {CONFIG_F_STREAM_STATUS,               STATS_CONTROL},
        {CONFIG_F_TRICK_PLAY,               TRICK_PLAY},
};
