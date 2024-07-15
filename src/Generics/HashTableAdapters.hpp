/**
 * @file   HashTableAdapters.hpp
 * @author Karen Aroutiounov
 */

#ifndef GENERICS_HASH_TABLE_ADAPTERS_HPP
#define GENERICS_HASH_TABLE_ADAPTERS_HPP

#include <String/SubString.hpp>

#include <Generics/Hash.hpp>

#include <Stream/MemoryStream.hpp>

/*
 * XXX!
 * StringHashAdapter and SubStringHashAdapter must return equal hash
 * values for equal strings.
 * XXX!
 */


namespace Generics
{
  class StringHashAdapter
  {
  public:
    typedef std::string text_type;

  public:
    StringHashAdapter(const char* text = 0) /*throw (eh::Exception)*/;
    StringHashAdapter(const String::SubString& text) /*throw (eh::Exception)*/;
    StringHashAdapter(const std::string& text) /*throw (eh::Exception)*/;
    StringHashAdapter(size_t hash, const char* text)
      /*throw (eh::Exception)*/;

    StringHashAdapter(const char* buffer, size_t buffer_len)
      /*throw (eh::Exception)*/;

    StringHashAdapter(const StringHashAdapter&) noexcept;

    StringHashAdapter(StringHashAdapter&&) noexcept;

    StringHashAdapter&
    assign(size_t hash, const char* text)
      /*throw (eh::Exception)*/;

    StringHashAdapter&
    assign(const char* text) /*throw (eh::Exception)*/;

    StringHashAdapter&
    operator=(const StringHashAdapter&) noexcept;

    bool
    operator ==(const StringHashAdapter& src) const /*throw (eh::Exception)*/;
    bool
    operator <(const StringHashAdapter& src) const /*throw (eh::Exception)*/;
    bool
    operator >(const StringHashAdapter& src) const /*throw (eh::Exception)*/;

    size_t
    hash() const noexcept;

    /**
     * @return The string on which was calculated hash
     */
    const std::string&
    text() const noexcept;
    operator const std::string&() const noexcept;

  protected:
    void
    hash_i() /*throw (eh::Exception)*/;

  protected:
    std::string text_;
    size_t hash_;
  };

  class SubStringHashAdapter
  {
  public:
    typedef String::SubString text_type;

  public:
    SubStringHashAdapter(const String::SubString& text =
      String::SubString())
      noexcept;

    template <typename Traits, typename Alloc>
    SubStringHashAdapter(const std::basic_string<char, Traits, Alloc>& text)
      noexcept;

    SubStringHashAdapter(size_t hash, const String::SubString& text)
      noexcept;

    bool
    operator ==(const SubStringHashAdapter& src) const
      noexcept;

    bool
    operator <(const SubStringHashAdapter& src) const
      noexcept;

    size_t
    hash() const
      noexcept;

    operator String::SubString() const
      noexcept;

    const String::SubString&
    text() const noexcept;

  protected:
    void
    calc_hash_()
      noexcept;

  protected:
    String::SubString text_;
    size_t hash_;
  };

  template <class T>
  class NumericHashAdapter
  {
  public:
    NumericHashAdapter() /*throw (eh::Exception)*/;
    NumericHashAdapter(const T& value) /*throw (eh::Exception)*/;

    bool
    operator ==(const NumericHashAdapter& src) const /*throw (eh::Exception)*/;
    bool
    operator <(const NumericHashAdapter& src) const /*throw (eh::Exception)*/;
    bool
    operator >(const NumericHashAdapter& src) const /*throw (eh::Exception)*/;

    size_t
    hash() const noexcept;

    const T&
    value() const /*throw (eh::Exception)*/;

  protected:
    T value_;
  };
}

///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace Generics
{
  //
  // StringHashAdapter class
  //
  inline
  StringHashAdapter::StringHashAdapter(const char* text)
    /*throw (eh::Exception)*/
    : text_(text ? text : "")
  {
    hash_i();
  }

  inline
  StringHashAdapter::StringHashAdapter(const StringHashAdapter& init) noexcept
    : text_(init.text_),
      hash_(init.hash_)
  {}

  inline
  StringHashAdapter::StringHashAdapter(StringHashAdapter&& init) noexcept
    : text_(std::move(init.text_)),
      hash_(init.hash_)
  {}

  inline
  StringHashAdapter::StringHashAdapter(const String::SubString& text)
    /*throw (eh::Exception)*/
    : text_(text.str())
  {
    hash_i();
  }

  inline
  StringHashAdapter::StringHashAdapter(const std::string& text)
    /*throw (eh::Exception)*/
    : text_(text)
  {
    hash_i();
  }

  inline
  StringHashAdapter::StringHashAdapter(size_t hash, const char* text)
    /*throw (eh::Exception)*/
    : text_(text), hash_(hash)
  {
  }

  inline
  StringHashAdapter::StringHashAdapter(const char* buffer, size_t buffer_len)
    /*throw (eh::Exception)*/
    : text_(buffer, buffer_len)
  {
    hash_i();
  }

  inline
  StringHashAdapter&
  StringHashAdapter::operator=(const StringHashAdapter& init) noexcept
  {
    text_ = init.text_;
    hash_ = init.hash_;
    return *this;
  }

  inline
  StringHashAdapter&
  StringHashAdapter::assign(size_t hash, const char* text)
    /*throw (eh::Exception)*/
  {
    text_.assign(text);
    hash_ = hash;
    return *this;
  }

  inline
  StringHashAdapter&
  StringHashAdapter::assign(const char* text) /*throw (eh::Exception)*/
  {
    text_.assign(text);
    hash_i();

    return *this;
  }

  inline
  bool
  StringHashAdapter::operator ==(const StringHashAdapter& src) const
    /*throw (eh::Exception)*/
  {
    return text_ == src.text_;
  }

  inline
  bool
  StringHashAdapter::operator <(const StringHashAdapter& src)
    const /*throw (eh::Exception)*/
  {
    return text_ < src.text_;
  }

  inline
  bool
  StringHashAdapter::operator >(const StringHashAdapter& src)
    const /*throw (eh::Exception)*/
  {
    return text_ > src.text_;
  }

  inline
  size_t
  StringHashAdapter::hash() const noexcept
  {
    return hash_;
  }

  inline
  void
  StringHashAdapter::hash_i() /*throw (eh::Exception)*/
  {
    Murmur64Hash hash(hash_);
    hash_add(hash, text_);
  }

  inline
  const std::string&
  StringHashAdapter::text() const noexcept
  {
    return text_;
  }

  inline
  StringHashAdapter::operator const std::string&() const
    noexcept
  {
    return text_;
  }

//
// SubStringHashAdapter class
//
  inline
  SubStringHashAdapter::SubStringHashAdapter(const String::SubString& text)
    noexcept
    : text_(text)
  {
    calc_hash_();
  }

  template <typename Traits, typename Alloc>
  SubStringHashAdapter::SubStringHashAdapter(
    const std::basic_string<char, Traits, Alloc>& text) noexcept
    : text_(text)
  {
    calc_hash_();
  }

  inline
  SubStringHashAdapter::SubStringHashAdapter(size_t hash,
    const String::SubString& text)
    noexcept
    : text_(text), hash_(hash)
  {
  }

  inline
  bool
  SubStringHashAdapter::operator ==(const SubStringHashAdapter& src) const
    noexcept
  {
    return text_ == src.text_;
  }

  inline
  bool
  SubStringHashAdapter::operator <(const SubStringHashAdapter& src) const
    noexcept
  {
    return text_ < src.text_;
  }

  inline
  size_t
  SubStringHashAdapter::hash() const noexcept
  {
    return hash_;
  }

  inline
  SubStringHashAdapter::operator String::SubString() const noexcept
  {
    return text_;
  }

  inline
  const String::SubString&
  SubStringHashAdapter::text() const noexcept
  {
    return text_;
  }

  inline
  void
  SubStringHashAdapter::calc_hash_() noexcept
  {
    Murmur64Hash hash(hash_);
    hash_add(hash, text_);
  }

//
// NumericHashAdapter template
//
  template <class T>
  NumericHashAdapter<T>::NumericHashAdapter() /*throw (eh::Exception)*/
    : value_(0)
  {
  }

  template <class T>
  NumericHashAdapter<T>::NumericHashAdapter(const T& value)
    /*throw (eh::Exception)*/
    : value_(value)
  {
  }

  template <class T>
  bool
  NumericHashAdapter<T>::operator ==(const NumericHashAdapter& src) const
    /*throw (eh::Exception)*/
  {
    return value_ == src.value_;
  }

  template <class T>
  bool
  NumericHashAdapter<T>::operator <(const NumericHashAdapter& src) const
    /*throw (eh::Exception)*/
  {
    return value_ < src.value_;
  }

  template <class T>
  bool
  NumericHashAdapter<T>::operator >(const NumericHashAdapter& src) const
    /*throw (eh::Exception)*/
  {
    return value_ > src.value_;
  }

  template <class T>
  size_t
  NumericHashAdapter<T>::hash() const noexcept
  {
    return static_cast<size_t>(value_);
  }

  template <class T>
  const T&
  NumericHashAdapter<T>::value() const /*throw (eh::Exception)*/
  {
    return value_;
  }

  inline
  std::ostream&
  operator <<(std::ostream& ostr, const StringHashAdapter& str)
    /*throw (eh::Exception)*/
  {
    ostr << str.text();
    return ostr;
  }

  inline
  std::istream&
  operator >>(std::istream& istr, StringHashAdapter& str)
    /*throw (eh::Exception)*/
  {
    std::string str_;
    istr >> str_;
    str.assign(str_.c_str());
    return istr;
  }
}

namespace Stream::MemoryStream
{
  template<>
  struct ToCharsLenHelper<Generics::StringHashAdapter>
  {
    size_t
    operator()(const Generics::StringHashAdapter& sha) noexcept
    {
      return sha.text().size();
    }
  };

  template<>
  struct ToCharsHelper<Generics::StringHashAdapter>
  {
    std::to_chars_result
    operator()(char* first, char* last, const Generics::StringHashAdapter& sha) noexcept
    {
      size_t capacity = last - first;
      if (sha.text().size() > capacity)
      {
        return {last, std::errc::value_too_large};
      }
      memcpy(first, sha.text().data(), sha.text().size());
      return {first + sha.text().size(), std::errc()};
    }
  };

  template<>
  struct ToStringHelper<Generics::StringHashAdapter>
  {
    std::string
    operator()(const Generics::StringHashAdapter& sha) noexcept
    {
      return sha.text();
    }
  };

  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  struct OutputMemoryStreamHelper<Elem, Traits, Allocator, AllocatorInitializer,
    SIZE, Generics::StringHashAdapter>
  {
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator()(OutputMemoryStream<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE>& ostr, const Generics::StringHashAdapter& arg)
    {
      typedef typename Generics::StringHashAdapter ArgT;
      return OutputMemoryStreamHelperImpl(ostr, arg,
        ToCharsLenHelper<ArgT>(), ToCharsHelper<ArgT>(), ToStringHelper<ArgT>());
    }
  };
}

#endif
