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

  // old ui compatibility symbols
  JNIEXPORT jstring JNICALL
  Java_com_foros_util_unixcommons_UnixCommonsTools_initialize(
    JNIEnv* env, jclass cls);

  JNIEXPORT jstring JNICALL
  Java_com_foros_util_unixcommons_UnixCommonsTools_normalizeURL(
    JNIEnv* env, jobject object, jstring url);

  JNIEXPORT jstring JNICALL
  Java_com_foros_util_unixcommons_UnixCommonsTools_normalizeKeyword(
    JNIEnv* env, jobject object, jstring keyword);

  JNIEXPORT jboolean JNICALL
  Java_com_foros_util_unixcommons_UnixCommonsTools_validateURL(
    JNIEnv* env, jobject object, jstring url);
}

#endif
