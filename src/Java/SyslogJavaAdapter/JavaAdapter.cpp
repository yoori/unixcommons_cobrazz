#include <stdio.h>

#include <Logger/SyslogAdapter.hpp>
#include <String/StringManip.hpp>
#include <JavaCommons/JavaCommons.hpp>
#include <SyslogJavaAdapter/JavaAdapter.hpp>


static char saved_identity[1024];

JNIEXPORT jstring JNICALL
Java_com_phorm_oix_logging_UnixcommonsSyslog_init(
  JNIEnv* env, jobject /*cls*/, jstring identity)
{
  {
    JavaCommons::StrPtr ident(env, identity);
    String::StringManip::strlcpy(saved_identity, ident.c_str(),
      sizeof(saved_identity));
  }
  openlog(saved_identity, LOG_PID | LOG_CONS, LOG_USER);
  return env->NewStringUTF("INIT_SUCCESS");
}

JNIEXPORT jstring JNICALL
Java_com_phorm_oix_logging_UnixcommonsSyslog_publish(
  JNIEnv* env, jobject /*obj*/, jint priority, jstring text)
{
  {
    JavaCommons::StrPtr message(env, text);
    syslog(priority, "%s", message.c_str());
  }
  return env->NewStringUTF("LOG_SUCCESS");
}
