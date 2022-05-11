#!/bin/sh
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

###########################################################
# Updater for STREAMFS
###########################################################
set -e
GREEN='\033[0;32m'
NC='\033[0m'
WS_URI="http://localhost/jsonrpc"


STREAMFS_PATH=/usr/lib/streamfs

RW_MOUNTS="${STREAMFS_PATH}"
RW_PATH="/systemdata/"

STREAMFS_RW_MOUNT="$RW_PATH/$STREAMFS_PATH"

PAYLOAD_LINE=`awk '/^__PAYLOAD_BELOW__/ {print NR + 1; exit 0; }' $0`

update_pid() {
   export CDM_PID=$(ps aux | grep libWPEFrameworkOCDM.so | grep WPEProcess | awk '{print $2}')
}



setup_rw_mounts() {
	for mpoint in ${RW_MOUNTS}
	do
		MOUNT_POINT=$(mount | grep "$mpoint" | awk '{print $3}')

		if [ -z "$MOUNT_POINT" ]
		then
			echo "Mount directory $MOUNT_POINT"
			mkdir -p ${RW_PATH}/$mpoint
			cp -af $mpoint/* ${RW_PATH}/$mpoint/ || true
			mount --bind ${RW_PATH}/$mpoint/ $mpoint
		else 
  			echo "Skip bind mount. Directory $MOUNT_POINT already mounted."
		fi
	done
}


extract_files() {
   tail -n+$PAYLOAD_LINE $0 | tar xzv -C ${STREAMFS_RW_MOUNT}
}

setup_rw_mounts

echo "Stopping OCDM..."
curl -d  '{"jsonrpc":"2.0","id":15343,"method":"Controller.1.deactivate","params":{"callsign":"OCDM"}}'  $WS_URI &> /dev/null
sleep 1


update_pid

if [ -z "$CDM_PID" ]
then
   echo "Stopped OCDM"
else
  echo "Asked nicely, but lost patience. Killing OCDM"
  kill -9 $CDM_PID
fi


echo "Stopping STREAMFS..."
systemctl stop streamfs
sleep 1

umount -f /mnt/streamfs || true

echo "Updating STREAMFS"

extract_files

echo "Start STREAMFS"

systemctl start streamfs

echo "Start OCDM"

curl -d  '{"jsonrpc":"2.0","id":15343,"method":"Controller.1.activate","params":{"callsign":"OCDM"}}' $WS_URI &> /dev/null

update_pid

if [ -z "$CDM_PID" ]
then
   echo "ERROR: Failed to start OCDM"
   exit -1
else
	echo "** Success: OCDM restarted **"
fi


exit 0

__PAYLOAD_BELOW__
