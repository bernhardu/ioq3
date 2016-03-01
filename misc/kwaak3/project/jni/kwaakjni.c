/*
 * Kwaak3 - Java to quake3 interface
 * Copyright (C) 2010 Roderick Colenbrander
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <android/log.h>

#include "org_kwaak3_KwaakJNI.h"

#define DEBUG

#define _LOG_OUTPUT(level, args...) \
    __android_log_print(level, "Quake_JNI", args)

#define LOG_DEBUG(args...) _LOG_OUTPUT(ANDROID_LOG_DEBUG, args)
#define LOG_ERROR(args...) _LOG_OUTPUT(ANDROID_LOG_ERROR, args)
#ifdef DEBUG
#define LOG_TRACE(args...) _LOG_OUTPUT(ANDROID_LOG_DEBUG, args)
#else
#define LOG_TRACE(args...)
#endif

/* Function prototypes to Quake3 code */
int  q3main(int argc, char **argv);
void nextFrame();
void queueKeyEvent(int key, int state);
void queueMotionEvent(int action, float x, float y, float pressure);
void queueTrackballEvent(int action, float x, float y);
void requestAudioData();
void setAudioCallbacks(void *func, void *func2, void *func3);
void setInputCallbacks(void *func);

/* Callbacks to Android */
jmethodID android_getPos=NULL;
jmethodID android_initAudio=NULL;
jmethodID android_writeAudio=NULL;
jmethodID android_setMenuState=NULL;

/* Contains the game directory e.g. /mnt/sdcard/quake3 */
static char* game_dir=NULL;

/* Containts the path to /data/data/(package_name)/libs */
static char* lib_dir=NULL;

static JavaVM *jVM;
static jboolean audioEnabled=1;
static jboolean benchmarkEnabled=0;
static jboolean lightmapsEnabled=0;
static jboolean showFramerateEnabled=0;
static jobject audioBuffer=0;
static jobject kwaakAudioObj=0;
static jobject kwaakRendererObj=0;

typedef enum fp_type
{
     FP_TYPE_NONE = 0,
     FP_TYPE_VFP  = 1,
     FP_TYPE_NEON = 2
} fp_type_t;

static fp_type_t fp_support()
{
    char buf[80];
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if(!fp)
    {
        LOG_ERROR("Unable to open /proc/cpuinfo\n");
        return FP_TYPE_NONE;
    }

    while(fgets(buf, 80, fp) != NULL)
    {
        char *features = strstr(buf, "Features");

        if(features)
        {
            fp_type_t fp_supported_type = FP_TYPE_NONE;
            char *feature;
            features += strlen("Features");
            feature = strtok(features, ": ");
            while(feature)
            {
                /* We prefer Neon if it is around, else VFP is also okay */
                if(!strcmp(feature, "neon"))
                    return FP_TYPE_NEON;
                else if(!strcmp(feature, "vfp"))
                    fp_supported_type = FP_TYPE_VFP;

                feature = strtok(NULL, ": ");
            }
            return fp_supported_type;
        }
    }
    return FP_TYPE_NONE;
}

const char *get_quake3_library()
{
    /* We ship a library with Neon FPU support. This boosts performance a lot but it only works on a few CPUs. */
    fp_type_t fp_supported_type = fp_support();
    if(fp_supported_type == FP_TYPE_NEON)
        return "libquake3_neon.so";
    else if (fp_supported_type == FP_TYPE_VFP)
        return "libquake3_vfp.so";

    return "libquake3.so";
}

void get_quake3_library_path(char *path)
{
    const char *libquake3 = get_quake3_library();
    if(lib_dir)
    {
        sprintf(path, "%s/%s", lib_dir, libquake3);
    }
    else
    {
        LOG_ERROR("Library path not set, trying /data/data/org.kwaak3/lib");
        sprintf(path, "/data/data/org.kwaak3/lib/%s", libquake3);
    }
}

int getPos()
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    LOG_TRACE("getPos");

    if (!android_getPos) {
        LOG_ERROR("android_getPos is not set.");
        return 0;
    }

    return (*env)->CallIntMethod(env, kwaakAudioObj, android_getPos);
}

void initAudio(void *buffer, int size)
{
    JNIEnv *env;
    jobject tmp;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    LOG_TRACE("initAudio");

    tmp = (*env)->NewDirectByteBuffer(env, buffer, size);
    audioBuffer = (jobject)(*env)->NewGlobalRef(env, tmp);

    if(!audioBuffer)
        LOG_ERROR("yikes, unable to initialize audio buffer");

    if (!android_initAudio) {
        LOG_ERROR("android_initAudio is not set.");
        return;
    }

    return (*env)->CallVoidMethod(env, kwaakAudioObj, android_initAudio);
}

void writeAudio(int offset, int length)
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    /*LOG_TRACE("writeAudio audioBuffer=%p offset=%d length=%d", audioBuffer, offset, length); // too noisy */

    if (!android_writeAudio) {
        LOG_ERROR("android_writeAudio is not set.");
        return;
    }

    (*env)->CallVoidMethod(env, kwaakAudioObj, android_writeAudio, audioBuffer, offset, length);
}

void setMenuState(int state)
{
    JNIEnv *env;
    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    LOG_TRACE("setMenuState state=%d", state);

    if (!android_setMenuState) {
        LOG_ERROR("android_setMenuState is not set.");
        return;
    }

    (*env)->CallVoidMethod(env, kwaakRendererObj, android_setMenuState, state);
}

int JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv *env;
    jVM = vm;

    LOG_TRACE("JNI_OnLoad called");

    if((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        LOG_ERROR("Failed to get the environment using GetEnv()");
        return -1;
    }

    return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_enableAudio(JNIEnv *env, jclass c, jboolean enable)
{
    audioEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_enableBenchmark(JNIEnv *env, jclass c, jboolean enable)
{
    benchmarkEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_enableLightmaps(JNIEnv *env, jclass c, jboolean enable)
{
    lightmapsEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_showFramerate(JNIEnv *env, jclass c, jboolean enable)
{
    showFramerateEnabled = enable;
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setAudio(JNIEnv *env, jclass c, jobject obj)
{
    kwaakAudioObj = obj;
    jclass kwaakAudioClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    kwaakAudioObj = (jobject)(*env)->NewGlobalRef(env, obj);
    kwaakAudioClass = (*env)->GetObjectClass(env, kwaakAudioObj);

    android_getPos = (*env)->GetMethodID(env,kwaakAudioClass,"getPos","()I");
    android_initAudio = (*env)->GetMethodID(env,kwaakAudioClass,"initAudio","()V");
    android_writeAudio = (*env)->GetMethodID(env,kwaakAudioClass,"writeAudio","(Ljava/nio/ByteBuffer;II)V");
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setRenderer(JNIEnv *env, jclass c, jobject obj)
{
    kwaakRendererObj = obj;
    jclass kwaakRendererClass;

    (*jVM)->GetEnv(jVM, (void**) &env, JNI_VERSION_1_4);
    kwaakRendererObj = (jobject)(*env)->NewGlobalRef(env, obj);
    kwaakRendererClass = (*env)->GetObjectClass(env, kwaakRendererObj);

    android_setMenuState = (*env)->GetMethodID(env,kwaakRendererClass,"setMenuState","(I)V");
}


JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_initGame(JNIEnv *env, jclass c, jint width, jint height)
{
    char buf[1000];
    char libquake3_path[100];
    get_quake3_library_path(libquake3_path);

    char *argv[100] = {0};
    int argc=0;

    /* TODO: integrate settings with quake3, right now there is no synchronization */

    argv[argc++] = strdup(libquake3_path);

    if (!audioEnabled) {
        argv[argc++] = strdup("+set");
        argv[argc++] = strdup("s_initsound");
        argv[argc++] = strdup("0");
    }

    argv[argc++] = strdup("+set");
    argv[argc++] = strdup("r_vertexlight");
    argv[argc++] = strdup(lightmapsEnabled ? "0" : "1");

    argv[argc++] = strdup("+set");
    argv[argc++] = strdup("cg_drawfps");
    argv[argc++] = strdup(showFramerateEnabled ? "1" : "0");

    if(benchmarkEnabled) {
        argv[argc++] = strdup("+demo");
        argv[argc++] = strdup("four");
        argv[argc++] = strdup("+timedemo");
        argv[argc++] = strdup("1");
    }

    sprintf(buf, "%d", width);
    argv[argc++] = strdup("+set");
    argv[argc++] = strdup("r_customwidth");
    argv[argc++] = strdup(buf);

    sprintf(buf, "%d", height);
    argv[argc++] = strdup("+set");
    argv[argc++] = strdup("r_customheight");
    argv[argc++] = strdup(buf);

    argv[argc++] = strdup("+set");
    argv[argc++] = strdup("fs_homepath");
    argv[argc++] = strdup(game_dir);

    LOG_TRACE("Setting callback functions.");

    setAudioCallbacks(&getPos, &writeAudio, &initAudio);
    setInputCallbacks(&setMenuState);

    buf[0] = '\0';
    int i;
    for (i = 0; i < argc; i++) {
        strcat(buf, argv[i]);
        strcat(buf, " ");
    }
    LOG_TRACE("Calling: %s q3main(%d, %s)", argv[0], argc, buf);

    /* In the future we might want to pass arguments using argc/argv e.g. to start a benchmark at startup, to load a mod or whatever */
    q3main(argc, argv);

    LOG_TRACE("Returned from q3main.");
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_nextFrame(JNIEnv *env, jclass c)
{
    /*LOG_TRACE("nextFrame()"); // too noisy */
    nextFrame();
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_queueKeyEvent(JNIEnv *env, jclass c, jint key, jint state)
{
    LOG_TRACE("queueKeyEvent(%d, %d)", key, state);
    queueKeyEvent(key, state);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_queueMotionEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y, jfloat pressure)
{
    LOG_TRACE("queueMotionEvent(%d, %f, %f, %f)", action, x, y, pressure);
    queueMotionEvent(action, x, y, pressure);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_queueTrackballEvent(JNIEnv *env, jclass c, jint action, jfloat x, jfloat y)
{
    LOG_TRACE("queueTrackballEvent(%d, %f, %f)", action, x, y);
    queueTrackballEvent(action, x, y);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_requestAudioData(JNIEnv *env, jclass c)
{
    /*LOG_TRACE("requestAudioData"); // too noisy */
    requestAudioData();
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setGameDirectory(JNIEnv *env, jclass c, jstring jpath)
{
    jboolean iscopy;
    const jbyte *path = (*env)->GetStringUTFChars(env, jpath, &iscopy);
    game_dir = strdup(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    LOG_TRACE("game path=%s\n", game_dir);
}

JNIEXPORT void JNICALL Java_org_kwaak3_KwaakJNI_setLibraryDirectory(JNIEnv *env, jclass c, jstring jpath)
{
    jboolean iscopy;
    const jbyte *path = (*env)->GetStringUTFChars(env, jpath, &iscopy);
    lib_dir = strdup(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);

    LOG_TRACE("library path=%s\n", lib_dir);
}
