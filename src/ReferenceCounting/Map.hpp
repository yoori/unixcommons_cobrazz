#ifndef REFERENCECOUNTING_MAP_HPP
#define REFERENCECOUNTING_MAP_HPP

#include <map>

#include <ReferenceCounting/Containers.hpp>


namespace ReferenceCounting
{
  /**
   * Const-preserving version of std::map.
   * No f(const T&) functions are available, they are replaced with
   * f(T&) and f(T&&) ones.
   */
  template <typename Key, typename T, typename Compare = std::less<Key>,
    typename Allocator = std::allocator<std::pair<const Key, T>>>
  class Map :
    protected std::map<Key, T, Compare,
      typename Helper::Allocator<std::pair<const Key, T>, Allocator>>
  {
  public:
    typedef std::map<Key, T, Compare,
      typename Helper::Allocator<std::pair<const Key, T>, Allocator>>
      Base;

    typedef typename Base::key_type key_type;
    typedef typename Base::mapped_type mapped_type;
    typedef typename Base::value_type value_type;
    typedef typename Base::key_compare key_compare;
    typedef typename Base::value_compare value_compare;
    typedef typename Base::pointer pointer;
    typedef typename Base::const_pointer const_pointer;
    typedef typename Base::reference reference;
    typedef typename Base::const_reference const_reference;
    typedef typename Base::iterator iterator;
    typedef typename Base::const_iterator const_iterator;
    typedef typename Base::reverse_iterator reverse_iterator;
    typedef typename Base::const_reverse_iterator const_reverse_iterator;
    typedef typename Base::size_type size_type;
    typedef typename Base::difference_type difference_type;

    using Base::begin;
    using Base::end;
    using Base::cbegin;
    using Base::cend;
    using Base::rbegin;
    using Base::rend;
    using Base::crbegin;
    using Base::crend;
    using Base::empty;
    using Base::size;
    using Base::max_size;
    using Base::operator [];
    using Base::at;
    using Base::erase;
    using Base::clear;
    using Base::key_comp;
    using Base::value_comp;
    using Base::find;
    using Base::count;
    using Base::lower_bound;
    using Base::upper_bound;
    using Base::equal_range;

    explicit
    Map(const Compare& cmp = Compare()) /*throw (eh::Exception)*/;

    Map(Map& m) /*throw (eh::Exception)*/;

    Map(const Map&) = delete;

    Map(Map&& m) noexcept;

    template <typename InputIterator>
    Map(InputIterator first, InputIterator last,
      const Compare& cmp = Compare()) /*throw (eh::Exception)*/;

    Map&
    operator =(Map& m) /*throw (eh::Exception)*/;

    Map&
    operator =(Map&& m) noexcept;

    std::pair<iterator, bool>
    insert(value_type& x) /*throw (eh::Exception)*/;

    std::pair<iterator, bool>
    insert(value_type&& x) /*throw (eh::Exception)*/;

    iterator
    insert(iterator position, value_type& x) /*throw (eh::Exception)*/;

    iterator
    insert(iterator position, value_type&& x) /*throw (eh::Exception)*/;

    template<class... Args>
    std::pair<iterator,bool>
    emplace(Args&&... args);

    template <typename InputIterator>
    void
    insert(InputIterator first, InputIterator last) /*throw (eh::Exception)*/;

    void
    swap(Map& m) noexcept;

  private:
    value_type
    value_type_(value_type& x) /*throw (eh::Exception)*/;
  };

  template <typename Allocator = std::allocator<char>,
    template <typename> class Compare = std::less>
  struct MapBind
  {
    template <typename Key, typename T>
    struct Rebind
    {
      typedef Map<Key, T, Compare<Key>, Allocator> Type;
    };
  };
}

namespace ReferenceCounting
{
  template <typename Key, typename T, typename Compare, typename Allocator>
  Map<Key, T, Compare, Allocator>::Map(const Compare& cmp)
    /*throw (eh::Exception)*/
    : Base(cmp)
  {
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  Map<Key, T, Compare, Allocator>::Map(Map& m) /*throw (eh::Exception)*/
    : Base(m.key_comp())
  {
    insert(m.begin(), m.end());
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  Map<Key, T, Compare, Allocator>::Map(Map&& m) noexcept
    : Base(std::move(m))
  {
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  template <typename InputIterator>
  Map<Key, T, Compare, Allocator>::Map(InputIterator first,
    InputIterator last, const Compare& cmp) /*throw (eh::Exception)*/
    : Base(cmp)
  {
    insert(first, last);
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  Map<Key, T, Compare, Allocator>&
  Map<Key, T, Compare, Allocator>::operator =(Map& m) /*throw (eh::Exception)*/
  {
    {
      Map m1(m);
      swap(m1);
    }
    return *this;
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  Map<Key, T, Compare, Allocator>&
  Map<Key, T, Compare, Allocator>::operator =(Map&& m) noexcept
  {
    Base::operator =(std::move(m));
    return *this;
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  std::pair<typename Map<Key, T, Compare, Allocator>::iterator, bool>
  Map<Key, T, Compare, Allocator>::insert(value_type& x)
    /*throw (eh::Exception)*/
  {
    return insert(value_type_(x));
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  std::pair<typename Map<Key, T, Compare, Allocator>::iterator, bool>
  Map<Key, T, Compare, Allocator>::insert(value_type&& x)
    /*throw (eh::Exception)*/
  {
    return Base::insert(std::forward<value_type>(x));
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  typename Map<Key, T, Compare, Allocator>::iterator
  Map<Key, T, Compare, Allocator>::insert(iterator position, value_type& x)
    /*throw (eh::Exception)*/
  {
    return insert(position, value_type_(x));
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  typename Map<Key, T, Compare, Allocator>::iterator
  Map<Key, T, Compare, Allocator>::insert(iterator position, value_type&& x)
    /*throw (eh::Exception)*/
  {
    return Base::insert(position, std::forward<value_type>(x));
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  template <typename InputIterator>
  void
  Map<Key, T, Compare, Allocator>::insert(InputIterator first,
    InputIterator last) /*throw (eh::Exception)*/
  {
    for (; first != last; ++first)
    {
      insert(value_type((*first).first, (*first).second));
    }
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  template<class... Args>
  std::pair<typename Map<Key, T, Compare, Allocator>::iterator, bool>
  Map<Key, T, Compare, Allocator>::emplace(Args&&... args)
  {
    return Base::emplace(std::forward<Args>(args)...);
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  void
  Map<Key, T, Compare, Allocator>::swap(Map& m) noexcept
  {
    Base::swap(m);
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  typename Map<Key, T, Compare, Allocator>::value_type
  Map<Key, T, Compare, Allocator>::value_type_(value_type& x)
    /*throw (eh::Exception)*/
  {
    return value_type(x.first, x.second);
  }


  template <typename Key, typename T, typename Compare, typename Allocator>
  void
  swap(Map<Key, T, Compare, Allocator>& x,
    Map<Key, T, Compare, Allocator>& y) noexcept
  {
    x.swap(y);
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  void
  swap(Map<Key, T, Compare, Allocator>&& x,
    Map<Key, T, Compare, Allocator>& y) noexcept
  {
    x.swap(y);
  }

  template <typename Key, typename T, typename Compare, typename Allocator>
  void
  swap(Map<Key, T, Compare, Allocator>& x,
    Map<Key, T, Compare, Allocator>&& y) noexcept
  {
    x.swap(y);
  }
}

#endif
