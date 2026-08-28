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
      static size_t convert(Integer value, char* str) noexcept;
    };

    template <typename Integer>
    size_t IntToStrSign<Integer, false>::convert(Integer value, char* str) noexcept
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
      static size_t convert(Integer value, char* str) noexcept;
    };

    template <typename Integer>
    size_t IntToStrSign<Integer, true>::convert(Integer value, char* str) noexcept
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
  size_t int_to_str(Integer value, char* str, size_t size) noexcept
  {
    static_assert(std::numeric_limits<Integer>::is_integer, "Integer is not an integer type");

    if (size < std::numeric_limits<Integer>::digits10 + 3)
    {
      return 0;
    }

    using Converter =
      IntToStrHelper::IntToStrSign<Integer, std::numeric_limits<Integer>::is_signed>;
    return Converter::convert(value, str);
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
      using Type = std::make_unsigned_t<Integer>;
    };

    template <>
    struct Unsigned<bool>
    {
      using Type = unsigned char;
    };

    template <std::size_t Size>
    __attribute__((always_inline))
    inline bool parse_uint32_scalar(const char* data, std::uint32_t& value) noexcept
    {
      static_assert(Size >= 1 && Size <= 3);

      std::uint32_t result = 0;
      for (std::size_t index = 0; index < Size; ++index)
      {
        const unsigned int digit =
          static_cast<unsigned char>(data[index]) - static_cast<unsigned char>('0');
        if (digit > 9)
        {
          return false;
        }
        result = result * 10 + digit;
      }
      value = result;
      return true;
    }

    __attribute__((always_inline))
    inline bool parse_uint32_digit4(const char* data, std::uint32_t& value) noexcept
    {
      std::uint32_t digits;
      std::memcpy(&digits, data, sizeof(digits));

      // Either operation sets a byte's high bit for values outside '0'..'9'.
      if (((digits + 0x46464646U) | (digits - 0x30303030U)) & 0x80808080U)
      {
        return false;
      }

      // Fold 4 digits into 2 pairs, then into the final value.
      digits = (digits & 0x0f0f0f0fU) * 2561U >> 8;
      digits = (digits & 0x00ff00ffU) * 6553601U >> 16;
      value = digits;
      return true;
    }

    __attribute__((always_inline))
    inline bool parse_uint32_digit8(const char* data, std::uint32_t& value) noexcept
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

    inline bool is_zero_prefix(const char* data, std::size_t size) noexcept
    {
      while (size >= 8)
      {
        std::uint64_t block;
        std::memcpy(&block, data, sizeof(block));
        if (block != 0x3030303030303030ULL)
        {
          return false;
        }
        data += 8;
        size -= 8;
      }

      if (size >= 4)
      {
        std::uint32_t block;
        std::memcpy(&block, data, sizeof(block));
        if (block != 0x30303030U)
        {
          return false;
        }
        data += 4;
        size -= 4;
      }

      while (size)
      {
        if (*data != '0')
        {
          return false;
        }
        ++data;
        --size;
      }
      return true;
    }

    template <std::size_t Size>
    __attribute__((always_inline))
    inline bool parse_uint32(const char* data, std::uint32_t& value) noexcept
    {
      static_assert(Size >= 1 && Size <= 10);

      if constexpr (Size <= 3)
      {
        return parse_uint32_scalar<Size>(data, value);
      }
      else if constexpr (Size == 4)
      {
        return parse_uint32_digit4(data, value);
      }
      else if constexpr (Size <= 7)
      {
        constexpr std::size_t PrefixSize = Size - 4;
        std::uint32_t high = 0;
        std::uint32_t low = 0;
        if (!parse_uint32_scalar<PrefixSize>(data, high) ||
          !parse_uint32_digit4(data + PrefixSize, low))
        {
          return false;
        }
        value = high * 10'000U + low;
        return true;
      }
      else if constexpr (Size == 8)
      {
        return parse_uint32_digit8(data, value);
      }
      else
      {
        constexpr std::size_t PrefixSize = Size - 8;
        std::uint32_t high = 0;
        std::uint32_t low = 0;
        if (!parse_uint32_scalar<PrefixSize>(data, high) ||
          !parse_uint32_digit8(data + PrefixSize, low))
        {
          return false;
        }

        if constexpr (Size == 10)
        {
          const std::uint64_t result = static_cast<std::uint64_t>(high) * 100'000'000U + low;
          if (result > std::numeric_limits<std::uint32_t>::max())
          {
            return false;
          }
          value = static_cast<std::uint32_t>(result);
        }
        else
        {
          value = high * 100'000'000U + low;
        }
        return true;
      }
    }

    template <typename Integer>
    inline bool str_to_int_generic(const char* current, const char* end,
      [[maybe_unused]] bool negative, Integer& value) noexcept
    {
      using UnsignedType = typename Unsigned<Integer>::Type;

      constexpr std::ptrdiff_t SAFE_DIGITS = std::numeric_limits<UnsignedType>::digits10;
      const char* const safe_end = end - current < SAFE_DIGITS ? end : current + SAFE_DIGITS;

      UnsignedType magnitude = 0;
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

      constexpr UnsignedType MAX_VALUE = std::numeric_limits<UnsignedType>::max();
      constexpr UnsignedType MAX_VALUE_DIV_10 = MAX_VALUE / 10;
      constexpr unsigned int MAX_VALUE_MOD_10 = MAX_VALUE % 10;
      for (; current != end; ++current)
      {
        const unsigned int digit =
          static_cast<unsigned char>(*current) - static_cast<unsigned char>('0');
        if (digit > 9 || magnitude > MAX_VALUE_DIV_10 ||
          (magnitude == MAX_VALUE_DIV_10 && digit > MAX_VALUE_MOD_10))
        {
          return false;
        }
        magnitude = magnitude * 10 + digit;
      }

      if constexpr (std::numeric_limits<Integer>::is_signed)
      {
        constexpr UnsignedType POSITIVE_LIMIT =
          static_cast<UnsignedType>(std::numeric_limits<Integer>::max());
        constexpr UnsignedType NEGATIVE_LIMIT = POSITIVE_LIMIT + 1;
        const UnsignedType limit = negative ? NEGATIVE_LIMIT : POSITIVE_LIMIT;
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
        constexpr UnsignedType LIMIT =
          static_cast<UnsignedType>(std::numeric_limits<Integer>::max());
        if (magnitude > LIMIT)
        {
          return false;
        }
        value = static_cast<Integer>(magnitude);
      }

      return true;
    }
  }

  template <typename Integer>
  bool str_to_int(const String::SubString& str, Integer& value) noexcept
  {
    static_assert(std::numeric_limits<Integer>::is_integer, "Integer is not an integer type");

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
      switch (digits_count)
      {
      case 1:
        return StrToIntHelper::parse_uint32<1>(current, value);
      case 2:
        return StrToIntHelper::parse_uint32<2>(current, value);
      case 3:
        return StrToIntHelper::parse_uint32<3>(current, value);
      case 4:
        return StrToIntHelper::parse_uint32<4>(current, value);
      case 5:
        return StrToIntHelper::parse_uint32<5>(current, value);
      case 6:
        return StrToIntHelper::parse_uint32<6>(current, value);
      case 7:
        return StrToIntHelper::parse_uint32<7>(current, value);
      case 8:
        return StrToIntHelper::parse_uint32<8>(current, value);
      case 9:
        return StrToIntHelper::parse_uint32<9>(current, value);
      case 10:
        return StrToIntHelper::parse_uint32<10>(current, value);
      default:
      {
        const std::size_t prefix_size = digits_count - 10;
        if (!StrToIntHelper::is_zero_prefix(current, prefix_size))
        {
          return false;
        }
        return StrToIntHelper::parse_uint32<10>(current + prefix_size, value);
      }
      }
    }
    else
    {
      return StrToIntHelper::str_to_int_generic(current, end, negative, value);
    }
  }

  template <typename Integer>
  bool str_to_int(std::string_view str, Integer& value) noexcept
  {
    if (str.empty())
    {
      return false;
    }

    return str_to_int(SubString(str.data(), str.size()), value);
  }

  template <typename Integer>
  bool str_to_int(const std::string& str, Integer& value) noexcept
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
  bool InverseCategory<Category>::is_owned(Character ch) const noexcept
  {
    return !Category::is_owned(ch);
  }

  template <class Category>
  template <typename Character>
  bool InverseCategory<Category>::operator ()(Character ch) const noexcept
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

  inline SubString IntToStr::str() const noexcept
  {
    return SubString(buf_, length_);
  }

  inline IntToStr::operator SubString() const noexcept
  {
    return str();
  }
}
