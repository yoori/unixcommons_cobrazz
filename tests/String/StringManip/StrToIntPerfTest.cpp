#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

  std::size_t
  parse_size(std::string_view value, std::string_view option)
  {
    std::size_t result = 0;
    const auto parsed = std::from_chars(
      value.data(),
      value.data() + value.size(),
      result);
    if (parsed.ec != std::errc() ||
      parsed.ptr != value.data() + value.size() ||
      result == 0)
    {
      throw std::invalid_argument(
        std::string(option) + " requires a positive integer");
    }
    return result;
  }

  Options
  parse_options(int argc, char** argv)
  {
    Options options;
    for (int index = 1; index < argc; ++index)
    {
      const std::string_view argument(argv[index]);
      if (argument == "--help")
      {
        std::cout
          << "Usage: " << argv[0]
          << " [--iterations N] [--values N]\n";
        std::exit(0);
      }

      if (argument != "--iterations" && argument != "--values")
      {
        throw std::invalid_argument(
          "unknown option: " + std::string(argument));
      }
      if (++index == argc)
      {
        throw std::invalid_argument(
          std::string(argument) + " requires a value");
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
    bool
    operator()(std::string_view input, IntegerType& value) const noexcept
    {
      return String::StringManip::str_to_int(input, value);
    }
  };

  struct LegacyParser
  {
    template <typename IntegerType>
    bool
    operator()(std::string_view input, IntegerType& value) const noexcept
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
          const unsigned char digit =
            static_cast<unsigned char>(*current) -
            static_cast<unsigned char>('0');
          if (digit > 9 ||
            value < -limit ||
            (value == -limit &&
              digit > static_cast<unsigned char>(
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
          const unsigned char digit =
            static_cast<unsigned char>(*current) -
            static_cast<unsigned char>('0');
          if (digit > 9 ||
            value > limit ||
            (value == limit &&
              digit > static_cast<unsigned char>(
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
    bool
    operator()(std::string_view input, IntegerType& value) const noexcept
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
    if (success != expected_success ||
      (success && value != expected_value))
    {
      throw std::runtime_error(
        "unexpected result for '" + std::string(input) + "'");
    }
  }

  template <typename IntegerType, typename ParserType>
  void
  verify_parser(const ParserType& parser)
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

    std::vector<std::string> invalid_values =
    {
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
  void
  verify_parser(const ParserType& parser)
  {
    verify_parser<std::uint64_t>(parser);
    verify_parser<std::int64_t>(parser);

    for (std::int32_t number = std::numeric_limits<std::int16_t>::min();
      number <= std::numeric_limits<std::int16_t>::max(); ++number)
    {
      const std::string input = std::to_string(number);
      verify_value<std::int16_t>(
        parser,
        input,
        true,
        static_cast<std::int16_t>(number));
    }

    for (std::uint32_t number = 0;
      number <= std::numeric_limits<std::uint16_t>::max(); ++number)
    {
      const std::string input = std::to_string(number);
      verify_value<std::uint16_t>(
        parser,
        input,
        true,
        static_cast<std::uint16_t>(number));
    }
  }

  void
  verify_bool_parser()
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

  template <typename IntegerType>
  void
  verify_equivalence()
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
      if (legacy_success != current_success ||
        (legacy_success && legacy_value != current_value))
      {
        throw std::runtime_error(
          "legacy equivalence failed for '" + input + "'");
      }
    }
  }

  std::vector<std::string>
  make_small_unsigned_values(std::size_t count)
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

  std::vector<std::string>
  make_rbc_unsigned_values(std::size_t count)
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

  std::vector<std::string>
  make_full_unsigned_values(std::size_t count)
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

  std::vector<std::string>
  make_mixed_signed_values(std::size_t count)
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

  std::vector<std::string>
  make_full_signed_values(std::size_t count)
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
  measure(
    const ParserType& parser,
    const std::vector<std::string>& inputs,
    std::size_t iterations)
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

    const double count =
      static_cast<double>(inputs.size()) * static_cast<double>(iterations);
    std::cout
      << "  " << std::left << std::setw(28) << name << std::right
      << std::fixed << std::setprecision(3)
      << std::setw(10) << result.seconds * 1e9 / count << " ns/value"
      << std::setw(12) << count / result.seconds / 1e6 << " M/s\n";
    return result.checksum;
  }

  template <typename IntegerType>
  void
  run_dataset(
    std::string_view name,
    const std::vector<std::string>& inputs,
    std::size_t iterations)
  {
    std::cout
      << name << ": values=" << inputs.size()
      << ", iterations=" << iterations << '\n';

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
    print_measurement<IntegerType>(
      "std::from_chars",
      FromCharsParser(),
      inputs,
      iterations,
      checksum);
  }
}

int
main(int argc, char** argv)
{
  try
  {
    const Options options = parse_options(argc, argv);

    verify_parser(LegacyParser());
    verify_parser(StringManipParser());
    verify_parser(FromCharsParser());
    verify_bool_parser();
    verify_equivalence<std::uint8_t>();
    verify_equivalence<std::int8_t>();
    verify_equivalence<std::uint32_t>();
    verify_equivalence<std::int32_t>();
    verify_equivalence<std::uint64_t>();
    verify_equivalence<std::int64_t>();

    run_dataset<std::uint64_t>(
      "unsigned/small (1-3 digits)",
      make_small_unsigned_values(options.values),
      options.iterations);
    run_dataset<std::uint64_t>(
      "unsigned/RBC-like (7-9 digits)",
      make_rbc_unsigned_values(options.values),
      options.iterations);
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
