/**
 * @file   ParseErrorReporter.hpp
 * @author Karen Aroutiounov <karen@ipmce.ru>
 */

#include <iostream>
#include <string>

#include <eh/Exception.hpp>

#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include "StringManip.hpp"
#include "ParseErrorReporter.hpp"

namespace XMLUtility
{
  void
  ParseErrorReporter::warning(const SAXParseException& toCatch)
    /*throw (eh::Exception)*/
  {
    if (show_warnings_)
    {
      std::string msg;
      StringManip::xmlch_to_mbc(toCatch.getMessage(), msg);

      std::string file;
      StringManip::xmlch_to_mbc(toCatch.getSystemId(), file);

      ostream_ << "XMLUtility::ParseErrorReporter: "
        "Warning at file \"" << file.c_str() <<
        "\", line " << toCatch.getLineNumber() << ", column " <<
        toCatch.getColumnNumber() << std::endl << "   Message: " <<
        msg.c_str() << std::endl;
    }
  }

  void
  ParseErrorReporter::error(const SAXParseException& toCatch)
    /*throw (eh::Exception)*/
  {
    errors_ = true;

    std::string msg;
    StringManip::xmlch_to_mbc(toCatch.getMessage(), msg);

    std::string file;
    StringManip::xmlch_to_mbc(toCatch.getSystemId(), file);

    ostream_ << "XMLUtility::ParseErrorReporter: "
      "Error at file \"" << file.c_str() <<
      "\", line " << toCatch.getLineNumber() << ", column " <<
      toCatch.getColumnNumber() << std::endl << "   Message: " <<
      msg.c_str() << std::endl;
  }

  void
  ParseErrorReporter::fatalError(const SAXParseException& toCatch)
    /*throw (eh::Exception)*/
  {
    errors_ = true;

    std::string msg;
    StringManip::xmlch_to_mbc(toCatch.getMessage(), msg);

    std::string file;
    StringManip::xmlch_to_mbc(toCatch.getSystemId(), file);

    ostream_ << "XMLUtility::ParseErrorReporter: "
      "Fatal Error at file \"" << file.c_str() <<
      "\", line " << toCatch.getLineNumber() << ", column " <<
      toCatch.getColumnNumber() << std::endl << "   Message: " <<
      msg.c_str() << std::endl;
  }

  void
  ParseErrorReporter::resetErrors() /*throw (eh::Exception)*/
  {
    errors_ = false;
  }
}
