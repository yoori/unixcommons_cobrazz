/**
 * @file   XMLUtility/StringManip.hpp
 * @author Karen Aroutiounov <karen@ipmce.ru>
 *
 * File contains group of XML string conversion function
 * interface.
 */

#ifndef XML_UTILITY_STRING_MANIP_HPP
#define XML_UTILITY_STRING_MANIP_HPP

#include <string>
#include <iostream>

#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLString.hpp>

#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <String/StringManip.hpp>

#include <Stream/MemoryStream.hpp>


XERCES_CPP_NAMESPACE_USE

namespace XMLUtility
{
  struct XMLChTraits
  {
    typedef XMLCh char_type;
    typedef int int_type;
    typedef std::streampos pos_type;
    typedef std::streamoff off_type;
    typedef mbstate_t state_type;

    static
    void
    assign(char_type& c1, const char_type& c2) noexcept;

    static
    bool
    eq(const char_type& c1, const char_type& c2) noexcept;

    static
    bool
    lt(const char_type& c1, const char_type& c2) noexcept;

    static
    int
    compare(const char_type* s1, const char_type* s2, size_t n) noexcept;

    static
    size_t
    length(const char_type* s) noexcept;

    static
    const char_type*
    find(const char_type* s, size_t n, const char_type& a) noexcept;

    static
    char_type*
    move(char_type* s1, const char_type* s2, size_t n) noexcept;

    static
    char_type*
    copy(char_type* s1, const char_type* s2, size_t n) noexcept;

    static
    char_type*
    assign(char_type* s, size_t n, char_type a) noexcept;

    static
    char_type
    to_char_type(const int_type& c) noexcept;

    // To keep both the byte 0xff and the eof symbol 0xffffffff
    // from ending up as 0xffffffff.
    static
    int_type
    to_int_type(const char_type& c) noexcept;

    static
    bool
    eq_int_type(const int_type&_c1, const int_type& c2) noexcept;

    static
    int_type
    eof() noexcept;

    static
    int_type
    not_eof(const int_type& c) noexcept;
  };

  typedef std::basic_string<XMLCh, XMLChTraits> XMLChString;

  namespace StringManip
  {
    typedef String::StringManip::InvalidFormatException
      InvalidFormatException;

    /**
     * Adapts UTF-8 strings to Xerces strings.
     */
    class XMLChAdapter : private Generics::Uncopyable
    {
    public:
      /**
       * Parametric constructor. Initializes the <code>XMLChAdapter</code>
       * instance from a UTF-8 C string.
       *
       * @param text The source C string.
       */
      XMLChAdapter(const char* text = 0)
        /*throw (eh::Exception, InvalidFormatException)*/;

      /**
       * Destructor.
       */
      ~XMLChAdapter() noexcept;

      /**
       * Operator ().
       * @return the string contained by the adapter as Xerces string.
       */
      operator const XMLCh*() const noexcept;

      /**
       * Assignment operator.
       * Initializes the adapter instance from a UTF-8 C string.
       */
      XMLChAdapter&
      operator =(const char* text)
        /*throw (eh::Exception, InvalidFormatException)*/;

    private:
      XMLCh* string_;
    };

    /**
     * Adapts Xerces strings to UTF-8 C strings.
     */
    class XMLMbcAdapter : private Generics::Uncopyable
    {
    public:
      /**
       * Parametric constructor.
       * Initializes the <code>XMLMbcAdapter</code> instance
       * from a Xerces string.
       *
       * @param text The source Xerces string.
       */
      XMLMbcAdapter(const XMLCh* text = 0)
        /*throw (eh::Exception, InvalidFormatException)*/;

      /**
       * Operator ().
       * @return the string contained by the adapter as C string.
       */
      operator const char*() const noexcept;

      /**
       * Assignment operator.
       * Initializes the adapter instance from a Xerces string.
       */
      XMLMbcAdapter&
      operator =(const XMLCh* text)
        /*throw (eh::Exception, InvalidFormatException)*/;

    private:
      std::string string_;
    };

    /**
     * Converts a Xerces string into UTF-8 C string.
     *
     * @param src The source Xerces string.
     * @param dest Destination <code>std::string</code> instance.
     */
    void
    xmlch_to_mbc(const XMLCh* src, std::string& dest)
      /*throw (eh::Exception, InvalidFormatException)*/;

    /**
     * Converts a UTF-8 C string into Xerces string (as XMLChAdapter).
     *
     * @param src The source C string.
     * @param dest Destination <code>XMLChAdapter</code> instance.
     */
    void
    mbc_to_xmlch(const char* src, XMLChAdapter& dest)
      /*throw (eh::Exception, InvalidFormatException)*/;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace XMLUtility
{
  //
  // XMLChTraits
  //
  inline
  void
  XMLChTraits::assign(char_type& c1, const char_type& c2) noexcept
  {
    c1 = c2;
  }

  inline
  bool
  XMLChTraits::eq(const char_type& c1, const char_type& c2) noexcept
  {
    return c1 == c2;
  }

  inline
  bool
  XMLChTraits::lt(const char_type& c1, const char_type& c2) noexcept
  {
    return c1 < c2;
  }

  inline
  int
  XMLChTraits::compare(const char_type* s1, const char_type* s2, size_t n)
    noexcept
  {
    return XMLString::compareNString(s1, s2, n);
  }

  inline
  size_t
  XMLChTraits::length(const char_type* s) noexcept
  {
    return XMLString::stringLen(s);
  }

  inline
  const XMLChTraits::char_type*
  XMLChTraits::find(const char_type* s, size_t n, const char_type& a) noexcept
  {
    for (size_t i = 0; i < n; i++)
    {
      if (s[i] == a)
      {
        return s + i;
      }
    }

    return static_cast<const char_type*>(0);
  }

  inline
  XMLChTraits::char_type*
  XMLChTraits::move(char_type* s1, const char_type* s2, size_t n) noexcept
  {
    XMLString::moveChars(s1, s2, n);
    return s1;
  }

  inline
  XMLChTraits::char_type*
  XMLChTraits::copy(char_type* s1, const char_type* s2, size_t n) noexcept
  {
    XMLString::moveChars(s1, s2, n);
    return s1;
  }

  inline
  XMLChTraits::char_type*
  XMLChTraits::assign(char_type* s, size_t n, char_type a) noexcept
  {
    for (size_t i = 0; i < n; i++)
    {
      s[i] = a;
    }
    return s;
  }

  inline
  XMLChTraits::char_type
  XMLChTraits::to_char_type(const int_type& c) noexcept
  {
    return static_cast<char_type>(c);
  }

  inline
  XMLChTraits::int_type
  XMLChTraits::to_int_type(const char_type& c) noexcept
  {
    return static_cast<int_type>(static_cast<unsigned int>(c));
  }

  inline
  bool
  XMLChTraits::eq_int_type(const int_type& c1, const int_type& c2) noexcept
  {
    return c1 == c2;
  }

  inline
  XMLChTraits::int_type
  XMLChTraits::eof() noexcept
  {
    return static_cast<int_type>(EOF);
  }

  inline
  XMLChTraits::int_type
  XMLChTraits::not_eof(const int_type& c) noexcept
  {
    return c == eof() ? 0 : c;
  }


  namespace StringManip
  {
    //
    // XMLChAdapter class
    //
    inline
    XMLChAdapter::~XMLChAdapter() noexcept
    {
      try
      {
        XMLString::release(&string_);
      }
      catch (...)
      {
      }
    }

    inline
    XMLChAdapter::operator const XMLCh*() const noexcept
    {
      return string_;
    }

    //
    // XMLMbcAdapter class
    //
    inline
    XMLMbcAdapter::XMLMbcAdapter(const XMLCh* text)
      /*throw (eh::Exception, InvalidFormatException)*/
    {
      xmlch_to_mbc(text, string_);
    }

    inline
    XMLMbcAdapter::operator const char*() const noexcept
    {
      return string_.c_str();
    }

    inline
    XMLMbcAdapter&
    XMLMbcAdapter::operator =(const XMLCh* text)
      /*throw (eh::Exception, InvalidFormatException)*/
    {
      xmlch_to_mbc(text, string_);
      return *this;
    }
  }
}

inline
std::ostream&
operator <<(std::ostream& ostr,
  const XMLUtility::StringManip::XMLMbcAdapter& val) /*throw (eh::Exception)*/
{
  ostr << val.operator const char*();
  return ostr;
}

namespace Stream::MemoryStream
{
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  struct OutputMemoryStreamHelper<Elem, Traits, Allocator, AllocatorInitializer,
    SIZE, XMLUtility::StringManip::XMLMbcAdapter>
  {
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator()(OutputMemoryStream<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE>& ostr, const XMLUtility::StringManip::XMLMbcAdapter& arg);
  };
}

#endif
