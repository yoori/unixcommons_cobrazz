#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace String::StringManip
{
  //
  // int_to_str function
  //

  namespace IntToStrHelper
  {
    template <typename Integer, const bool is_signed>
    struct IntToStrSign;

    template <typename Integer>
    struct IntToStrSign<Integer, false>
    {
      static size_t
      convert(Integer value, char* str) noexcept;
    };

    template <typename Integer>
    size_t
    IntToStrSign<Integer, false>::convert(Integer value, char* str) noexcept
    {
      char* ptr = str;
      do
      {
        *ptr++ = '0' + value % 10;
      }
      while (value /= 10);

      size_t size = ptr - str;

      for (*ptr-- = '\0'; str < ptr; str++, ptr--)
      {
        std::swap(*str, *ptr);
      }

      return size;
    }

    template <typename Integer>
    struct IntToStrSign<Integer, true>
    {
      static size_t
      convert(Integer value, char* str) noexcept;
    };

    template <typename Integer>
    size_t
    IntToStrSign<Integer, true>::convert(Integer value, char* str) noexcept
    {
      if (value < -std::numeric_limits<Integer>::max())
      {
        return 0;
      }

      if (value < 0)
      {
        *str = '-';
        return IntToStrSign<Integer, false>::convert(-value, str + 1) + 1;
      }

      return IntToStrSign<Integer, false>::convert(value, str);
    }
  }

  template <typename Integer>
  size_t
  int_to_str(Integer value, char* str, size_t size) noexcept
  {
    static_assert(std::numeric_limits<Integer>::is_integer,
      "Integer is not an integer type");

    if (size < std::numeric_limits<Integer>::digits10 + 3)
    {
      return 0;
    }

    return IntToStrHelper::IntToStrSign<Integer,
      std::numeric_limits<Integer>::is_signed>::convert(value, str);
  }

  namespace StrToIntHelper
  {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    constexpr bool UINT32_SWAR_SUPPORTED = true;
#elif defined(_WIN32)
    constexpr bool UINT32_SWAR_SUPPORTED = true;
#else
    constexpr bool UINT32_SWAR_SUPPORTED = false;
#endif

    template <typename Integer>
    struct Unsigned
    {
      typedef typename std::make_unsigned<Integer>::type Type;
    };

    template <>
    struct Unsigned<bool>
    {
      typedef unsigned char Type;
    };

    inline bool
    parse_uint32_eight_digits(const char* data, std::uint32_t& value) noexcept
    {
      std::uint64_t digits;
      std::memcpy(&digits, data, sizeof(digits));

      // Either operation sets a byte's high bit for values outside '0'..'9'.
      if (((digits + 0x4646464646464646ULL) |
        (digits - 0x3030303030303030ULL)) & 0x8080808080808080ULL)
      {
        return false;
      }

      // Fold 8 digits into 4 pairs, then 2 groups, then the final value.
      digits = (digits & 0x0f0f0f0f0f0f0f0fULL) * 2561 >> 8;
      digits = (digits & 0x00ff00ff00ff00ffULL) * 6553601 >> 16;
      digits = (digits & 0x0000ffff0000ffffULL) * 42949672960001ULL >> 32;
      value = static_cast<std::uint32_t>(digits);
      return true;
    }
  }

  template <typename Integer>
  bool
  str_to_int(const String::SubString& str, Integer& value) noexcept
  {
    static_assert(std::numeric_limits<Integer>::is_integer, "Integer is not an integer type");

    typedef typename StrToIntHelper::Unsigned<Integer>::Type Unsigned;

    const char* current = str.begin();
    const char* const end = str.end();
    if (current == end)
    {
      return false;
    }

    const bool negative = *current == '-';
    if (negative)
    {
      if (!std::numeric_limits<Integer>::is_signed)
      {
        return false;
      }
      ++current;
    }
    else if (*current == '+')
    {
      ++current;
    }

    if (current == end)
    {
      return false;
    }

    value = 0;

    if constexpr (std::is_same_v<Integer, std::uint32_t> && StrToIntHelper::UINT32_SWAR_SUPPORTED)
    {
      const std::size_t digits_count = end - current;
      if (digits_count == 8)
      {
        return StrToIntHelper::parse_uint32_eight_digits(current, value);
      }

      if (digits_count == 9)
      {
        const unsigned int first_digit =
          static_cast<unsigned char>(*current) -
          static_cast<unsigned char>('0');
        std::uint32_t tail;
        if (first_digit > 9 || !StrToIntHelper::parse_uint32_eight_digits(current + 1, tail))
        {
          return false;
        }
        value = first_digit * 100'000'000U + tail;
        return true;
      }
    }

    constexpr std::ptrdiff_t SAFE_DIGITS = std::numeric_limits<Unsigned>::digits10;
    const char* const safe_end = end - current < SAFE_DIGITS ? end : current + SAFE_DIGITS;

    Unsigned magnitude = 0;
    for (; current != safe_end; ++current)
    {
      const unsigned int digit =
        static_cast<unsigned char>(*current) - static_cast<unsigned char>('0');
      if (digit > 9)
      {
        return false;
      }
      magnitude = magnitude * 10 + digit;
    }

    constexpr Unsigned MAX_VALUE = std::numeric_limits<Unsigned>::max();
    constexpr Unsigned MAX_VALUE_DIV_10 = MAX_VALUE / 10;
    constexpr unsigned int MAX_VALUE_MOD_10 = MAX_VALUE % 10;
    for (; current != end; ++current)
    {
      const unsigned int digit =
        static_cast<unsigned char>(*current) - static_cast<unsigned char>('0');
      if (digit > 9 ||
        magnitude > MAX_VALUE_DIV_10 ||
        (magnitude == MAX_VALUE_DIV_10 && digit > MAX_VALUE_MOD_10))
      {
        return false;
      }
      magnitude = magnitude * 10 + digit;
    }

    if constexpr (std::numeric_limits<Integer>::is_signed)
    {
      constexpr Unsigned POSITIVE_LIMIT = static_cast<Unsigned>(std::numeric_limits<Integer>::max());
      constexpr Unsigned NEGATIVE_LIMIT = POSITIVE_LIMIT + 1;
      const Unsigned limit = negative ? NEGATIVE_LIMIT : POSITIVE_LIMIT;
      if (magnitude > limit)
      {
        return false;
      }

      if (negative)
      {
        value = magnitude == NEGATIVE_LIMIT ? std::numeric_limits<Integer>::min() :
          -static_cast<Integer>(magnitude);
      }
      else
      {
        value = static_cast<Integer>(magnitude);
      }
    }
    else
    {
      constexpr Unsigned LIMIT = static_cast<Unsigned>(std::numeric_limits<Integer>::max());
      if (magnitude > LIMIT)
      {
        return false;
      }
      value = static_cast<Integer>(magnitude);
    }

    return true;
  }

  template <typename Integer>
  bool
  str_to_int(std::string_view str, Integer& value) noexcept
  {
    if (str.empty())
    {
      return false;
    }

    return str_to_int(SubString(str.data(), str.size()), value);
  }

  template <typename Integer>
  bool
  str_to_int(const std::string& str, Integer& value) noexcept
  {
    return str_to_int(std::string_view(str.data(), str.size()), value);
  }

  //
  // InverseCategory class
  //

  template <class Category>
  InverseCategory<Category>::InverseCategory() /*throw (eh::Exception)*/
    : Category()
  {}

  template <class Category>
  template <typename... T>
  InverseCategory<Category>::InverseCategory(T... args)
    /*throw (eh::Exception)*/
    : Category(std::forward<T>(args)...)
  {}

  template <class Category>
  template <typename Character>
  bool
  InverseCategory<Category>::is_owned(Character ch) const noexcept
  {
    return !Category::is_owned(ch);
  }

  template <class Category>
  template <typename Character>
  bool
  InverseCategory<Category>::operator ()(Character ch) const noexcept
  {
    return is_owned(ch);
  }

  template <class Category>
  const char*
  InverseCategory<Category>::find_owned(
    const char* begin, const char* end, unsigned long* octets) const
    noexcept
  {
    return Category::find_nonowned(begin, end, octets);
  }

  template <class Category>
  const char*
  InverseCategory<Category>::find_nonowned(
    const char* begin, const char* end, unsigned long* octets) const
    noexcept
  {
    return Category::find_owned(begin, end, octets);
  }

  template <class Category>
  const char*
  InverseCategory<Category>::rfind_owned(
    const char* begin, const char* end, unsigned long* octets) const
    noexcept
  {
    return Category::rfind_nonowned(begin, end, octets);
  }

  template <class Category>
  const char*
  InverseCategory<Category>::rfind_nonowned(
    const char* begin, const char* end, unsigned long* octets) const
    noexcept
  {
    return Category::rfind_owned(begin, end, octets);
  }

  //
  // IntToStr class
  //

  template <typename Integer>
  IntToStr::IntToStr(Integer value) noexcept
    : length_(int_to_str(value, buf_, sizeof(buf_)))
  {}

  inline
  SubString
  IntToStr::str() const noexcept
  {
    return SubString(buf_, length_);
  }

  inline
  IntToStr::operator SubString() const noexcept
  {
    return str();
  }
}
