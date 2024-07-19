#ifndef REFERENCECOUNTING_DEQUE_HPP
#define REFERENCECOUNTING_DEQUE_HPP

#include <deque>

#include <eh/Exception.hpp>


namespace ReferenceCounting
{
  /**
   * Const-preserving version of std::deque.
   * No f(const T&) functions are available, they are replaced with
   * f(T&) and f(T&&) ones.
   */
  template <typename T, typename Allocator = std::allocator<T>>
  class Deque : protected std::deque<T, Allocator>
  {
  public:
    typedef std::deque<T, Allocator> Base;

    typedef typename Base::value_type value_type;
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
    using Base::size;
    using Base::max_size;
    using Base::empty;
    using Base::operator [];
    using Base::at;
    using Base::front;
    using Base::back;
    using Base::emplace_front;
    using Base::emplace_back;
    using Base::pop_front;
    using Base::pop_back;
    using Base::emplace;
    using Base::erase;
    using Base::clear;

    Deque() noexcept;
    Deque(size_type n) /*throw (eh::Exception)*/;
    Deque(size_type n, value_type& x) /*throw (eh::Exception)*/;
    Deque(Deque& d) /*throw (eh::Exception)*/;
    Deque(const Deque&) = delete;
    Deque(Deque&& d) noexcept;
    template <typename InputIterator>
    Deque(InputIterator first, InputIterator last) /*throw (eh::Exception)*/;

    Deque&
    operator =(Deque& d) /*throw (eh::Exception)*/;
    Deque&
    operator =(Deque&& d) noexcept;

    void
    assign(size_type n, value_type& x) /*throw (eh::Exception)*/;
    template <typename InputIterator>
    void
    assign(InputIterator first, InputIterator last) /*throw (eh::Exception)*/;

    void
    resize(size_type n) /*throw (eh::Exception)*/;
    void
    resize(size_type n, value_type& v) /*throw (eh::Exception)*/;

    void
    push_front(value_type& x) /*throw (eh::Exception)*/;
    void
    push_front(value_type&& x) /*throw (eh::Exception)*/;
    void
    push_back(value_type& x) /*throw (eh::Exception)*/;
    void
    push_back(value_type&& x) /*throw (eh::Exception)*/;

    iterator
    insert(iterator position, value_type& x) /*throw (eh::Exception)*/;
    iterator
    insert(iterator position, value_type&& x) /*throw (eh::Exception)*/;
    void
    insert(iterator position, size_type n, value_type& x)
      /*throw (eh::Exception)*/;
    template <typename InputIterator>
    void
    insert(iterator position, InputIterator first, InputIterator last)
      /*throw (eh::Exception)*/;

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
    void
    swap(Deque&& d) noexcept;
#else
    void
    swap(Deque& d) noexcept;
#endif

  private:
    size_type
    resize_(size_type n) /*throw (eh::Exception)*/;
  };
}

namespace ReferenceCounting
{
  template <typename T, typename Allocator>
  Deque<T, Allocator>::Deque() noexcept
  {
  }

  template <typename T, typename Allocator>
  Deque<T, Allocator>::Deque(size_type n) /*throw (eh::Exception)*/
  {
    resize(n);
  }

  template <typename T, typename Allocator>
  Deque<T, Allocator>::Deque(size_type n, value_type& x)
    /*throw (eh::Exception)*/
    : Base()
  {
    assign(n, x);
  }

  template <typename T, typename Allocator>
  Deque<T, Allocator>::Deque(Deque& d) /*throw (eh::Exception)*/
    : Base()
  {
    assign(d.begin(), d.end());
  }

  template <typename T, typename Allocator>
  Deque<T, Allocator>::Deque(Deque&& d) noexcept
    : Base(std::move(d))
  {
  }

  template <typename T, typename Allocator>
  template <typename InputIterator>
  Deque<T, Allocator>::Deque(InputIterator first, InputIterator last)
    /*throw (eh::Exception)*/
  {
    assign(first, last);
  }

  template <typename T, typename Allocator>
  Deque<T, Allocator>&
  Deque<T, Allocator>::operator =(Deque& d) /*throw (eh::Exception)*/
  {
    assign(d.begin(), d.end());
    return *this;
  }

  template <typename T, typename Allocator>
  Deque<T, Allocator>&
  Deque<T, Allocator>::operator =(Deque&& d) noexcept
  {
    Base::operator =(std::move(d));
    return *this;
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::assign(size_type n, value_type& x)
    /*throw (eh::Exception)*/
  {
    n = resize_(n);
    for (iterator i(begin()); i != end(); ++i)
    {
      *i = x;
    }
    while (n--)
    {
      emplace_back(x);
    }
  }

  template <typename T, typename Allocator>
  template <typename InputIterator>
  void
  Deque<T, Allocator>::assign(InputIterator first, InputIterator last)
    /*throw (eh::Exception)*/
  {
    size_type n = std::distance(first, last);
    n = resize_(n);
    for (iterator i(begin()); i != end(); ++i)
    {
      *i = *first;
      ++first;
    }
    while (n--)
    {
      emplace_back(*first);
      ++first;
    }
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::resize(size_type n) /*throw (eh::Exception)*/
  {
    for (n = resize_(n); n--;)
    {
      emplace_back();
    }
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::resize(size_type n, value_type& x)
    /*throw (eh::Exception)*/
  {
    for (n = resize_(n); n--;)
    {
      emplace_back(x);
    }
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::push_front(value_type& x) /*throw (eh::Exception)*/
  {
    emplace_front(x);
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::push_front(value_type&& x) /*throw (eh::Exception)*/
  {
    emplace_front(std::move(x));
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::push_back(value_type& x) /*throw (eh::Exception)*/
  {
    emplace_back(x);
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::push_back(value_type&& x) /*throw (eh::Exception)*/
  {
    emplace_back(std::move(x));
  }

  template <typename T, typename Allocator>
  typename Deque<T, Allocator>::iterator
  Deque<T, Allocator>::insert(iterator position, value_type& x)
    /*throw (eh::Exception)*/
  {
    return emplace(position, value_type(x));
  }

  template <typename T, typename Allocator>
  typename Deque<T, Allocator>::iterator
  Deque<T, Allocator>::insert(iterator position, value_type&& x)
    /*throw (eh::Exception)*/
  {
    return emplace(position, std::move(x));
  }

  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::insert(iterator position, size_type n, value_type& x)
    /*throw (eh::Exception)*/
  {
    if (!n)
    {
      return;
    }

    size_type sz = size();
    size_type offset = position - begin();
    size_type r = sz - offset;
    if (n >= r)
    {
      for (size_type i = n - r; i--;)
      {
        emplace_back(x);
      }
      iterator it = begin() + offset;
      for (size_type i = r; i--;)
      {
        value_type& v = *it;
        emplace_back(std::move(v));
        v = x;
        ++it;
      }
    }
    else
    {
      iterator it = begin() + (sz - n);
      for (size_type i = n; i--;)
      {
        emplace_back(std::move(*it));
        ++it;
      }
      it = begin() + sz;
      iterator it2 = it - n;
      for (size_type i = r - n; i--;)
      {
        --it;
        --it2;
        *it = std::move(*it2);
      }
      it = begin() + offset;
      for (size_type i = n; i--;)
      {
        *it = x;
        ++it;
      }
    }
  }

  template <typename T, typename Allocator>
  template <typename InputIterator>
  void
  Deque<T, Allocator>::insert(iterator position, InputIterator first,
    InputIterator last) /*throw (eh::Exception)*/
  {
    size_type offset = position - begin();
    size_type n = std::distance(first, last);
    {
      value_type v;
      insert(position, n, v);
    }
    for (iterator i(begin() + offset); n--; ++i)
    {
      *i = *first;
      ++first;
    }
  }

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::swap(Deque&& d) noexcept
  {
    Base::swap(std::move(d));
  }
#else
  template <typename T, typename Allocator>
  void
  Deque<T, Allocator>::swap(Deque& d) noexcept
  {
    Base::swap(d);
  }
#endif

  template <typename T, typename Allocator>
  typename Deque<T, Allocator>::size_type
  Deque<T, Allocator>::resize_(size_type n) /*throw (eh::Exception)*/
  {
    size_type sz = size();
    if (sz >= n)
    {
      if (sz > n)
      {
        erase(begin() + n, end());
      }
      return 0;
    }
    return n - sz;
  }


  template <typename T, typename Allocator>
  void
  swap(Deque<T, Allocator>& x, Deque<T, Allocator>& y) noexcept
  {
    x.swap(y);
  }

#if __GNUC__ == 4 && __GNUC_MINOR__ == 4
  template <typename T, typename Allocator>
  void
  swap(Deque<T, Allocator>&& x, Deque<T, Allocator>& y) noexcept
  {
    x.swap(y);
  }

  template <typename T, typename Allocator>
  void
  swap(Deque<T, Allocator>& x, Deque<T, Allocator>&& y) noexcept
  {
    x.swap(y);
  }
#endif
}

#endif
