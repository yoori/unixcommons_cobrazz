#include <iostream>
#include <cstdlib>

#include <dlfcn.h>


/*
  Keep one reference to libACE.so (see ADSC-2465).
*/
static
void
__attribute__((__constructor__))
init()
{
  if (!dlopen("libACE.so.6.2.1", RTLD_LAZY | RTLD_LOCAL))
  {
    std::cout << "PreloadACE: dlopen(): " << dlerror() << std::endl;
    std::exit(1);
  }
}
