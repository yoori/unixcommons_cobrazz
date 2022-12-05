// Generics/CompositeActiveObject.cpp
#include <Generics/CompositeActiveObject.hpp>


namespace Generics
{
  //
  // CompositeSetActiveObject class
  //

  CompositeSetActiveObject::CompositeSetActiveObject(bool sync_termination)
    /*throw (eh::Exception)*/
    : CompositeActiveObjectBase<std::set<ActiveObject*>,
        Inserter<std::set<ActiveObject*>>,
        Inserter<std::set<ActiveObject*>>>(sync_termination)
  {
  }

  void
  CompositeSetActiveObject::remove_child_(ActiveObject* child) throw ()
  {
    Sync::PosixGuard guard(cond_);
    this->child_objects_.erase(child);
  }


  //
  // RemovableActiveObject class
  //

  RemovableActiveObject::RemovableActiveObject(
    ActiveObjectChildRemover* owner) throw ()
    : owner_(ReferenceCounting::add_ref(owner))
  {
  }

  void
  RemovableActiveObject::delete_this_() const throw ()
  {
    if (owner_)
    {
      RemovableActiveObject* ths =
        const_cast<RemovableActiveObject*>(this);
      ths->before_remove_child_();
      ths->owner_->remove_child_(ths);
    }
    AtomicImpl::delete_this_();
  }

  void
  RemovableActiveObject::before_remove_child_() throw ()
  {
  }
}
