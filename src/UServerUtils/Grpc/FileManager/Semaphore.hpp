#ifndef USERVER_FILEMANAGER_SEMAPHORE_HPP
#define USERVER_FILEMANAGER_SEMAPHORE_HPP

// POSIX
#include <sys/eventfd.h>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::Grpc::FileManager
{

class Semaphore final : private Generics::Uncopyable
{
public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Semaphore(
    const bool is_nonblocking,
    const std::uint32_t init_value);

  ~Semaphore();

  int fd() const noexcept;

  bool add() const noexcept;

  bool fetch() const noexcept;

private:
  int descriptor_ = -1;
};

} // namespace UServerUtils::Grpc::FileManager

#endif // USERVER_FILEMANAGER_SEMAPHORE_HPP
