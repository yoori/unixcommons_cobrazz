#ifndef GRPC_CORE_COMMON_UTILS_H_
#define GRPC_CORE_COMMON_UTILS_H_

// STD
#include <algorithm>
#include <list>
#include <string_view>

namespace UServerUtils::Grpc::Core::Common::Utils
{

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

} // namespace UServerUtils::Grpc::Core::Common::Utils

#endif // GRPC_CORE_COMMON_UTILS_H_
