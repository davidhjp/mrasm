/*
 * Sung Chul Lee
 */
#ifndef TVM_SIMULATOR
#define TVM_SIMULATOR

#include <jni.h>
#include "machine.h"

#ifdef  __cplusplus
extern "C" {
#endif

void simulate(INSTRUCTION* p, JNIEnv* env, jobject obj);

#ifdef  __cplusplus
}
#endif

#endif
