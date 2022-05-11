#!/bin/bash
 #
 # If not stated otherwise in this file or this component's LICENSE
 # file the following copyright and licenses apply:
 #
 # Copyright (c) 2022 Nuuday.
 #
 # Licensed under the Apache License, Version 2.0 (the "License");
 # you may not use this file except in compliance with the License.
 # You may obtain a copy of the License at
 #
 # http://www.apache.org/licenses/LICENSE-2.0
 #
 # Unless required by applicable law or agreed to in writing, software
 # distributed under the License is distributed on an "AS IS" BASIS,
 # WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 # See the License for the specific language governing permissions and
 # limitations under the License.
 #

##################################################################
#
# UPDATE SCRIPT FOR YS5000 StreamFS FCC plugin
#
# Usage: ./update_ocdm.sh /path_to/vmx_ocdm.drm <ssh_user>@<ip_address>
#
##################################################################

set -e

CDM_FILE_PATH="$1"
SSH_DEVICE=$2

TEMP_FILE=$(mktemp)
FILE_PATH=$(dirname $(realpath $CDM_FILE_PATH))
CDM_FILE=$(basename $CDM_FILE_PATH)
CUR_DIR="$(dirname "$(readlink -f "$0")")"

tar  -C $FILE_PATH -cvzf $TEMP_FILE  $CDM_FILE
cat $CUR_DIR/ys5000_update_streamfs.sh $TEMP_FILE > updater.sh

chmod ua+x updater.sh

scp updater.sh $SSH_DEVICE:/tmp/
ssh $SSH_DEVICE "bash /tmp/updater.sh && rm /tmp/updater.sh"

rm -f $TEMP_FILE
rm -f update.sh
