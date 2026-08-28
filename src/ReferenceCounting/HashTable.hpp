#pragma once

#include <unordered_map>

#include <ReferenceCounting/Containers.hpp>


namespace ReferenceCounting
{
  /**
   * Const-preserving version of std::unordered_map.
   * No f(const T&) functions are available, they are replaced with
   * f(T&) and f(T&&) ones.
   */
  template <typename Key, typename T,
    typename EqualKey = std::equal_to<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>>
  class HashTable :
    protected std::unordered_map<Key, T,
      Helper::HashFunForHashAdapter<Key>, EqualKey,
      typename Helper::Allocator<std::pair<const Key, T>, Allocator>>
  {
  public:
    using Base = std::unordered_map<Key, T,
      Helper::HashFunForHashAdapter<Key>, EqualKey,
      typename Helper::Allocator<std::pair<const Key, T>, Allocator>>;

    using key_type = typename Base::key_type;
    using mapped_type = typename Base::mapped_type;
    using value_type = typename Base::value_type;
    using hasher = typename Base::hasher;
    using key_equal = typename Base::key_equal;
    using pointer = typename Base::pointer;
    using const_pointer = typename Base::const_pointer;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using size_type = typename Base::size_type;
    using difference_type = typename Base::difference_type;

    using Base::begin;
    using Base::end;
    using Base::cbegin;
    using Base::cend;
    using Base::size;
    using Base::max_size;
    using Base::bucket_count;
    using Base::max_bucket_count;
    using Base::operator [];
    using Base::at;
    using Base::empty;
    using Base::erase;
    using Base::clear;
    using Base::key_eq;
    using Base::find;
    using Base::count;
    using Base::equal_range;

    explicit HashTable(size_type n = 10) /*throw (eh::Exception)*/;
    HashTable(HashTable& h) /*throw (eh::Exception)*/;
    HashTable(const HashTable&) = delete;
    HashTable(HashTable&& h) noexcept;
    template <typename InputIterator>
    HashTable(InputIterator first, InputIterator last, size_type n = 10)
      /*throw (eh::Exception)*/;

    HashTable& operator =(HashTable& h) /*throw (eh::Exception)*/;
    HashTable& operator =(HashTable&& h) noexcept;

    std::pair<iterator, bool>
    insert(value_type& x) /*throw (eh::Exception)*/;
    std::pair<iterator, bool>
    insert(value_type&& x) /*throw (eh::Exception)*/;
    iterator insert(iterator position, value_type& x) /*throw (eh::Exception)*/;
    iterator insert(iterator position, value_type&& x) /*throw (eh::Exception)*/;
    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last) /*throw (eh::Exception)*/;

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
    void swap(HashTable&& h) noexcept;
#else
    void swap(HashTable& h) noexcept;
#endif

  private:
    value_type value_type_(value_type& x) /*throw (eh::Exception)*/;
  };

  template <typename Allocator = std::allocator<char>,
    template <typename> class EqualKey = std::equal_to>
  struct HashTableBind
  {
    template <typename Key, typename T>
    struct Rebind
    {
      using Type = HashTable<Key, T, EqualKey<Key>, Allocator>;
    };
  };
}

namespace ReferenceCounting
{
  template <typename Key, typename T, typename EqualKey, typename Allocator>
  HashTable<Key, T, EqualKey, Allocator>::HashTable(size_type n)
    /*throw (eh::Exception)*/
    : Base(n)
  {
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  HashTable<Key, T, EqualKey, Allocator>::HashTable(HashTable& h)
    /*throw (eh::Exception)*/
    : Base()
  {
    insert(h.begin(), h.end());
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  HashTable<Key, T, EqualKey, Allocator>::HashTable(HashTable&& h) noexcept
    : Base(std::move(h))
  {
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  template <typename InputIterator>
  HashTable<Key, T, EqualKey, Allocator>::HashTable(InputIterator first,
    InputIterator last, size_type n) /*throw (eh::Exception)*/
    : Base(n)
  {
    insert(first, last);
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  HashTable<Key, T, EqualKey, Allocator>&
  HashTable<Key, T, EqualKey, Allocator>::operator =(HashTable& h)
    /*throw (eh::Exception)*/
  {
    {
      HashTable h1(h);
      swap(h1);
    }
    return *this;
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  HashTable<Key, T, EqualKey, Allocator>&
  HashTable<Key, T, EqualKey, Allocator>::operator =(HashTable&& h) noexcept
  {
    Base::operator =(std::move(h));
    return *this;
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  std::pair<typename HashTable<Key, T, EqualKey, Allocator>::iterator, bool>
  HashTable<Key, T, EqualKey, Allocator>::insert(value_type& x)
    /*throw (eh::Exception)*/
  {
    return insert(value_type_(x));
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  std::pair<typename HashTable<Key, T, EqualKey, Allocator>::iterator, bool>
  HashTable<Key, T, EqualKey, Allocator>::insert(value_type&& x)
    /*throw (eh::Exception)*/
  {
    return Base::insert(std::forward<value_type>(x));
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  typename HashTable<Key, T, EqualKey, Allocator>::iterator
  HashTable<Key, T, EqualKey, Allocator>::insert(
    iterator position, value_type& x) /*throw (eh::Exception)*/
  {
    return insert(position, value_type_(x));
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  typename HashTable<Key, T, EqualKey, Allocator>::iterator
  HashTable<Key, T, EqualKey, Allocator>::insert(
    iterator position, value_type&& x) /*throw (eh::Exception)*/
  {
    return Base::insert(position, std::forward<value_type>(x));
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  template <typename InputIterator>
  void
  HashTable<Key, T, EqualKey, Allocator>::insert(InputIterator first,
    InputIterator last) /*throw (eh::Exception)*/
  {
    for (; first != last; ++first)
    {
      insert(value_type((*first).first, (*first).second));
    }
  }

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
  template <typename Key, typename T, typename EqualKey, typename Allocator>
  void HashTable<Key, T, EqualKey, Allocator>::swap(HashTable&& h) noexcept
  {
    Base::swap(std::move(h));
  }
#else
  template <typename Key, typename T, typename EqualKey, typename Allocator>
  void HashTable<Key, T, EqualKey, Allocator>::swap(HashTable& h) noexcept
  {
    Base::swap(h);
  }
#endif

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  typename HashTable<Key, T, EqualKey, Allocator>::value_type
  HashTable<Key, T, EqualKey, Allocator>::value_type_(value_type& x)
    /*throw (eh::Exception)*/
  {
    return value_type(x.first, x.second);
  }


  template <typename Key, typename T, typename EqualKey, typename Allocator>
  void
  swap(HashTable<Key, T, EqualKey, Allocator>& x,
    HashTable<Key, T, EqualKey, Allocator>& y) noexcept
  {
    x.swap(y);
  }

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
  template <typename Key, typename T, typename EqualKey, typename Allocator>
  void
  swap(HashTable<Key, T, EqualKey, Allocator>&& x,
    HashTable<Key, T, EqualKey, Allocator>& y) noexcept
  {
    x.swap(y);
  }

  template <typename Key, typename T, typename EqualKey, typename Allocator>
  void
  swap(HashTable<Key, T, EqualKey, Allocator>& x,
    HashTable<Key, T, EqualKey, Allocator>&& y) noexcept
  {
    x.swap(y);
  }
#endif
}
