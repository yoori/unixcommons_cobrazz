#pragma once

#include <memory>
#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <String/SubString.hpp>

#include <Generics/Allocator.hpp>


namespace String
{
  #ifndef PCRE_ANCHORED
  #define PCRE_ANCHORED PCRE2_ANCHORED
  #endif
  #ifndef PCRE_CASELESS
  #define PCRE_CASELESS PCRE2_CASELESS
  #endif
  #ifndef PCRE_DOLLAR_ENDONLY
  #define PCRE_DOLLAR_ENDONLY PCRE2_DOLLAR_ENDONLY
  #endif
  #ifndef PCRE_DOTALL
  #define PCRE_DOTALL PCRE2_DOTALL
  #endif
  #ifndef PCRE_DUPNAMES
  #define PCRE_DUPNAMES PCRE2_DUPNAMES
  #endif
  #ifndef PCRE_EXTENDED
  #define PCRE_EXTENDED PCRE2_EXTENDED
  #endif
  #ifndef PCRE_FIRSTLINE
  #define PCRE_FIRSTLINE PCRE2_FIRSTLINE
  #endif
  #ifndef PCRE_MULTILINE
  #define PCRE_MULTILINE PCRE2_MULTILINE
  #endif
  #ifndef PCRE_NOTBOL
  #define PCRE_NOTBOL PCRE2_NOTBOL
  #endif
  #ifndef PCRE_NOTEOL
  #define PCRE_NOTEOL PCRE2_NOTEOL
  #endif
  #ifndef PCRE_NOTEMPTY
  #define PCRE_NOTEMPTY PCRE2_NOTEMPTY
  #endif
  #ifndef PCRE_NO_AUTO_CAPTURE
  #define PCRE_NO_AUTO_CAPTURE PCRE2_NO_AUTO_CAPTURE
  #endif
  #ifndef PCRE_NO_UTF8_CHECK
  #define PCRE_NO_UTF8_CHECK PCRE2_NO_UTF_CHECK
  #endif
  #ifndef PCRE_UNGREEDY
  #define PCRE_UNGREEDY PCRE2_UNGREEDY
  #endif
  #ifndef PCRE_UTF8
  #define PCRE_UTF8 PCRE2_UTF
  #endif

  /**
   * Wrapper for pcre library
   */
  class RegEx
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    class MatchContext
    {
    public:
      MatchContext() noexcept;
      MatchContext(const MatchContext&) = delete;
      MatchContext& operator=(const MatchContext&) = delete;
      MatchContext(MatchContext&& init) noexcept;
      MatchContext& operator=(MatchContext&& init) noexcept;
      ~MatchContext() noexcept;

    private:
      friend class RegEx;

      bool ensure_ovector_(uint32_t ovector_count) noexcept;

      pcre2_match_data_8* match_data_;
      uint32_t ovector_count_;
    };

    /**
     * Constructor
     * Compiles regexp if required
     * @param regex regular expression
     * @param options compilation options (see pcreapi(3))
     * @param allocator custom allocator for expression and compiled regex
     */
    explicit RegEx(const String::SubString& regex = String::SubString(),
      int options = 0, Generics::Allocator::Base* allocator = 0)
      /*throw (Exception, eh::Exception)*/;

    /**
     * Copy constructor
     * Increases compiled regexp reference count
     * @param side source regexp
     */
    RegEx(const RegEx& side)
      /*throw (Exception, eh::Exception)*/;

    /**
     * Destructor
     * Decreases compiled regexp reference count and deletes if appropriate
     */
    ~RegEx() noexcept;

    /**
     * Assignment operator
     * Increases compiled regexp reference count
     * @param side source regexp
     * @return reference to destination regexp
     */
    RegEx& operator =(const RegEx& side)
      /*throw (Exception, eh::Exception)*/;


    /**
     * Reinitializes regexp with a new regular expression
     * @param regex regular expression
     * @param options compilation options (see pcreapi(3))
     * @param allocator custom allocator for expression and compiled regex
     */
    void
    set_expression(const String::SubString& regex, int options = 0,
      Generics::Allocator::Base* allocator = 0)
      /*throw (Exception, eh::Exception)*/;


    using Result = std::vector<SubString>;

    /**
     * Returns total number of substrings in regular expressions
     * @return expected number of substrings
     */
    int sub_strings() const /*throw (Exception)*/;

    /**
     * Performes execution of compiled regular expression
     * and returns all of the found substrings
     * @param result resulted list of substrings
     * @param subject string to match
     * @param options execution options (see pcreapi(3))
     * @return if match occurred or not
     */
    bool search(Result& result, const String::SubString& subject, int options = 0) const
      /*throw (Exception, eh::Exception)*/;

    bool
    search(Result& result, const String::SubString& subject,
      MatchContext& match_context, int options = 0) const
      /*throw (Exception, eh::Exception)*/;

    /**
     * Performes execution of compiled regular expression and returns
     * all of the found substrings, in the sense of /g Perl regexp
     * modifier.
     * @param result resulted list of substrings
     * @param subject string to match
     * @param options execution options (see pcreapi(3))
     */
    void gsearch(Result& result, const String::SubString& subject, int options = 0) const
      /*throw (Exception, eh::Exception)*/;

    void
    gsearch(Result& result, const String::SubString& subject,
      MatchContext& match_context, int options = 0) const
      /*throw (Exception, eh::Exception)*/;

    /**
     * Performes "quick" execution of compiled regular expression
     * Neither exceptions nor implicit memory allocation is preformed
     * @param subject string to match
     * @param options execution options (see pcreapi(3))
     * @return if subject matches compiled regular expression or not
     */
    bool match(const String::SubString& subject, int options = 0) const noexcept;

    bool match(const String::SubString& subject, MatchContext& match_context, int options = 0) const
      noexcept;

    /**
     * Compiled regular expression
     * @return original regular expression
     */
    String::SubString expression() const noexcept;


  private:
    /**
     * Provides data members initialization
     */
    void init_() noexcept;

    /**
     * Provides data members clearance
     */
    void clear_() noexcept;

    Generics::Allocator::SmartBase_var allocator_;
    char* expr_;
    size_t expr_len_;
    size_t expr_size_;
    std::shared_ptr<pcre2_code_8> re_;
    int substrcount_;
  };

  /**
   * Template version of RegEx with custom allocator.
   * Default allocator constructor is used.
   */
  template <typename Alloc = std::allocator<char> >
  class BasicRegEx : public RegEx
  {
  public:
    /**
     * Constructor
     * Compiles regexp if required
     * @param regex regular expression
     * @param options compilation options (see pcreapi(3))
     */
    explicit BasicRegEx(const String::SubString& regex = String::SubString(),
      int options = 0) /*throw (Exception, eh::Exception)*/;

  private:
    static Generics::Allocator::Base_var alloc_;
  };
}

////////////////////////
// INLINES
////////////////////////

namespace String
{
  //
  // RegEx class
  //

  inline void RegEx::init_() noexcept
  {
    expr_ = 0;
    expr_len_ = 0;
    expr_size_ = 0;
    re_.reset();
    substrcount_ = 0;
  }

  inline void RegEx::clear_() noexcept
  {
    if (expr_)
    {
      allocator_->deallocate(expr_, expr_size_);
    }

    re_.reset();
    allocator_.reset();
    init_();
  }

  inline
  RegEx::RegEx(const String::SubString& regex, int options, Generics::Allocator::Base* allocator)
    /*throw (Exception, eh::Exception)*/
  {
    init_();

    if (regex.data())
    {
      set_expression(regex, options, allocator);
    }
  }

  inline RegEx::RegEx(const RegEx& side)
    /*throw (Exception, eh::Exception)*/
  {
    init_();

    *this = side;
  }

  inline RegEx::~RegEx() noexcept
  {
    clear_();
  }

  inline int RegEx::sub_strings() const /*throw (Exception)*/
  {
    if (!re_)
    {
      Stream::Error ostr;
      ostr << FNS << "Expression is not compiled";
      throw Exception(ostr);
    }

    return substrcount_;
  }

  inline String::SubString RegEx::expression() const noexcept
  {
    return SubString(expr_, expr_len_);
  }


  //
  // BasicRegEx class
  //

  template <typename Alloc>
  Generics::Allocator::Base_var BasicRegEx<Alloc>::alloc_(
    Generics::Allocator::Template<Alloc>::allocator());

  template <typename Alloc>
  BasicRegEx<Alloc>::BasicRegEx(const String::SubString& regex,
    int options) /*throw (Exception, eh::Exception)*/
    : RegEx(regex, options, alloc_)
  {
  }
}
