#include <sys/resource.h>
#include <time.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <String/AsciiStringManip.hpp>
#include <String/StringManip.hpp>
#include <String/StringManipJsonEscape.hpp>

namespace
{
  namespace JsonEscape = String::StringManip::JsonEscape;

  struct CpuUsage
  {
    double user = 0.0;
    double sys = 0.0;
  };

  struct Measurement
  {
    double wall = 0.0;
    double user_cpu = 0.0;
    double sys_cpu = 0.0;
    uint64_t checksum = 0;
  };

  struct Options
  {
    uint64_t count = 1000000;
    size_t size = 1024;
    size_t input_count = 1024;
    JsonEscape::SimdLevel simd_level = JsonEscape::SimdLevel::AUTO;
  };

  JsonEscape::SimdLevel selected_simd_level = JsonEscape::SimdLevel::AUTO;

  CpuUsage get_cpu_usage()
  {
    rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) != 0)
    {
      throw std::runtime_error("getrusage failed");
    }

    return CpuUsage{
      static_cast<double>(usage.ru_utime.tv_sec) +
        static_cast<double>(usage.ru_utime.tv_usec) / 1000000.0,
      static_cast<double>(usage.ru_stime.tv_sec) +
        static_cast<double>(usage.ru_stime.tv_usec) / 1000000.0};
  }

  double get_wall_time()
  {
    timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
      throw std::runtime_error("clock_gettime failed");
    }

    return static_cast<double>(ts.tv_sec) +
      static_cast<double>(ts.tv_nsec) / 1000000000.0;
  }

  uint64_t parse_uint(const char* value, const char* option)
  {
    char* end = 0;
    const unsigned long long result = std::strtoull(value, &end, 10);
    if (end == value || *end != '\0')
    {
      throw std::runtime_error(std::string("invalid ") + option + ": " + value);
    }

    return result;
  }

  JsonEscape::SimdLevel parse_simd_level(const char* value)
  {
    if (std::strcmp(value, "auto") == 0)
    {
      return JsonEscape::SimdLevel::AUTO;
    }
    else if (std::strcmp(value, "scalar") == 0)
    {
      return JsonEscape::SimdLevel::SCALAR;
    }
    else if (std::strcmp(value, "sse2") == 0)
    {
      return JsonEscape::SimdLevel::SSE2;
    }
    else if (std::strcmp(value, "avx2") == 0)
    {
      return JsonEscape::SimdLevel::AVX2;
    }
    else if (std::strcmp(value, "avx512") == 0 || std::strcmp(value, "avx512bw") == 0)
    {
      return JsonEscape::SimdLevel::AVX512BW;
    }

    throw std::runtime_error(std::string("invalid --simd: ") + value);
  }

  Options parse_options(int argc, char* argv[])
  {
    Options options;

    for (int i = 1; i < argc; ++i)
    {
      const char* const arg = argv[i];

      if (std::strcmp(arg, "--help") == 0)
      {
        std::cout << "Usage: " << argv[0] << " [--count N] [--size N]"
          << " [--input-count N]" << " [--simd auto|scalar|sse2|avx2|avx512bw]" << std::endl;
        std::exit(0);
      }
      else if (std::strncmp(arg, "--count=", 8) == 0)
      {
        options.count = parse_uint(arg + 8, "--count");
      }
      else if (std::strcmp(arg, "--count") == 0 && i + 1 < argc)
      {
        options.count = parse_uint(argv[++i], "--count");
      }
      else if (std::strncmp(arg, "--size=", 7) == 0)
      {
        options.size = parse_uint(arg + 7, "--size");
      }
      else if (std::strcmp(arg, "--size") == 0 && i + 1 < argc)
      {
        options.size = parse_uint(argv[++i], "--size");
      }
      else if (std::strncmp(arg, "--input-count=", 14) == 0)
      {
        options.input_count = parse_uint(arg + 14, "--input-count");
      }
      else if (std::strcmp(arg, "--input-count") == 0 && i + 1 < argc)
      {
        options.input_count = parse_uint(argv[++i], "--input-count");
      }
      else if (std::strncmp(arg, "--simd=", 7) == 0)
      {
        options.simd_level = parse_simd_level(arg + 7);
      }
      else if (std::strcmp(arg, "--simd") == 0 && i + 1 < argc)
      {
        options.simd_level = parse_simd_level(argv[++i]);
      }
      else
      {
        throw std::runtime_error(std::string("unknown option: ") + arg);
      }
    }

    if (options.count == 0)
    {
      throw std::runtime_error("--count must be greater than 0");
    }

    if (options.size == 0)
    {
      throw std::runtime_error("--size must be greater than 0");
    }

    if (options.input_count == 0)
    {
      throw std::runtime_error("--input-count must be greater than 0");
    }

    if (!JsonEscape::simd_level_available(options.simd_level))
    {
      throw std::runtime_error(
        std::string("requested json_escape SIMD level '") +
        JsonEscape::simd_level_name(options.simd_level) +
        "' isn't available, default level is '" +
        JsonEscape::simd_level_name(JsonEscape::default_simd_level()) +
        "'");
    }

    return options;
  }

  std::string format_float(double value)
  {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << value;
    return out.str();
  }

  std::string make_base_input(size_t size, size_t index)
  {
    std::string input;
    input.reserve(size);

    static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "0123456789"
      ".:/?&=_-";

    for (size_t i = 0; i < size; ++i)
    {
      input.push_back(alphabet[(i + index * 17) % (sizeof(alphabet) - 1)]);
    }

    return input;
  }

  std::vector<std::string> make_inputs(const char* scenario, const Options& options)
  {
    std::vector<std::string> inputs;
    inputs.reserve(options.input_count);

    for (size_t index = 0; index < options.input_count; ++index)
    {
      std::string input = make_base_input(options.size, index);

      if (std::strcmp(scenario, "sparse") == 0)
      {
        if (!input.empty())
        {
          input[(index * 37) % input.size()] = '"';
        }

        if (input.size() > 128)
        {
          input[(index * 53 + 127) % input.size()] = '\\';
        }
      }
      else if (std::strcmp(scenario, "dense") == 0)
      {
        static const char escaped[] = {'"', '\\', '\n', '\r', '\t', '\b'};
        for (size_t i = 7; i < input.size(); i += 16)
        {
          input[i] = escaped[(i + index) % (sizeof(escaped) / sizeof(*escaped))];
        }
      }
      else if (std::strcmp(scenario, "utf8") == 0)
      {
        static const char utf8[] = "\xD1\x80\xD1\x83\xD1\x82\xD1\x83\xD0\xB1\xD0\xB5.";
        for (size_t i = 0; i < input.size(); i += 64)
        {
          const size_t copy_size = std::min(sizeof(utf8) - 1, input.size() - i);
          std::memcpy(&input[i], utf8, copy_size);
        }
      }
      else if (std::strcmp(scenario, "clean") != 0)
      {
        throw std::runtime_error(std::string("unknown scenario: ") + scenario);
      }

      inputs.push_back(std::move(input));
    }

    return inputs;
  }

  void old_json_escape_append(std::string& dest, const String::SubString& src)
  {
    static const String::AsciiStringManip::CharCategory NON_JSON( "\\\"\n\r\x01-\x1F", true);

    static const String::SubString REPL[] = {
      String::SubString("\\u0000", 6),
      String::SubString("\\u0001", 6),
      String::SubString("\\u0002", 6),
      String::SubString("\\u0003", 6),
      String::SubString("\\u0004", 6),
      String::SubString("\\u0005", 6),
      String::SubString("\\u0006", 6),
      String::SubString("\\u0007", 6),
      String::SubString("\\b", 2),
      String::SubString("\\t", 2),
      String::SubString("\\n", 2),
      String::SubString("\\u000B", 6),
      String::SubString("\\f", 2),
      String::SubString("\\r", 2),
      String::SubString("\\u000E", 6),
      String::SubString("\\u000F", 6),
      String::SubString("\\u0010", 6),
      String::SubString("\\u0011", 6),
      String::SubString("\\u0012", 6),
      String::SubString("\\u0013", 6),
      String::SubString("\\u0014", 6),
      String::SubString("\\u0015", 6),
      String::SubString("\\u0016", 6),
      String::SubString("\\u0017", 6),
      String::SubString("\\u0018", 6),
      String::SubString("\\u0019", 6),
      String::SubString("\\u001A", 6),
      String::SubString("\\u001B", 6),
      String::SubString("\\u001C", 6),
      String::SubString("\\u001D", 6),
      String::SubString("\\u001E", 6),
      String::SubString("\\u001F", 6),
      String::SubString(),
      String::SubString(),
      String::SubString("\\\"", 2)
    };

    const char* cur = src.begin();
    const char* const END = src.end();

    for (;;)
    {
      const char* ptr = NON_JSON.find_owned(cur, END);

      if (ptr != cur)
      {
        dest.append(cur, ptr);
      }

      if (ptr == END)
      {
        break;
      }

      cur = ptr + 1;

      char ch = *ptr;

      if (ch == '\\')
      {
        dest.append("\\\\", 2);
      }
      else
      {
        REPL[static_cast<uint8_t>(ch)].append_to(dest);
      }
    }
  }

  void new_json_escape_append(std::string& dest, const String::SubString& src)
  {
    JsonEscape::json_escape_append(dest, src, selected_simd_level);
  }

  using EscapeFunction = void (*)(std::string&, const String::SubString&);

  uint64_t
  run_loop(
    EscapeFunction escape,
    const std::vector<std::string>& inputs,
    uint64_t count,
    std::string& dest)
  {
    uint64_t checksum = 0;

    for (uint64_t i = 0; i < count; ++i)
    {
      const std::string& input = inputs[i % inputs.size()];
      dest.clear();
      escape(dest, String::SubString(input.data(), input.size()));
      checksum += dest.size();
      if (!dest.empty())
      {
        checksum += static_cast<unsigned char>(dest[0]);
        checksum += static_cast<unsigned char>(dest[dest.size() - 1]);
      }
    }

    return checksum;
  }

  Measurement
  measure( EscapeFunction escape, const std::vector<std::string>& inputs, const Options& options)
  {
    std::string dest;
    dest.reserve(options.size * 6);

    run_loop(escape, inputs, std::min<uint64_t>(options.count, 10000), dest);

    const CpuUsage cpu_start = get_cpu_usage();
    const double wall_start = get_wall_time();

    const uint64_t checksum = run_loop(escape, inputs, options.count, dest);

    const double wall_finish = get_wall_time();
    const CpuUsage cpu_finish = get_cpu_usage();

    return Measurement{
      wall_finish - wall_start,
      cpu_finish.user - cpu_start.user,
      cpu_finish.sys - cpu_start.sys,
      checksum};
  }

  void verify(const std::vector<std::string>& inputs)
  {
    std::string old_result;
    std::string new_result;

    for (const auto& input : inputs)
    {
      old_result.clear();
      new_result.clear();

      old_json_escape_append( old_result, String::SubString(input.data(), input.size()));
      new_json_escape_append( new_result, String::SubString(input.data(), input.size()));

      if (old_result != new_result)
      {
        throw std::runtime_error("old and new json escape results differ");
      }
    }
  }

  void print_measurement(const char* name, const Measurement& result, uint64_t count)
  {
    const double cpu = result.user_cpu + result.sys_cpu;

    std::cout << name << ":\n" << "  wall_sec=" << format_float(result.wall) << '\n'
      << "  cpu_sec=" << format_float(cpu) << '\n'
      << "  user_cpu_sec=" << format_float(result.user_cpu) << '\n'
      << "  sys_cpu_sec=" << format_float(result.sys_cpu) << '\n' << "  ns_per_call_cpu="
      << format_float(cpu * 1000000000.0 / static_cast<double>(count)) << '\n'
      << "  checksum=" << result.checksum << '\n';
  }

  void run_scenario(const char* scenario, const Options& options)
  {
    const std::vector<std::string> inputs = make_inputs(scenario, options);
    verify(inputs);

    const Measurement old_result = measure(old_json_escape_append, inputs, options);
      const Measurement new_result = measure(new_json_escape_append, inputs, options);

    const double old_cpu = old_result.user_cpu + old_result.sys_cpu;
    const double new_cpu = new_result.user_cpu + new_result.sys_cpu;

    std::cout << '\n' << scenario << ":\n";
    print_measurement("old", old_result, options.count);
    print_measurement("new", new_result, options.count);
    std::cout << "new_to_old_cpu_ratio=" << format_float(new_cpu / old_cpu) << '\n'
      << "old_to_new_cpu_ratio=" << format_float(old_cpu / new_cpu) << '\n';
  }
}

int main(int argc, char* argv[])
{
  try
  {
    const Options options = parse_options(argc, argv);
    selected_simd_level = options.simd_level;

    const JsonEscape::SimdLevel effective_simd_level =
      options.simd_level == JsonEscape::SimdLevel::AUTO ?
        JsonEscape::default_simd_level() :
        options.simd_level;

    std::cout << "count=" << options.count << '\n'
      << "size=" << options.size << '\n' << "input_count=" << options.input_count << '\n'
      << "simd=" << JsonEscape::simd_level_name(effective_simd_level) << '\n' << "default_simd="
      << JsonEscape::simd_level_name(JsonEscape::default_simd_level()) << '\n';

    run_scenario("clean", options);
    run_scenario("sparse", options);
    run_scenario("dense", options);
    run_scenario("utf8", options);
  }
  catch(const std::exception& ex)
  {
    std::cerr << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
