#pragma once

#include <sys/select.h>


namespace Generics
{
  union FDSet
  {
  public:
    FDSet() noexcept;

    fd_set*
    operator &() noexcept;

    const fd_set*
    operator &() const noexcept;

  private:
    unsigned data_[16384 / (8 * sizeof(unsigned))];
    fd_set set_;
  };
};

namespace Generics
{
  FDSet::FDSet() noexcept
  {
    for (unsigned i = 0; i < sizeof(data_) / sizeof(*data_); i++)
    {
      data_[i] = 0;
    }
  }

  fd_set*
  FDSet::operator &() noexcept
  {
    return &set_;
  }

  const fd_set*
  FDSet::operator &() const noexcept
  {
    return &set_;
  }
}
