#include <userver/storages/secdist/secdist.hpp>

#include <cerrno>
#include <fstream>

#include <userver/compiler/demangle.hpp>
#include <userver/concurrent/async_event_channel.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/secdist/exceptions.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

static formats::json::Value Parse(const formats::yaml::Value& yaml,
                                  formats::parse::To<formats::json::Value>) {
  formats::json::ValueBuilder json_vb;

  if (yaml.IsBool()) {
    json_vb = yaml.As<bool>();
  } else if (yaml.IsInt64()) {
    json_vb = yaml.As<int64_t>();
  } else if (yaml.IsUInt64()) {
    json_vb = yaml.As<uint64_t>();
  } else if (yaml.IsDouble()) {
    json_vb = yaml.As<double>();
  } else if (yaml.IsString()) {
    json_vb = yaml.As<std::string>();
  } else if (yaml.IsArray()) {
    json_vb = {formats::common::Type::kArray};
    for (const auto& elem : yaml) {
      json_vb.PushBack(elem.As<formats::json::Value>());
    }
  } else if (yaml.IsObject()) {
    json_vb = {formats::common::Type::kObject};
    for (auto it = yaml.begin(); it != yaml.end(); ++it) {
      json_vb[it.GetName()] = it->As<formats::json::Value>();
    }
  }
  return json_vb.ExtractValue();
}

}  // namespace formats::parse

namespace storages::secdist {

namespace {

std::vector<std::function<std::any(const formats::json::Value&)>>&
GetConfigFactories() {
  static std::vector<std::function<std::any(const formats::json::Value&)>>
      factories;
  return factories;
}

formats::json::Value DoLoadFromFile(const std::string& path,
                                    SecdistFormat format, bool missing_ok) {
  formats::json::Value doc;
  if (path.empty()) return doc;

  std::ifstream stream(path);
  try {
    if (format == SecdistFormat::kJson) {
      doc = formats::json::FromStream(stream);
    } else if (format == SecdistFormat::kYaml) {
      const auto yaml_doc = formats::yaml::FromStream(stream);
      doc = yaml_doc.As<formats::json::Value>();
    }
  } catch (const std::exception& e) {
    if (missing_ok) {
      LOG_WARNING() << "Failed to load secdist from file: " << e
                    << ", booting without secdist";
    } else {
      throw SecdistError(
          "Cannot load secdist config. File '" + path +
          "' doesn't exist, unrechable or in invalid format:" + e.what());
    }
  }

  return doc;
}

formats::json::Value LoadFromFile(
    const std::string& path, SecdistFormat format, bool missing_ok,
    engine::TaskProcessor* blocking_task_processor) {
  if (blocking_task_processor)
    return utils::Async(*blocking_task_processor, "load_secdist_from_file",
                        &DoLoadFromFile, std::cref(path), format, missing_ok)
        .Get();
  else
    return DoLoadFromFile(path, format, missing_ok);
}

void MergeJsonObj(formats::json::ValueBuilder& builder,
                  const formats::json::Value& update) {
  if (!update.IsObject()) {
    builder = update;
    return;
  }

  for (auto it = update.begin(); it != update.end(); ++it) {
    auto sub_node = builder[it.GetName()];
    MergeJsonObj(sub_node, *it);
  }
}

void UpdateFromEnv(formats::json::Value& doc,
                   const std::optional<std::string>& environment_secrets_key) {
  if (!environment_secrets_key) return;

  const auto& env_vars =
      engine::subprocess::GetCurrentEnvironmentVariablesPtr();
  const auto* value = env_vars->GetValueOptional(*environment_secrets_key);
  if (value) {
    formats::json::Value value_json;
    try {
      value_json = formats::json::FromString(*value);
    } catch (const std::exception& ex) {
      throw SecdistError("Can't parse '" + *environment_secrets_key +
                         "' env variable: " + ex.what());
    }
    formats::json::ValueBuilder doc_builder(doc);
    MergeJsonObj(doc_builder, value_json);
    doc = doc_builder.ExtractValue();
  }
}

}  // namespace

SecdistConfig::SecdistConfig() = default;

SecdistConfig::SecdistConfig(const SecdistConfig::Settings& settings) {
  // if we don't want to read secdist, then we don't need to initialize
  if (GetConfigFactories().empty()) return;

  auto doc =
      LoadFromFile(settings.config_path, settings.format, settings.missing_ok,
                   settings.blocking_task_processor);
  UpdateFromEnv(doc, settings.environment_secrets_key);

  Init(doc);
}

void SecdistConfig::Init(const formats::json::Value& doc) {
  for (const auto& config_factory : GetConfigFactories()) {
    configs_.emplace_back(config_factory(doc));
  }
}

std::size_t SecdistConfig::Register(
    std::function<std::any(const formats::json::Value&)>&& factory) {
  auto& config_factories = GetConfigFactories();
  config_factories.emplace_back(std::move(factory));
  return config_factories.size() - 1;
}

const std::any& SecdistConfig::Get(const std::type_index& type,
                                   std::size_t index) const {
  try {
    return configs_.at(index);
  } catch (const std::out_of_range&) {
    throw std::out_of_range("Type " + compiler::GetTypeName(type) +
                            " is not registered as config");
  }
}

class Secdist::Impl {
 public:
  explicit Impl(SecdistConfig::Settings settings);
  ~Impl();

  const storages::secdist::SecdistConfig& Get() const;

  rcu::ReadablePtr<storages::secdist::SecdistConfig> GetSnapshot() const;

  bool IsPeriodicUpdateEnabled() const;

  void EnsurePeriodicUpdateEnabled(const std::string& msg) const;

  concurrent::AsyncEventSubscriberScope DoUpdateAndListen(
      concurrent::FunctionId id, std::string_view name,
      EventSource::Function&& func);

 private:
  void StartUpdateTask();

  storages::secdist::SecdistConfig::Settings settings_;

  SecdistConfig secdist_config_;
  rcu::Variable<storages::secdist::SecdistConfig> dynamic_secdist_config_;
  concurrent::AsyncEventChannel<const SecdistConfig&> channel_{"secdist"};
  utils::PeriodicTask update_task_;
};

Secdist::Impl::Impl(SecdistConfig::Settings settings)
    : settings_(std::move(settings)) {
  if (IsPeriodicUpdateEnabled() && !settings_.blocking_task_processor) {
    throw SecdistError(
        "'blocking-task-processor' is required for periodic updates");
  }
  dynamic_secdist_config_.Assign(storages::secdist::SecdistConfig(settings_));

  if (IsPeriodicUpdateEnabled()) {
    StartUpdateTask();
  }

  secdist_config_ = dynamic_secdist_config_.ReadCopy();
}

Secdist::Impl::~Impl() {
  if (IsPeriodicUpdateEnabled()) update_task_.Stop();
}

const SecdistConfig& Secdist::Impl::Get() const { return secdist_config_; }

rcu::ReadablePtr<SecdistConfig> Secdist::Impl::GetSnapshot() const {
  return dynamic_secdist_config_.Read();
}

bool Secdist::Impl::IsPeriodicUpdateEnabled() const {
  return settings_.update_period != std::chrono::milliseconds::zero();
}

concurrent::AsyncEventSubscriberScope Secdist::Impl::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    EventSource::Function&& func) {
  EnsurePeriodicUpdateEnabled(
      "Secdist update must be enabled to subscribe on it");
  auto func_copy = func;
  return channel_.DoUpdateAndListen(id, name, std::move(func), [&] {
    const auto snapshot = GetSnapshot();
    func_copy(*snapshot);
  });
}

void Secdist::Impl::EnsurePeriodicUpdateEnabled(const std::string& msg) const {
  if (!IsPeriodicUpdateEnabled()) {
    throw SecdistError(msg);
  }
}

void Secdist::Impl::StartUpdateTask() {
  LOG_INFO() << "Start task for secdist periodic updates";
  utils::PeriodicTask::Settings periodic_settings(
      settings_.update_period, {utils::PeriodicTask::Flags::kCritical});
  update_task_.Start("secdist_update", periodic_settings, [this]() {
    try {
      dynamic_secdist_config_.Assign(
          storages::secdist::SecdistConfig(settings_));
      auto snapshot = dynamic_secdist_config_.Read();
      channel_.SendEvent(*snapshot);
    } catch (const std::exception& ex) {
      LOG_ERROR() << "Secdist loading failed: " << ex;
    }
  });
}

Secdist::Secdist(SecdistConfig::Settings settings)
    : impl_(std::move(settings)) {}

Secdist::~Secdist() = default;

const SecdistConfig& Secdist::Get() const { return impl_->Get(); }

rcu::ReadablePtr<SecdistConfig> Secdist::GetSnapshot() const {
  return impl_->GetSnapshot();
}

bool Secdist::IsPeriodicUpdateEnabled() const {
  return impl_->IsPeriodicUpdateEnabled();
}

concurrent::AsyncEventSubscriberScope Secdist::DoUpdateAndListen(
    concurrent::FunctionId id, std::string_view name,
    EventSource::Function&& func) {
  return impl_->DoUpdateAndListen(id, name, std::move(func));
}

}  // namespace storages::secdist

USERVER_NAMESPACE_END
