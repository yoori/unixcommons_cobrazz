#ifndef LOGGER_SYSLOG_JAVA_ADAPTER_HPP
#define LOGGER_SYSLOG_JAVA_ADAPTER_HPP

#include <jni.h>


extern "C"
{
  JNIEXPORT jstring JNICALL
  Java_com_phorm_oix_logging_UnixcommonsSyslog_init(
    JNIEnv* env, jobject cls, jstring identity);

  JNIEXPORT jstring JNICALL
  Java_com_phorm_oix_logging_UnixcommonsSyslog_publish(
    JNIEnv* env, jobject obj, jint priority, jstring text);
}

#endif
