#ifndef USERVER_FILEMANAGER_IOURING_HPP
#define USERVER_FILEMANAGER_IOURING_HPP

// POSIX
#include <liburing.h>

// STD
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/Grpc/FileManager/Config.hpp>

namespace UServerUtils::Grpc::FileManager
{

class IoUring final : private Generics::Uncopyable
{
private:
  using Version = std::vector<std::uint32_t>;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit IoUring(const Config& config);

  ~IoUring();

  const io_uring_params& params() const noexcept;

  io_uring* get() noexcept;

private:
  Version linux_kernel_version() const;

private:
  io_uring_params params_;

  io_uring ring_;
};

} // namespace UServerUtils::Grpc::FileManager

#endif // USERVER_FILEMANAGER_IOURING_HPP
