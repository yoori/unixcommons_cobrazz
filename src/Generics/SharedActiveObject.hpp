#pragma once

#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>

#include <Generics/ActiveObject.hpp>

namespace Generics
{
  class SharedActiveObject:
    public SimpleActiveObject,
    public std::enable_shared_from_this<SharedActiveObject>
  {
  public:
    class ActivateGuard
    {
    public:
      ActivateGuard() noexcept = default;
      ActivateGuard(ActivateGuard&& guard) noexcept;
      ActivateGuard& operator=(ActivateGuard&& guard) noexcept;
      ActivateGuard(const ActivateGuard&) = delete;
      ActivateGuard& operator=(const ActivateGuard&) = delete;
      ~ActivateGuard() noexcept;

      explicit operator bool() const noexcept;
      // Release from the consumer's wait_object(), after its last use of the resource.
      void reset() noexcept;

    private:
      friend class SharedActiveObject;
      explicit ActivateGuard(std::shared_ptr<SharedActiveObject> owner) noexcept;

      std::shared_ptr<SharedActiveObject> owner_;
    };

    // Guards can reserve activity before explicit activation, but never after deactivation.
    ActivateGuard activate_guard();
    void deactivate_object() override;
    // Release the explicit activation. Remaining guards defer final shutdown to their owner.
    void wait_object() override;

  protected:
    SharedActiveObject() = default;
    ~SharedActiveObject() noexcept override;

    virtual void activate_shared_object_();
    void wait_object_() override = 0;

  private:
    void activate_object_() final;

    void release_guard_() noexcept;
    void finish_() noexcept;

    std::once_flag finish_once_;
    bool guards_closed_ = false;
    std::size_t activations_ = 0;
    bool own_activation_ = false;
    std::exception_ptr error_;
  };
}
