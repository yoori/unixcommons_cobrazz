#ifndef LANGUAGE_BLOGIC_NORMALIZETRIGGER_HPP
#define LANGUAGE_BLOGIC_NORMALIZETRIGGER_HPP

#include <vector>
#include <string>

#include <String/SubString.hpp>
#include <String/AsciiStringManip.hpp>

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language
{
  namespace Trigger
  {
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);


    /**
     * Normalizes trigger according to specification (leaves letters
     * and digits only).
     * @param trigger trigger to normalize
     * @param result normalized trigger
     * @param segmentor optional segmentor
     */
    void
    normalize(const String::SubString& trigger, std::string& result,
      const Segmentor::SegmentorInterface* segmentor = 0)
      /*throw (eh::Exception, Exception)*/;


    struct Trigger
    {
      struct Part
      {
        String::SubString part;
        bool quotes;
      };

      typedef std::vector<Part> Parts;

      std::string trigger;
      bool exact;
      Parts parts;
    };

    /**
     * Normalizes trigger according to specification (leaves letters
     * and digits only).
     * @param trigger trigger to normalize
     * @param result normalized trigger, containing split to hard ones.
     * @param segmentor optional segmentor
     */
    void
    normalize(const String::SubString& trigger, Trigger& result,
      const Segmentor::SegmentorInterface* segmentor = 0)
      /*throw (eh::Exception, Exception)*/;


    /**
     * Normalizes phrase according to trigger specification (leaves letters
     * and digits only).
     * @param phrase phrase to normalize
     * @param result normalized phrase
     * @param segmentor optional segmentor
     */
    void
    normalize_phrase(const String::SubString& phrase, std::string& result,
      const Language::Segmentor::SegmentorInterface* segmentor = 0)
      /*throw (eh::Exception, Exception)*/;
  }
}

#endif
