// Common/UTF8TreeLoader.hpp
#ifndef _UTF8_TREE_LOADER_HPP_INCLUDED_
#define _UTF8_TREE_LOADER_HPP_INCLUDED_

#include <fstream>
#include <sstream>
#include <iostream>

#include <eh/Exception.hpp>

#include <String/UTF8Category.hpp>
#include <String/UnicodeSymbol.hpp>

namespace Utf8Loading
{
  DECLARE_EXCEPTION(FileOpenError, eh::DescriptiveException);

  template <typename Container>
  void
  load_properties(const char* file_name, Container& container)
    /*throw (eh::Exception, FileOpenError)*/;
}

//////////////////////////////////////////////////////////////////////////
//    Implementation
//////////////////////////////////////////////////////////////////////////
namespace Utf8Loading
{
  template <typename Container>
  void
  load_properties(const char* file_name, Container& container)
    /*throw (eh::Exception, FileOpenError)*/
  {
    std::ifstream ifs(file_name);
    if (!ifs)
    {
      Stream::Error ostr;
      ostr << "File " << file_name << " open error";
      throw FileOpenError(ostr);
    }

    while (ifs)
    {
      std::string line;
      std::getline(ifs, line);

      if (line.empty() || !isalnum(line[0]))
      {
        continue;
      }

      Stream::Parser sstr(line);

      String::UnicodeSymbol first;
      sstr >> first;
      if (!sstr)
      {
        break; // or failed on broken file ?
      }

      if (sstr.get() == '-')
      {
        String::UnicodeSymbol second;

        sstr >> second;
        if (sstr.fail())
        {
          break; // or failed on broken file ?
        }
        container.insert(first, second);
      }
      else
      {
        container.insert(first, first);
      }
    }
  }
}

#endif  //_UTF8_TREE_LOADER_HPP_INCLUDED_
