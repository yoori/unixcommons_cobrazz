#include <iostream>
#include <sstream>
#include <stdlib.h>

#include <Stream/MemoryStream.hpp>


void
test_output(std::string& result) /*throw (eh::Exception)*/
{
  std::ostringstream ostr;
  Stream::MemoryStream::OutputMemoryStream<char> omem;
  int symbols = 0;

  for (int i = 0; i < 1000; i++)
  {
    std::string out;
    for (int j = rand() % 100; j >= 0; j--)
    {
      out.push_back(rand () % 254 + 1);
      symbols++;
    }
    ostr << out;
    omem << out;
    if (ostr.str() != omem.str())
    {
      std::cerr << "Failure after output of '" << out << "'" << std::endl;
      return;
    }
  }

  result = ostr.str();

  std::cout << symbols << " symbols sent to output" << std::endl;
}

void
test_input(const char* result) /*throw (eh::Exception)*/
{
  std::istringstream istr(result);
  Stream::Parser imem(result);
  std::string in1, in2;
  int readings = 0;

  while (istr >> in1)
  {
    imem >> in2;
    if (in1 != in2)
    {
      std::cerr << "Failure reading of '" << in1 << "' vs '" << in2 <<
        "'" << std::endl;
      return;
    }
    readings++;
  }
  if (imem.str().size())
  {
    std::cerr << "Invalid finish state" << std::endl;
    return;
  }
  std::cout << readings << " readings complete" << std::endl;
}

int
main()
{
  std::string result;
  test_output(result);
  test_input(result.c_str());

  return 0;
}
