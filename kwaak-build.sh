#!/bin/sh

export ANDROID_SDK_DIR=/home/bernhard/data/entwicklung/android/android-sdk-linux
export ANDROID_NDK_DIR=/home/bernhard/data/entwicklung/android/android-ndk-r10e
export CC=$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc

export ANDROID_SDK_HOME=/home/bernhard/data/entwicklung/android
    #just needed if .android is not in HOME, for debug signing key

set -e

#SDK one-time setup:
    if [ ! -f kwaak/project/build.xml ]; then
        $ANDROID_SDK_DIR/tools/android update project --path $PWD/kwaak/project/ --name Kwaak3 --target android-11
    fi

#SRC in ioquake3:
    cd ioquake3
    make
    cd ..

#JNI:
    $ANDROID_NDK_DIR/ndk-build -C $PWD/kwaak/ NDK_PROJECT_PATH=$PWD/kwaak/project/

#copy .so
    cp ioquake3/build/release-linux-arm/libquake3.so kwaak/project/libs/armeabi/libquake3_neon.so
    cp ioquake3/build/release-linux-arm/baseq3/*.so kwaak/project/libs/armeabi/

#JAVA:
    cd kwaak/project
    ant debug
    cd ../..

echo "Finished. Could be installed with '$ANDROID_SDK_DIR/platform-tools/adb install -r kwaak/project/bin/Kwaak3-debug.apk'."
