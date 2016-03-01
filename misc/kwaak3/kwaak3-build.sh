#!/bin/sh

export ANDROID_SDK_DIR=/home/bernhard/data/entwicklung/android/android-sdk-linux
export ANDROID_NDK_DIR=/home/bernhard/data/entwicklung/android/android-ndk-r10e

export ANDROID_SDK_HOME=/home/bernhard/data/entwicklung/android
    #just needed if .android is not in HOME, for debug signing key

export _APILEVEL=9

export CC="$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-gcc --sysroot=$ANDROID_NDK_DIR/platforms/android-$_APILEVEL/arch-arm"
export CPP="$ANDROID_NDK_DIR/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/bin/arm-linux-androideabi-cpp --sysroot=$ANDROID_NDK_DIR/platforms/android-$_APILEVEL/arch-arm"

set -e

#SDK one-time setup:
    if [ ! -f project/build.xml ]; then
        $ANDROID_SDK_DIR/tools/android update project --path $PWD/project/ --name Kwaak3 --target android-$_APILEVEL
    fi

#generate qgl.h
    #cd ../../code/android
    #/usr/bin/perl ./GenerateQGL.pl > qgl.h
    #   #warning: #import is a deprecated GCC extension [-Wdeprecated]
    #   #"import" is probably needed to force one time inclusion ...
    #cd misc/kwaak3

#SRC in ioquake3:
    ln -sf misc/kwaak3/Makefile.local ../../Makefile.local
    cd ../..
    make -j2
    cd misc/kwaak3

#JNI:
    $ANDROID_NDK_DIR/ndk-build V=1 -C $PWD/ NDK_PROJECT_PATH=$PWD/project/

#copy .so
    #libioquake3.so is copied in misc/kwaak3/project/jni/Android.mk
    cp -a ../../build/release-linux-arm/renderer_opengl1_arm.so    project/libs/armeabi/librenderer_opengl1_arm.so
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

#regenerate JNI header:
    #(cd project/bin/classes; javah -d ../../jni/  org.kwaak3.KwaakJNI)

echo "Finished. Could be installed with '$ANDROID_SDK_DIR/platform-tools/adb install -r project/bin/Kwaak3-debug.apk'."

#clean:
#    rm ../../build/ project/{gen,libs,obj,bin,build.xml,local.properties,proguard-project.txt,project.properties} ../../Makefile.local -rf
