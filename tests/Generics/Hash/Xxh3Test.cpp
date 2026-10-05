#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#define XXH_INLINE_ALL
#define XXH_VECTOR XXH_SCALAR
#include <Generics/third_party/xxhash/xxhash.h>

#include <Generics/Hash.hpp>
#include <Generics/HashXxh3Backend.hpp>
#include <Generics/SimdCapabilities.hpp>

namespace
{
  void check(std::size_t actual, std::size_t expected, const char* what, std::size_t len = 0)
  {
    if (actual != expected)
    {
      std::cerr << what << " at length " << len << ": " << std::hex << actual
        << " != " << expected << std::endl;
      std::exit(1);
    }
  }

  void check_backend(const Generics::HashHelper::Xxh3Backend& backend, const void* data,
    std::size_t len, std::uint64_t seed, std::size_t expected)
  {
    check(backend.hash(data, len, seed), expected, "backend one-shot", len);
    XXH3_state_t state;
    XXH3_INITSTATE(&state);
    XXH3_64bits_reset_withSeed(&state, seed);
    const auto* bytes = static_cast<const char*>(data);
    const std::size_t split = len / 3;
    backend.update(&state, data, split);
    backend.update(&state, bytes + split, len - split);
    check(XXH3_64bits_digest(&state), expected, "backend streaming", len);
  }
}

int main()
{
  using Generics::XXH3Hasher;
  check(XXH3Hasher::hash(nullptr, 0), 0x2d06800538d394c2ULL, "empty vector");
  check(XXH3Hasher::hash("abc", 3), 0x78af5f94892f3950ULL, "abc vector");

  std::array<char, 8201> input;
  std::uint64_t random = 1;
  for (auto& byte : input)
  {
    random ^= random << 13;
    random ^= random >> 7;
    random ^= random << 17;
    byte = static_cast<char>(random);
  }

  std::vector<std::size_t> lengths;
  for (std::size_t len = 0; len <= 1025; ++len)
  {
    lengths.push_back(len);
  }
  for (const std::size_t len : {2047, 2048, 2049, 4095, 4096, 4097, 8191, 8192, 8193})
  {
    lengths.push_back(len);
  }

  for (const auto seed : {0ULL, 1ULL, 0x123456789abcdef0ULL, 0xffffffffffffffffULL})
  {
    XXH3Hasher empty(seed);
    empty.add(nullptr, 0);
    check(empty.finalize(), XXH3_64bits_withSeed(nullptr, 0, seed), "empty streaming");
    for (std::size_t offset = 0; offset < 8; ++offset)
    {
      const auto* data = input.data() + offset;
      for (const auto len : lengths)
      {
        const auto expected = XXH3_64bits_withSeed(data, len, seed);
        check(XXH3Hasher::hash(data, len, seed), expected, "one-shot", len);
        check_backend(Generics::HashHelper::XXH3_BASELINE, data, len, seed, expected);
#ifdef GENERICS_XXH3_HAS_AVX2
        if (Generics::Simd::has(Generics::Simd::AVX2))
        {
          check_backend(Generics::HashHelper::XXH3_AVX2, data, len, seed, expected);
        }
#endif
#ifdef GENERICS_XXH3_HAS_AVX512
        if (Generics::Simd::has(Generics::Simd::AVX512F | Generics::Simd::AVX512BW))
        {
          check_backend(Generics::HashHelper::XXH3_AVX512, data, len, seed, expected);
        }
#endif
        for (const std::size_t step : {1, 7, 64, 239, 256, 1024})
        {
          XXH3Hasher hasher(seed);
          for (std::size_t pos = 0; pos < len; pos += step)
          {
            hasher.add(data + pos, std::min(step, len - pos));
          }
          check(hasher.finalize(), expected, "streaming", len);
        }

        const auto split = len / 2;
        XXH3Hasher prefix(seed);
        prefix.add(data, split);
        check(prefix.finalize(), XXH3_64bits_withSeed(data, split, seed), "prefix", len);
        XXH3Hasher copied(prefix);
        XXH3Hasher assigned(seed + 1);
        assigned = prefix;
        assigned = assigned;
        prefix.add("x", 1);
        copied.add(data + split, len - split);
        assigned.add(data + split, len - split);
        check(copied.finalize(), expected, "copy", len);
        check(assigned.finalize(), expected, "assignment", len);
        check(copied.finalize(), expected, "repeat digest", len);

        std::size_t adapted = 0;
        {
          Generics::XXH3Hash hash(adapted, seed);
          hash.add(data, split);
          hash.add(data + split, len - split);
        }
        check(adapted, expected, "RAII adapter", len);
      }
    }
  }

  const std::string first = "http://test.me/";
  const std::string second = "test1/test2";
  XXH3Hasher strings;
  Generics::hash_add(strings, first);
  Generics::hash_add(strings, second);
  const auto joined = first + second;
  check(strings.finalize(), XXH3Hasher::hash(joined.data(), joined.size()), "hash_add");

  std::cout << "XXH3 tests passed; CPU capabilities: " << Generics::Simd::CPU_CAPABILITIES
    << std::endl;
}
