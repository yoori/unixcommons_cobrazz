#include <atomic>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdlib.h>
#include <string>
#include <string_view>

#include <Generics/HashTableAdapters.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <Generics/Time.hpp>
#include <Generics/Uuid.hpp>
#include <Stream/MemoryStream.hpp>
#include <String/SubString.hpp>
//#include <XMLUtility/StringManip.hpp>

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

// test Stream::OutputMemoryStream::operator<< for all types
// helpers section

template<typename OStream, typename Type>
void
compare(const Type& value, bool remove_const = false)
{
  std::ostringstream ostr;
  ostr << (remove_const ? const_cast<Type&>(value) : value);

  OStream omem;
  omem << (remove_const ? const_cast<Type&>(value) : value);

  assert(ostr.str() == omem.str());
}

template<typename OStream, typename Type>
void
compare_width_out(const Type& value, size_t width = 0, char fill = ' ')
{
  std::ostringstream ostr;
  ostr << std::setw(width) << std::setfill(fill) << value;

  OStream omem;
  omem << Stream::MemoryStream::width_out(value, width, fill);

  assert(ostr.str() == omem.str());
}

template<typename OStream, typename Type>
void
compare_hex_out(const Type& value, bool upcase = false)
{
  std::ostringstream ostr;
  ostr << std::hex;
  if (upcase)
  {
    ostr << std::uppercase;
  }
  ostr << value;

  OStream omem;
  omem << Stream::MemoryStream::hex_out(value, upcase);

  assert(ostr.str() == omem.str());
}

template<typename OStream, typename Type>
void
compare_double_out(const Type& value, size_t precision = 0)
{
  std::ostringstream ostr;
  ostr << std::setprecision(precision) << std::fixed << value;

  OStream omem;
  omem << Stream::MemoryStream::double_out(value, precision);

  assert(ostr.str() == omem.str());
}

// test Stream::OutputMemoryStream::operator<< for all types
// tests section

template<typename OStream = Stream::Dynamic>
void
run_tests()
{
  static constexpr bool is_on = true;

  // bool
  if constexpr (is_on)
  {
    compare<OStream>(true);
    compare<OStream>(false);
  }
  // char
  if constexpr (is_on)
  {
    char ch = 'a';
    compare<OStream>(ch);
    const char cch = 'a';
    compare<OStream>(cch);

    unsigned char uch = 'a';
    compare<OStream>(uch);

    signed char sch = 'a';
    compare<OStream>(sch);
  }
  // char* | char[n] | const char* | const char[n]
  if constexpr (is_on)
  {
    // char* | char[n]
    char ch[5] = {'a', 'b', 'c', 'd', '\0'};
    compare<OStream>(ch, true);
    char* pch = ch;
    compare<OStream>(pch, true);

    // const char* | const char[n]
    const char cch[5] = "abcd";
    compare<OStream>(cch);
    const char* pcch = cch;
    compare<OStream>(pcch);
  }
  // ArgT* or const ArgT* (ArgT != Elem)
  if constexpr (is_on)
  {
    int* pinteger = nullptr;
    compare<OStream>(pinteger);

    int integer = 123;
    pinteger = &integer;
    compare<OStream>(pinteger);

    const int* cpinteger = &integer;
    compare<OStream>(cpinteger);

    // TODO
    // https://stackoverflow.com/questions/2064692/how-to-print-function-pointers-with-cout
    //auto* fptr = test_simple_dynamic;
    //compare<OStream>(fptr);

    struct Cls {int xyz = 0;} cls;
    Cls* ptr = &cls;
    compare<OStream>(ptr);

    const Cls* cptr = &cls;
    compare<OStream>(cptr);
  }
  // std::string + std::string_view + BasicSubString
  if constexpr (is_on)
  {
    std::string str = "this is string";
    compare<OStream>(str);

    std::string_view strv(str.begin(), str.end());
    compare<OStream>(strv);

    String::BasicSubString<const char> bsubstr(str);
    compare<OStream>(bsubstr);
  }
  // integral
  if constexpr (is_on)
  {
    // int
    int x = 0;
    compare<OStream>(x);
    x = -1;
    compare<OStream>(x);
    x = 1;
    compare<OStream>(x);
    x = std::numeric_limits<int>::max();
    compare<OStream>(x);
    x = std::numeric_limits<int>::min();
    compare<OStream>(x);

    unsigned int ux = 0;
    compare<OStream>(ux);
    ux = std::numeric_limits<unsigned int>::max();
    compare<OStream>(ux);

    // short
    short int sx = 0;
    compare<OStream>(sx);
    sx = std::numeric_limits<short int>::max();
    compare<OStream>(sx);
    sx = std::numeric_limits<short int>::min();
    compare<OStream>(sx);

    unsigned short int sux = 0;
    compare<OStream>(sux);
    sux = std::numeric_limits<short unsigned int>::max();
    compare<OStream>(sux);

    // long long
    long long int lx = 0;
    compare<OStream>(lx);
    lx = std::numeric_limits<long long int>::max();
    compare<OStream>(lx);
    lx = std::numeric_limits<long long int>::min();
    compare<OStream>(lx);

    unsigned long long int lux = 0;
    compare<OStream>(lux);
    lux = std::numeric_limits<unsigned long long int>::max();
    compare<OStream>(lux);

    // enum
    enum TEnum1
    {
      EValue1 = 0,
      EValue2 = 1,
      EValue3 = -1
    };
    enum TEnum2
    {
      EValue4 = std::numeric_limits<unsigned long long int>::max()
    };
    enum TEnum3
    {
      EValue5 = std::numeric_limits<long long int>::min()
    };

    compare<OStream>(TEnum1::EValue1);
    compare<OStream>(TEnum1::EValue2);
    compare<OStream>(TEnum1::EValue3);
    compare<OStream>(TEnum2::EValue4);
    compare<OStream>(TEnum3::EValue5);

    // atomic
    std::atomic<int> ax = 123;
    compare<OStream>(ax);
  }
  // floating point
  if constexpr (is_on)
  {
    double x = 0.0;
    compare<OStream>(x);

    x = -1;
    compare<OStream>(x);

    x = 1;
    compare<OStream>(x);

    x = std::numeric_limits<double>::min();
    compare<OStream>(x);

    x = std::numeric_limits<double>::max();
    compare<OStream>(x);

    long double lx = std::numeric_limits<long double>::min();
    compare<OStream>(lx);

    lx = std::numeric_limits<long double>::max();
    compare<OStream>(lx);

    float fx = -123.123;
    compare<OStream>(fx);
  }
  // widthout
  if constexpr (is_on)
  {
    compare_width_out<OStream>(0);
    compare_width_out<OStream>(1);
    compare_width_out<OStream>(-1);
    compare_width_out<OStream>(std::numeric_limits<int>::max());
    compare_width_out<OStream>(std::numeric_limits<int>::min());
    compare_width_out<OStream>(std::numeric_limits<unsigned int>::max());
    compare_width_out<OStream>(std::numeric_limits<long long>::max());
    compare_width_out<OStream>(std::numeric_limits<long long>::min());
    compare_width_out<OStream>(std::numeric_limits<unsigned long long>::max());
    compare_width_out<OStream>(123, 5);
    compare_width_out<OStream>(123, 5, '*');
    compare_width_out<OStream>(-123, 2);
    compare_width_out<OStream>(-123, 2, '*');
    compare_width_out<OStream>(-123, 4);
    compare_width_out<OStream>(-123, 5, '*');
    compare_width_out<OStream>(-123, 6, '#');
    compare_width_out<OStream>(0, 5, '!');
    compare_width_out<OStream>(0, 10);
  }
  // hexout
  if constexpr (is_on)
  {
    compare_hex_out<OStream>(0);
    compare_hex_out<OStream>(1);
    compare_hex_out<OStream>(123123);
    compare_hex_out<OStream>(-1);
    compare_hex_out<OStream>(-123123);
    compare_hex_out<OStream>(std::numeric_limits<int>::max());
    compare_hex_out<OStream>(std::numeric_limits<int>::min());
    compare_hex_out<OStream>(std::numeric_limits<unsigned int>::max());
    compare_hex_out<OStream>(std::numeric_limits<long long>::max());
    compare_hex_out<OStream>(std::numeric_limits<long long>::min());
    compare_hex_out<OStream>(std::numeric_limits<unsigned long long>::max());
    compare_hex_out<OStream>(0, true);
    compare_hex_out<OStream>(1, true);
    compare_hex_out<OStream>(123123, true);
    compare_hex_out<OStream>(-1, true);
    compare_hex_out<OStream>(-123123, true);
    compare_hex_out<OStream>(std::numeric_limits<int>::max(), true);
    compare_hex_out<OStream>(std::numeric_limits<int>::min(), true);
    compare_hex_out<OStream>(std::numeric_limits<unsigned int>::max(), true);
    compare_hex_out<OStream>(std::numeric_limits<long long>::max(), true);
    compare_hex_out<OStream>(std::numeric_limits<long long>::min(), true);
    compare_hex_out<OStream>(std::numeric_limits<unsigned long long>::max(), true);
  }
  // doubleout
  if constexpr (is_on)
  {
    compare_double_out<OStream>(0.0);
    compare_double_out<OStream>(0.0, 5);
    compare_double_out<OStream>(-0.0);
    compare_double_out<OStream>(-0.0, 5);
    compare_double_out<OStream>(0.123);
    compare_double_out<OStream>(0.123, 3);
    compare_double_out<OStream>(0.123, 4);
    compare_double_out<OStream>(0.123, 5);
    compare_double_out<OStream>(-0.123);
    compare_double_out<OStream>(-0.123, 2);
    compare_double_out<OStream>(-0.123, 3);
    compare_double_out<OStream>(-0.123, 4);
    compare_double_out<OStream>(-0.123, 5);
    compare_double_out<OStream>(123.0);
    compare_double_out<OStream>(-123.0);
    compare_double_out<OStream>(0.59, 1);
    compare_double_out<OStream>(0.59, 2);
    compare_double_out<OStream>(0.55, 1);
    compare_double_out<OStream>(0.55, 2);
    compare_double_out<OStream>(0.9);
    compare_double_out<OStream>(0.9, 1);
    compare_double_out<OStream>(1.5, 1);
    compare_double_out<OStream>(1.500001, 1);
    compare_double_out<OStream>(1.4999999, 1);
    compare_double_out<OStream>(1.4999999, 2);
    compare_double_out<OStream>(2.5, 0);
    compare_double_out<OStream>(2.5, 1);
    compare_double_out<OStream>(std::numeric_limits<double>::max(), 10);
    compare_double_out<OStream>(std::numeric_limits<double>::min(), 10);
    compare_double_out<OStream>(std::numeric_limits<long double>::max(), 10);
    compare_double_out<OStream>(std::numeric_limits<long double>::min(), 10);
    compare_double_out<OStream>(123.123, 5);
  }
  // XMLUtility::StringManip::XMLMbcAdapter
  if constexpr (is_on)
  {
    // TODO
    //XMLUtility::StringManip::XMLChAdapter adapter("");
    //XMLUtility::StringManip::XMLMbcAdapter xml_adapter(adapter);
    //compare<OStream>(xml_adapter);
  }
  if constexpr (is_on)
  {
    Generics::StringHashAdapter sha("abc 123");
    compare<OStream>(sha);
  }
  // Generics::Uuid
  if constexpr (is_on)
  {
    Generics::Uuid uuid;
    compare<OStream>(uuid);
  }
  // Generics::[Extended]Time
  if constexpr (is_on)
  {
    Generics::ExtendedTime etime(2000, 1, 1, 1, 1, 1, 0);
    compare<OStream>(etime);
    Generics::Time time(etime);
    compare<OStream>(time);

    // width_out<Generics::Time>
    compare_width_out<OStream>(time, 50, '*');
  }
  // Generics::SimpleDecimal
  if constexpr (is_on)
  {
    const String::SubString num("-1234567890.87654321");
    Generics::SimpleDecimal<uint64_t, 18, 8> sd(num);
    compare<OStream>(sd);

    // double_out<const char*>
    compare_double_out<OStream>(std::string("123.123456").c_str(), 2);
  }
}

int
main()
{
  std::string result;
  test_output(result);
  test_input(result.c_str());

  run_tests();
  run_tests<Stream::Error>();

  return 0;
}
