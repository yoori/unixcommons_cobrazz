#ifndef USERVER_GRPC_COMPONENT_HPP
#define USERVER_GRPC_COMPONENT_HPP

// THIS
#include <Generics/CompositeActiveObject.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>

namespace UServerUtils::Grpc
{

class Component : public Generics::CompositeActiveObject
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

#endif //USERVER_GRPC_COMPONENT_HPP