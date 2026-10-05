#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <vector>

#include <Generics/HashTableAdapters.hpp>

namespace
{
  volatile std::size_t sink;

  struct Murmur
  {
    std::size_t operator()(const char* data, std::size_t size) const noexcept
    {
      std::size_t result;
      {
        Generics::Murmur64Hash hash(result);
        Generics::hash_add(hash, String::SubString(data, size));
      }
      return result;
    }
  };

  struct Fast
  {
    std::size_t operator()(const char* data, std::size_t size) const noexcept
    {
      return Generics::FastHash::hash(data, size);
    }
  };

  struct SubString
  {
    std::size_t operator()(const char* data, std::size_t size) const noexcept
    {
      return Generics::SubStringHashAdapter(String::SubString(data, size)).hash();
    }
  };

  struct View
  {
    std::size_t operator()(const char* data, std::size_t size) const noexcept
    {
      return Generics::StringViewHashAdapter(std::string_view(data, size)).hash();
    }
  };

  template <typename Hash>
  __attribute__((noinline))
  double measure(const char* data, std::size_t size, const std::vector<std::size_t>& sizes)
  {
    constexpr std::size_t count = 1000000;
    std::size_t checksum = 0;
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < count; ++i)
    {
      const auto length = sizes.empty() ? size : sizes[i & (sizes.size() - 1)];
      checksum += Hash{}(data + (i & 63) * 4160, length);
    }
    const auto end = std::chrono::steady_clock::now();
    sink = checksum;
    return std::chrono::duration<double, std::nano>(end - start).count() / count;
  }

  void run(const char* name, const char* data, std::size_t size,
    const std::vector<std::size_t>& sizes = {})
  {
    using Measure = double (*)(const char*, std::size_t, const std::vector<std::size_t>&);
    const std::array<Measure, 4> measures = {
      measure<Murmur>, measure<Fast>, measure<SubString>, measure<View>};
    std::array<std::array<double, 7>, 4> samples;
    for (auto function : measures)
    {
      function(data, size, sizes);
    }
    for (std::size_t round = 0; round < 7; ++round)
    {
      for (std::size_t index = 0; index < measures.size(); ++index)
      {
        const auto variant = (round + index) % measures.size();
        samples[variant][round] = measures[variant](data, size, sizes);
      }
    }
    for (auto& sample : samples)
    {
      std::sort(sample.begin(), sample.end());
    }
    std::printf("%s,%zu,%.3f,%.3f,%.3f,%.3f\n", name, size, samples[0][3], samples[1][3],
      samples[2][3], samples[3][3]);
    std::fflush(stdout);
  }
}

int main()
{
  std::vector<char> input(4160 * 64);
  std::uint64_t random = 1;
  for (auto& byte : input)
  {
    random ^= random << 13;
    random ^= random >> 7;
    random ^= random << 17;
    byte = static_cast<char>(random);
  }
  std::puts("mode,bytes,murmur_inline_ns,fast_inline_ns,substring_ns,stringview_ns");
  for (const std::size_t size : {4, 8, 16, 24, 48, 49, 64, 128, 256, 512, 1024, 4096})
  {
    run("fixed", input.data(), size);
  }

  for (const std::size_t max_size : {48, 128, 512})
  {
    std::vector<std::size_t> sizes(4096);
    for (auto& size : sizes)
    {
      random ^= random << 13;
      random ^= random >> 7;
      random ^= random << 17;
      size = 1 + random % max_size;
    }
    run("mixed", input.data(), max_size, sizes);
  }
}
