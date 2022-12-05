#include <iostream>

#include <PrivacyFilter/Filter.hpp>

int
main(int argc, char* argv[])
{
  std::cout << PrivacyFilter::filter(
    argc > 1 ? argv[1] : "A",
    argc > 2 ? argv[2] : "B");
  return 0;
}
