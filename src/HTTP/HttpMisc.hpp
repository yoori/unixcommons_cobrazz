#pragma once

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
    SubHeader() noexcept;
    SubHeader(const char* nm, const char* vl) noexcept;
    SubHeader(const String::SubString& nm, const String::SubString& vl)
      noexcept;
    SubHeader(const Header& header) noexcept; // implicit

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
    SubParam() noexcept;
    SubParam(const char* nm, const char* vl) noexcept;
    SubParam(const String::SubString& nm, const String::SubString& vl)
      noexcept;
    SubParam(const Param& param) noexcept; // implicit

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
  SubHeader::SubHeader() noexcept
  {
  }

  inline
  SubHeader::SubHeader(const char* nm, const char* vl) noexcept
    : name(nm), value(vl)
  {
  }

  inline
  SubHeader::SubHeader(const String::SubString& nm,
    const String::SubString& vl) noexcept
    : name(nm), value(vl)
  {
  }

  inline
  SubHeader::SubHeader(const Header& header) noexcept
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
  SubParam::SubParam() noexcept
  {
  }

  inline
  SubParam::SubParam(const char* nm, const char* vl) noexcept
    : name(nm), value(vl)
  {
  }

  inline
  SubParam::SubParam(const String::SubString& nm,
    const String::SubString& vl) noexcept
    : name(nm), value(vl)
  {
  }

  inline
  SubParam::SubParam(const Param& param) noexcept
    : name(param.name), value(param.value)
  {
  }
}
