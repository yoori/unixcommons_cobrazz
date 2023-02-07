#include <userver/cache/cache_config.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/dump/config.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/string_to_duration.hpp>
#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace {

constexpr std::string_view kUpdateIntervalMs = "update-interval-ms";
constexpr std::string_view kUpdateJitterMs = "update-jitter-ms";
constexpr std::string_view kFullUpdateIntervalMs = "full-update-interval-ms";
constexpr std::string_view kExceptionIntervalMs = "exception-interval-ms";
constexpr std::string_view kUpdatesEnabled = "updates-enabled";
constexpr std::string_view kTaskProcessor = "task-processor";

constexpr std::string_view kUpdateInterval = "update-interval";
constexpr std::string_view kUpdateJitter = "update-jitter";
constexpr std::string_view kFullUpdateInterval = "full-update-interval";
constexpr std::string_view kExceptionInterval = "exception-interval";
constexpr std::string_view kCleanupInterval = "additional-cleanup-interval";
constexpr std::string_view kIsStrongPeriod = "is-strong-period";

constexpr std::string_view kFirstUpdateFailOk = "first-update-fail-ok";
constexpr std::string_view kUpdateTypes = "update-types";
constexpr std::string_view kForcePeriodicUpdates =
    "testsuite-force-periodic-update";
constexpr std::string_view kConfigSettings = "config-settings";

constexpr std::string_view kFirstUpdateMode = "first-update-mode";
constexpr std::string_view kFirstUpdateType = "first-update-type";

constexpr auto kDefaultCleanupInterval = std::chrono::seconds{10};

std::chrono::milliseconds GetDefaultJitter(std::chrono::milliseconds interval) {
  return interval / 10;
}

AllowedUpdateTypes ParseUpdateMode(const yaml_config::YamlConfig& config) {
  const auto update_types =
      config[kUpdateTypes].As<std::optional<AllowedUpdateTypes>>();
  if (update_types) return *update_types;

  if (config.HasMember(kFullUpdateInterval) &&
      config.HasMember(kUpdateInterval)) {
    return AllowedUpdateTypes::kFullAndIncremental;
  } else {
    return AllowedUpdateTypes::kOnlyFull;
  }
}

}  // namespace

using dump::impl::ParseMs;

FirstUpdateMode Parse(const yaml_config::YamlConfig& config,
                      formats::parse::To<FirstUpdateMode>) {
  const auto as_string = config.As<std::string>();

  if (as_string == "required") return FirstUpdateMode::kRequired;
  if (as_string == "best-effort") return FirstUpdateMode::kBestEffort;
  if (as_string == "skip") return FirstUpdateMode::kSkip;

  throw yaml_config::ParseException(fmt::format(
      "Invalid first update mode '{}' at '{}'", as_string, config.GetPath()));
}

FirstUpdateType Parse(const yaml_config::YamlConfig& config,
                      formats::parse::To<FirstUpdateType>) {
  const auto as_string = config.As<std::string>();

  if (as_string == "full") return FirstUpdateType::kFull;
  if (as_string == "incremental") return FirstUpdateType::kIncremental;
  if (as_string == "incremental-then-async-full")
    return FirstUpdateType::kIncrementalThenAsyncFull;

  throw yaml_config::ParseException(fmt::format(
      "Invalid first update type '{}' at '{}'", as_string, config.GetPath()));
}

ConfigPatch Parse(const formats::json::Value& value,
                  formats::parse::To<ConfigPatch>) {
  ConfigPatch config{ParseMs(value[kUpdateIntervalMs]),
                     ParseMs(value[kUpdateJitterMs]),
                     ParseMs(value[kFullUpdateIntervalMs]), std::nullopt,
                     value[kUpdatesEnabled].As<bool>(true)};

  if (!config.update_interval.count() && !config.full_update_interval.count()) {
    throw utils::impl::AttachTraceToException(
        ConfigError("Update interval is not set for cache"));
  } else if (!config.full_update_interval.count()) {
    config.full_update_interval = config.update_interval;
  } else if (!config.update_interval.count()) {
    config.update_interval = config.full_update_interval;
  }

  if (config.update_jitter > config.update_interval) {
    config.update_jitter = GetDefaultJitter(config.update_interval);
  }
  if (value.HasMember(kExceptionIntervalMs)) {
    config.exception_interval = ParseMs(value[kExceptionIntervalMs]);
  }

  return config;
}

Config::Config(const yaml_config::YamlConfig& config,
               const std::optional<dump::Config>& dump_config)
    : allowed_update_types(ParseUpdateMode(config)),
      allow_first_update_failure(config[kFirstUpdateFailOk].As<bool>(false)),
      force_periodic_update(
          config[kForcePeriodicUpdates].As<std::optional<bool>>()),
      config_updates_enabled(config[kConfigSettings].As<bool>(true)),
      task_processor_name(
          config[kTaskProcessor].As<std::optional<std::string>>()),
      cleanup_interval(config[kCleanupInterval].As<std::chrono::milliseconds>(
          kDefaultCleanupInterval)),
      is_strong_period(config[kIsStrongPeriod].As<bool>(false)),
      first_update_mode(
          config[dump::kDump][kFirstUpdateMode].As<FirstUpdateMode>(
              FirstUpdateMode::kSkip)),
      first_update_type(
          config[dump::kDump][kFirstUpdateType].As<FirstUpdateType>(
              FirstUpdateType::kFull)),
      update_interval(config[kUpdateInterval].As<std::chrono::milliseconds>(0)),
      update_jitter(config[kUpdateJitter].As<std::chrono::milliseconds>(
          GetDefaultJitter(update_interval))),
      full_update_interval(
          config[kFullUpdateInterval].As<std::chrono::milliseconds>(0)),
      exception_interval(config[kExceptionInterval]
                             .As<std::optional<std::chrono::milliseconds>>()),
      updates_enabled(config[kUpdatesEnabled].As<bool>(true)) {
  switch (allowed_update_types) {
    case AllowedUpdateTypes::kFullAndIncremental:
      if (!update_interval.count() || !full_update_interval.count()) {
        throw ConfigError(fmt::format("Both {} and {} must be set at '{}'",
                                      kUpdateInterval, kFullUpdateInterval,
                                      config.GetPath()));
      }
      if (update_interval >= full_update_interval) {
        LOG_WARNING() << "Incremental updates requested for cache at '"
                      << config.GetPath()
                      << "' but have lower frequency than full updates and "
                         "will never happen. Remove "
                      << kFullUpdateInterval
                      << " config field if this is intended.";
      }
      break;
    case AllowedUpdateTypes::kOnlyFull:
    case AllowedUpdateTypes::kOnlyIncremental:
      if (full_update_interval.count()) {
        throw ConfigError(fmt::format(
            "{} config field must only be used with full-and-incremental "
            "updated cache at '{}'. Please rename it to {}.",
            kFullUpdateInterval, config.GetPath(), kUpdateInterval));
      }
      if (!update_interval.count()) {
        throw ConfigError(fmt::format("{} is not set for cache at '{}'",
                                      kUpdateInterval, config.GetPath()));
      }
      full_update_interval = update_interval;
      break;
  }

  if (config.HasMember(dump::kDump)) {
    if (!config[dump::kDump].HasMember(kFirstUpdateMode)) {
      throw ConfigError(fmt::format(
          "If dumps are enabled, then '{}' must be set for cache at '{}'",
          kFirstUpdateMode, config.GetPath()));
    }

    if (first_update_mode != FirstUpdateMode::kRequired &&
        !dump_config->max_dump_age_set) {
      throw ConfigError(fmt::format(
          "If '{}' is not 'required', then '{}' must be set for cache at '{}'. "
          "If using severely outdated data is not harmful for this cache, "
          "please add to config.yaml: '{}:  # outdated data is not harmful'",
          kFirstUpdateMode, dump::kMaxDumpAge, config.GetPath(),
          dump::kMaxDumpAge, dump::kMaxDumpAge));
    }

    if (first_update_mode == FirstUpdateMode::kSkip) {
      if (config[dump::kDump].HasMember(kFirstUpdateType)) {
        LOG_WARNING() << fmt::format(
            "If '{}' is 'skip' for cache at '{}', setting '{}' is meaningless",
            kFirstUpdateMode, config.GetPath(), kFirstUpdateType);
      }
    }

    if (allowed_update_types == AllowedUpdateTypes::kOnlyFull) {
      if (first_update_type != FirstUpdateType::kFull) {
        throw ConfigError(fmt::format(
            "Cache at '{}' can't perform the update specified in '{}'",
            config.GetPath(), kFirstUpdateType));
      }
    } else if (first_update_mode != FirstUpdateMode::kSkip) {
      if (!config[dump::kDump].HasMember(kFirstUpdateType)) {
        throw ConfigError(fmt::format("'{}' must be set for cache at '{}'",
                                      kFirstUpdateType, config.GetPath()));
      }
    }
  }
}

Config Config::MergeWith(const ConfigPatch& patch) const {
  Config copy = *this;
  copy.update_interval = patch.update_interval;
  copy.update_jitter = patch.update_jitter;
  copy.full_update_interval = patch.full_update_interval;
  copy.updates_enabled = patch.updates_enabled;
  if (patch.exception_interval)
    copy.exception_interval = patch.exception_interval;
  return copy;
}

std::unordered_map<std::string, ConfigPatch> ParseCacheConfigSet(
    const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_CACHES")
      .As<std::unordered_map<std::string, ConfigPatch>>();
}

}  // namespace cache

USERVER_NAMESPACE_END
