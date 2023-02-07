#pragma once

#include <array>
#include <atomic>

USERVER_NAMESPACE_BEGIN

namespace logging {

inline auto& GetShouldLogCache() noexcept {
  static std::array<std::atomic<bool>, kLevelMax + 1> values{
      false, false, true, true, true, true, false};
  return values;
}

}  // namespace logging

USERVER_NAMESPACE_END
