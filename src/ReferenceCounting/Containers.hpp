#ifndef REFERENCECOUNTING_CONTAINERS_HPP
#define REFERENCECOUNTING_CONTAINERS_HPP

#include <eh/Exception.hpp>


namespace ReferenceCounting
{
  namespace Helper
  {
    template <typename T, typename Alloc>
    struct Allocator : public std::allocator_traits<Alloc>::template rebind_alloc<T>
    {
      template <typename D>
      struct rebind
      {
        typedef Allocator<D, Alloc> other;
      };
      Allocator() throw () = default;
      template <typename D>
      Allocator(const Allocator<D, Alloc>&) throw ();
      template <typename... Args>
      void
      construct(T* p, Args&&... args) /*throw (eh::Exception)*/;
    };

    template <class Key>
    struct HashFunForHashAdapter
    {
      size_t
      operator ()(const Key& key) const /*throw (eh::Exception)*/;
    };
  }
}

namespace ReferenceCounting
{
  namespace Helper
  {
    //
    // Allocator class
    //

    template <typename T, typename Alloc>
    template <typename D>
    Allocator<T, Alloc>::Allocator(const Allocator<D, Alloc>&) throw ()
      : std::allocator_traits<Alloc>::template rebind_alloc<T>()
    {
    }

    template <typename T, typename Alloc>
    template <typename... Args>
    void
    Allocator<T, Alloc>::construct(T* p, Args&&... args)
      /*throw (eh::Exception)*/
    {
      ::new(p) T(const_cast<typename std::remove_const<
        typename std::remove_reference<Args>::type>::type&&>(args)...);
    }


    //
    // HashFunForHashAdapter class
    //

    template <class Key>
    size_t
    HashFunForHashAdapter<Key>::operator ()(const Key& key) const
      /*throw (eh::Exception)*/
    {
      return key.hash();
    }
  }
}

#endif
