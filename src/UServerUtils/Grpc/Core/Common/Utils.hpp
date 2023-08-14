#ifndef GRPC_CORE_COMMON_UTILS_H_
#define GRPC_CORE_COMMON_UTILS_H_

// STD
#include <algorithm>
#include <list>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>

// THIS
#include <Logger/Logger.hpp>
#include <UServerUtils/Grpc/Core/Common/Scheduler.hpp>

namespace UServerUtils::Grpc::Core::Common::Utils
{

namespace Aspect
{

constexpr const char UTILS[] = "FACTORY";

} // namespace Aspect

inline bool is_integer(std::string_view str)
{
  return !str.empty() &&
    std::find_if(str.begin(), str.end(),
      [] (const auto ch) {
        return !std::isdigit(ch);}) == str.end();
}

inline std::list<std::string_view> split(
  const std::string_view str,
  const std::string_view delimeter)
{
  std::size_t begin = 0;
  const auto size = str.size();
  const auto size_delimeter = delimeter.size();

  std::list<std::string_view> result;
  while (begin < size)
  {
    const auto end = str.find(delimeter, begin);
    auto data = str.substr(begin, end - begin);
    result.emplace_back(data);

    if (end == std::string_view::npos)
      break;

    begin = end + size_delimeter;
  }

  return result;
}

inline auto create_scheduler(
  std::optional<std::size_t> number_threads,
  Logging::Logger* logger)
{
  //using SchedulerQueue = typename Common::Scheduler::Queue;
  using SchedulerQueues = typename Common::Scheduler::Queues;

  if (!number_threads)
  {
    const auto best_thread_number =
      std::thread::hardware_concurrency();
    if (best_thread_number == 0)
    {
      std::stringstream stream;
      stream << FNS
             << ": hardware_concurrency is failed";
      logger->error(stream.str(), Aspect::UTILS);
    }
    number_threads =
      best_thread_number ? best_thread_number : 25;
  }

  SchedulerQueues scheduler_queues;
  scheduler_queues.reserve(*number_threads);
  for (std::size_t i = 1; i <= *number_threads; ++i)
  {
    auto completion_queue = std::make_shared<grpc::CompletionQueue>();
    scheduler_queues.emplace_back(std::move(completion_queue));
  }

  Common::SchedulerPtr scheduler(
    new Common::Scheduler(
      logger,
      std::move(scheduler_queues)));

  return scheduler;
}

} // namespace UServerUtils::Grpc::Core::Common::Utils

#endif // GRPC_CORE_COMMON_UTILS_H_
