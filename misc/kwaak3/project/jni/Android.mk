# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ../../../../build/release-linux-arm/libioquake3.a
LOCAL_MODULE := ioquake3
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ../../SDL2-2.0.4-build/inst-prefix/lib/libSDL2.a
LOCAL_MODULE := SDL2
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := ../../SDL2-2.0.4-build/inst-prefix/lib/libSDL2main.a
LOCAL_MODULE := SDL2main
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_CFLAGS    := -ISDL2-2.0.4/src/main/android -ISDL2-2.0.4-build/inst-prefix/include/SDL2
LOCAL_LDLIBS    :=  -ldl -llog -lGLESv1_CM -lGLESv2 -landroid
LOCAL_LDFLAGS   := -Wl,--undefined=Java_org_libsdl_app_SDLActivity_nativeInit
LOCAL_MODULE    := kwaakjni
LOCAL_SRC_FILES := SDL_android_main.c
LOCAL_WHOLE_STATIC_LIBRARIES := ioquake3 SDL2 SDL2main
include $(BUILD_SHARED_LIBRARY)
