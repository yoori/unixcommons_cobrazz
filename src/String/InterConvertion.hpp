/**
 * @file   String/InterConvertion.hpp
 * @author Andrey Gusev
 *
 * Contains international conversion interface
 */
#ifndef STRING_INTERCONVERTION_HPP
#define STRING_INTERCONVERTION_HPP

#include <iconv.h>

#include <eh/Exception.hpp>

#include <Generics/Uncopyable.hpp>


namespace String
{
  /**
   * iconv utility routines
   */
  namespace International
  {
    /**
     * Allows you to change the encoding of text through using
     * the system utility iconv
     */
    class Convertion : private Generics::Uncopyable
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(NotSupported, Exception);
      DECLARE_EXCEPTION(BadSequence, Exception);

    public:
      /**
       * Simple constructor don't acquire system resources
       * and do not load iconv
       */
      Convertion() throw ();

      /**
       * Constructor load iconv and set encodings for conversion
       * @param to_code Character encoding conversion to
       * @param from_code Character encoding conversion from
       */
      Convertion(const char* to_code, const char* from_code)
        /*throw (NotSupported)*/;

      /**
       * Destructor free open iconv handle if necessary
       */
      ~Convertion() throw ();

      /**
       * Set encodings for conversion
       * @param to_code Character encoding conversion to
       * @param from_code Character encoding conversion from
       */
      void
      set_encodings(const char* to_code, const char* from_code)
        /*throw (NotSupported)*/;

      /**
       * Perform conversion.
       * @param input_chars pointer to data should to be converted
       * @param count size in bytes of input data
       * @param out Result of conversion stored here
       */
      void
      encode(const char* input_chars, size_t count, std::string& out)
        /*throw (eh::Exception, BadSequence)*/;

    private:
      static const iconv_t INVALID_;

      iconv_t fd_;
      size_t mult_;
    };
  }
}

namespace String
{
  namespace International
  {
    inline
    Convertion::Convertion() throw ()
      : fd_(INVALID_), mult_(1)
    {
    }

    inline
    Convertion::Convertion(const char* to_code, const char* from_code)
      /*throw(NotSupported)*/
      : fd_(INVALID_), mult_(1)
    {
      set_encodings(to_code, from_code);
    }

    inline
    Convertion::~Convertion() throw ()
    {
      if (fd_ != INVALID_)
      {
        iconv_close(fd_);
      }
    }
  }
}

#endif
