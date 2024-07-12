/**
 * @file   StringManip.cpp
 * @author Karen Aroutiounov <karen@ipmce.ru>
 */

#include <charconv>
#include <cstring>
#include <string>
#include <system_error>

#include <xercesc/util/XMLString.hpp>

#include <eh/Exception.hpp>

#include <XMLUtility/StringManip.hpp>


namespace XMLUtility
{
  namespace StringManip
  {
    void
    xmlch_to_mbc(const XMLCh* src, std::string& dest)
      /*throw (eh::Exception, InvalidFormatException)*/
    {
      char* str = 0;

      try
      {
        if (src)
        {
          str = XMLString::transcode(src);
        }
        else
        {
          XMLCh tmp[2];
          XMLString::transcode("", tmp, 1);

          str = XMLString::transcode(tmp);
        }

        if (str)
        {
          try
          {
            dest = str;
          }
          catch (...)
          {
            XMLString::release(&str);
            throw;
          }

          XMLString::release(&str);
        }
        else
        {
          throw InvalidFormatException(
            "XMLUtility::StringManip::xmlch_to_mbc(): "
            "XMLString::transcode failed");
        }
      }
      catch (const XMLException& ex)
      {
        throw InvalidFormatException(
          "XMLUtility::StringManip::xmlch_to_mbc(): "
          "Can't transcode text.");
      }
    }

    void
    mbc_to_xmlch(const char* src, XMLChAdapter& dest)
      /*throw (eh::Exception, InvalidFormatException)*/
    {
      dest = src;
    }

    //
    // XMLChAdapter class
    //

    XMLChAdapter::XMLChAdapter(const char* text)
      /*throw (eh::Exception, InvalidFormatException)*/
    {
      try
      {
        string_ = XMLString::transcode(text ? text : "");
      }
      catch (const XMLException& ex)
      {
        throw InvalidFormatException(
          "XMLUtility::StringManip::XMLChAdapter::XMLChAdapter(): "
          "Can't transcode text.");
      }
    }

    XMLChAdapter&
    XMLChAdapter::operator =(const char* text)
      /*throw (eh::Exception, InvalidFormatException)*/
    {
      if (string_)
      {
        try
        {
          XMLString::release(&string_);
        }
        catch(...)
        {
        }

        string_ = 0;
      }

      try
      {
        string_ = XMLString::transcode(text ? text : "");
      }
      catch (const XMLException& ex)
      {
        throw InvalidFormatException(
          "XMLUtility::StringManip::XMLChAdapter::operator =(): "
          "Can't transcode text.");
      }

      return *this;
    }
  }
}


namespace Stream::MemoryStream
{
  template<>
  struct ToCharsLenHelper<XMLUtility::StringManip::XMLMbcAdapter>
  {
    size_t
    operator()(const XMLUtility::StringManip::XMLMbcAdapter& xml_adapter) noexcept
    {
      return strlen(xml_adapter.operator const char*());
    }
  };

  template<>
  struct ToCharsHelper<XMLUtility::StringManip::XMLMbcAdapter>
  {
    std::to_chars_result
    operator()(char* first, char* last, const XMLUtility::StringManip::XMLMbcAdapter& xml_adapter)
      noexcept
    {
      std::string str{xml_adapter.operator const char*()};
      if (first + str.size() > last)
      {
        return {last, std::errc::value_too_large};
      }
      memcpy(first, str.data(), str.size());
      return {first + str.size(), std::errc()};
    }
  };

  template<>
  struct ToStringHelper<XMLUtility::StringManip::XMLMbcAdapter>
  {
    std::string
    operator()(const XMLUtility::StringManip::XMLMbcAdapter& xml_adapter)
      noexcept
    {
      return std::string(xml_adapter.operator const char*());
    }
  };

  template<typename Elem, typename Traits, typename Allocator, typename AllocatorInitializer,
    const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  OutputMemoryStreamHelper<Elem, Traits, Allocator, AllocatorInitializer, SIZE,
    XMLUtility::StringManip::XMLMbcAdapter>::
  operator()(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const XMLUtility::StringManip::XMLMbcAdapter& arg)
  {
    typedef typename XMLUtility::StringManip::XMLMbcAdapter ArgT;
    return OutputMemoryStreamHelperImpl(ostr, arg,
      ToCharsLenHelper<ArgT>(), ToCharsHelper<ArgT>(), ToStringHelper<ArgT>());
  }
}
