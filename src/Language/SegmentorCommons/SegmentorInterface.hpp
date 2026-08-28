#pragma once

#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/Singleton.hpp>


namespace Language::Segmentor
{
  using WordsList = std::list<std::string>;

  DECLARE_EXCEPTION(BaseSegmException, eh::DescriptiveException);

  class SegmentorInterface : public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(SegmException, BaseSegmException);

    virtual
    void
    segmentation(WordsList& result, const char* phrase,
      size_t phrase_len) const /*throw (SegmException)*/ = 0;

    virtual
    void
    put_spaces(std::string& result, const char* phrase,
      size_t phrase_len) const /*throw (SegmException)*/ = 0;

  protected:
    virtual ~SegmentorInterface() noexcept;
  };
  using SegmentorInterface_var = ReferenceCounting::ConstPtr<SegmentorInterface>;

  template <typename Implementation>
  class UniqueSegmentorInterface :
    public SegmentorInterface,
    private Generics::Unique<Implementation,
      SegmentorInterface::SegmException>
  {
  protected:
    using UniqueException = typename Generics::Unique<Implementation,
      SegmentorInterface::SegmException>::Exception;

    virtual ~UniqueSegmentorInterface() noexcept;
  };
}

namespace Language::Segmentor
{
  inline SegmentorInterface::~SegmentorInterface() noexcept
  {
  }

  template <typename Implementation>
  UniqueSegmentorInterface<Implementation>::
    ~UniqueSegmentorInterface() noexcept
  {
  }
}
