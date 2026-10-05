#include <array>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

#include <boost/unordered/unordered_flat_map.hpp>

#include <Generics/HashTableAdapters.hpp>

namespace
{
  void check(bool condition, const char* what, std::size_t len = 0)
  {
    if (!condition)
    {
      std::cerr << what << " at length " << len << std::endl;
      std::exit(1);
    }
  }

  struct Hash
  {
    using is_transparent = void;

    template <typename Adapter>
    std::size_t operator()(const Adapter& value) const noexcept
    {
      return value.hash();
    }
  };

  struct Equal
  {
    using is_transparent = void;

    template <typename Left, typename Right>
    bool operator()(const Left& left, const Right& right) const noexcept
    {
      return std::string_view(left.text().data(), left.text().size()) ==
        std::string_view(right.text().data(), right.text().size());
    }
  };
}

int main()
{
  using namespace Generics;
  std::array<char, 4104> input;
  std::uint64_t random = 1;
  for (auto& byte : input)
  {
    random ^= random << 13;
    random ^= random >> 7;
    random ^= random << 17;
    byte = static_cast<char>(random);
  }

  std::vector<std::size_t> lengths;
  for (std::size_t len = 0; len <= 300; ++len)
  {
    lengths.push_back(len);
  }
  for (const std::size_t len : {511, 512, 513, 1023, 1024, 1025, 4095, 4096})
  {
    lengths.push_back(len);
  }

  boost::unordered_flat_map<StringHashAdapter, std::size_t, Hash, Equal> table;
  for (const auto len : lengths)
  {
    for (std::size_t offset = 0; offset < 8; ++offset)
    {
      const auto* data = input.data() + offset;
      for (const auto seed : {0ULL, 1ULL, 0x123456789abcdef0ULL, 0xffffffffffffffffULL})
      {
        Murmur64Hasher murmur(seed);
        murmur.add(data, len);
        const auto expected = len <= 48 ? murmur.finalize() : XXH3Hasher::hash(data, len, seed);
        check(FastHash::hash(data, len, seed) == expected, "algorithm/seed", len);
      }

      const std::string text(data, len);
      const String::SubString substring(data, len);
      const std::string_view view(data, len);
      const auto expected = FastHash::hash(data, len);
      StringHashAdapter owning(text);
      check(owning.hash() == expected, "owning adapter", len);
      check(StringHashAdapter(substring).hash() == expected, "owning from substring", len);
      check(StringHashAdapter(data, len).hash() == expected, "owning from buffer", len);
      check(StringHashAdapter(std::string(text)).hash() == expected, "owning move", len);
      check(SubStringHashAdapter(substring).hash() == expected, "substring adapter", len);
      check(SubStringHashAdapter(text).hash() == expected, "substring from string", len);
      check(StringViewHashAdapter(view).hash() == expected, "view adapter", len);
      check(StringViewHashAdapter(text).hash() == expected, "view from string", len);
      check(StringViewHashAdapter(owning).hash() == expected, "view from owning", len);
      owning.assign(std::string_view("different"));
      owning.assign(view);
      check(owning.hash() == expected, "assign recalculates", len);
      const StringHashAdapter copied(owning);
      check(copied.hash() == expected, "copy preserves", len);
      const StringHashAdapter moved(std::move(owning));
      check(moved.hash() == expected && moved.text() == text, "move preserves", len);
      check(StringHashAdapter(expected, view).hash() == expected, "precomputed owning", len);
      check(SubStringHashAdapter(expected, substring).hash() == expected, "precomputed sub", len);
      check(StringViewHashAdapter(expected, view).hash() == expected, "precomputed view", len);
      table.insert_or_assign(StringHashAdapter(text), len);
      check(table.find(SubStringHashAdapter(substring)) != table.end(), "find substring", len);
      check(table.find(StringViewHashAdapter(view)) != table.end(), "find view", len);
    }
  }

  // Rehashing and subsequent growth must preserve cross-adapter lookup of long keys.
  for (const auto len : lengths)
  {
    for (std::size_t offset = 0; offset < 8; ++offset)
    {
      const auto found = table.find(StringViewHashAdapter(
        std::string_view(input.data() + offset, len)));
      check(found != table.end() && found->second == len, "lookup after growth", len);
    }
  }

  const auto page_size = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
  void* mapping = mmap(nullptr, 2 * page_size, PROT_READ | PROT_WRITE,
    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  check(mapping != MAP_FAILED, "guard mapping");
  auto* end = static_cast<char*>(mapping) + page_size;
  check(mprotect(end, page_size, PROT_NONE) == 0, "guard protection");
  for (const auto len : lengths)
  {
    if (len > page_size)
    {
      continue;
    }
    auto* data = end - len;
    std::memcpy(data, input.data(), len);
    const auto expected = FastHash::hash(input.data(), len);
    check(FastHash::hash(data, len) == expected, "guard page", len);
    check(SubStringHashAdapter(String::SubString(data, len)).hash() == expected,
      "adapter guard page", len);
  }
  check(munmap(mapping, 2 * page_size) == 0, "guard unmap");

  const auto empty = FastHash::hash(nullptr, 0);
  check(StringHashAdapter().hash() == empty, "default owning");
  check(SubStringHashAdapter().hash() == empty, "default substring");
  check(StringViewHashAdapter().hash() == empty, "default view");
  const char* c_string = "http://test.me/test1/test2";
  StringHashAdapter c_adapter;
  c_adapter.assign(c_string);
  check(c_adapter.hash() == StringHashAdapter(c_string).hash(), "C string assign");
  std::cout << "FastHash and cross-adapter lookup tests passed" << std::endl;
}
