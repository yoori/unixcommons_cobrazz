#ifndef JAVA_JAVACOMMONS_JAVACOMMONS_HPP
#define JAVA_JAVACOMMONS_JAVACOMMONS_HPP

#include <jni.h>

#include <Generics/Uncopyable.hpp>
#include <Generics/Function.hpp>

#include <Stream/MemoryStream.hpp>


namespace JavaCommons
{
  class StrPtr : private Generics::Uncopyable
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    StrPtr(JNIEnv* env, jstring jstr) /*throw (Exception)*/;
    ~StrPtr() throw ();

    const char*
    c_str() const throw ();

  private:
    JNIEnv* env_;
    jstring jstr_;
    const char* str_;
  };
}

namespace JavaCommons
{
  inline
  StrPtr::StrPtr(JNIEnv* env, jstring jstr) /*throw (Exception)*/
    : env_(env), jstr_(jstr), str_(env->GetStringUTFChars(jstr, 0))
  {
    if (!str_)
    {
      Stream::Error ostr;
      ostr << FNS << "failed to make C-string from jstring";
      throw Exception(ostr);
    }
  }

  inline
  StrPtr::~StrPtr() throw ()
  {
    env_->ReleaseStringUTFChars(jstr_, str_);
  }

  inline
  const char*
  StrPtr::c_str() const throw ()
  {
    return str_;
  }
}

#endif
