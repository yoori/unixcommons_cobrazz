#include <unistd.h>

#include <iostream>
#include <fstream>

#include <Generics/CRC.hpp>
#include <Logger/StreamLogger.hpp>

int main(int argc, char** argv)
{
  const size_t CrcBufLength = 1024 * 1024;
  int quant = 0;
  bool has_expected;
  unsigned long expected = 0;

  if (argc > 1)
  {
    quant = atoi(argv[1]);

    if (argc > 2)
    {
      has_expected = true;
      expected = strtoull(argv[2], 0, 10);
    }

    if (!quant)
    {
      std::cerr << "Calculates CRC32. Acts as a filter. Maximum file size is "
        << CrcBufLength << " bytes" << std::endl;
      std::cerr << "Accepts command line argument (if numeric) as quantifying "
        "factor: calculates as many times as specified for speed measurement "
        "purposes" << std::endl;
      exit(3);
    }
  }

  unsigned char buffer[CrcBufLength];
  unsigned long bufLen;

  if (argc > 3)
  {
    std::ifstream in(argv[3]);
    if (!in)
    {
      std::cerr << "Failed to open " << argv[3] << std::endl;
      return 1;
    }
    in.read((char*)buffer, sizeof(buffer));
    bufLen = in.gcount();
  }
  else
  {
    std::cin.read((char*)buffer, sizeof(buffer));
    bufLen = std::cin.gcount();
  }

  unsigned long crc = 0;

  for (int i = 0; i < (quant ? quant : 1); ++i)
  {
    crc = Generics::CRC::quick(0, buffer, bufLen);
  }

  if (has_expected)
  {
    if (crc != expected)
    {
      std::cerr << "Got " << crc << " while expecting " << expected << std::endl;
      return 1;
    }
  }
  else
  {
    std::cout << crc << std::endl;
  }
}
