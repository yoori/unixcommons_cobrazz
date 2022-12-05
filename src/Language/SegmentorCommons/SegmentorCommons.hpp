#ifndef LANGUAGE_SEGMENTOR_COMMONS_SEGMENTORCOMMONS_HPP
#define LANGUAGE_SEGMENTOR_COMMONS_SEGMENTORCOMMONS_HPP

#include <string>

#include <String/SubString.hpp>

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language
{
  namespace Segmentor
  {
    inline
    void
    append(std::string& target, const String::SubString& str)
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

    inline
    void
    append(WordsList& target, const String::SubString& str)
      /*throw (eh::Exception)*/
    {
      if (!str.empty())
      {
        target.push_back(str.str());
      }
    }
  }//namespace Segmentor
}//namespace Language

#endif
