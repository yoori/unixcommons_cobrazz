#pragma once

#include <memory>

#include <Generics/Function.hpp>

#include <Language/SegmentorCommons/SegmentorInterface.hpp>

#include <Language/SegmentorManager/SegmentorFilter.hpp>

#include <Language/Polyglot/DictionaryLoader.hpp>
#include <Language/Polyglot/Tokenizer.hpp>


namespace Language::Segmentor
{
  template <typename Tokenizer, typename Dictionary, typename SuffixDictionary>
  class PolyglotSegmentorWrap :
    public UniqueSegmentorInterface<
      PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>>
  {
  public:
    using UniqueException = typename UniqueSegmentorInterface<
      PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>>::
        UniqueException;
    using SegmException = typename UniqueSegmentorInterface<
      PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>>::
        SegmException;

    explicit PolyglotSegmentorWrap(const char* config_file)
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
    virtual ~PolyglotSegmentorWrap() noexcept;

  private:
    Dictionary dict_;
    SuffixDictionary suffix_dict_;
    std::unique_ptr<Tokenizer> tokenizer_;
  };

  struct DefaultPolyglotSymbols
  {
    using CategoryType = String::StringManip::InverseCategory<String::Utf8Category>;
    static const CategoryType INVALID_SYMBOLS;
  };

  using PolyglotSegmentor = AutomaticFilterSegmentor<
      PolyglotSegmentorWrap<
        Polyglot::Tokenizer,
        Polyglot::Dictionary,
        Polyglot::SuffixDictionary>,
      DefaultPolyglotSymbols>;

  using NormalizePolyglotSegmentor = AutomaticFilterSegmentor<
      PolyglotSegmentorWrap<
        Polyglot::NormalizeTokenizer,
        Polyglot::DictionaryWithNorm,
        Polyglot::SuffixDictionary>,
      DefaultPolyglotSymbols>;

  using PolyglotSegmentor_var = ReferenceCounting::ConstPtr<PolyglotSegmentor>;

  using NormalizePolyglotSegmentor_var = ReferenceCounting::ConstPtr<NormalizePolyglotSegmentor>;
}

namespace Language::Segmentor
{
  /**X
   * class PolyglotSegmentor
   */
  template <typename Tokenizer, typename Dictionary, typename SuffixDictionary>
  PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>::
    PolyglotSegmentorWrap(const char* config_file)
      /*throw (UniqueException, SegmException)*/
  {
    try
    {
      Polyglot::DictionaryLoader::load(config_file, dict_);
      Polyglot::DictionaryLoader::load_suffixes(config_file, suffix_dict_);
      tokenizer_.reset(new Tokenizer(dict_, suffix_dict_));
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error error;
      error << FNS << "can't initialize dictionary: eh::Exception caught: " << ex.what();
      throw SegmException(error);
    }
    catch (...)
    {
      Stream::Error error;
      error << FNS << "can't initialize dictionary: unknown exception caught";
      throw SegmException(error);
    }
  }

  template <typename Tokenizer, typename Dictionary, typename SuffixDictionary>
  PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>::
    ~PolyglotSegmentorWrap() noexcept
  {
  }

  template <typename Tokenizer, typename Dictionary, typename SuffixDictionary>
  void
  PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>::
    segmentation(WordsList& result, const char* phrase,
      size_t phrase_len) const /*throw (SegmException)*/
  {
    try
    {
      tokenizer_->segment(String::SubString(phrase, phrase_len), result);
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error error;
      error << FNS << "eh::Exception caught: " << ex.what();
      throw SegmException(error);
    }
    catch (...)
    {
      Stream::Error error;
      error << FNS << "unknown Exception";
      throw SegmException(error);
    }
  }

  template <typename Tokenizer, typename Dictionary, typename SuffixDictionary>
  void
  PolyglotSegmentorWrap<Tokenizer, Dictionary, SuffixDictionary>::
    put_spaces(std::string& result, const char* phrase,
      size_t phrase_len) const /*throw (SegmException)*/
  {
    try
    {
      tokenizer_->put_spaces(result, String::SubString(phrase, phrase_len));
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error error;
      error << FNS << "eh::Exception caught: " << ex.what();
      throw SegmException(error);
    }
    catch (...)
    {
      Stream::Error error;
      error << FNS << "unknown Exception";
      throw SegmException(error);
    }
  }
}
