#include <iostream>
#include <sys/select.h>

int main()
{
  std::cout << FD_SETSIZE << std::endl;
}
