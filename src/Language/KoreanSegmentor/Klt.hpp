#ifndef LANGUAGE_KOREAN_SEGMENTOR_KLT_HPP
#define LANGUAGE_KOREAN_SEGMENTOR_KLT_HPP

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language
{
  namespace Segmentor
  {
    namespace Korean
    {
      class KltSegmentor : public UniqueSegmentorInterface<KltSegmentor>
      {
      public:
        explicit
        KltSegmentor(const char* config_file = 0,
          const char* additional_params = 0)
          /*throw (UniqueException, SegmException)*/;

        virtual
        void
        segmentation(WordsList& result, const char* phrase,
          size_t phrase_len) const /*throw (SegmException)*/;

        virtual
        void
        put_spaces(std::string& result, const char* phrase,
          size_t phrase_len) const /*throw (SegmException)*/;

      protected:
        virtual
        ~KltSegmentor() throw ();
      };
      typedef ReferenceCounting::ConstPtr<KltSegmentor>
        KltSegmentor_var;
    } //namespace Korean
  } //namespace Segmentor
} //namespace Language

#endif
