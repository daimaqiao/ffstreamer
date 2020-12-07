#/usr/bin/env

mkdir -p build && cd build

rm -rf a.out a.out.*

SRC_FILES="../main.cpp ../ffstreamer.cpp"

# for macos
g++ -g -std=c++11 -L/usr/local/lib -I/usr/local/include -lavcodec -lavformat -lavfilter -lavdevice -lswresample -lswscale -lavutil $SRC_FILES 

