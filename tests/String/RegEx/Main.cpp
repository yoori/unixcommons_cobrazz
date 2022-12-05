#include <iostream>

#include <String/RegEx.hpp>

using String::RegEx;

int
main()
{
  try
  {
    {
      RegEx r1;
      {
        RegEx r2(r1);
      }
      RegEx r3;
      r3 = r1;
    }

    {
      RegEx r1(String::SubString(".*"));
      {
        RegEx r2(r1);
      }
      RegEx r3;
      r3 = r1;
      RegEx r4;
      r1 = r4;
    }

    {
      const String::SubString REGEXP("A(.*)Z");
      const String::SubString SUBJECT("q9f834fAf434f43f4");
      RegEx r(REGEXP);
      if (r.match(SUBJECT))
      {
        std::cerr << "Illegally matched" << std::endl;
      }
      RegEx::Result result;
      if (r.search(result, SUBJECT))
      {
        std::cerr << "Illegally found" << std::endl;
      }
    }

    {
      const String::SubString REGEXP("A(.*)Z");
      const String::SubString SUBJECT("q9f834fAf434Zf43f4");
      RegEx r(REGEXP);
      if (!r.match(SUBJECT))
      {
        std::cerr << "Failed to match" << std::endl;
      }
      RegEx::Result result;
      if (!r.search(result, SUBJECT))
      {
        std::cerr << "Failed to find" << std::endl;
      }
      if (result.size() != 2)
      {
        std::cerr << "Invalid search result" << std::endl;
      }
    }

    {
      const String::SubString REGEXP("");
      const String::SubString SUBJECT("");
      RegEx r(REGEXP);
      RegEx::Result result;
      r.gsearch(result, SUBJECT);
      if (result.size() != 1)
      {
        std::cerr << "Invalid search result" << std::endl;
      }
    }

    {
      Generics::Allocator::Base_var alloc(
        new Generics::Allocator::Universal);

      const String::SubString REGEXP("");
      const String::SubString SUBJECT("123");
      RegEx r(REGEXP, 0, alloc);
      RegEx::Result result;
      r.gsearch(result, SUBJECT);
      if (result.size() != 4)
      {
        std::cerr << "Invalid search result" << std::endl;
      }
    }

    {
      typedef std::allocator<char> Allocator;
      Generics::Allocator::Base_var alloc(
        Generics::Allocator::Template<Allocator>::allocator());

      const String::SubString REGEXP("b(.)");
      const String::SubString SUBJECT("abcabc");
      RegEx r(REGEXP, 0, alloc);
      RegEx::Result result;
      r.gsearch(result, SUBJECT);
      if (result.size() != 2 || result[0] != "c" || result[1] != "c")
      {
        std::cerr << "Invalid search result" << std::endl;
      }
    }

    {
      const String::SubString REGEXP("b(c(.?))");
      const String::SubString SUBJECT("abcabc");
      RegEx r(REGEXP);
      RegEx::Result result;
      r.gsearch(result, SUBJECT);
      if (result.size() != 4 || result[0] != "ca" || result[1] != "a"
          || result[2] != "c" || result[3] != "")
      {
        std::cerr << "Invalid search result" << std::endl;
      }
    }

    return 0;
  }
  catch (const eh::Exception& ex)
  {
    std::cerr << "Exception caught " << ex.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception caught" << std::endl;
  }
  return -1;
}
