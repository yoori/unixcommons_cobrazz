namespace Stream
{
  namespace MemoryStream
  {
    //
    // InputMemoryBuffer class
    //

    template <typename Elem, typename Traits>
    InputMemoryBuffer<Elem, Traits>::InputMemoryBuffer(Pointer ptr,
      Size size) /*throw (eh::Exception)*/
    {
      this->setg(ptr, ptr, ptr + size);
    }

    template <typename Elem, typename Traits>
    typename InputMemoryBuffer<Elem, Traits>::ConstPointer
    InputMemoryBuffer<Elem, Traits>::data() const throw ()
    {
      return this->gptr();
    }

    template <typename Elem, typename Traits>
    typename InputMemoryBuffer<Elem, Traits>::Size
    InputMemoryBuffer<Elem, Traits>::size() const throw ()
    {
      return this->egptr() - this->gptr();
    }

    template <typename Elem, typename Traits>
    typename InputMemoryBuffer<Elem, Traits>::Position
    InputMemoryBuffer<Elem, Traits>::seekoff(Offset off,
      std::ios_base::seekdir way, std::ios_base::openmode which)
      /*throw (eh::Exception)*/
    {
      if (which != std::ios_base::in)
      {
        return Position(Offset(-1)); // Standard requirements
      }

      Position pos(off);

      switch (way)
      {
      case std::ios_base::beg:
        break;

      case std::ios_base::cur:
        pos += this->gptr() - this->eback();
        break;

      case std::ios_base::end:
        pos = this->egptr() - this->eback() + pos;
        break;

      default:
        return Position(Offset(-1)); // Standard requirements
      }

      return seekpos(pos, which);
    }

    template <typename Elem, typename Traits>
    typename InputMemoryBuffer<Elem, Traits>::Position
    InputMemoryBuffer<Elem, Traits>::seekpos(Position pos,
      std::ios_base::openmode which) /*throw (eh::Exception)*/
    {
      if (which != std::ios_base::in)
      {
        return Position(Offset(-1)); // Standard requirements
      }

      Offset offset(pos);

      if (offset < 0 || offset > this->egptr() - this->eback())
      {
        return Position(Offset(-1)); // Standard requirements
      }

      return pos;
    }

    template <typename Elem, typename Traits>
    typename InputMemoryBuffer<Elem, Traits>::Int
    InputMemoryBuffer<Elem, Traits>::underflow() throw ()
    {
      return this->gptr() < this->egptr() ? *(this->gptr()) : Traits::eof();
    }


    //
    // OutputMemoryBuffer class
    //

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      OutputMemoryBuffer(Size initial_size,
      const AllocatorInitializer& allocator_initializer) /*throw (eh::Exception)*/
      : allocator_(allocator_initializer)
      , begin_(0)
      , ptr_(0)
      , end_(0)
    {
      Pointer ptr = 0;
      if (initial_size > 0)
      {
        ptr = allocator_.allocate(initial_size);
      }
      begin_ = ptr;
      ptr_ = begin_;
      end_ = begin_ + initial_size;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      ~OutputMemoryBuffer() throw ()
    {
      allocator_.deallocate(begin_, end_ - begin_);
      begin_ = 0;
      ptr_ = 0;
      end_ = 0;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::ConstPointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      data() const throw ()
    {    
      return begin_;
    }    

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Size
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      size() const throw ()
    {
      return ptr_ - begin_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Pointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      begin() throw ()
    {
      return begin_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Pointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      ptr() throw ()
    {
      return ptr_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Pointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      end() throw ()
    {
      return end_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    bool
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      extend() /*throw (eh::Exception)*/
    {
      Size old_size = end_ - begin_;
      Offset offset = ptr_ - begin_;

      Size new_size = 4096;
      while (new_size <= old_size)
      {
        new_size <<= 1;
        if (!new_size)
        {
          return false;
        }
      }

      Pointer ptr = allocator_.allocate(new_size);
      if (!ptr)
      {
        return false;
      }

      if (old_size > 0)
      {
        Traits::copy(ptr, begin_, old_size);
        allocator_.deallocate(begin_, old_size);
      }

      begin_ = ptr;
      ptr_ = begin_ + offset;
      end_ = begin_ + new_size;

      return true;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    void
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      incptr() throw()
    {
      ++ptr_;
    }

    //
    // MemoryBufferHolder class
    //

    template <typename Buffer>
    MemoryBufferHolder<Buffer>::MemoryBufferHolder() /*throw (eh::Exception)*/
    {
    }

    template <typename Buffer>
    template <typename T>
    MemoryBufferHolder<Buffer>::MemoryBufferHolder(T arg)
      /*throw (eh::Exception)*/
      : buffer_(arg)
    {
    }

    template <typename Buffer>
    template <typename T1, typename T2>
    MemoryBufferHolder<Buffer>::MemoryBufferHolder(T1 arg1, T2 arg2)
      /*throw (eh::Exception)*/
      : buffer_(arg1, arg2)
    {
    }

    template <typename Buffer>
    typename MemoryBufferHolder<Buffer>::SubString
    MemoryBufferHolder<Buffer>::str() const /*throw (eh::Exception)*/
    {
      return SubString(this->buffer()->data(),
        this->buffer()->size());
    }

    template <typename Buffer>
    template <typename Traits, typename Checker>
    String::BasicSubString<const typename MemoryBufferHolder<Buffer>::Elem,
      Traits, Checker>
    MemoryBufferHolder<Buffer>::str() const /*throw (eh::Exception)*/
    {
      return String::BasicSubString<const Elem, Traits, Checker>(
        this->buffer()->data(),
        this->buffer()->size());
    }

    template <typename Buffer>
    Buffer*
    MemoryBufferHolder<Buffer>::buffer() throw ()
    {
      return &buffer_;
    }

    template <typename Buffer>
    const Buffer*
    MemoryBufferHolder<Buffer>::buffer() const throw ()
    {
      return &buffer_;
    }


    //
    // InputMemoryStream class
    //

    template <typename Elem, typename Traits>
    InputMemoryStream<Elem, Traits>::InputMemoryStream(ConstPointer data)
      /*throw (eh::Exception)*/
      : Holder(const_cast<Pointer>(data), Traits::length(data)),
        Stream(this->buffer())
    {
    }

    template <typename Elem, typename Traits>
    InputMemoryStream<Elem, Traits>::InputMemoryStream(
      ConstPointer data, Size size) /*throw (eh::Exception)*/
      : Holder(const_cast<Pointer>(data), size), Stream(this->buffer())
    {
    }

    template <typename Elem, typename Traits>
    template <typename Char, typename STraits, typename Checker>
    InputMemoryStream<Elem, Traits>::InputMemoryStream(
      const String::BasicSubString< Char, STraits, Checker>& str)
      /*throw (eh::Exception)*/
      : Holder(const_cast<Pointer>(str.data()), str.size()),
        Stream(this->buffer())
    {
    }

    template <typename Elem, typename Traits>
    template <typename Char, typename STraits, typename Alloc>
    InputMemoryStream<Elem, Traits>::InputMemoryStream(
      const std::basic_string< Char, STraits, Alloc>& str)
      /*throw (eh::Exception)*/
      : Holder(const_cast<Pointer>(str.data()), str.size()),
        Stream(this->buffer())
    {
    }


    //
    // OutputMemoryStream class
    //

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      OutputMemoryStream(typename Allocator::size_type initial_size,
        const AllocatorInitializer& allocator_initializer)
      /*throw (eh::Exception)*/
      : Holder(initial_size, allocator_initializer)
      , bad_(false)
    {
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      append(Elem ch) /*throw (eh::Exception)*/
    {
      if (bad()) {
        return;
      }
      auto* buffer = this->buffer();
      if (buffer->ptr() == buffer->end() && !buffer->extend()) {
        bad_ = true;
        return;
      }
      *buffer->ptr() = ch;
      buffer->incptr();
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      append(const Elem* str) /*throw (eh::Exception)*/
    {
      while (*str) {
        append(*str++);
      }
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    bool
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      bad() throw ()
    {
      return bad_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void 
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      write(const Elem*, int) /*throw (eh::Exception)*/
    {
      // TODO
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void 
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      fill(Elem) /*throw (eh::Exception)*/
    {
      // TODO
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void 
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      width(int) /*throw (eh::Exception)*/
    {
      // TODO
    }

    template<typename Elem, typename Traits, typename Allocator, 
      typename AllocatorInitializer, const size_t SIZE, typename ArgT>
    Stream::MemoryStream::OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator<<(Stream::MemoryStream::OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
      const ArgT& arg) /*throw eh::Exception*/
    { 
      std::ostringstream oss;
      oss << arg;
      ostr << oss.str();
      return ostr;
    }

    namespace Allocator
    {
      //
      // Simple class
      //

      template <typename Elem, const size_t SIZE, typename Buffer,
        typename BufferInitializer>
      Simple<Elem, SIZE, Buffer, BufferInitializer>::Simple() throw ()
        : allocated_(false)
      {
        buffer_[SIZE - 1] = '\0';
      }

      template <typename Elem, const size_t SIZE, typename Buffer,
        typename BufferInitializer>
      Simple<Elem, SIZE, Buffer, BufferInitializer>::Simple(
        BufferInitializer buffer_initializer) throw ()
        : buffer_(buffer_initializer), allocated_(false)
      {
        buffer_[SIZE - 1] = '\0';
      }

      template <typename Elem, const size_t SIZE, typename Buffer,
        typename BufferInitializer>
      typename Simple<Elem, SIZE, Buffer, BufferInitializer>::Pointer
      Simple<Elem, SIZE, Buffer, BufferInitializer>::allocate(
        Size size, const void*) throw ()
      {
        if (allocated_ || size >= SIZE)
        {
          return 0;
        }
        allocated_ = true;
        return buffer_;
      }

      template <typename Elem, const size_t SIZE, typename Buffer,
        typename BufferInitializer>
      void
      Simple<Elem, SIZE, Buffer, BufferInitializer>::deallocate(
        Pointer ptr, Size size)
        throw ()
      {
        if (!allocated_ || ptr != buffer_ || size >= SIZE)
        {
          return;
        }
        allocated_ = false;
      }


      //
      // ArrayBuffer class
      //

      template <typename Elem, const size_t SIZE, typename Initializer>
      ArrayBuffer<Elem, SIZE, Initializer>::ArrayBuffer(
        Initializer /*initializer*/) throw ()
      {
      }

      template <typename Elem, const size_t SIZE, typename Initializer>
      ArrayBuffer<Elem, SIZE, Initializer>::operator Elem*() throw ()
      {
        return buffer_;
      }


      //
      // SimpleBuffer class
      //

      template <typename Elem, const size_t SIZE>
      SimpleBuffer<Elem, SIZE>::SimpleBuffer(Elem* buffer) throw ()
        : Simple<Elem, SIZE, Elem*>(buffer)
      {
      }


      //
      // SimpleStack class
      //

      template <typename Elem, const size_t SIZE>
      SimpleStack<Elem, SIZE>::SimpleStack(size_t /*allocator_initializer*/)
        throw ()
      {
      }
    }
  }


  //
  // Buffer class
  //

  template <const size_t SIZE>
  Buffer<SIZE>::Buffer(char* buffer) throw ()
    : MemoryStream::OutputMemoryStream<char, std::char_traits<char>,
        Allocator, Allocator, SIZE - 1>(SIZE - 1, Allocator(buffer))
  {
  }

  template <const size_t SIZE>
  Buffer<SIZE>::~Buffer() throw ()
  {
    *this << '\0';
  }
}

namespace eh
{
  template <typename Tag, typename Base>
  Composite<Tag, Base>::Composite(const Stream::Error& stream,
    const char* code) throw ()
  {
    const String::SubString& substr = stream.str();
    Base::init_(substr.data(), substr.size(), code);
  }
}
