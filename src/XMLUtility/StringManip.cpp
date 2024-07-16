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
