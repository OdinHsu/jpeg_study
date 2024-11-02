#ifndef DEBUG_TRACE_H
#define DEBUG_TRACE_H

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _LOG_ANDROID_

#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , "jxEncoder", __VA_ARGS__)
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "jxEncoder",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "jxEncoder",__VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "jxEncoder",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "jxEncoder",__VA_ARGS__)

#endif

void DBGTRACE(int mask,const char  *fmt, ...);
void MSG(const char  *fmt, ...);

extern unsigned int debug_mask;

#define DBG_USB     0x00000001
#define DBG_ENCODER 0x00000002
#define DBG_THREAD  0x00000004
#define DBG_ENGINE  0x00000008
#define DBG_CAPTURE 0x00000010
#define DBG_PROFILE 0x00000020
#define DBG_JNI     0x00000040

#define DBG_ALL     0xFFFFFFFF


#ifdef __cplusplus
}
#endif

#endif
