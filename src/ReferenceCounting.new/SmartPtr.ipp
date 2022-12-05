#include <cassert>

#include <Generics/Proc.hpp>


namespace ReferenceCounting
{
  //
  // PolicyThrow class
  //

  inline
  void
  PolicyThrow::check_init(const void* /*ptr*/) /*throw (NullPointer)*/
  {
  }

  inline
  void
  PolicyThrow::check_dereference(const void* ptr)
    /*throw (NotInitialized)*/
  {
    if (!ptr)
    {
      char buf[sizeof(NotInitialized)] =
        "ReferenceCounting::PolicyThrow::check_dereference(): "
        "unable to dereference NULL pointer: ";
      Generics::Proc::backtrace(buf + 89, sizeof(buf) - 89, 1, 5);
      throw NotInitialized(buf);
    }
  }

  inline
  void
  PolicyThrow::default_constructor() throw ()
  {
  }

  inline
  void
  PolicyThrow::retn() throw ()
  {
  }


  //
  // PolicyAssert class
  //

  inline
  void
  PolicyAssert::check_init(const void* /*ptr*/) /*throw (NullPointer)*/
  {
  }

  inline
  void
  PolicyAssert::check_dereference(const void* ptr)
    /*throw (NotInitialized)*/
  {
    (void)ptr;
    assert(ptr);
  }

  inline
  void
  PolicyAssert::default_constructor() throw ()
  {
  }

  inline
  void
  PolicyAssert::retn() throw ()
  {
  }


  //
  // PolicyNotNull class
  //

  inline
  void
  PolicyNotNull::check_init(const void* ptr) /*throw (NullPointer)*/
  {
    if (!ptr)
    {
      throw NullPointer(
        "ReferenceCounting::PolicyNotNull::check_init(): "
        "unable to init with NULL pointer.");
    }
  }

  inline
  void
  PolicyNotNull::check_dereference(const void* /*ptr*/)
    /*throw (NotInitialized)*/
  {
  }


  //
  // PolicyChecker class
  //

  inline
  void
  PolicyChecker::check_policy_(const PolicyThrow* /*policy*/)
    throw ()
  {
  }

  inline
  void
  PolicyChecker::check_policy_(const PolicyAssert* /*policy*/)
    throw ()
  {
  }

  inline
  void
  PolicyChecker::check_policy_(const PolicyNotNull* /*policy*/)
    throw ()
  {
  }
}
