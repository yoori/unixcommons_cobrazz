#pragma once

// THIS
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

namespace UServerUtils::Grpc
{
  class Component: public virtual Generics::ActiveObject
  {
  public:
    enum class HealthStatus
    {
      Ok,
      Fail
    };

  public:
    virtual HealthStatus get_health() const
    {
      return HealthStatus::Ok;
    }

  protected:
    Component() = default;

    ~Component() override = default;
  };

  using Component_var = ReferenceCounting::SmartPtr<Component>;

} // namespace UServerUtils::Grpc
