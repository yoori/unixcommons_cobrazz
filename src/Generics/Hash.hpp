#pragma once

#include <string>
#include <limits>
#include <utility>

#include <ReferenceCounting/NullPtr.hpp>

#include <Generics/Uncopyable.hpp>
#include <Generics/CRC.hpp>


namespace Generics
{
  /*
   * Below provided some hash classes able to incremental hash calculation,
   *   Usage:
   * std::size_t result_hash;
   * {
   *   SomeHash hash(result_hash);
   *   hash.add(data1, size1);
   *   hash_add(hash, String::SubString("text"));
   *   ...
   *   hash_add(hash, static_cast<short>(N));
   * }
   * And here result_hash is calculated.
   *
   *
   * Another interface is Hasher
   *  Usage:
   * SomeHasher hasher;
   * hasher.add(data1, size1);
   * hash_add(hasher, String::SubString("text"));
   * ...
   * hash_add(hasher, static_cast<short>(N));
   * std::size_t result_hash = hasher.finalize ();
   * hasher is not usable here.
   * Hashers are copyable.
   */

  class CRC32Hasher
  {
  public:
    using Calc = uint32_t;

    explicit CRC32Hasher(Calc seed = 0) noexcept;

    void add(const void* key, std::size_t len) noexcept;

    std::size_t finalize () noexcept;

  private:
    Calc hash_;
  };

}

namespace Generics::HashHelper
{
  template <typename Mix>
  class Aggregator
  {
  public:
    using Calc = typename Mix::Calc;

    explicit Aggregator(Calc seed = 0) noexcept;

    inline void add(const void* key, std::size_t len) noexcept __attribute__((always_inline));

    inline std::size_t finalize () noexcept __attribute__((always_inline));

  private:
    std::size_t count_;
    Calc tail_;
    Calc size_;
    Mix mix_;
  };

  class Murmur64
  {
  public:
    using Calc = std::size_t;

    explicit Murmur64(Calc seed) noexcept;
    void operator ()(Calc key) noexcept;
    std::size_t operator ()(std::size_t count, Calc tail, Calc size) noexcept;

  private:
    static const std::size_t MULTIPLIER_ = 0xC6A4A7935BD1E995ull;
    static const std::size_t R_ = 47;

    Calc hash_;
  };

  class Murmur32v3
  {
  public:
    using Calc = uint32_t;

    explicit Murmur32v3(Calc seed) noexcept;
    void operator ()(Calc key, bool rh = true) noexcept;
    std::size_t operator ()(std::size_t count, Calc tail, Calc size) noexcept;

  private:
    Calc hash_;
  };
}

namespace Generics
{
  /**
   * The algorithm is based on MurmurHash64A.
   * Hash function with added Merkle–Damgård construction.
   */
  using Murmur64Hasher = HashHelper::Aggregator<HashHelper::Murmur64>;

  /**
   * Murmur3 x86 32
   */
  using Murmur32v3Hasher = HashHelper::Aggregator<HashHelper::Murmur32v3>;

}

namespace Generics::HashHelper
{
  template <typename Hasher>
  class Adapter : private Generics::Uncopyable
  {
  public:
    using BaseHasher = Hasher;
    using Calc = typename Hasher::Calc;

    explicit Adapter(std::size_t& result, Calc seed = 0) noexcept;
    ~Adapter() noexcept;
    void add(const void* key, std::size_t len) noexcept;

  private:
    Hasher hasher_;
    std::size_t& result_;
  };
}

namespace Generics
{
  using CRC32Hash = HashHelper::Adapter<CRC32Hasher>;
  using Murmur64Hash = HashHelper::Adapter<Murmur64Hasher>;
  using Murmur32v3Hash = HashHelper::Adapter<Murmur32v3Hasher>;

  template <typename Hash, typename Value, typename Check = typename
    std::enable_if<std::numeric_limits<Value>::is_specialized>::type>
  void hash_add(Hash& hash, const Value& value) noexcept;

  template <typename Hash, typename Value,
    typename Check1 = decltype(std::declval<Value>().data()),
    typename Check2 = decltype(std::declval<Value>().size()),
    typename Check3 = typename std::enable_if<
      std::numeric_limits<typename std::remove_const<
        typename std::remove_reference< decltype(*std::declval<Value>().data())>::type>::
      type>::is_specialized>::type> void hash_add(Hash& hash, const Value& value) noexcept;
}

namespace Generics
{
  //
  // CRC32Hasher class
  //

  inline CRC32Hasher::CRC32Hasher(uint32_t seed) noexcept
    : hash_(seed)
  {
  }

  inline void CRC32Hasher::add(const void* key, std::size_t len) noexcept
  {
    hash_ = CRC::quick(hash_, key, len);
  }

  inline std::size_t CRC32Hasher::finalize() noexcept
  {
    return hash_;
  }


}

namespace Generics::HashHelper
{
  template <typename Calc>
  std::size_t get_unsafe(const void* key) noexcept
  {
    return *static_cast<const Calc*>(key);
  }

  inline std::size_t get_safe(const uint8_t* key, std::size_t size) noexcept
  {
    std::size_t key_part = 0;
    for (size_t i = 0; i < size; i++)
    {
      key_part |= static_cast<uint64_t>(key[i]) << (i * 8);
    }
    return key_part;
  }


  //
  // Aggregator class
  //

  template <typename Mix>
  Aggregator<Mix>::Aggregator(Calc seed) noexcept
    : count_(0), tail_(0), size_(0), mix_(seed)
  {
  }

  template <typename Mix>
  inline void Aggregator<Mix>::add(const void* key, std::size_t len) noexcept
  {
    if (!len)
    {
      return;
    }

    size_ += len;

    const uint8_t* data = static_cast<const uint8_t*>(key);
    const bool LEN = len < sizeof(Calc);

    if (LEN || count_)
    {
      std::size_t required = sizeof(Calc) - count_;
      if (required <= len)
      {
        mix_(tail_ | ((LEN ? get_safe(data, required) : get_unsafe<Calc>(data)) << (count_ * 8)));
        tail_ = 0;
        count_ = 0;
        len -= required;
        data += required;
      }
      else
      {
        tail_ |= get_safe(data, len) << (count_ * 8);
        count_ += len;
        return;
      }
    }

    while (len >= sizeof(Calc))
    {
      mix_(get_unsafe<Calc>(data));
      data += sizeof(Calc);
      len -= sizeof(Calc);
    }

    if (len)
    {
      tail_ = get_safe(data, len);
      count_ = len;
    }
  }

  template <typename Mix>
  inline std::size_t Aggregator<Mix>::finalize() noexcept
  {
    return mix_(count_, tail_, size_);
  }


  //
  // MurMur64 class
  //

  inline Murmur64::Murmur64(Calc seed) noexcept
    : hash_(seed)
  {
  }

  inline void Murmur64::operator ()(Calc key) noexcept
  {
    key *= MULTIPLIER_;
    key ^= key >> R_;
    key *= MULTIPLIER_;
    hash_ *= MULTIPLIER_;
    hash_ ^= key;
  }

  inline std::size_t Murmur64::operator ()(std::size_t /*count*/, Calc tail, Calc size) noexcept
  {
    operator()(tail);
    // Merkle–Damgård strengthening
    operator()(size);
    // finalization
    hash_ ^= hash_ >> R_;
    hash_ *= MULTIPLIER_;
    hash_ ^= hash_ >> R_;
    return hash_;
  }


  //
  // MurMur32v3 class
  //

  inline Murmur32v3::Murmur32v3(Calc seed) noexcept
    : hash_(seed)
  {
  }

  inline void Murmur32v3::operator ()(Calc key, bool rh) noexcept
  {
    key *= static_cast<Calc>(0xCC9E2D51u);
    key = (key << 15) | (key >> 17);
    key *= static_cast<Calc>(0x1B873593u);
    hash_ ^= key;
    if (rh)
    {
      hash_ = ((hash_ << 13) | (hash_ >> 19)) * 5 +
        static_cast<Calc>(0xE6546B64u);
    }
  }

  inline std::size_t Murmur32v3::operator ()(std::size_t count, Calc tail, Calc size) noexcept
  {
    if (count)
    {
      operator()(tail, false);
    }
    hash_ ^= size;
    hash_ ^= hash_ >> 16;
    hash_ *= static_cast<Calc>(0x85EBCA6Bu);
    hash_ ^= hash_ >> 13;
    hash_ *= static_cast<Calc>(0xC2B2AE35u);
    hash_ ^= hash_ >> 16;
    return hash_;
  }


  template <typename Hasher>
  Adapter<Hasher>::Adapter(std::size_t& result, Calc seed) noexcept
    : hasher_(seed), result_(result)
  {
  }

  template <typename Hasher>
  Adapter<Hasher>::~Adapter() noexcept
  {
    result_ = hasher_.finalize();
  }

  template <typename Hasher>
  void Adapter<Hasher>::add(const void* key, std::size_t len) noexcept
  {
    hasher_.add(key, len);
  }
}

namespace Generics
{

  //
  // Hash adders' implementations
  //

  template <typename Hash, typename Value, typename Check>
  void hash_add(Hash& hash, const Value& value) noexcept
  {
    hash.add(&value, sizeof(value));
  }

  template <typename Hash, typename Value, typename Check1, typename Check2, typename Check3>
  void hash_add(Hash& hash, const Value& value) noexcept
  {
    hash.add(value.data(), value.size() * sizeof(*value.data()));
  }
}
