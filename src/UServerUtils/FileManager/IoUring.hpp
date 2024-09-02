#ifndef USERVER_FILEMANAGER_IOURING_HPP
#define USERVER_FILEMANAGER_IOURING_HPP

// POSIX
#include <liburing.h>

// STD
#include <optional>
#include <vector>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>
#include <UServerUtils/FileManager/Config.hpp>

namespace UServerUtils::FileManager
{

class IoUring final : private Generics::Uncopyable
{
private:
  using Version = std::vector<std::uint32_t>;
  using UringFd = std::uint32_t;

public:
  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit IoUring(
    const Config& config,
    const std::optional<UringFd> uring_fd = {});

  ~IoUring();

  const io_uring_params& params() const noexcept;

  io_uring* get() noexcept;

  std::size_t sq_size() const noexcept;

  std::size_t cq_size() const noexcept;

private:
  Version linux_kernel_version() const;

private:
  io_uring_params params_;

  io_uring ring_;
};

} // namespace UServerUtils::FileManager

#include <UServerUtils/FileManager/IoUring.ipp>

#endif // USERVER_FILEMANAGER_IOURING_HPP
