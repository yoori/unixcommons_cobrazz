#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include <Generics/Uuid.hpp>

namespace
{
  using Words = std::pair<uint64_t, uint64_t>;
  using Generics::Uuid;

  static_assert(sizeof(Uuid) == 16);
  static_assert(alignof(Uuid) == 1);

  void check(bool condition, const char* message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  Uuid make_uuid(const Words& words)
  {
    std::array<uint8_t, 16> bytes;
    std::memcpy(bytes.data(), &words.first, sizeof(uint64_t));
    std::memcpy(bytes.data() + sizeof(uint64_t), &words.second, sizeof(uint64_t));
    return Uuid(bytes.begin(), bytes.end());
  }

  void check_pair(const Uuid& left, const Uuid& right, const Words& lhs, const Words& rhs)
  {
    check((left == right) == (lhs == rhs), "UUID equality mismatch");
    check((left != right) == (lhs != rhs), "UUID inequality mismatch");
    check((left < right) == (lhs < rhs), "UUID less-than mismatch");
    check((left > right) == (lhs > rhs), "UUID greater-than mismatch");
    check((left <= right) == (lhs <= rhs), "UUID less-or-equal mismatch");
    check((left >= right) == (lhs >= rhs), "UUID greater-or-equal mismatch");
  }

  void test_comparisons()
  {
    const auto maximum = std::numeric_limits<uint64_t>::max();
    std::vector<Words> words{
      {0, 0}, {0, 1}, {0, 256}, {1, 0}, {256, 0}, {1, maximum},
      {maximum, 0}, {maximum, maximum}, {0, maximum}, {1ULL << 63, 0}, {0, 1ULL << 63}};
    for (const auto& left : words)
    {
      for (const auto& right : words)
      {
        check_pair(make_uuid(left), make_uuid(right), left, right);
      }
    }

    std::mt19937_64 random(1234567);
    for (unsigned i = 0; i < 1024; ++i)
    {
      const Words left{random(), random()};
      const Words right{random(), random()};
      check_pair(make_uuid(left), make_uuid(left), left, left);
      check_pair(make_uuid(left), make_uuid(right), left, right);
      check_pair(make_uuid(left), make_uuid({left.first, right.second}),
        left, {left.first, right.second});
      words.push_back(left);
    }

    std::vector<Uuid> uuids;
    for (const auto& value : words)
    {
      uuids.push_back(make_uuid(value));
    }

    std::sort(words.begin(), words.end());
    std::sort(uuids.begin(), uuids.end());
    for (std::size_t i = 0; i < words.size(); ++i)
    {
      check(uuids[i] == make_uuid(words[i]), "UUID sort order mismatch");
    }
  }

  void test_equality_and_alignment()
  {
    const Words words{0x1234567890abcdefULL, 0xfedcba0987654321ULL};
    const auto reference = make_uuid(words);
    for (std::size_t i = 0; i < Uuid::size(); ++i)
    {
      auto changed = reference;
      changed.begin()[i] ^= 0x80;
      check(changed != reference, "UUID equality ignored a byte");
      check(reference != changed, "UUID inequality is asymmetric");
    }

    alignas(uint64_t) std::array<uint8_t, sizeof(Uuid) + 1> storage;
    auto* unaligned = new (storage.data() + 1) Uuid(reference);
    check_pair(*unaligned, reference, words, words);
    const Words other{words.first, words.second + 1};
    check_pair(*unaligned, make_uuid(other), words, other);
    check_pair(make_uuid(other), *unaligned, other, words);
    unaligned->~Uuid();
  }
}

int main()
{
  try
  {
    test_comparisons();
    test_equality_and_alignment();
    std::cout << "TestUuidComparison: PASS" << std::endl;
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << error.what() << std::endl;
    return 1;
  }
}
