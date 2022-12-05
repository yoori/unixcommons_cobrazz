#include <TestCommons/Memory.hpp>


namespace TestCommons
{
  void
  print_mallinfo(std::ostream& ostr, struct mallinfo* info)
    /*throw (eh::Exception)*/
  {
    struct mallinfo real_info;
    if (!info)
    {
      real_info = mallinfo();
      info = &real_info;
    }

    ostr <<
      " non-mmapped space allocated from system " << info->arena << "\n" <<
      " number of free chunks " << info->ordblks << "\n" <<
      " number of fastbin blocks " << info->smblks << "\n" <<
      " number of mmapped regions " << info->hblks << "\n" <<
      " space in mmapped regions " << info->hblkhd << "\n" <<
      " maximum total allocated space " << info->usmblks << "\n" <<
      " space available in freed fastbin blocks " << info->fsmblks << "\n" <<
      " total allocated space " << info->uordblks << "\n" <<
      " total free space " << info->fordblks << "\n" <<
      " top-most, releasable (via malloc_trim) space " << info->keepcost <<
      "\n" << std::endl;
  }
}
