#pragma once

#include <Language/SegmentorCommons/SegmentorInterface.hpp>


namespace Language
{
  namespace Segmentor
  {
    namespace Korean
    {
      class MoranSegmentor : public UniqueSegmentorInterface<MoranSegmentor>
      {
      public:
        explicit
        MoranSegmentor(const char* config_file) /*throw (UniqueException)*/;

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
        ~MoranSegmentor() noexcept;

        bool
        is_valid_utf8_(const char* str, size_t str_len) const noexcept;
      };
      typedef ReferenceCounting::ConstPtr<MoranSegmentor>
        MoranSegmentor_var;
    } //namespace Korean
  } //namespace Segmentor
} //namespace Language
