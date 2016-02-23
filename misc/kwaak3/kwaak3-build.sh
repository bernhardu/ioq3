#!/bin/sh

export ANDROID_SDK_DIR=/home/bernhard/data/entwicklung/android/android-sdk-linux
export ANDROID_NDK_DIR=/home/bernhard/data/entwicklung/android/android-ndk-r10e
export CC=$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc

export ANDROID_SDK_HOME=/home/bernhard/data/entwicklung/android
    #just needed if .android is not in HOME, for debug signing key

set -e

#SDK one-time setup:
    if [ ! -f project/build.xml ]; then
        $ANDROID_SDK_DIR/tools/android update project --path $PWD/project/ --name Kwaak3 --target android-11
    fi

#SRC in ioquake3:
    cd ../..
    make
    cd misc/kwaak3

#JNI:
    $ANDROID_NDK_DIR/ndk-build -C $PWD/ NDK_PROJECT_PATH=$PWD/project/

#copy .so
    cp ../../build/release-linux-arm/libquake3.so project/libs/armeabi/libquake3_neon.so
    cp ../../build/release-linux-arm/baseq3/*.so project/libs/armeabi/

#JAVA:
    cd project
    ant debug
    cd ../..

echo "Finished. Could be installed with '$ANDROID_SDK_DIR/platform-tools/adb install -r project/bin/Kwaak3-debug.apk'."
