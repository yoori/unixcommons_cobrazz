/**
 * @file   HttpMisc.hpp
 * @author Karen Aroutiounov
 *
 * Contains HTTP protocol basis definitions
 */

#ifndef HTTP_HTTPMISC_HPP
#define HTTP_HTTPMISC_HPP

#include <list>

#include <String/SubString.hpp>


namespace HTTP
{
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

  /**
   * HTTP Header
   */
  struct Header
  {
    Header() /*throw (eh::Exception)*/;
    Header(const char* nm, const char* vl) /*throw (eh::Exception)*/;
    Header(const std::string& nm, const std::string& vl)
      /*throw (eh::Exception)*/;

    std::string name;
    std::string value;
  };

  typedef std::list<Header> HeaderList;

  /**
   * HTTP Header based on SubString
   */
  struct SubHeader
  {
    SubHeader() throw ();
    SubHeader(const char* nm, const char* vl) throw ();
    SubHeader(const String::SubString& nm, const String::SubString& vl)
      throw ();
    SubHeader(const Header& header) throw (); // implicit

    String::SubString name;
    String::SubString value;
  };

  typedef std::list<SubHeader> SubHeaderList;

  /**
   * HTTP Parameter
   */
  struct Param
  {
    Param() /*throw (eh::Exception)*/;
    Param(const char* nm, const char* vl) /*throw (eh::Exception)*/;
    Param(const std::string& nm, const std::string& vl)
      /*throw (eh::Exception)*/;

    std::string name;
    std::string value;
  };

  typedef std::list<Param> ParamList;

  /**
   * HTTP Parameter based on SubString
   */
  struct SubParam
  {
    SubParam() throw ();
    SubParam(const char* nm, const char* vl) throw ();
    SubParam(const String::SubString& nm, const String::SubString& vl)
      throw ();
    SubParam(const Param& param) throw (); // implicit

    String::SubString name;
    String::SubString value;
  };

  typedef std::list<SubParam> SubParamList;
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace HTTP
{
  //
  // Header class
  //

  inline
  Header::Header() /*throw (eh::Exception)*/
  {
  }

  inline
  Header::Header(const char* nm, const char* vl) /*throw (eh::Exception)*/
    : name(nm), value(vl)
  {
  }

  inline
  Header::Header(const std::string& nm, const std::string& vl)
    /*throw (eh::Exception)*/
    : name(nm), value(vl)
  {
  }


  //
  // SubHeader class
  //

  inline
  SubHeader::SubHeader() throw ()
  {
  }

  inline
  SubHeader::SubHeader(const char* nm, const char* vl) throw ()
    : name(nm), value(vl)
  {
  }

  inline
  SubHeader::SubHeader(const String::SubString& nm,
    const String::SubString& vl) throw ()
    : name(nm), value(vl)
  {
  }

  inline
  SubHeader::SubHeader(const Header& header) throw ()
    : name(header.name), value(header.value)
  {
  }


  //
  // Param class
  //

  inline
  Param::Param() /*throw (eh::Exception)*/
  {
  }

  inline
  Param::Param(const char* nm, const char* vl) /*throw (eh::Exception)*/
    : name(nm), value(vl)
  {
  }

  inline
  Param::Param(const std::string& nm, const std::string& vl)
    /*throw (eh::Exception)*/
    : name(nm), value(vl)
  {
  }


  //
  // SubParam class
  //

  inline
  SubParam::SubParam() throw ()
  {
  }

  inline
  SubParam::SubParam(const char* nm, const char* vl) throw ()
    : name(nm), value(vl)
  {
  }

  inline
  SubParam::SubParam(const String::SubString& nm,
    const String::SubString& vl) throw ()
    : name(nm), value(vl)
  {
  }

  inline
  SubParam::SubParam(const Param& param) throw ()
    : name(param.name), value(param.value)
  {
  }
}

#endif
