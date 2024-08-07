#ifndef USERVER_REGISTRATORDYNAMICSETTINGS_HPP
#define USERVER_REGISTRATORDYNAMICSETTINGS_HPP

// USERVER
#include <userver/dynamic_config/value.hpp>

// THIS
#include <Generics/Uncopyable.hpp>

namespace UServerUtils
{

class RegistratorDynamicSettings :
  private Generics::Uncopyable
{
public:
  using DocsMap = userver::dynamic_config::DocsMap;

public:
  RegistratorDynamicSettings();

  ~RegistratorDynamicSettings() = default;

  DocsMap& docs_map() noexcept;

private:
  void registrate();

private:
  DocsMap docs_map_;
};

using RegistratorDynamicSettingsPtr = std::shared_ptr<RegistratorDynamicSettings>;

} // namespace UServerUtils

#endif // USERVER_REGISTRATORDYNAMICSETTINGS_HPP