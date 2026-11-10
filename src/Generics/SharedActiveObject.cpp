#include <Generics/SharedActiveObject.hpp>

#include <cassert>
#include <utility>

namespace Generics
{
  SharedActiveObject::ActivateGuard::ActivateGuard(std::shared_ptr<SharedActiveObject> owner)
    noexcept
    : owner_(std::move(owner))
  {}

  SharedActiveObject::ActivateGuard::ActivateGuard(ActivateGuard&& guard) noexcept
    : owner_(std::move(guard.owner_))
  {}

  SharedActiveObject::ActivateGuard&
  SharedActiveObject::ActivateGuard::operator=(ActivateGuard&& guard) noexcept
  {
    if (this != &guard)
    {
      reset();
      owner_ = std::move(guard.owner_);
    }

    return *this;
  }

  SharedActiveObject::ActivateGuard::~ActivateGuard() noexcept
  {
    reset();
  }

  SharedActiveObject::ActivateGuard::operator bool() const noexcept
  {
    return static_cast<bool>(owner_);
  }

  void SharedActiveObject::ActivateGuard::reset() noexcept
  {
    if (auto owner = std::move(owner_))
    {
      owner->release_guard_();
    }
  }

  SharedActiveObject::~SharedActiveObject() noexcept
  {
    assert(activations_ == 0);
  }

  SharedActiveObject::ActivateGuard SharedActiveObject::activate_guard()
  {
    auto owner = shared_from_this();
    Sync::PosixGuard lock(cond_);
    if (guards_closed_)
    {
      throw NotSupported("SharedActiveObject is stopping");
    }

    ++activations_;
    return ActivateGuard(std::move(owner));
  }

  void SharedActiveObject::activate_object_()
  {
    // SimpleActiveObject invokes this hook while holding cond_.
    if (guards_closed_)
    {
      throw AlreadyActive("SharedActiveObject has already been stopped");
    }

    ++activations_;
    own_activation_ = true;
    try
    {
      activate_shared_object_();
    }
    catch (...)
    {
      own_activation_ = false;
      --activations_;
      throw;
    }
  }

  void SharedActiveObject::deactivate_object()
  {
    {
      Sync::PosixGuard lock(cond_);
      // Close reservations even if explicit activation has not happened yet.
      guards_closed_ = true;
    }

    SimpleActiveObject::deactivate_object();
  }

  void SharedActiveObject::wait_object()
  {
    {
      Sync::ConditionalGuard lock(cond_);
      while (state_ == AS_ACTIVE)
      {
        lock.wait();
      }

      if (!guards_closed_)
      {
        return;
      }

      if (own_activation_)
      {
        own_activation_ = false;
        --activations_;
      }

      if (activations_ != 0)
      {
        return;
      }
    }

    finish_();
    if (error_)
    {
      std::rethrow_exception(error_);
    }
  }

  void SharedActiveObject::activate_shared_object_()
  {}

  void SharedActiveObject::release_guard_() noexcept
  {
    {
      Sync::PosixGuard lock(cond_);
      assert(activations_ != 0);
      --activations_;
      if (activations_ != 0 || !guards_closed_)
      {
        return;
      }
    }

    finish_();
  }

  void SharedActiveObject::finish_() noexcept
  {
    std::call_once(finish_once_, [this]
      {
        try
        {
          SimpleActiveObject::wait_object();
        }
        catch (...)
        {
          error_ = std::current_exception();
          // Guard destruction cannot retry or propagate a failed shutdown.
          Sync::PosixGuard lock(cond_);
          state_ = AS_NOT_ACTIVE;
        }
      });
  }
}
