// Generics/CompositeActiveObject.cpp
#include <Generics/CompositeActiveObject.hpp>


namespace Generics
{
  //
  // RefCountableCompositeActiveObject class
  //

  RefCountableCompositeActiveObject::RefCountableCompositeActiveObject(
    bool sync_termination,
    bool clear_on_exit) noexcept
    : CompositeActiveObject(sync_termination, clear_on_exit)
  {
  }

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

  void CompositeSetActiveObject::remove_child_(ActiveObject* child) noexcept
  {
    Sync::PosixGuard guard(cond_);
    this->child_objects_.erase(child);
  }


  //
  // RemovableActiveObject class
  //

  RemovableActiveObject::RemovableActiveObject( ActiveObjectChildRemover* owner) noexcept
    : owner_(ReferenceCounting::add_ref(owner))
  {
  }

  void RemovableActiveObject::delete_this_() const noexcept
  {
    if (owner_)
    {
      RemovableActiveObject* ths = const_cast<RemovableActiveObject*>(this);
      ths->before_remove_child_();
      ths->owner_->remove_child_(ths);
    }
    AtomicImpl::delete_this_();
  }

  void RemovableActiveObject::before_remove_child_() noexcept
  {
  }
}
