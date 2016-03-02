#!/bin/sh

export ANDROID_SDK_DIR=/home/bernhard/data/entwicklung/android/android-sdk-linux
export ANDROID_NDK_DIR=/home/bernhard/data/entwicklung/android/android-ndk-r10e

export _APILEVEL=14

export CC="$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc --sysroot=$ANDROID_NDK_DIR/platforms/android-$_APILEVEL/arch-arm"
export CXX="$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-g++ --sysroot=$ANDROID_NDK_DIR/platforms/android-$_APILEVEL/arch-arm"
export AR="$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-ar"

#export PATH=$ANDROID_SDK_DIR/tools:$ANDROID_SDK_DIR/platform-tools:$PATH

set -e

#build SDL2
    if [ ! -f SDL2-2.0.4.tar.gz ]; then wget https://www.libsdl.org/release/SDL2-2.0.4.tar.gz; fi
        #44fc4a023349933e7f5d7a582f7b886e  SDL2-2.0.4.tar.gz
    if [ ! -d SDL2-2.0.4 ]; then
        tar -zxf SDL2-2.0.4.tar.gz
        cd SDL2-2.0.4
        patch -p1 < ../sdl-patches/0001-Avoid-warning-in-include-files.patch
        patch -p1 < ../sdl-patches/0002-Do-not-WarpMouse.patch
        patch -p1 < ../sdl-patches/0003-Generate-in-SDL-relative-Events.-Also-when-we-are-on.patch
        cd ..
    fi
    if [ ! -d SDL2-2.0.4-build ]; then
        mkdir SDL2-2.0.4-build
        cd    SDL2-2.0.4-build
        $PWD/../SDL2-2.0.4/configure \
                    --host=arm-linux-androideabi --prefix=${PWD}/inst-prefix \
                    --disable-shared --disable-pulseaudio --disable-esd --disable-video-wayland --disable-dbus --disable-ibus --disable-libudev \
                    --disable-video-opengles2 # ES2 seems not to work on a Sony Xperia MT15i "hallon" (Legacy Xperia)
        cd ..
    fi
    cd SDL2-2.0.4-build
    make -j2 all install
    cd ..
    if [ ! -f project/src/org/libsdl/app/SDLActivity.java ]; then cp -a ./SDL2-2.0.4/android-project/src/org project/src/ ;           fi
    if [ ! -f project/jni/SDL_android_main.c ];              then cp -a SDL2-2.0.4/src/main/android/SDL_android_main.c project/jni/ ; fi

#SDK one-time setup:
    if [ ! -f project/build.xml ]; then
        $ANDROID_SDK_DIR/tools/android update project --path $PWD/project/ --name Kwaak3 --target android-$_APILEVEL
    fi

#SRC in ioquake3:
    ln -sf misc/kwaak3/Makefile.local ../../Makefile.local
    cd ../..
    make -j2
    cd misc/kwaak3

#JNI:
    $ANDROID_NDK_DIR/ndk-build V=1 -C $PWD/ NDK_PROJECT_PATH=$PWD/project/

#copy .so
    cp -a ../../build/release-linux-arm/baseq3/cgamearm.so         project/libs/armeabi/libcgamearm.so
    cp -a ../../build/release-linux-arm/baseq3/qagamearm.so        project/libs/armeabi/libqagamearm.so
    cp -a ../../build/release-linux-arm/baseq3/uiarm.so            project/libs/armeabi/libuiarm.so

#copy icons
    (cd project/res; mkdir -p drawable-ldpi drawable-mdpi drawable-hdpi )
    cp -a ../quake3_flat.iconset/icon_512x512.png project/res/drawable-ldpi/icon.png
    cp -a ../quake3_flat.iconset/icon_512x512.png project/res/drawable-mdpi/icon.png
    cp -a ../quake3_flat.iconset/icon_512x512.png project/res/drawable-hdpi/icon.png

#JAVA:
    cd project
    ant debug
    cd ..

echo "Finished. Could be installed with '$ANDROID_SDK_DIR/platform-tools/adb install -r project/bin/Kwaak3-debug.apk'."

#clean:
#    rm ../../build/ project/{gen,libs,obj,bin,build.xml,local.properties,proguard-project.txt,project.properties} ../../Makefile.local SDL2-2.0.4-build -rf
