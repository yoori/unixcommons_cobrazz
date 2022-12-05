#ifndef CORBACOMMONS_CORBAREFCOUNTING_HPP
#define CORBACOMMONS_CORBAREFCOUNTING_HPP

#include <tao/ORB.h>
#include <tao/PortableServer/PortableServer.h>
#include <tao/PortableServer/Servant_Base.h>
#include <tao/Valuetype/ValueFactory.h>
#include <tao/Valuetype/ValueBase.h>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/Singleton.hpp>


namespace CORBACommons
{
  /**X
   * Interface for using corba ..._var & ReferenceCounting ..._var
   * concurrently.
   */
  namespace ReferenceCounting
  {
    template <typename Object, typename RefCountStrategy>
    class RefCountStrategySetter :
      public virtual Object,
      public virtual RefCountStrategy
    {
    };

    /**X
     * OB::RefCount use mutex for sync, therefore
     * sync strategy was excess parameter
     */

    template <typename Object>
    class CorbaRefCountImpl :
      public virtual ::ReferenceCounting::Interface,
      public virtual Object
    {
    protected:
      CorbaRefCountImpl() /*throw (eh::Exception)*/;

      virtual
      ~CorbaRefCountImpl() throw ();

#ifndef NVALGRIND
      CORBA::ULong
      ref_count_(CORBA::Object* ptr) throw ();

      CORBA::ULong
      ref_count_(PortableServer::ServantBase* ptr) throw ();

      CORBA::ULong
      ref_count_(CORBA::ValueFactoryBase* ptr) throw ();

      CORBA::ULong
      ref_count_(CORBA::ValueBase* ptr) throw ();
#endif

    public:
      /* CORBA RefCountable */
      virtual
      void
      _add_ref() throw ();

      virtual
      void
      _remove_ref() throw ();

    public:
      /* ReferenceCounting::Interface */
      virtual
      void
      add_ref() const throw ();

      virtual
      void
      remove_ref() const throw ();
    };

    template <typename Object>
    class ServantImpl :
      public CorbaRefCountImpl<
        RefCountStrategySetter<Object, PortableServer::RefCountServantBase> >,
      private Generics::AllDestroyer<ServantImpl<Object> >
    {
    public:
      static const char PRINTABLE_NAME[];
    protected:
      virtual
      ~ServantImpl() throw ();
    };
  }
}

//
// INLINES
//

namespace CORBACommons
{
  namespace ReferenceCounting
  {
    namespace Helper
    {
    }

    /**X CorbaRefCountImpl */
    template <typename Object>
    CorbaRefCountImpl<Object>::CorbaRefCountImpl()
      /*throw (eh::Exception)*/
    {
    }

#ifndef NVALGRIND
    template <typename Object>
    CORBA::ULong
    CorbaRefCountImpl<Object>::ref_count_(CORBA::Object* ptr) throw ()
    {
      return ptr->_refcount_value();
    }

    template <typename Object>
    CORBA::ULong
    CorbaRefCountImpl<Object>::ref_count_(PortableServer::ServantBase* ptr)
      throw ()
    {
      return ptr->_refcount_value();
    }

    template <typename Object>
    CORBA::ULong
    CorbaRefCountImpl<Object>::ref_count_(CORBA::ValueFactoryBase* /*ptr*/)
      throw ()
    {
      return 0;//ptr->_tao_reference_count_.value();
    }

    template <typename Object>
    CORBA::ULong
    CorbaRefCountImpl<Object>::ref_count_(CORBA::ValueBase* ptr) throw ()
    {
      return ptr->_refcount_value();
    }
#endif

    template <typename Object>
    CorbaRefCountImpl<Object>::~CorbaRefCountImpl()
      throw()
    {
#ifndef NVALGRIND
      ::ReferenceCounting::RunningOnValgrind<>::check_ref_count(
        ref_count_(this));
#endif
    }

    template <typename Object>
    void
    CorbaRefCountImpl<Object>::add_ref() const throw ()
    {
      try
      {
        const_cast<CorbaRefCountImpl<Object>*>(this)->Object::_add_ref();
      }
      catch (...)
      {
      }
    }

    template <typename Object>
    void
    CorbaRefCountImpl<Object>::remove_ref() const throw ()
    {
      try
      {
        const_cast<CorbaRefCountImpl<Object>*>(this)->Object::_remove_ref();
      }
      catch (...)
      {
      }
    }

    template <typename Object>
    void
    CorbaRefCountImpl<Object>::_add_ref() throw ()
    {
      add_ref();
    }

    template <typename Object>
    void
    CorbaRefCountImpl<Object>::_remove_ref() throw ()
    {
      remove_ref();
    }

    //
    // ServantImpl class
    //

    template <typename Object>
    const char ServantImpl<Object>::PRINTABLE_NAME[] = "ServantImpl";

    template <typename Object>
    ServantImpl<Object>::~ServantImpl() throw ()
    {
    }
  } /* ReferenceCounting */
} /* CORBACommons */

#endif
