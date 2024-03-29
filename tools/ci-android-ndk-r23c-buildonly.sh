#!/bin/sh
set -ev

rm -Rf build/android-ndk-r23c
mkdir -p build/android-ndk-r23c
cd build/android-ndk-r23c

cmake -DCMAKE_BUILD_TYPE=Release -DANDROID_ABI=arm64-v8a -DANDROID_NDK=${ANDROID_HOME}/ndk/23.2.8568313 -DCMAKE_TOOLCHAIN_FILE=${ANDROID_HOME}/ndk/23.2.8568313/build/cmake/android.toolchain.cmake -G Ninja ../..

cmake --build .

# TODO, push to virtual device and run tests there
# ctest --output-on-failure

