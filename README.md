## Dependencies

Build and install streamfs master from here: https://github.com/kuscsik/streamfs

**Boost regex:** sudo apt-get install -y libboost-regex-dev (or with system using boost 1.7 as default: sudo apt-get install -y libboost-regex1.67-dev)

**Curses:** sudo apt-get install libncurses5-dev libncursesw5-dev (required for test target only )

For X86 Linux builds, install the perfetto profiler:

```
$ wget https://github.com/kuscsik/perfetto/releases/download/0.1-2/perfetto_0.1-2ubuntu4_amd64.deb
$ sudo dpkg -i perfetto_0.1-2ubuntu4_amd64.deb
```

## Building

```
$ mkdir obj
$ cd obj
$ cmake ../streamfs_fcc
$ make -j8
$ sudo make install
```

## Usage

```
$ streamfs -o direct_io -f temp/
$ echo echo 239.100.0.1 > temp/fcc/chan_select0
$ cat temp/fcc/stream0.ts  | gst-launch-1.0 -v fdsrc ! tsdemux  ! queue ! h264parse ! avdec
_h264 ! videoconvert ! videoscale ! ximagesink
```

## Enable tracing

If libperfetto tracing is available on the platform the tracing library can be enabled using

```sh
$ cmake ../ -DPERFETTO_TRACING=ON
```

## Running unit tests

Build the plugin with unit tests enabled

```
$ cmake  -DBUILD_UNIT_TESTS=ON
```

Run the tests
```
(shell2) $ ./test/unit_tests
```

## Running http streams through streamfs

$ export STREAM_TYPE=http
$ streamfs -o direct_io -f temp/
$ echo [http video path] > temp/fcc/chan_select0
  for example : echo http://192.168.1.5:8080/BBCEarth.ts > temp/fcc/chan_select0
$ gst-play-1.0 temp/fcc/stream0.ts

## Running http functional tests

$ create a new file "streamfsvideofile.conf" under /home/root
$ add the hosted streams url in the file
  for example : echo http://192.168.1.5:8080/BBCEarth.ts > temp/fcc/chan_select0
$ export STREAM_TYPE=http
$ streamfs -o direct_io -f temp/
$ ./http_functional_tests
