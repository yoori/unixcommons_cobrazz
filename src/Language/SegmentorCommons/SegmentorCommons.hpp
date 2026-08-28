#pragma once

#include <string>

#include <String/SubString.hpp>

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language::Segmentor
{
  inline void append(std::string& target, const String::SubString& str)
    /*throw (eh::Exception)*/
  {
    if (!str.empty())
    {
      if (!target.empty() && *target.rbegin() != ' ' &&
        *str.begin() != ' ')
      {
        target += ' ';
      }
      str.append_to(target);
    }
  }

  inline void append(WordsList& target, const String::SubString& str)
    /*throw (eh::Exception)*/
  {
    if (!str.empty())
    {
      target.push_back(str.str());
    }
  }
}
