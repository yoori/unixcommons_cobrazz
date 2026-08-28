#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <String/StringManip.hpp>

namespace
{
  struct Options
  {
    std::size_t iterations = 5000;
    std::size_t values = 4096;
  };

  std::size_t parse_size(std::string_view value, std::string_view option)
  {
    std::size_t result = 0;
    const auto parsed = std::from_chars( value.data(), value.data() + value.size(), result);
    if (parsed.ec != std::errc() || parsed.ptr != value.data() + value.size() || result == 0)
    {
      throw std::invalid_argument( std::string(option) + " requires a positive integer");
    }
    return result;
  }

  Options parse_options(int argc, char** argv)
  {
    Options options;
    for (int index = 1; index < argc; ++index)
    {
      const std::string_view argument(argv[index]);
      if (argument == "--help")
      {
        std::cout << "Usage: " << argv[0]
          << " [--iterations N] [--values N]\n";
        std::exit(0);
      }

      if (argument != "--iterations" && argument != "--values")
      {
        throw std::invalid_argument( "unknown option: " + std::string(argument));
      }

      if (++index == argc)
      {
        throw std::invalid_argument( std::string(argument) + " requires a value");
      }

      const std::size_t parsed = parse_size(argv[index], argument);
      if (argument == "--iterations")
      {
        options.iterations = parsed;
      }
      else
      {
        options.values = parsed;
      }
    }
    return options;
  }

  struct StringManipParser
  {
    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      return String::StringManip::str_to_int(input, value);
    }
  };

  struct LegacyParser
  {
    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      const char* current = input.data();
      const char* const end = current + input.size();
      if (current == end)
      {
        return false;
      }

      bool negative = false;
      switch (*current)
      {
      case '-':
        if (!std::numeric_limits<IntegerType>::is_signed)
        {
          return false;
        }
        negative = true;
        [[fallthrough]];
      case '+':
        if (++current == end)
        {
          return false;
        }
      }

      value = 0;
      const IntegerType limit = std::numeric_limits<IntegerType>::max() / 10;
      if (negative)
      {
        do
        {
          const unsigned char digit = static_cast<unsigned char>(*current) -
            static_cast<unsigned char>('0');
          if (digit > 9 || value < -limit || (value == -limit && digit > static_cast<unsigned char>(
                -(std::numeric_limits<IntegerType>::min() + limit * 10))))
          {
            return false;
          }
          value = value * static_cast<IntegerType>(10) -
            static_cast<IntegerType>(digit);
        }
        while (++current != end);
      }
      else
      {
        do
        {
          const unsigned char digit = static_cast<unsigned char>(*current) -
            static_cast<unsigned char>('0');
          if (digit > 9 || value > limit || (value == limit && digit > static_cast<unsigned char>(
                std::numeric_limits<IntegerType>::max() - limit * 10)))
          {
            return false;
          }
          value = value * static_cast<IntegerType>(10) +
            static_cast<IntegerType>(digit);
        }
        while (++current != end);
      }
      return true;
    }
  };

  struct FromCharsParser
  {
    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      if (input.empty())
      {
        return false;
      }

      const char* begin = input.data();
      const char* const end = begin + input.size();
      if (*begin == '+')
      {
        ++begin;
        if (begin == end || *begin == '+' || *begin == '-')
        {
          return false;
        }
      }

      const auto parsed = std::from_chars(begin, end, value);
      return parsed.ec == std::errc() && parsed.ptr == end;
    }
  };

  struct PaddedSwarUint32Parser
  {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    static constexpr bool SUPPORTED = true;
#elif defined(_WIN32)
    static constexpr bool SUPPORTED = true;
#else
    static constexpr bool SUPPORTED = false;
#endif

    static bool parse_eight_digits(const char* data, std::size_t size, std::uint32_t& value)
      noexcept
    {
      std::uint64_t digits = 0x3030303030303030ULL;
      std::memcpy(reinterpret_cast<char*>(&digits) + sizeof(digits) - size, data, size);

      if (((digits + 0x4646464646464646ULL) |
        (digits - 0x3030303030303030ULL)) & 0x8080808080808080ULL)
      {
        return false;
      }

      digits = (digits & 0x0f0f0f0f0f0f0f0fULL) * 2561 >> 8;
      digits = (digits & 0x00ff00ff00ff00ffULL) * 6553601 >> 16;
      digits = (digits & 0x0000ffff0000ffffULL) * 42949672960001ULL >> 32;
      value = static_cast<std::uint32_t>(digits);
      return true;
    }

    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      if constexpr (std::is_same_v<IntegerType, std::uint32_t> && SUPPORTED)
      {
        const char* current = input.data();
        const char* const end = current + input.size();
        if (current == end)
        {
          return false;
        }

        if (*current == '-')
        {
          return false;
        }

        if (*current == '+' && ++current == end)
        {
          return false;
        }

        const std::size_t digits_count = end - current;
        if (digits_count <= 8)
        {
          return parse_eight_digits(current, digits_count, value);
        }

        if (digits_count == 9)
        {
          const unsigned int first_digit =
            static_cast<unsigned char>(*current) - static_cast<unsigned char>('0');
          std::uint32_t tail = 0;
          if (first_digit > 9 || !parse_eight_digits(current + 1, 8, tail))
          {
            return false;
          }
          value = first_digit * 100'000'000U + tail;
          return true;
        }
      }

      return String::StringManip::str_to_int(input, value);
    }
  };

  struct ConstexprPaddedSwarUint32Parser
  {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    static constexpr bool SUPPORTED = true;
#elif defined(_WIN32)
    static constexpr bool SUPPORTED = true;
#else
    static constexpr bool SUPPORTED = false;
#endif

    template <std::size_t Size>
    static bool parse_digits(const char* data, std::uint32_t& value) noexcept
    {
      static_assert(Size >= 1 && Size <= 8);

      std::uint64_t digits = 0x3030303030303030ULL;
      std::memcpy(reinterpret_cast<char*>(&digits) + sizeof(digits) - Size, data, Size);

      if (((digits + 0x4646464646464646ULL) |
        (digits - 0x3030303030303030ULL)) & 0x8080808080808080ULL)
      {
        return false;
      }

      digits = (digits & 0x0f0f0f0f0f0f0f0fULL) * 2561 >> 8;
      digits = (digits & 0x00ff00ff00ff00ffULL) * 6553601 >> 16;
      digits = (digits & 0x0000ffff0000ffffULL) * 42949672960001ULL >> 32;
      value = static_cast<std::uint32_t>(digits);
      return true;
    }

    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      if constexpr (std::is_same_v<IntegerType, std::uint32_t> && SUPPORTED)
      {
        const char* current = input.data();
        const char* const end = current + input.size();
        if (current == end)
        {
          return false;
        }

        if (*current == '-')
        {
          return false;
        }

        if (*current == '+' && ++current == end)
        {
          return false;
        }

        switch (end - current)
        {
        case 1:
          return parse_digits<1>(current, value);
        case 2:
          return parse_digits<2>(current, value);
        case 3:
          return parse_digits<3>(current, value);
        case 4:
          return parse_digits<4>(current, value);
        case 5:
          return parse_digits<5>(current, value);
        case 6:
          return parse_digits<6>(current, value);
        case 7:
          return parse_digits<7>(current, value);
        case 8:
          return parse_digits<8>(current, value);
        case 9:
        {
          const unsigned int first_digit =
            static_cast<unsigned char>(*current) - static_cast<unsigned char>('0');
          std::uint32_t tail = 0;
          if (first_digit > 9 || !parse_digits<8>(current + 1, tail))
          {
            return false;
          }
          value = first_digit * 100'000'000U + tail;
          return true;
        }
        default:
          break;
        }
      }

      return String::StringManip::str_to_int(input, value);
    }
  };

  struct Uint32ChunkSwarParser
  {
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    static constexpr bool SUPPORTED = true;
#elif defined(_WIN32)
    static constexpr bool SUPPORTED = true;
#else
    static constexpr bool SUPPORTED = false;
#endif

    template <std::size_t Size>
    static bool parse_scalar(const char* data, std::uint32_t& value) noexcept
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

    template <std::size_t Size>
    static bool parse_digits(const char* data, std::uint32_t& value) noexcept
    {
      static_assert(Size >= 1 && Size <= 4);

      std::uint32_t digits = 0x30303030U;
      std::memcpy(reinterpret_cast<char*>(&digits) + sizeof(digits) - Size, data, Size);

      if (((digits + 0x46464646U) | (digits - 0x30303030U)) & 0x80808080U)
      {
        return false;
      }

      digits = (digits & 0x0f0f0f0fU) * 2561U >> 8;
      digits = (digits & 0x00ff00ffU) * 6553601U >> 16;
      value = digits;
      return true;
    }

    template <std::size_t Size>
    static bool parse_uint32(const char* data, std::uint32_t& value) noexcept
    {
      static_assert(Size >= 1 && Size <= 10);

      if constexpr (Size <= 3)
      {
        return parse_scalar<Size>(data, value);
      }
      else if constexpr (Size == 4)
      {
        return parse_digits<4>(data, value);
      }
      else if constexpr (Size <= 7)
      {
        constexpr std::size_t PrefixSize = Size - 4;
        std::uint32_t high = 0;
        std::uint32_t low = 0;
        if (!parse_scalar<PrefixSize>(data, high) || !parse_digits<4>(data + PrefixSize, low))
        {
          return false;
        }
        value = high * 10'000U + low;
        return true;
      }
      else if constexpr (Size == 8)
      {
        return ConstexprPaddedSwarUint32Parser::parse_digits<8>(data, value);
      }
      else
      {
        constexpr std::size_t PrefixSize = Size - 8;
        std::uint32_t high = 0;
        std::uint32_t low = 0;
        if (!parse_scalar<PrefixSize>(data, high) ||
          !ConstexprPaddedSwarUint32Parser::parse_digits<8>(data + PrefixSize, low))
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

    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      if constexpr (std::is_same_v<IntegerType, std::uint32_t> && SUPPORTED)
      {
        const char* current = input.data();
        const char* const end = current + input.size();
        if (current == end)
        {
          return false;
        }

        if (*current == '-')
        {
          return false;
        }

        if (*current == '+' && ++current == end)
        {
          return false;
        }

        switch (end - current)
        {
        case 1:
          return parse_uint32<1>(current, value);
        case 2:
          return parse_uint32<2>(current, value);
        case 3:
          return parse_uint32<3>(current, value);
        case 4:
          return parse_uint32<4>(current, value);
        case 5:
          return parse_uint32<5>(current, value);
        case 6:
          return parse_uint32<6>(current, value);
        case 7:
          return parse_uint32<7>(current, value);
        case 8:
          return parse_uint32<8>(current, value);
        case 9:
          return parse_uint32<9>(current, value);
        case 10:
          return parse_uint32<10>(current, value);
        default:
          break;
        }
      }

      return String::StringManip::str_to_int(input, value);
    }
  };

  struct Uint32JumpTableParser : Uint32ChunkSwarParser
  {
    using Parser = bool (*)(const char*, std::uint32_t&) noexcept;

    static constexpr std::array<Parser, 10> PARSERS = {
      &parse_uint32<1>,
      &parse_uint32<2>,
      &parse_uint32<3>,
      &parse_uint32<4>,
      &parse_uint32<5>,
      &parse_uint32<6>,
      &parse_uint32<7>,
      &parse_uint32<8>,
      &parse_uint32<9>,
      &parse_uint32<10>
    };

    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      if constexpr (std::is_same_v<IntegerType, std::uint32_t> && SUPPORTED)
      {
        const char* current = input.data();
        const char* const end = current + input.size();
        if (current == end)
        {
          return false;
        }

        if (*current == '-')
        {
          return false;
        }

        if (*current == '+' && ++current == end)
        {
          return false;
        }

        const std::size_t size = end - current;
        if (size <= PARSERS.size())
        {
          return PARSERS[size - 1](current, value);
        }
      }

      return String::StringManip::str_to_int(input, value);
    }
  };

  struct Uint32BinaryParser : Uint32ChunkSwarParser
  {
    template <typename IntegerType>
    bool operator()(std::string_view input, IntegerType& value) const noexcept
    {
      if constexpr (std::is_same_v<IntegerType, std::uint32_t> && SUPPORTED)
      {
        const char* current = input.data();
        const char* const end = current + input.size();
        if (current == end)
        {
          return false;
        }

        if (*current == '-')
        {
          return false;
        }

        if (*current == '+' && ++current == end)
        {
          return false;
        }

        const std::size_t size = end - current;
        if (size < 4)
        {
          if (size < 2)
          {
            return parse_uint32<1>(current, value);
          }

          if (size > 2)
          {
            return parse_uint32<3>(current, value);
          }
          return parse_uint32<2>(current, value);
        }

        if (size > 4)
        {
          if (size < 8)
          {
            if (size < 6)
            {
              return parse_uint32<5>(current, value);
            }

            if (size > 6)
            {
              return parse_uint32<7>(current, value);
            }
            return parse_uint32<6>(current, value);
          }

          if (size > 8)
          {
            if (size == 9)
            {
              return parse_uint32<9>(current, value);
            }

            if (size == 10)
            {
              return parse_uint32<10>(current, value);
            }
            return String::StringManip::str_to_int(input, value);
          }
          return parse_uint32<8>(current, value);
        }
        return parse_uint32<4>(current, value);
      }

      return String::StringManip::str_to_int(input, value);
    }
  };

  template <typename IntegerType, typename ParserType>
  void
  verify_value(
    const ParserType& parser,
    std::string_view input,
    bool expected_success,
    IntegerType expected_value = 0)
  {
    IntegerType value = 0;
    const bool success = parser(input, value);
    if (success != expected_success || (success && value != expected_value))
    {
      throw std::runtime_error( "unexpected result for '" + std::string(input) + "'");
    }
  }

  template <typename IntegerType, typename ParserType>
  void verify_parser(const ParserType& parser)
  {
    const IntegerType min_value = std::numeric_limits<IntegerType>::min();
    const IntegerType max_value = std::numeric_limits<IntegerType>::max();
    const std::string min_string = std::to_string(min_value);
    const std::string max_string = std::to_string(max_value);

    std::string above_max = max_string;
    for (auto it = above_max.rbegin(); it != above_max.rend(); ++it)
    {
      if (*it != '9')
      {
        ++*it;
        break;
      }
      *it = '0';
      if (it + 1 == above_max.rend())
      {
        above_max.insert(above_max.begin(), '1');
        break;
      }
    }

    verify_value(parser, "0", true, IntegerType(0));
    verify_value(parser, "+0", true, IntegerType(0));
    verify_value(parser, max_string, true, max_value);
    verify_value(parser, "+" + max_string, true, max_value);
    verify_value(parser, "0000000000000000000000000001", true, IntegerType(1));

    if constexpr (std::numeric_limits<IntegerType>::is_signed)
    {
      verify_value(parser, "-0", true, IntegerType(0));
      verify_value(parser, min_string, true, min_value);
    }
    else
    {
      verify_value<IntegerType>(parser, "-0", false);
    }

    std::vector<std::string> invalid_values = {
      "",
      "+",
      "-",
      " 1",
      "1 ",
      "1x",
      "++1",
      "--1",
      "+-1",
      "-+1",
      above_max,
      max_string + '0'
    };
    if constexpr (std::numeric_limits<IntegerType>::is_signed)
    {
      using UnsignedType = std::make_unsigned_t<IntegerType>;
      constexpr UnsignedType NEGATIVE_LIMIT =
        static_cast<UnsignedType>(std::numeric_limits<IntegerType>::max()) + 1;
      invalid_values.push_back("-" + std::to_string(NEGATIVE_LIMIT + 1));
    }
    for (const auto& invalid_value : invalid_values)
    {
      verify_value<IntegerType>(parser, invalid_value, false);
    }
  }

  template <typename ParserType>
  void verify_parser(const ParserType& parser)
  {
    verify_parser<std::uint64_t>(parser);
    verify_parser<std::int64_t>(parser);

    for (std::int32_t number = std::numeric_limits<std::int16_t>::min();
      number <= std::numeric_limits<std::int16_t>::max(); ++number)
    {
      const std::string input = std::to_string(number);
      verify_value<std::int16_t>( parser, input, true, static_cast<std::int16_t>(number));
    }

    for (std::uint32_t number = 0; number <= std::numeric_limits<std::uint16_t>::max(); ++number)
    {
      const std::string input = std::to_string(number);
      verify_value<std::uint16_t>( parser, input, true, static_cast<std::uint16_t>(number));
    }
  }

  void verify_bool_parser()
  {
    bool value = false;
    if (!String::StringManip::str_to_int(std::string_view("0"), value) || value ||
      !String::StringManip::str_to_int(std::string_view("1"), value) || !value ||
      !String::StringManip::str_to_int(std::string_view("+1"), value) || !value ||
      String::StringManip::str_to_int(std::string_view("2"), value) ||
      String::StringManip::str_to_int(std::string_view("-0"), value))
    {
      throw std::runtime_error("bool parsing verification failed");
    }
  }

  void verify_uint32_fast_path()
  {
    const StringManipParser current_parser;
    const PaddedSwarUint32Parser padded_swar_parser;
    const ConstexprPaddedSwarUint32Parser constexpr_swar_parser;
    const Uint32ChunkSwarParser uint32_chunk_swar_parser;
    const Uint32JumpTableParser jump_table_parser;
    const Uint32BinaryParser binary_parser;
    const LegacyParser reference_parser;
    verify_value<std::uint32_t>(current_parser, "1234", true, 1'234U);
    verify_value<std::uint32_t>(current_parser, "+1234", true, 1'234U);
    verify_value<std::uint32_t>(current_parser, "12345678", true, 12'345'678U);
    verify_value<std::uint32_t>(current_parser, "+12345678", true, 12'345'678U);
    verify_value<std::uint32_t>(current_parser, "123456789", true, 123'456'789U);
    verify_value<std::uint32_t>(current_parser, "+123456789", true, 123'456'789U);
    verify_value<std::uint32_t>(
      current_parser, "4294967295", true, std::numeric_limits<std::uint32_t>::max());
    verify_value<std::uint32_t>(current_parser, "4294967296", false);
    for (std::size_t prefix_size = 1; prefix_size <= 24; ++prefix_size)
    {
      std::string input(prefix_size, '0');
      input += "4294967295";
      verify_value<std::uint32_t>(
        current_parser, input, true, std::numeric_limits<std::uint32_t>::max());

      input[prefix_size - 1] = '1';
      verify_value<std::uint32_t>(current_parser, input, false);
      input[prefix_size - 1] = 'x';
      verify_value<std::uint32_t>(current_parser, input, false);
    }
    verify_value<std::uint32_t>(current_parser, "00000000004294967296", false);
    verify_value<std::uint32_t>(
      uint32_chunk_swar_parser, "4294967295", true, std::numeric_limits<std::uint32_t>::max());
    verify_value<std::uint32_t>(uint32_chunk_swar_parser, "4294967296", false);
    verify_value<std::uint32_t>(
      jump_table_parser, "4294967295", true, std::numeric_limits<std::uint32_t>::max());
    verify_value<std::uint32_t>(jump_table_parser, "4294967296", false);
    verify_value<std::uint32_t>(
      binary_parser, "4294967295", true, std::numeric_limits<std::uint32_t>::max());
    verify_value<std::uint32_t>(binary_parser, "4294967296", false);

    for (std::size_t size = 1; size <= 10; ++size)
    {
      for (std::size_t position = 0; position < size; ++position)
      {
        for (unsigned int byte = 0; byte <= 255; ++byte)
        {
          std::string input(size, '0');
          input[position] = static_cast<char>(byte);

          std::uint32_t current_value = 0;
          std::uint32_t padded_swar_value = 0;
          std::uint32_t constexpr_swar_value = 0;
          std::uint32_t uint32_chunk_swar_value = 0;
          std::uint32_t jump_table_value = 0;
          std::uint32_t binary_value = 0;
          std::uint32_t reference_value = 0;
          const bool current_success = current_parser(input, current_value);
          const bool padded_swar_success = padded_swar_parser(input, padded_swar_value);
          const bool constexpr_swar_success = constexpr_swar_parser(input, constexpr_swar_value);
          const bool uint32_chunk_swar_success =
            uint32_chunk_swar_parser(input, uint32_chunk_swar_value);
          const bool jump_table_success = jump_table_parser(input, jump_table_value);
          const bool binary_success = binary_parser(input, binary_value);
          const bool reference_success = reference_parser(input, reference_value);
          if (current_success != reference_success ||
            (current_success && current_value != reference_value) ||
            padded_swar_success != reference_success ||
            (padded_swar_success && padded_swar_value != reference_value) ||
            constexpr_swar_success != reference_success ||
            (constexpr_swar_success && constexpr_swar_value != reference_value) ||
            uint32_chunk_swar_success != reference_success ||
            (uint32_chunk_swar_success && uint32_chunk_swar_value != reference_value) ||
            jump_table_success != reference_success ||
            (jump_table_success && jump_table_value != reference_value) ||
            binary_success != reference_success ||
            (binary_success && binary_value != reference_value))
          {
            throw std::runtime_error("padded SWAR uint32 verification failed");
          }
        }
      }
    }
  }

  template <typename IntegerType>
  void verify_equivalence()
  {
    constexpr char ALPHABET[] = "0123456789+-x ";
    constexpr std::size_t CASE_COUNT = 100000;

    std::uint64_t state = 0xa0761d6478bd642fULL;
    for (std::size_t index = 0; index < CASE_COUNT; ++index)
    {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      const std::size_t size = state % 32;
      std::string input(size, '0');
      for (char& ch : input)
      {
        state = state * 2862933555777941757ULL + 3037000493ULL;
        ch = ALPHABET[state % (sizeof(ALPHABET) - 1)];
      }

      IntegerType legacy_value = 0;
      IntegerType current_value = 0;
      const bool legacy_success = LegacyParser()(input, legacy_value);
      const bool current_success = StringManipParser()(input, current_value);
      if (legacy_success != current_success || (legacy_success && legacy_value != current_value))
      {
        throw std::runtime_error( "legacy equivalence failed for '" + input + "'");
      }
    }
  }

  std::vector<std::string> make_small_unsigned_values(std::size_t count)
  {
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0x9e3779b97f4a7c15ULL;
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      values.push_back(std::to_string(state % 1000));
    }
    return values;
  }

  std::vector<std::string> make_uint32_values(std::size_t count, std::size_t digits)
  {
    if (digits == 0 || digits > 10)
    {
      throw std::invalid_argument("uint32 digit count must be in 1..10");
    }

    std::uint64_t min_value = 0;
    std::uint64_t max_value = 9;
    for (std::size_t current_digits = 2; current_digits <= digits; ++current_digits)
    {
      min_value = max_value + 1;
      max_value = max_value * 10 + 9;
    }

    if (max_value > std::numeric_limits<std::uint32_t>::max())
    {
      max_value = std::numeric_limits<std::uint32_t>::max();
    }

    const std::uint64_t range = max_value - min_value + 1;
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0x94d049bb133111ebULL ^ (digits * 0x9e3779b97f4a7c15ULL);
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      values.push_back(std::to_string(min_value + state % range));
    }
    return values;
  }

  std::vector<std::string> make_long_uint32_values(std::size_t count)
  {
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0xd1b54a32d192ed03ULL;
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      const std::size_t prefix_size = 16 + state % 49;
      const std::uint32_t value = static_cast<std::uint32_t>(state >> 32);
      values.push_back(std::string(prefix_size, '0') + std::to_string(value));
    }
    return values;
  }

  std::vector<std::string> make_rbc_unsigned_values(std::size_t count)
  {
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0x94d049bb133111ebULL;
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      const std::uint64_t value = 1'000'000 + state % 100'000'000;
      values.push_back(std::to_string(value));
    }
    return values;
  }

  std::vector<std::string> make_full_unsigned_values(std::size_t count)
  {
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0xbf58476d1ce4e5b9ULL;
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 2862933555777941757ULL + 3037000493ULL;
      values.push_back(std::to_string(state));
    }

    if (!values.empty())
    {
      values.front() = std::to_string(std::numeric_limits<std::uint64_t>::max());
    }
    return values;
  }

  std::vector<std::string> make_mixed_signed_values(std::size_t count)
  {
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0xd1b54a32d192ed03ULL;
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 2862933555777941757ULL + 3037000493ULL;
      std::int64_t value = static_cast<std::int64_t>(state % 200'000'001) -
        100'000'000;
      if (index == 0)
      {
        value = std::numeric_limits<std::int64_t>::min();
      }
      else if (index == 1)
      {
        value = std::numeric_limits<std::int64_t>::max();
      }
      values.push_back(std::to_string(value));
    }
    return values;
  }

  std::vector<std::string> make_full_signed_values(std::size_t count)
  {
    std::vector<std::string> values;
    values.reserve(count);
    std::uint64_t state = 0xdb4f0b9175ae2165ULL;
    for (std::size_t index = 0; index < count; ++index)
    {
      state = state * 6364136223846793005ULL + 1442695040888963407ULL;
      const std::int64_t magnitude = static_cast<std::int64_t>(
        state & static_cast<std::uint64_t>(
          std::numeric_limits<std::int64_t>::max()));
      const std::int64_t value = state >> 63 ? -magnitude : magnitude;
      values.push_back(std::to_string(value));
    }

    if (!values.empty())
    {
      values.front() = std::to_string(std::numeric_limits<std::int64_t>::min());
    }
    return values;
  }

  struct Measurement
  {
    double seconds;
    std::uint64_t checksum;
  };

  template <typename IntegerType, typename ParserType>
  Measurement
  measure( const ParserType& parser, const std::vector<std::string>& inputs, std::size_t iterations)
  {
    using UnsignedType = std::make_unsigned_t<IntegerType>;

    std::uint64_t checksum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration)
    {
      for (const auto& input : inputs)
      {
        IntegerType value = 0;
        if (!parser(input, value))
        {
          throw std::runtime_error("benchmark parser rejected a valid value");
        }
        checksum += static_cast<UnsignedType>(value);
      }
    }
    const double seconds = std::chrono::duration<double>(
      std::chrono::steady_clock::now() - start).count();
    return {seconds, checksum};
  }

  template <typename IntegerType, typename ParserType>
  std::uint64_t
  print_measurement(
    std::string_view name,
    const ParserType& parser,
    const std::vector<std::string>& inputs,
    std::size_t iterations,
    std::uint64_t expected_checksum)
  {
    const Measurement result = measure<IntegerType>(parser, inputs, iterations);
    if (expected_checksum != 0 && result.checksum != expected_checksum)
    {
      throw std::runtime_error("benchmark checksum mismatch");
    }

    const double count = static_cast<double>(inputs.size()) * static_cast<double>(iterations);
    std::cout << "  " << std::left << std::setw(28) << name << std::right
      << std::fixed << std::setprecision(3)
      << std::setw(10) << result.seconds * 1e9 / count << " ns/value"
      << std::setw(12) << count / result.seconds / 1e6 << " M/s\n";
    return result.checksum;
  }

  template <typename IntegerType>
  void
  run_dataset(std::string_view name, const std::vector<std::string>& inputs,
    std::size_t iterations, bool include_uint32_candidates = true)
  {
    static_cast<void>(include_uint32_candidates);

    std::cout << name << ": values=" << inputs.size() << ", iterations=" << iterations << '\n';

    const std::uint64_t checksum = print_measurement<IntegerType>(
      "legacy str_to_int",
      LegacyParser(),
      inputs,
      iterations,
      0);
    print_measurement<IntegerType>(
      "StringManip::str_to_int",
      StringManipParser(),
      inputs,
      iterations,
      checksum);
    if constexpr (std::is_same_v<IntegerType, std::uint32_t>)
    {
      if (include_uint32_candidates)
      {
        print_measurement<IntegerType>(
          "runtime memcpy SWAR", PaddedSwarUint32Parser(), inputs, iterations, checksum);
        print_measurement<IntegerType>(
          "constexpr memcpy SWAR", ConstexprPaddedSwarUint32Parser(), inputs, iterations, checksum);
        print_measurement<IntegerType>(
          "composed/switch", Uint32ChunkSwarParser(), inputs, iterations, checksum);
        print_measurement<IntegerType>(
          "composed/jump table", Uint32JumpTableParser(), inputs, iterations, checksum);
        print_measurement<IntegerType>(
          "composed/binary", Uint32BinaryParser(), inputs, iterations, checksum);
      }
    }
    print_measurement<IntegerType>(
      "std::from_chars",
      FromCharsParser(),
      inputs,
      iterations,
      checksum);
  }
}

int main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);

    verify_parser(LegacyParser());
    verify_parser(StringManipParser());
    verify_parser(PaddedSwarUint32Parser());
    verify_parser(ConstexprPaddedSwarUint32Parser());
    verify_parser(Uint32ChunkSwarParser());
    verify_parser(FromCharsParser());
    verify_bool_parser();
    verify_uint32_fast_path();
    verify_equivalence<std::uint8_t>();
    verify_equivalence<std::int8_t>();
    verify_equivalence<std::uint32_t>();
    verify_equivalence<std::int32_t>();
    verify_equivalence<std::uint64_t>();
    verify_equivalence<std::int64_t>();

    run_dataset<std::uint32_t>(
      "unsigned/small (1-3 digits)",
      make_small_unsigned_values(options.values),
      options.iterations);
    run_dataset<std::uint32_t>(
      "unsigned/RBC-like (7-9 digits)",
      make_rbc_unsigned_values(options.values),
      options.iterations);
    for (std::size_t digits = 1; digits <= 10; ++digits)
    {
      const std::string name = "unsigned/uint32 (" + std::to_string(digits) + " digits)";
      run_dataset<std::uint32_t>(
        name, make_uint32_values(options.values, digits), options.iterations);
    }
    run_dataset<std::uint32_t>(
      "unsigned/uint32 (long leading zeros)", make_long_uint32_values(options.values),
      options.iterations, false);
    run_dataset<std::uint64_t>(
      "unsigned/full-width",
      make_full_unsigned_values(options.values),
      options.iterations);
    run_dataset<std::int64_t>(
      "signed/mixed",
      make_mixed_signed_values(options.values),
      options.iterations);
    run_dataset<std::int64_t>(
      "signed/full-width",
      make_full_signed_values(options.values),
      options.iterations);
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "StringManipStrToIntPerfTest: " << error.what() << '\n';
  }
  return 1;
}
