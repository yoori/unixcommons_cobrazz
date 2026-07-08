#include <String/StringManip.hpp>
#include <String/RegEx.hpp>


namespace String
{
  namespace
  {
    using MatchDataPtr = std::unique_ptr<pcre2_match_data_8, decltype(&pcre2_match_data_free_8)>;

    MatchDataPtr
    create_match_data_(const pcre2_code_8* re)
    {
      return MatchDataPtr(
        pcre2_match_data_create_from_pattern_8(re, nullptr),
        &pcre2_match_data_free_8);
    }
  }

  RegEx&
  RegEx::operator =(const RegEx& side)
    /*throw (Exception, eh::Exception)*/
  {
    if (this != &side)
    {
      clear_();
      if (side.re_)
      {
        allocator_ = side.allocator_;
        expr_ = side.expr_;
        expr_len_ = side.expr_len_;
        expr_size_ = side.expr_size_;
        re_ = side.re_;
        substrcount_ = side.substrcount_;
      }
    }

    return *this;
  }

  void
  RegEx::set_expression(const String::SubString& regex, int options,
    Generics::Allocator::Base* allocator)
    /*throw (Exception, eh::Exception)*/
  {
    if (!regex.data())
    {
      Stream::Error ostr;
      ostr << FNS << "Couldn't compile expression. Null pointer passed.";
      throw Exception(ostr);
    }

    Generics::Allocator::Base_var alloc(
      ReferenceCounting::add_ref(allocator ? allocator :
        Generics::Allocator::Base::get_default_allocator()));

    size_t expr_size = regex.size() + 1;
    char* expr = static_cast<char*>(alloc->allocate(expr_size));
    std::copy(regex.data(), regex.data() + regex.size(), expr);
    expr[regex.size()] = '\0';

    int error_code = 0;
    PCRE2_SIZE error_offset = 0;
    pcre2_code_8* compiled_re = pcre2_compile_8(
      reinterpret_cast<PCRE2_SPTR8>(expr),
      regex.size(),
      options,
      &error_code,
      &error_offset,
      nullptr);
    if (!compiled_re)
    {
      alloc->deallocate(expr, expr_size);

      PCRE2_UCHAR8 error_buffer[256];
      pcre2_get_error_message_8(
        error_code,
        error_buffer,
        sizeof(error_buffer));

      Stream::Error ostr;
      ostr << FNS << "Couldn't compile expression '" << regex <<
        "', Reason: " << reinterpret_cast<const char*>(error_buffer) <<
        ". At position: " << error_offset;
      throw Exception(ostr);
    }

    std::shared_ptr<pcre2_code_8> re(
      compiled_re,
      [](pcre2_code_8* code) noexcept
      {
        pcre2_code_free_8(code);
      });

    uint32_t capture_count = 0;
    pcre2_pattern_info_8(
      re.get(),
      PCRE2_INFO_CAPTURECOUNT,
      &capture_count);

    clear_();

    allocator_ = alloc;
    expr_ = expr;
    expr_len_ = regex.size();
    expr_size_ = expr_size;
    re_ = std::move(re);
    substrcount_ = static_cast<int>(capture_count) + 1;
  }

  bool
  RegEx::search(Result& result, const String::SubString& subject,
    int options) const
    /*throw (Exception, eh::Exception)*/
  {
    if (!re_)
    {
      Stream::Error ostr;
      ostr << FNS << "Expression is not compiled";
      throw Exception(ostr);
    }

    auto match_data = create_match_data_(re_.get());
    if (!match_data)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't create match data";
      throw Exception(ostr);
    }

    if (pcre2_match_8(
      re_.get(),
      reinterpret_cast<PCRE2_SPTR8>(subject.data()),
      subject.size(),
      0,
      options,
      match_data.get(),
      nullptr) <= 0)
    {
      return false;
    }

    PCRE2_SIZE* ovector = pcre2_get_ovector_pointer_8(match_data.get());
    result.resize(substrcount_);
    for (int i = 0; i < substrcount_; i++)
    {
      const auto start = ovector[2 * i];
      const auto end = ovector[2 * i + 1];
      if (start != end)
      {
        result[i] = subject.substr(start, end - start);
      }
    }

    return true;
  }

  void
  RegEx::gsearch(Result& result, const String::SubString& subject,
    int options) const
    /*throw (Exception, eh::Exception)*/
  {
    if (!re_)
    {
      Stream::Error ostr;
      ostr << FNS << "Expression is not compiled";
      throw Exception(ostr);
    }

    // If there are no capturing parenthesis in the regexp, the whole
    // match is returned.
    const int first_capture = substrcount_ > 1 ? 1 : 0;

    auto match_data = create_match_data_(re_.get());
    if (!match_data)
    {
      Stream::Error ostr;
      ostr << FNS << "Can't create match data";
      throw Exception(ostr);
    }

    result.clear();
    PCRE2_SIZE offset = 0;
    while (offset <= subject.size() &&
      pcre2_match_8(
        re_.get(),
        reinterpret_cast<PCRE2_SPTR8>(subject.data()),
        subject.size(),
        offset,
        options,
        match_data.get(),
        nullptr) > 0)
    {
      PCRE2_SIZE* ovector = pcre2_get_ovector_pointer_8(match_data.get());
      size_t res_offset = result.size() - first_capture;
      result.resize(res_offset + substrcount_);
      for (int i = first_capture; i < substrcount_; ++i)
      {
        const auto start = ovector[2 * i];
        const auto end = ovector[2 * i + 1];

        // For both zero-length match (start == end >= 0) and not
        // executed capture (start == end == -1) there's an empty
        // string in the result already.
        if (end > start)
        {
          result[res_offset + i] = subject.substr(start, end - start);
        }
      }
      offset = ovector[1];
      // Provide progress for zero-length match.
      if (ovector[1] == ovector[0])
      {
        ++offset;
      }
    }
  }

  bool
  RegEx::match(const String::SubString& subject, int options) const
    throw ()
  {
    if (!re_)
    {
      return false;
    }

    auto match_data = create_match_data_(re_.get());
    if (!match_data)
    {
      return false;
    }

    return pcre2_match_8(
      re_.get(),
      reinterpret_cast<PCRE2_SPTR8>(subject.data()),
      subject.size(),
      0,
      options,
      match_data.get(),
      nullptr) > 0;
  }
}
