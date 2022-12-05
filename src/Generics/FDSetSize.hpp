#ifndef GENERICS_FDSETSIZE
#define GENERICS_FDSETSIZE

#include <sys/select.h>


namespace Generics
{
  union FDSet
  {
  public:
    FDSet() throw ();

    fd_set*
    operator &() throw ();

    const fd_set*
    operator &() const throw ();

  private:
    unsigned data_[16384 / (8 * sizeof(unsigned))];
    fd_set set_;
  };
};

namespace Generics
{
  FDSet::FDSet() throw ()
  {
    for (unsigned i = 0; i < sizeof(data_) / sizeof(*data_); i++)
    {
      data_[i] = 0;
    }
  }

  fd_set*
  FDSet::operator &() throw ()
  {
    return &set_;
  }

  const fd_set*
  FDSet::operator &() const throw ()
  {
    return &set_;
  }
}

#endif
