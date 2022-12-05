#ifndef JAVA_HTTPJAVAFUNCTIONS_HPP
#define JAVA_HTTPJAVAFUNCTIONS_HPP

#include <jni.h>

extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_phorm_oix_util_normalization_UnixCommonsNormalizer_initialize(
    JNIEnv* env, jclass cls);

  JNIEXPORT jstring JNICALL
  Java_com_phorm_oix_util_normalization_UnixCommonsNormalizer_normalizeURL(
    JNIEnv* env, jobject object, jstring url);

  JNIEXPORT jstring JNICALL
  Java_com_phorm_oix_util_normalization_UnixCommonsNormalizer_normalizeKeyword(
    JNIEnv* env, jobject object, jstring keyword);

  JNIEXPORT jstring JNICALL
  Java_com_phorm_oix_util_normalization_UnixCommonsNormalizer_normalizeChineseKeyword(
    JNIEnv* env, jobject object, jstring keyword);
}

#endif
