/**
 * @file SimpleDecimal.hpp
 * @author Konstantin Sadov
 * Source for the class SimpleDecimal
 * for decimal number with fixed fraction part containing in the single
 * variable of unsigned integer type
 */

#ifndef GENERICS_SIMPLE_DECIMAL_HPP
#define GENERICS_SIMPLE_DECIMAL_HPP

#include <String/SubString.hpp>

#include <Generics/CommonDecimal.hpp>


namespace Generics
{
  /**
   * SimpleDecimal number class
   * provide fixed point decimal number
   * @param Base type of base implementation element
   * @param TOTAL total rank
   * @param FRACTION fraction rank
   */
  template <typename Base, const unsigned TOTAL, const unsigned FRACTION>
  class SimpleDecimal :
    public SimpleDecimalBase<DecimalBase<Base, TOTAL, FRACTION> >
  {
  public:
    typedef SimpleDecimalBase<DecimalBase<Base, TOTAL, FRACTION> >
      Parent;

    using Parent::TOTAL_RANK;
    using Parent::FRACTION_RANK;
    using Parent::INTEGER_RANK;
    static const unsigned PACK_SIZE = sizeof(Base) + 1;

    DECLARE_EXCEPTION(Exception, DecimalException);
    DECLARE_EXCEPTION(Overflow, Exception);
    DECLARE_EXCEPTION(NotNumber, Exception);
    DECLARE_EXCEPTION(Sign, Exception);

    static const SimpleDecimal ZERO;
    static const SimpleDecimal EPSILON;
    static const SimpleDecimal MAXIMUM;

  public:
    /**
     * Constructor
     * Initializes the number with INVALID_FLAG_
     */
    SimpleDecimal() noexcept;

    /**
     * Construct from parts
     * @tparam Integer integer type to construct from
     * @tparam Fraction fraction type to construct from
     * @param negative sign
     * @param integer integer part
     * @param fraction fraction part
     * @exception Overflow if passed values are too big
     */
    template <typename Integer, typename Fraction>
    SimpleDecimal(bool negative, Integer integer, Fraction fraction)
      /*throw (Overflow)*/;

    /**
     * Construct from decimal rational.
     * The constructed number is integer / 10 ^ power.
     * The least significant digits could be lost.
     * @param integer numerator
     * @param power power of ten in denominator
     */
    template <typename Integer>
    SimpleDecimal(Integer integer, unsigned power)
      /*throw (Overflow)*/;

    /**
     * Construct from string
     * @param str string of decimal number in format [+|-]abcd[.[efg]]
     * @exception Overflow if passed string is bigger value
     * @exception NotNumber if passed string contains not digits
     */
    explicit
    SimpleDecimal(const String::SubString& str) /*throw (Overflow, NotNumber)*/;

    /**
     * Construct from general. Firstly converted to string.
     * @param num general number to construct from
     * @exception Overflow if passed num is bigger value
     * @exception NotNumber if passed num is invalid
     */
    template <typename General>
    explicit
    SimpleDecimal(General num) /*throw (Overflow, NotNumber)*/;

    /**
     * Construct from different SimpleDecimal
     * @param diff different SimpleDecimal
     */
    template <typename DiffBase, const unsigned DIFF_TOTAL,
      const unsigned DIFF_FRACTION>
    explicit
    SimpleDecimal(const SimpleDecimal<DiffBase, DIFF_TOTAL, DIFF_FRACTION>&
      diff) /*throw (Overflow)*/;

    /**
     * Integer representation of this number
     * @tparam ToInteger integer type to convert to
     * @return integer part of this number
     * @exception Overflow if return value can't fit return type
     * @exception Sign if value is negative but conversion is
     * unapplicable
     */
    template <typename ToInteger>
    ToInteger
    integer() const /*throw (Overflow, Sign)*/;

    /**
     * Integer representation of this number
     * @tparam ToInteger integer type to convert to
     * @param val integer part of this number
     * @exception Overflow if return value can't fit return type
     * @exception Sign if value is negative but conversion is
     * inapplicable
     */
    template <typename ToInteger>
    void
    to_integer(ToInteger& val) const /*throw (Overflow, Sign)*/;

    /**
     * Floating representation of this number
     * Precision loss is possible
     * @tparam ToFloating floating type to convert to
     * @return integer part of this number
     */
    template <typename ToFloating>
    ToFloating
    floating() const noexcept;

    /**
     * Floating representation of this number
     * @tparam ToFloating integer type to return to
     * @param val integer part of this number
     */
    template <typename ToFloating>
    void
    to_floating(ToFloating& val) const noexcept;

    /**
     * String representation of this number
     * @return string representation of this number in format [-]abcd[.efg]
     */
    std::string
    str() const noexcept;

    /**
     * Internal dump of this number
     * @return Internal dump of this number
     */
    std::string
    dump() const /*throw (eh::Exception)*/;

    /**
     * Packs current value into PACK_SIZE bytes long buffer
     * @param buffer pointer to PACK_SIZE bytes long buffer
     */
    void
    pack(void* buffer) const noexcept;

    /**
     * Unpacks current value from PACK_SIZE bytes long buffer
     * @param buffer pointer to PACK_SIZE bytes long buffer
     */
    void
    unpack(const void* buffer) noexcept;

    /**
     * Revert sign of this number
     * @return this
     */
    SimpleDecimal&
    negate() noexcept;

    /**
     * Makes floor of absolute value of this
     * @param fraction fraction rank for floor (zero means integer)
     * @return this
     */
    SimpleDecimal&
    floor(unsigned fraction) noexcept;

    /**
     * Makes ceil of absolute value of this
     * @param fraction fraction rank for ceil (zero means integer)
     * @return this
     * @exception Overflow if result is too big
     */
    SimpleDecimal&
    ceil(unsigned fraction) /*throw (eh::Exception, Overflow)*/;

    /**
     * Test on zero
     * @return true if number is zero
     */
    bool
    is_zero() const noexcept;

    /**
     * Test on greater than or equal to zero
     * @return true if number greater than or equal to zero
     */
    bool
    is_nonnegative() const noexcept;

    /**
     * Test on less than or equal to zero
     * @return true if number less than or equal to zero
     */
    bool
    is_nonpositive() const noexcept;

    /**
     * Test on equality
     * @param test value to compare for equality
     * @return true if equal or false otherwise
     */
    bool
    operator ==(const SimpleDecimal& test) const noexcept;

    /**
     * Test on not equality
     * @param test value to compare for inequality
     * @return true if not equal or false otherwise
     */
    bool
    operator !=(const SimpleDecimal& test) const noexcept;

    /**
     * Test on minority
     * @param test value to compare for minority
     * @return true if less than or false otherwise
     */
    bool
    operator <(const SimpleDecimal& test) const noexcept;

    /**
     * Test on minority or equality
     * @param test value to compare for minority or equality
     * @return true if less than or equal to or false otherwise
     */
    bool
    operator <=(const SimpleDecimal& test) const noexcept;

    /**
     * Test on majority
     * @param test value to compare for majority
     * @return true if greater than or false otherwise
     */
    bool
    operator >(const SimpleDecimal& test) const noexcept;

    /**
     * Test on majority or equality
     * @param test value to compare for majority or equality
     * @return true if greater than or equal to or false otherwise
     */
    bool
    operator >=(const SimpleDecimal& test) const noexcept;

    /**
     * Add summand to this
     * @param summand summand
     * @return this
     * @exception Overflow if result is too big
     */
    SimpleDecimal&
    operator +=(const SimpleDecimal& summand)
      /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

    /**
     * Subtruct subtrahend from this
     * @param subtrahend subtrahend
     * @return this
     * @exception Overflow if result is too big
     */
    SimpleDecimal&
    operator -=(const SimpleDecimal& subtrahend)
      /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

    /**
     * Add summand to value of this
     * @param summand summand
     * @return new value result of summation
     * @exception Overflow if result is too big
     */
    SimpleDecimal
    operator +(const SimpleDecimal& summand) const
      /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

    /**
     * Subtract subtrahend from value of this
     * @param subtrahend subtrahend
     * @return new value result of subtraction
     * @exception Overflow if result is too big
     */
    SimpleDecimal
    operator -(const SimpleDecimal& subtrahend) const
      /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

    /**
     * Do multiplication of decimals
     * @param factor1 the first factor
     * @param factor2 the second factor
     * @param dmr remainder processing behaviour
     * @return result of multiplication
     * @exception Overflow if result is too big
     */
    static
    SimpleDecimal
    mul(const SimpleDecimal& factor1, const SimpleDecimal& factor2,
      DecimalMulRemainder dmr) /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

    /**
     * Do division of decimals
     * @param dividend dividend
     * @param divisor divisor
     * @param remainder remainder
     * @return quotient
     * @exception Overflow if result is too big
     */
    static
    SimpleDecimal
    div(const SimpleDecimal& dividend, const SimpleDecimal& divisor,
      SimpleDecimal& remainder) /*throw (eh::Exception, Overflow)*/;

    /**
     * Do division of decimals
     * @param dividend dividend
     * @param divisor divisor
     * @param ddr remainder processing behaviour
     * @return quotient
     * @exception Overflow if result is too big
     */
    static
    SimpleDecimal
    div(const SimpleDecimal& dividend, const SimpleDecimal& divisor,
      DecimalDivRemainder ddr = DDR_FLOOR)
      /*throw (eh::Exception, Overflow)*/;

    /**
     * Make sum of decimals
     * @param summand1 the first summand
     * @param summand2 the second summand
     * @param target result of operation
     * @exception Overflow if result is too big
     */
    static
    void
    add(const SimpleDecimal& summand1, const SimpleDecimal& summand2,
      SimpleDecimal& target) /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

    /**
     * Make subtruction of decimals
     * @param minuend minuend
     * @param subtrahend subtrahend
     * @param target result of operation
     * @exception Overflow if result is too big
     */
    static
    void
    sub(const SimpleDecimal& minuend, const SimpleDecimal& subtrahend,
      SimpleDecimal& target) /*throw (eh::Exception, Overflow)*/
      __attribute__((always_inline));

  private:
    using Parent::MAX_VALUE_;
    using Parent::MAX_INTEGER_;
    using Parent::MAX_FRACTION_;

    static const Base INVALID_FLAG_ = -1;

    /**
     * Construct from parts helper
     * @tparam Integer integer type to construct from
     * @tparam Fraction fraction type to construct from
     * @param negative sign
     * @param integer integer part
     * @param fraction fraction part
     * @exception Overflow if passed values too big
     */
    template <typename Integer, typename Fraction>
    void
    construct_(bool negative, Integer integer, Fraction fraction)
      /*throw (Overflow)*/;

    /**
     * Construct from decimal rational helper.
     * The constructed number is integer / 10 ^ power.
     * The least significant digits could be lost.
     * @param integer numerator
     * @param power power of ten in denominator
     */
    template <typename Integer>
    void
    construct_(Integer integer, unsigned power)
      /*throw (Overflow)*/;

    /**
     * Construct from string helper
     * @param str string of decimal number in format [+|-][abcd[.[efg]]]
     * @exception Overflow if passed string is bigger value
     * @exception NotNumber if passed string contains not digits
     */
    void
    construct_(const String::SubString& str)
      /*throw (Overflow, NotNumber)*/;

    /**
     * Do division of decimals
     * @param dividend dividend
     * @param divisor divisor
     * @param quotient quotient
     * @param ddr remainder processing behaviour
     * @exception Overflow if result is too big
     */
    static
    void
    div_(const SimpleDecimal& dividend, const SimpleDecimal& divisor,
      SimpleDecimal& quotient, DecimalDivRemainder ddr)
      /*throw (eh::Exception, Overflow)*/;

    /**
     * Numeric conversion to character.
     * Result is returned right-justified in the buffer.
     * @param buf_end The pointer to end of buffer enough to hold
     * string with SimpleDecimal
     * @return The pointer to begin of string with text form
     * of SimpleDecimal
     */
    char*
    decimal_to_char_(char* buf_end) const noexcept;

    static
    void
    throw_overflow(const char* func, const char* when,
      const SimpleDecimal& d1, const SimpleDecimal& d2)
      /*throw (Overflow)*/ __attribute__((__noinline__));

  private:
    bool negative_;
    Base data_;

    template <typename DiffBase, const unsigned DIFF_TOTAL,
      const unsigned DIFF_FRACTION>
    friend class SimpleDecimal;

    template <typename DiffBase, const unsigned DIFF_TOTAL,
      const unsigned DIFF_FRACTION>
    friend
    std::ostream&
    operator <<(std::ostream& ostr,
      const SimpleDecimal<DiffBase, DIFF_TOTAL, DIFF_FRACTION>& number)
    /*throw (eh::Exception)*/;

    template <typename DiffBase, const unsigned DIFF_TOTAL,
      const unsigned DIFF_FRACTION>
    friend
    std::istream&
    operator >>(std::istream& istr,
      SimpleDecimal<DiffBase, DIFF_TOTAL, DIFF_FRACTION>& number)
    /*throw (eh::Exception)*/;

    template <typename Hash, typename DiffBase, const unsigned DIFF_TOTAL,
      const unsigned DIFF_FRACTION>
    friend
    void
    hash_add(Hash& hash,
      const SimpleDecimal<DiffBase, DIFF_TOTAL, DIFF_FRACTION>& key)
      noexcept;

    template <typename DiffBase, const unsigned DIFF_TOTAL,
      const unsigned DIFF_FRACTION>
    friend class Decimal;
  };

  // Stream functions
  template <typename Base, const unsigned TOTAL, const unsigned FRACTION>
  std::ostream&
  operator <<(std::ostream& ostr,
    const SimpleDecimal<Base, TOTAL, FRACTION>& number)
    /*throw (eh::Exception)*/;

  template <typename Base, const unsigned TOTAL, const unsigned FRACTION>
  std::istream&
  operator >>(std::istream& istr,
    SimpleDecimal<Base, TOTAL, FRACTION>& number) /*throw (eh::Exception)*/;

  template <typename Hash, typename Base, const unsigned TOTAL,
    const unsigned FRACTION>
  void
  hash_add(Hash& hash, const SimpleDecimal<Base, TOTAL, FRACTION>& key)
    noexcept;
}

#include <Generics/SimpleDecimal.tpp>

#endif
