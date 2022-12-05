#ifndef LANGUAGE_SEGMENTOR_COMMONS_SEGMENTOR_INTERFACE_HPP
#define LANGUAGE_SEGMENTOR_COMMONS_SEGMENTOR_INTERFACE_HPP

#include <list>
#include <string>

#include <ReferenceCounting/ReferenceCounting.hpp>

#include <Generics/Singleton.hpp>


namespace Language
{
  namespace Segmentor
  {
    typedef std::list<std::string> WordsList;

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
      virtual
      ~SegmentorInterface() throw ();
    };
    typedef ReferenceCounting::ConstPtr<SegmentorInterface>
      SegmentorInterface_var;

    template <typename Implementation>
    class UniqueSegmentorInterface :
      public SegmentorInterface,
      private Generics::Unique<Implementation,
        SegmentorInterface::SegmException>
    {
    protected:
      typedef typename Generics::Unique<Implementation,
        SegmentorInterface::SegmException>::Exception
        UniqueException;

      virtual
      ~UniqueSegmentorInterface() throw ();
    };
  } //Segmentor
} //namespace Language

namespace Language
{
  namespace Segmentor
  {
    inline
    SegmentorInterface::~SegmentorInterface() throw ()
    {
    }

    template <typename Implementation>
    UniqueSegmentorInterface<Implementation>::
      ~UniqueSegmentorInterface() throw ()
    {
    }
  }
}

#endif
