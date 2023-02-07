#pragma once

/// @file userver/cache/cache_statistics.hpp
/// @brief Statistics collection for components::CachingComponentBase

#include <atomic>
#include <chrono>
#include <cstddef>

#include <userver/cache/update_type.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache {

namespace impl {

struct UpdateStatistics final {
  std::atomic<std::size_t> update_attempt_count{0};
  std::atomic<std::size_t> update_no_changes_count{0};
  std::atomic<std::size_t> update_failures_count{0};

  std::atomic<std::size_t> documents_read_count{0};
  std::atomic<std::size_t> documents_parse_failures{0};

  std::atomic<std::chrono::steady_clock::time_point> last_update_start_time{{}};
  std::atomic<std::chrono::steady_clock::time_point>
      last_successful_update_start_time{{}};
  std::atomic<std::chrono::milliseconds> last_update_duration{{}};
};

formats::json::Value Serialize(const UpdateStatistics& stats,
                               formats::serialize::To<formats::json::Value>);

struct Statistics final {
  UpdateStatistics full_update;
  UpdateStatistics incremental_update;
  std::atomic<std::size_t> documents_current_count{0};
};

formats::json::Value Serialize(const Statistics& stats,
                               formats::serialize::To<formats::json::Value>);

}  // namespace impl

/// @brief Allows a specific cache to fill cache statistics during an `Update`
///
/// Unless Finish or FinishNoChanges is called, the update is considered to be a
/// failure.
class UpdateStatisticsScope final {
 public:
  /// @cond
  // For internal use only
  UpdateStatisticsScope(impl::Statistics& stats, cache::UpdateType type);

  ~UpdateStatisticsScope();
  /// @endcond

  /// @brief Mark that the `Update` has finished with changes
  /// @param documents_count the new total number of items stored in the cache
  void Finish(std::size_t documents_count);

  /// @brief Mark that the `Update` has finished without changes
  void FinishNoChanges();

  /// @brief Each item received from the data source should be accounted with
  /// this function
  /// @note This method can be called multiple times per `Update`
  /// @param add the number of items (both valid and non-valid) newly received
  void IncreaseDocumentsReadCount(std::size_t add);

  /// @brief Each received item that failed validation should be accounted with
  /// this function, in addition to IncreaseDocumentsReadCount
  /// @note This method can be called multiple times per `Update`
  /// @param add the number of non-valid items newly received
  void IncreaseDocumentsParseFailures(std::size_t add);

 private:
  impl::Statistics& stats_;
  impl::UpdateStatistics& update_stats_;
  bool finished_{false};
  const std::chrono::steady_clock::time_point update_start_time_;
};

}  // namespace cache

USERVER_NAMESPACE_END
