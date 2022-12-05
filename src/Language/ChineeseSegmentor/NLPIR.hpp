#ifndef LANGUAGE_CHINEESE_SEGMENTOR_NLPIR_HPP
#define LANGUAGE_CHINEESE_SEGMENTOR_NLPIR_HPP

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language
{
  namespace Segmentor
  {
    namespace Chineese
    {
      class NlpirSegmentor : public UniqueSegmentorInterface<NlpirSegmentor>
      {
      public:
        explicit
        NlpirSegmentor(const char* path = "/usr/share/NLPIR")
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
        static
        const char*
        put_spaces_(const char* phrase, size_t phrase_len)
          /*throw (eh::Exception)*/;

        virtual
        ~NlpirSegmentor() throw ();
      };
      typedef ReferenceCounting::ConstPtr<NlpirSegmentor>
        NlpirSegmentor_var;
    }
  }
}

#endif
