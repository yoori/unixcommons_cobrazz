#ifndef USERVER_FILEMANAGER_SEMAPHORE_HPP
#define USERVER_FILEMANAGER_SEMAPHORE_HPP

// POSIX
#include <sys/eventfd.h>

// THIS
#include <eh/Exception.hpp>
#include <Generics/Uncopyable.hpp>

namespace UServerUtils::FileManager
{

// Thread safe
class Semaphore final : private Generics::Uncopyable
{
public:
  enum class Type
  {
    NonBlocking,
    Blocking
  };

  class NonBlockingScope;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit Semaphore(
    const Type type,
    const std::uint32_t init_value);

  ~Semaphore();

  int fd() const noexcept;

  bool add() const noexcept;

  bool consume() const noexcept;

  // This method can be called only from one thread
  std::uint32_t try_consume(const std::uint32_t v) const noexcept;

private:
  const Type type_ = Type::NonBlocking;

  int fd_ = -1;
};

class Semaphore::NonBlockingScope
{
public:
  NonBlockingScope(const int descriptor) noexcept;

  ~NonBlockingScope();

private:
  int add_flags(
    const int fd,
    const int flags) const noexcept;

  int set_flags(
    const int fd,
    const int flags) const noexcept;

private:
  const int fd_ = -1;

  int old_flags_ = -1;
};

} // namespace UServerUtils::FileManager

#include <UServerUtils/FileManager/Semaphore.ipp>

#endif // USERVER_FILEMANAGER_SEMAPHORE_HPP
