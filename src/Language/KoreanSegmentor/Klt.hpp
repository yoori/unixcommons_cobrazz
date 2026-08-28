#pragma once

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language::Segmentor
{
  namespace Korean
  {
    class KltSegmentor : public UniqueSegmentorInterface<KltSegmentor>
    {
    public:
      explicit KltSegmentor(const char* config_file = 0, const char* additional_params = 0)
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
      virtual ~KltSegmentor() noexcept;
    };
    using KltSegmentor_var = ReferenceCounting::ConstPtr<KltSegmentor>;
  } //namespace Korean
}
