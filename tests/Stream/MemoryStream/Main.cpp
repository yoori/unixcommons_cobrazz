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


void cut_ostr(std::ostringstream& ostr, size_t cut_len = 0)
{
  char buf[cut_len + 1] = {0};
  snprintf(buf, cut_len + 1, "%s", ostr.str().data());
  ostr = std::ostringstream();
  ostr << buf;
}

template<typename OStream, typename Type>
void
compare(const Type& value, size_t cmp_len = 0, bool remove_const = false)
{
  std::ostringstream ostr;
  ostr << (remove_const ? const_cast<Type&>(value) : value);

  OStream omem;
  omem << (remove_const ? const_cast<Type&>(value) : value);

  if (cmp_len)
  {
    cut_ostr(ostr, cmp_len);
  }

  assert(ostr.str() == omem.str());
}

template<typename OStream, typename Type>
void
compare_width_out(const Type& value, size_t cmp_len = 0, size_t width = 0, char fill = ' ')
{
  std::ostringstream ostr;
  ostr << std::setw(width) << std::setfill(fill) << value;

  OStream omem;
  omem << Stream::MemoryStream::width_out(value, width, fill);

  if (cmp_len)
  {
    cut_ostr(ostr, cmp_len);
  }

  assert(ostr.str() == omem.str());
}

template<typename OStream, typename Type>
void
compare_hex_out(const Type& value, size_t cmp_len = 0, bool upcase = false, size_t width = 0, char fill = ' ')
{
  std::ostringstream ostr;
  ostr << std::hex << std::setw(width) << std::setfill(fill);
  if (upcase)
  {
    ostr << std::uppercase;
  }
  ostr << value;

  OStream omem;
  omem << Stream::MemoryStream::hex_out(value, upcase, width, fill);

  if (cmp_len)
  {
    cut_ostr(ostr, cmp_len);
  }

  assert(ostr.str() == omem.str());
}

template<typename OStream, typename Type>
void
compare_double_out(const Type& value, size_t cmp_len = 0, size_t precision = 0)
{
  std::ostringstream ostr;
  ostr << std::setprecision(precision) << std::fixed << value;

  OStream omem;
  omem << Stream::MemoryStream::double_out(value, precision);

  if (cmp_len)
  {
    cut_ostr(ostr, cmp_len);
  }

  assert(ostr.str() == omem.str());
}

// test Stream::OutputMemoryStream::operator<< for all types
// tests section

template<typename OStream = Stream::Dynamic>
void
run_tests(size_t cmp_len = 0)
{
  static constexpr bool is_on = true;

  // bool
  if constexpr (is_on)
  {
    compare<OStream>(true, cmp_len);
    compare<OStream>(false, cmp_len);
  }
  // char
  if constexpr (is_on)
  {
    char ch = 'a';
    compare<OStream>(ch, cmp_len);
    const char cch = 'a';
    compare<OStream>(cch, cmp_len);

    unsigned char uch = 'a';
    compare<OStream>(uch, cmp_len);

    signed char sch = 'a';
    compare<OStream>(sch, cmp_len);
  }
  // char* | char[n] | const char* | const char[n]
  if constexpr (is_on)
  {
    // char* | char[n]
    char ch[5] = {'a', 'b', 'c', 'd', '\0'};
    compare<OStream>(ch, cmp_len, true);
    char* pch = ch;
    compare<OStream>(pch, cmp_len, true);

    // const char* | const char[n]
    const char cch[5] = "abcd";
    compare<OStream>(cch, cmp_len);
    const char* pcch = cch;
    compare<OStream>(pcch, cmp_len);
  }
  // ArgT* or const ArgT* (ArgT != Elem)
  if constexpr (is_on)
  {
    int* pinteger = nullptr;
    compare<OStream>(pinteger, cmp_len);

    int integer = 123;
    pinteger = &integer;
    compare<OStream>(pinteger, cmp_len);

    const int* cpinteger = &integer;
    compare<OStream>(cpinteger, cmp_len);

    // TODO
    // https://stackoverflow.com/questions/2064692/how-to-print-function-pointers-with-cout
    //auto* fptr = test_simple_dynamic;
    //compare<OStream>(fptr, cmp_len);

    struct Cls {int xyz = 0;} cls;
    Cls* ptr = &cls;
    compare<OStream>(ptr, cmp_len);

    const Cls* cptr = &cls;
    compare<OStream>(cptr, cmp_len);
  }
  // std::string + std::string_view + BasicSubString
  if constexpr (is_on)
  {
    std::string str = "this is string";
    compare<OStream>(str, cmp_len);

    std::string_view strv(str.begin(), str.end());
    compare<OStream>(strv, cmp_len);

    String::BasicSubString<const char> bsubstr(str);
    compare<OStream>(bsubstr, cmp_len);
  }
  // integral
  if constexpr (is_on)
  {
    // int
    int x = 0;
    compare<OStream>(x, cmp_len);
    x = -1;
    compare<OStream>(x, cmp_len);
    x = 1;
    compare<OStream>(x, cmp_len);
    x = std::numeric_limits<int>::max();
    compare<OStream>(x, cmp_len);
    x = std::numeric_limits<int>::min();
    compare<OStream>(x, cmp_len);

    unsigned int ux = 0;
    compare<OStream>(ux, cmp_len);
    ux = std::numeric_limits<unsigned int>::max();
    compare<OStream>(ux, cmp_len);

    // short
    short int sx = 0;
    compare<OStream>(sx, cmp_len);
    sx = std::numeric_limits<short int>::max();
    compare<OStream>(sx, cmp_len);
    sx = std::numeric_limits<short int>::min();
    compare<OStream>(sx, cmp_len);

    unsigned short int sux = 0;
    compare<OStream>(sux, cmp_len);
    sux = std::numeric_limits<short unsigned int>::max();
    compare<OStream>(sux, cmp_len);

    // long long
    long long int lx = 0;
    compare<OStream>(lx, cmp_len);
    lx = std::numeric_limits<long long int>::max();
    compare<OStream>(lx, cmp_len);
    lx = std::numeric_limits<long long int>::min();
    compare<OStream>(lx, cmp_len);

    unsigned long long int lux = 0;
    compare<OStream>(lux, cmp_len);
    lux = std::numeric_limits<unsigned long long int>::max();
    compare<OStream>(lux, cmp_len);

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

    compare<OStream>(TEnum1::EValue1, cmp_len);
    compare<OStream>(TEnum1::EValue2, cmp_len);
    compare<OStream>(TEnum1::EValue3, cmp_len);
    compare<OStream>(TEnum2::EValue4, cmp_len);
    compare<OStream>(TEnum3::EValue5, cmp_len);

    // atomic
    std::atomic<int> ax = 123;
    compare<OStream>(ax, cmp_len);
  }
  // floating point
  if constexpr (is_on)
  {
    double x = 0.0;
    compare<OStream>(x, cmp_len);

    x = -1;
    compare<OStream>(x, cmp_len);

    x = 1;
    compare<OStream>(x, cmp_len);

    x = std::numeric_limits<double>::min();
    compare<OStream>(x, cmp_len);

    x = std::numeric_limits<double>::max();
    compare<OStream>(x, cmp_len);

    long double lx = std::numeric_limits<long double>::min();
    compare<OStream>(lx, cmp_len);

    lx = std::numeric_limits<long double>::max();
    compare<OStream>(lx, cmp_len);

    float fx = -123.123;
    compare<OStream>(fx, cmp_len);
  }
  // widthout
  if constexpr (is_on)
  {
    compare_width_out<OStream>(0, cmp_len);
    compare_width_out<OStream>(1, cmp_len);
    compare_width_out<OStream>(-1, cmp_len);
    compare_width_out<OStream>(std::numeric_limits<int>::max(), cmp_len);
    compare_width_out<OStream>(std::numeric_limits<int>::min(), cmp_len);
    compare_width_out<OStream>(std::numeric_limits<unsigned int>::max(), cmp_len);
    compare_width_out<OStream>(std::numeric_limits<long long>::max(), cmp_len);
    compare_width_out<OStream>(std::numeric_limits<long long>::min(), cmp_len);
    compare_width_out<OStream>(std::numeric_limits<unsigned long long>::max(), cmp_len);
    compare_width_out<OStream>(123, cmp_len, 5);
    compare_width_out<OStream>(123, cmp_len, 5, '*');
    compare_width_out<OStream>(-123, cmp_len, 2);
    compare_width_out<OStream>(-123, cmp_len, 2, '*');
    compare_width_out<OStream>(-123, cmp_len, 4);
    compare_width_out<OStream>(-123, cmp_len, 5, '*');
    compare_width_out<OStream>(-123, cmp_len, 6, '#');
    compare_width_out<OStream>(0, cmp_len, 5, '!');
    compare_width_out<OStream>(0, cmp_len, 10);
  }
  // hexout
  if constexpr (is_on)
  {
    // upcase = false
    compare_hex_out<OStream>(0, cmp_len);
    compare_hex_out<OStream>(0, cmp_len, false, 3, '0');
    compare_hex_out<OStream>(0, cmp_len, false, 4, '0');
    compare_hex_out<OStream>(0, cmp_len, false, 5, '0');
    compare_hex_out<OStream>(1, cmp_len);
    compare_hex_out<OStream>(123123, cmp_len);
    compare_hex_out<OStream>(123123, cmp_len, false, 4, '0');
    compare_hex_out<OStream>(123123, cmp_len, false, 5, '0');
    compare_hex_out<OStream>(-1, cmp_len);
    compare_hex_out<OStream>(-1, cmp_len, false, 4, '0');
    compare_hex_out<OStream>(-1, cmp_len, false, 5, '0');
    compare_hex_out<OStream>(-123123, cmp_len);
    compare_hex_out<OStream>(-123123, cmp_len, false, 4, '0');
    compare_hex_out<OStream>(-123123, cmp_len, false, 5, '0');
    compare_hex_out<OStream>(std::numeric_limits<int>::max(), cmp_len);
    compare_hex_out<OStream>(std::numeric_limits<int>::max(), cmp_len, false, 10, '0');
    compare_hex_out<OStream>(std::numeric_limits<int>::min(), cmp_len);
    compare_hex_out<OStream>(std::numeric_limits<int>::min(), cmp_len, false, 12, ' ');
    compare_hex_out<OStream>(std::numeric_limits<unsigned int>::max(), cmp_len);
    compare_hex_out<OStream>(std::numeric_limits<unsigned int>::max(), cmp_len, false, 15, '0');
    compare_hex_out<OStream>(std::numeric_limits<long long>::max(), cmp_len);
    compare_hex_out<OStream>(std::numeric_limits<long long>::max(), cmp_len, false, 10, '0');
    compare_hex_out<OStream>(std::numeric_limits<long long>::min(), cmp_len);
    compare_hex_out<OStream>(std::numeric_limits<long long>::min(), cmp_len, false, 10, '0');
    compare_hex_out<OStream>(std::numeric_limits<unsigned long long>::max(), cmp_len);
    compare_hex_out<OStream>(std::numeric_limits<unsigned long long>::max(), cmp_len, false, 18, '0');
    // upcase = true
    compare_hex_out<OStream>(0, cmp_len, true);
    compare_hex_out<OStream>(0, cmp_len, true, 5, '0');
    compare_hex_out<OStream>(0, cmp_len, true, 5, 'a');
    compare_hex_out<OStream>(1, cmp_len, true);
    compare_hex_out<OStream>(123123, cmp_len, true);
    compare_hex_out<OStream>(-1, cmp_len, true);
    compare_hex_out<OStream>(-123123, cmp_len, true);
    compare_hex_out<OStream>(std::numeric_limits<int>::max(), cmp_len, true);
    compare_hex_out<OStream>(std::numeric_limits<int>::min(), cmp_len, true);
    compare_hex_out<OStream>(std::numeric_limits<unsigned int>::max(), cmp_len, true);
    compare_hex_out<OStream>(std::numeric_limits<long long>::max(), cmp_len, true);
    compare_hex_out<OStream>(std::numeric_limits<long long>::min(), cmp_len, true);
    compare_hex_out<OStream>(std::numeric_limits<unsigned long long>::max(), cmp_len, true);
  }
  // doubleout
  if constexpr (is_on)
  {
    // double_out for is_floating_point
    compare_double_out<OStream>(0.0, cmp_len);
    compare_double_out<OStream>(0.0, cmp_len, 5);
    compare_double_out<OStream>(-0.0, cmp_len);
    compare_double_out<OStream>(-0.0, cmp_len, 5);
    compare_double_out<OStream>(0.123, cmp_len);
    compare_double_out<OStream>(0.123, cmp_len, 3);
    compare_double_out<OStream>(0.123, cmp_len, 4);
    compare_double_out<OStream>(0.123, cmp_len, 5);
    compare_double_out<OStream>(-0.123, cmp_len);
    compare_double_out<OStream>(-0.123, cmp_len, 2);
    compare_double_out<OStream>(-0.123, cmp_len, 3);
    compare_double_out<OStream>(-0.123, cmp_len, 4);
    compare_double_out<OStream>(-0.123, cmp_len, 5);
    compare_double_out<OStream>(123.0, cmp_len);
    compare_double_out<OStream>(-123.0, cmp_len);
    compare_double_out<OStream>(0.59, cmp_len, 1);
    compare_double_out<OStream>(0.59, cmp_len, 2);
    compare_double_out<OStream>(0.55, cmp_len, 1);
    compare_double_out<OStream>(0.55, cmp_len, 2);
    compare_double_out<OStream>(0.9, cmp_len);
    compare_double_out<OStream>(0.9, cmp_len, 1);
    compare_double_out<OStream>(1.5, cmp_len, 1);
    compare_double_out<OStream>(1.500001, cmp_len, 1);
    compare_double_out<OStream>(1.4999999, cmp_len, 1);
    compare_double_out<OStream>(1.4999999, cmp_len, 2);
    compare_double_out<OStream>(2.5, cmp_len, 0);
    compare_double_out<OStream>(2.5, cmp_len, 1);
    compare_double_out<OStream>(std::numeric_limits<double>::max(), cmp_len, 10);
    compare_double_out<OStream>(std::numeric_limits<double>::min(), cmp_len, 10);
    compare_double_out<OStream>(std::numeric_limits<long double>::max(), cmp_len, 10);
    compare_double_out<OStream>(std::numeric_limits<long double>::min(), cmp_len, 10);
    compare_double_out<OStream>(123.123, cmp_len, 5);
    // double_out for is_integral
    compare_double_out<OStream>(0, cmp_len);
    compare_double_out<OStream>(123, cmp_len);
    compare_double_out<OStream>(123, cmp_len, 3);
    compare_double_out<OStream>(123, cmp_len, 6);
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
    compare<OStream>(sha, cmp_len);
  }
  // Generics::Uuid
  if constexpr (is_on)
  {
    Generics::Uuid uuid;
    compare<OStream>(uuid, cmp_len);
  }
  // Generics::[Extended]Time
  if constexpr (is_on)
  {
    Generics::ExtendedTime etime(2000, 1, 1, 1, 1, 1, 0);
    compare<OStream>(etime, cmp_len);
    Generics::Time time(etime);
    compare<OStream>(time, cmp_len);

    // width_out<Generics::Time>
    compare_width_out<OStream>(time, cmp_len, 50, '*');
  }
  // Generics::SimpleDecimal
  if constexpr (is_on)
  {
    const String::SubString num("-1234567890.87654321");
    Generics::SimpleDecimal<uint64_t, 18, 8> sd(num);
    compare<OStream>(sd, cmp_len);

    // double_out<const char*>
    compare_double_out<OStream>(std::string("123.123456").c_str(), cmp_len, 2);
    // double_out<std::string>
    compare_double_out<OStream>(std::string("123.123456"), cmp_len, 2);
  }
}

template<const size_t Start, const size_t End, const size_t Inc = 1>
constexpr void run_iter_tests()
{
  if constexpr (Start < End)
  {
    run_tests<Stream::Stack<Start>>(Start);
    run_iter_tests<Start + Inc, End, Inc>();
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
  run_iter_tests<1, 20>();

  std::cout << "Finished Successfully" << std::endl;

  return 0;
}
