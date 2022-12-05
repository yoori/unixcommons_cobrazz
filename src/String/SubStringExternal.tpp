// @file String/SubStringExternal.tpp

namespace String
{
  //
  // External functions
  //

  template <typename CharType, typename Traits, typename Checker>
  std::basic_ostream<typename BasicSubString<CharType, Traits, Checker>::
    BasicStringValueType>&
  operator <<(std::basic_ostream<
    typename BasicSubString<CharType, Traits, Checker>::
      BasicStringValueType>& ostr,
    const BasicSubString<CharType, Traits, Checker>& substr)
    /*throw (eh::Exception)*/
  {
    if (!substr.empty())
    {
      ostr.write(substr.data(), substr.length());
    }
    return ostr;
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(const BasicSubString<CharType, Traits, Checker>& substr,
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str)
    /*throw (typename BasicSubString<CharType, Traits, Checker>::LogicError)*/
  {
    return substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    /*throw (typename BasicSubString<CharType, Traits, Checker>::LogicError)*/
  {
    return substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator ==(const BasicSubString<CharType, Traits, Checker>& left_substr,
    const BasicSubString<CharType, Traits, Checker>& right_substr)
    throw ()
  {
    return left_substr.equal(right_substr);
  }

  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator ==(const BasicSubString<CharType, Traits, Checker>& substr,
    const std::basic_string<
      typename BasicSubString<CharType, Traits, Checker>::
        BasicStringValueType, BasicStringTraits, Allocator>& str)
    /*throw (eh::Exception)*/
  {
    return substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator ==(const std::basic_string<
    typename BasicSubString<CharType, Traits, Checker>::
      BasicStringValueType, BasicStringTraits, Allocator>& str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    /*throw (eh::Exception)*/
  {
    return substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(const BasicSubString<CharType, Traits, Checker>& substr,
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str)
    /*throw (typename BasicSubString<CharType, Traits, Checker>::LogicError)*/
  {
    return !substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    /*throw (typename BasicSubString<CharType, Traits, Checker>::LogicError)*/
  {
    return !substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator !=(const BasicSubString<CharType, Traits, Checker>& left_substr,
    const BasicSubString<CharType, Traits, Checker>& right_substr)
    throw ()
  {
    return !left_substr.equal(right_substr);
  }

  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator !=(const BasicSubString<CharType, Traits, Checker>& substr,
    const std::basic_string<
      typename BasicSubString<CharType, Traits, Checker>::
        BasicStringValueType, BasicStringTraits, Allocator>& str)
    /*throw (eh::Exception)*/
  {
    return !substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator !=(const std::basic_string<
    typename BasicSubString<CharType, Traits, Checker>::
      BasicStringValueType, BasicStringTraits, Allocator>& str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    /*throw (eh::Exception)*/
  {
    return !substr.equal(str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(const BasicSubString<CharType, Traits, Checker>& substr,
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str)
    /*throw (typename BasicSubString<CharType, Traits, Checker>::LogicError)*/
  {
    return substr.compare(str) < 0;
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(
    typename BasicSubString<CharType, Traits, Checker>::ConstPointer str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    /*throw (typename BasicSubString<CharType, Traits, Checker>::LogicError)*/
  {
    return !operator <(substr, str);
  }

  template <typename CharType, typename Traits, typename Checker>
  bool
  operator <(const BasicSubString<CharType, Traits, Checker>& left_substr,
    const BasicSubString<CharType, Traits, Checker>& right_substr) throw ()
  {
    return left_substr.compare(right_substr) < 0;
  }

  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator <(const BasicSubString<CharType, Traits, Checker>& substr,
    const std::basic_string<
      typename BasicSubString<CharType, Traits, Checker>::
        BasicStringValueType, BasicStringTraits, Allocator>& str)
    throw ()
  {
    return substr.compare(str) < 0;
  }

  template <typename CharType, typename Traits, typename Checker,
    typename BasicStringTraits, typename Allocator>
  bool
  operator <(const std::basic_string<
    typename BasicSubString<CharType, Traits, Checker>::
      BasicStringValueType, BasicStringTraits, Allocator>& str,
    const BasicSubString<CharType, Traits, Checker>& substr)
    throw ()
  {
    return substr.compare(str) > 0;
  }

  template <typename Hash,
    typename CharType, typename Traits, typename Checker>
  void
  hash_add(Hash& hash,
    const BasicSubString<CharType, Traits, Checker>& value) throw ()
  {
    hash.add(value.data(), value.size());
  }
}
