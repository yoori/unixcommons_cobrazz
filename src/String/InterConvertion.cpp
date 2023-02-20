/**
 * @file   InterConvertion.hpp
 * @author Andrey Gusev
 *
 * Contains international conversion interface
 */
#include <errno.h>

#include <String/InterConvertion.hpp>

#include <Generics/Function.hpp>

#include <Stream/MemoryStream.hpp>


namespace String
{
  namespace International
  {
    const iconv_t Convertion::INVALID_ =
      reinterpret_cast<iconv_t>(-1);

    void
    Convertion::set_encodings(const char* to_code, const char* from_code)
      /*throw (NotSupported)*/
    {
      if (fd_ != INVALID_)
      {
        iconv_close(fd_);
        mult_ = 1;
      }
      fd_ = iconv_open(to_code, from_code);
      if (fd_ == INVALID_)
      {
        Stream::Error ostr;
        ostr << FNS << "Encoding to '" << to_code << "' from '" <<
          from_code << "' not supported";
        throw NotSupported(ostr);
      }
    }

    void
    Convertion::encode(const char* input_chars, size_t count,
      std::string& out) /*throw (eh::Exception, BadSequence)*/
    {
      if (count)
      {
        size_t left_in, left_out;
        char* in_buf = const_cast<char*>(input_chars);
        char* out_buf;
        left_in = count;
        left_out = count * mult_;
        out.resize(left_out);
        out_buf = &out[0];
        while (iconv(fd_, &in_buf, &left_in, &out_buf, &left_out) ==
          static_cast<size_t>(-1))
        {
          if (errno == E2BIG)
          {
            out.resize(out.size() + count);
            left_out += count;
            out_buf = &out[out.size() - left_out];
            mult_++;
            continue;
          }
          Stream::Error ostr;
          ostr << FNS << "Bad characters sequence.";
          throw BadSequence(ostr);
        }
      }
    }
  }//namespace international
}//namespace String
