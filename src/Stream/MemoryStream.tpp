//
// Helpers for manipulators
//

namespace Stream::MemoryStream
{
  //
  // WidthOut
  //

  template<typename IntType>
  WidthOut<IntType>::WidthOut(const IntType& value, size_t width, char fill)
    noexcept
    : value_(value)
    , width_(width)
    , fill_(fill)
  {
  }

  template<typename IntType>
  IntType
  WidthOut<IntType>::Value() const noexcept
  {
    return value_;
  }

  template<typename IntType>
  size_t
  WidthOut<IntType>::Width() const noexcept
  {
    return width_;
  }

  template<typename IntType>
  char
  WidthOut<IntType>::Fill() const noexcept
  {
    return fill_;
  }

  template<typename IntType>
  WidthOut<IntType>
  width_out(const IntType& value, size_t width, char fill) noexcept
  {
    return WidthOut<IntType>(value, width, fill);
  }

  //
  // HexOut
  //

  template<typename Type>
  HexOut<Type>::HexOut(Type value, bool upcase) noexcept
    : value_(value)
    , upcase_(upcase)
  {
  }

  template<typename Type>
  Type
  HexOut<Type>::Value() const noexcept
  {
    return value_;
  }

  template<typename Type>
  bool
  HexOut<Type>::Upcase() const noexcept
  {
    return upcase_;
  }

  template<typename Type>
  HexOut<Type>
  hex_out(const Type& value, bool upcase) noexcept
  {
    return HexOut<Type>(value, upcase);
  }

  //
  // DoubleOut
  //

  template<typename Type>
  DoubleOut<Type>::DoubleOut(Type value, int precision) noexcept
    : value_(value)
    , precision_(precision)
  {
  }

  template<typename Type>
  Type
  DoubleOut<Type>::Value() const noexcept
  {
    return value_;
  }

  template<typename Type>
  int
  DoubleOut<Type>::Precision() const noexcept
  {
    return precision_;
  }

  template<typename Type>
  DoubleOut<Type>
  double_out(const Type& value, int precision) noexcept
  {
    return DoubleOut<Type>(value, precision);
  }
}

//
// Helpers for OutputMemoryStream& operator<<(OutputMemoryStream&,...)
//

namespace Stream::MemoryStream
{
  //
  // Helper class for
  //  template<typename...>
  //  OutputMemoryStream<...>& operator<<(OutputMemoryStream<...>&, const ArgT&)
  // to enable partial specialization
  //

  /**
   * Helper function for helper struct
   * @param arg - argument to be shrunk
   * @param remove_count - size of part to be removed
   */
  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, std::to_chars_result>::type
  to_chars_integral(char* first, char* last, const Type& arg)
  {
    // TODO optimize
    auto arg_str = std::to_string(arg);
    memcpy(first, arg_str.c_str(), last - first);
    return {last, std::errc()};
  }

  template<typename Type>
  std::enable_if<std::is_enum<Type>::value, std::to_chars_result>::type
  to_chars_integral(char* first, char* last, const Type& arg)
  {
    // TODO optimize
    auto arg_str = std::to_string(arg);
    memcpy(first, arg_str.c_str(), last - first);
    return {last, std::errc()};
  }

  template<typename T> auto constexpr is_integral_atomic = false;
  template<typename T> auto constexpr is_integral_atomic<std::atomic<T>> = std::is_integral<T>::value;
  template<typename T> auto constexpr is_integral_atomic<volatile std::atomic<T>> = std::is_integral<T>::value;

  template<typename Type>
  std::enable_if<is_integral_atomic<Type>, std::to_chars_result>::type
  to_chars_integral(char* first, char* last, const Type& value) /*throw (eh::Exception)*/
  {
//    static_assert(std::is_same<Type, typename std::remove_volatile<Type>::type>::value);
    return to_chars_integral(first, last, value.load());
  }

  template<typename Type>
  std::enable_if<!std::is_integral<Type>::value && !std::is_enum<Type>::value
    && !is_integral_atomic<Type>, std::to_chars_result>::type
  to_chars_integral(char*, char*, const Type&)
  {
    return std::to_chars_result{};
  }

  /**
   * Helper for operator<<(OutputMemoryStream&, const ArgT&), generalized version
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT,
    typename Enable = void>
  struct OutputMemoryStreamHelper
  {
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator()(OutputMemoryStream<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE>& ostr, const ArgT& arg) noexcept
    {
      if (ostr.bad())
      {
        return ostr;
      }
      try
      {
        std::ostringstream ss;
        ss << arg;
        ostr.append(ss.str().c_str());
      }
      catch (const eh::Exception&)
      {
        ostr.bad(true);
      }
      return ostr;
    }
  };

  /**
   * Helper for operator<<(OutputMemoryStream&, const ArgT&)
   * specialization for to_chars applicable types
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  struct OutputMemoryStreamHelper<Elem, Traits, Allocator, AllocatorInitializer, SIZE, ArgT,
    decltype(std::to_chars(std::string().data(), std::string().data(), ArgT()), void())>
  {
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
    operator()(OutputMemoryStream<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE>& ostr, const ArgT& arg)
    {
      if (ostr.bad())
      {
        return ostr;
      }
      try
      {
        size_t required = std::to_chars_len(arg);
        if (required == 0)
        {
          return ostr;
        }

        auto* buffer = ostr.buffer();
        size_t available = buffer->end() - buffer->ptr();

        while (required > available && buffer->extend()) {
          available = buffer->end() - buffer->ptr();
        }

        if (available > 0) {
          std::to_chars_result result;

          // workaround for integral types only
          // (for which std implements to_chars itself)
          if (required > available && (std::is_integral<ArgT>::value
             || std::is_enum<ArgT>::value || is_integral_atomic<ArgT>))
          {
            result = to_chars_integral(buffer->ptr(), buffer->end(), arg);
          }
          else
          {
            // original std::to_chars writes nothing if required > available.
            // therefore, for integral types (for which std implements to_chars itself)
            // we shrink arg to available memory and for all other custom types
            // we just implement to chars to partially fill memory region if required > available
            result = std::to_chars(buffer->ptr(), buffer->end(), arg);
          }

          if (result.ec == std::errc())
          {
            buffer->pbump(result.ptr - buffer->ptr());
          }
          else
          {
            ostr.bad(true);
          }
        }

        if (required > available) {
          ostr.bad(true);
        }
      }
      catch (const eh::Exception&)
      {
        ostr.bad(true);
      }
      return ostr;
    }
  };

  /**
   * Generalized template
   * Calls helper class operator(...)
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const ArgT& arg) /*throw eh::Exception*/
  {
    return OutputMemoryStreamHelper<Elem, Traits, Allocator,
      AllocatorInitializer, SIZE, ArgT>()(ostr, arg);
  }

  /**
   * String::BasicSubString
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE,
    typename SElem, typename STraits, typename SChecker>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const String::BasicSubString<SElem, STraits, SChecker>& arg) /*throw eh::Exception*/
  {
    ostr.write(arg.data(), arg.size());
    return ostr;
  }

  /**
   * String::string_view
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const std::string_view& arg) /*throw eh::Exception*/
  {
    ostr.write(arg.data(), arg.size());
    return ostr;
  }

  /**
   * std::string
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const std::string& arg) /*throw eh::Exception*/
  {
    ostr.append(arg.c_str());
    return ostr;
  }

  /**
   * const ArgT*, ArgT != char
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  std::enable_if<
    !std::is_same<Elem, ArgT>::value,
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>
  >::type&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const ArgT* arg) /*throw eh::Exception*/
  {
    // TODO optimize
    std::ostringstream stream;
    stream << arg;
    ostr << stream.str();
    return ostr;
  }

  /**
   * ArgT*, ArgT != char
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  std::enable_if<
    !std::is_same<Elem, ArgT>::value,
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>
  >::type&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    ArgT* arg) /*throw eh::Exception*/
  {
    // TODO optimize
    std::ostringstream stream;
    stream << arg;
    ostr << stream.str();
    return ostr;
  }

  /**
   * const char* OR const char[n]
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const Elem* arg) /*throw eh::Exception*/
  {
    ostr.append(arg);
    return ostr;
  }

  /**
   * char* OR char[n]
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    Elem* arg) /*throw eh::Exception*/
  {
    ostr.append(arg);
    return ostr;
  }

  /**
   * char overload
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    char arg) /*throw eh::Exception*/
  {
    ostr.append(arg);
    return ostr;
  }

  /**
   * bool overload
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    bool arg) /*throw eh::Exception*/
  {
    ostr.append(arg ? '1' : '0');
    return ostr;
  }

  /**
   * std::endl
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator<<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    std::basic_ostream<Elem, std::char_traits<Elem>>& (*)(std::basic_ostream<Elem, std::char_traits<Elem>>&))
    /*throw eh::Exception*/
  {
    ostr.append('\n');
    return ostr;
  }

}

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
      ~OutputMemoryBuffer() noexcept
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
      data() const noexcept
    {
      return begin_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Size
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      size() const noexcept
    {
      return ptr_ - begin_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Pointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      begin() noexcept
    {
      return begin_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Pointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      ptr() noexcept
    {
      return ptr_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer>
    typename OutputMemoryBuffer<Elem, Traits, Allocator,
      AllocatorInitializer>::Pointer
    OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer>::
      end() noexcept
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
      pbump(Offset off) noexcept
    {
      if (off > 0)
      {
        if (off > end_ - ptr_) {
          ptr_ = end_;
        } else {
          ptr_ += off;
        }
      }
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
    MemoryBufferHolder<Buffer>::buffer() noexcept
    {
      return &buffer_;
    }

    template <typename Buffer>
    const Buffer*
    MemoryBufferHolder<Buffer>::buffer() const noexcept
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
    // BaseOStream class
    //

    template<typename Elem>
    BaseOStream<Elem>::~BaseOStream() noexcept
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
      if (bad())
      {
        return;
      }
      auto* buffer = this->buffer();
      if (buffer->ptr() == buffer->end() && !buffer->extend())
      {
        bad_ = true;
        return;
      }
      *buffer->ptr() = ch;
      buffer->pbump(1);
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      append(const Elem* str) /*throw (eh::Exception)*/
    {
      int len = std::strlen(str);
      write(str, len);
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      write(const Elem* str, int len) /*throw (eh::Exception)*/
    {
      if (bad())
      {
        return;
      }
      auto* buffer = this->buffer();

      size_t required = len;
      size_t available = buffer->end() - buffer->ptr();
      while (required > available && buffer->extend()) {
        available = buffer->end() - buffer->ptr();
      }

      if (available > 0) {
        size_t append_count = std::min(available, required);
        std::memcpy(buffer->ptr(), str, append_count);
        buffer->pbump(append_count);
      }

      if (required > available) {
        bad_ = true;
      }
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    bool
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      bad() const  noexcept
    {
      return bad_;
    }

    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer, const size_t SIZE>
    void
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>::
      bad(bool value) noexcept
    {
      if (!bad_ && value) {
        bad_ = true;
      }
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
    this->append('\0');
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

namespace std
{
  //
  // WidthOut
  //

  template<typename IntType>
  size_t
  to_chars_len(const Stream::MemoryStream::WidthOut<IntType>& widthout)
    /*throw (eh::Exception)*/
  {
    IntType value = widthout.Value();
    bool is_neg = value < 0;
    if (is_neg) {
      value = -value;
    }
    size_t value_size = value == 0 ? 1 : trunc(log10(value)) + 1 + is_neg;
    size_t width = widthout.Width();

    return std::max(value_size, width);
  }

  template<typename IntType>
  std::to_chars_result
  to_chars(char* first, char* last, const Stream::MemoryStream::WidthOut<IntType>& widthout)
    /*throw (eh::Exception)*/
  {
    static_assert(std::is_integral<IntType>::value,
      "Only integral IntType is implemented for: template<IntType> class WidthOut<IntType>");

    IntType value = widthout.Value();
    bool is_neg = value < 0;
    if (is_neg) {
      value = -value;
    }
    size_t value_size = value == 0 ? 1 : trunc(log10(value)) + 1 + is_neg;
    size_t capacity = last - first;
    size_t width = widthout.Width();

    if (std::max(value_size, width) > capacity)
    {
      return {last, std::errc::value_too_large};
    }

    if (width > value_size)
    {
      size_t fill_size = width - value_size;
      std::fill_n(first, fill_size, widthout.Fill());
      first += fill_size;
    }

    if (value == 0)
    {
      *first++ = '0';
    }
    else
    {
      if (is_neg)
      {
        *first++ = '-';
      }
      for (char* ptr = first + (value_size - 1 - is_neg); value > 0; --ptr)
      {
        *ptr = static_cast<char>('0' + value % 10);
        value /= 10;
        ++first;
      }
    }

    return {first, std::errc()};
  }

  template<typename IntType>
  std::string
  to_string(const Stream::MemoryStream::WidthOut<IntType>& widthout)
    /*throw (eh::Exception) */
  {
    auto str = std::to_string(widthout.Value());
    auto width = widthout.Width();
    if (width > str.size())
    {
      return std::string(width - str.size(), widthout.Fill()) + str;
    }
    else
    {
      return str;
    }
  }

  //
  // HexOut
  //

  template<typename Type>
  size_t
  to_chars_len(const Stream::MemoryStream::HexOut<Type>& hexout)
    /*throw (eh::Exception) */
  {
    typedef typename std::make_unsigned<Type>::type UType;

    static constexpr UType HEX_MASK_WIDTH = 4;

    size_t len = 0;

    // convert to UType
    UType value = hexout.Value();
    if (!value)
    {
      len = 1;
    }
    else
    {
      while(value)
      {
        ++len;
        value >>= HEX_MASK_WIDTH;
      }
    }

    return len;
  }

  template<typename Type>
  std::to_chars_result
  to_chars(char* first, char* last, const Stream::MemoryStream::HexOut<Type>& hexout)
    /*throw (eh::Exception) */
  {
    static_assert(std::is_integral<Type>::value,
      "Only integral Type is implemented for: template<Type> class HexOut<Type>");

    // Problem:
    // int value = -10;
    // std::cout << std::hex << value; --> gives 'fffffff6'
    // std::to_chars(first, last, value, 16); --> gives '-a'
    //
    // Solution:
    // cast to unsigned :)

    static constexpr int BASE = 16;
    typename std::make_unsigned<Type>::type value = hexout.Value();

    auto result = std::to_chars(first, last, value, BASE);

    if (result.ec == std::errc() && hexout.Upcase())
    {
      std::for_each(first, result.ptr, [](char& c)
        {
          c = std::toupper(c);
        });
    }

    return result;
  }

  template<typename Type>
  std::string
  to_string(const Stream::MemoryStream::HexOut<Type>& hexout)
    /*throw (eh::Exception) */
  {
    typedef typename std::make_unsigned<Type>::type UType;

    static constexpr size_t MAX_UTYPE_HEX_WIDTH = 2 * sizeof(UType);
    std::string result;
    result.reserve(MAX_UTYPE_HEX_WIDTH);

    static constexpr UType HEX_MASK_WIDTH = 4;
    static constexpr UType HEX_MASK = (1 << HEX_MASK_WIDTH) - 1;
    static constexpr char DIGITS[] =
    {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    // convert to UType
    UType value = hexout.Value();
    if (!value)
    {
      result.push_back('0');
    }
    else
    {
      while(value)
      {
        result.push_back(DIGITS[value & HEX_MASK]);
        value >>= HEX_MASK_WIDTH;
      }
    }

    if (hexout.Upcase())
    {
      std::for_each(result.begin(), result.end(), [](char& c)
        {
          c = std::toupper(c);
        });
    }

    std::reverse(result.begin(), result.end());
    result.shrink_to_fit();
    return result;
  }

  //
  // DoubleOut
  //

  template<typename Type>
  size_t
  to_chars_len(const Stream::MemoryStream::DoubleOut<Type>& doubleout)
    /*throw (eh::Exception) */
  {
    // TODO : optimize
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(doubleout.Precision()) << doubleout.Value();
    return stream.str().size();
  }

  template<typename Type>
  std::to_chars_result
  to_chars(char* first, char* last, const Stream::MemoryStream::DoubleOut<Type>& doubleout)
    /*throw (eh::Exception) */
  {
    static_assert(std::is_floating_point<Type>::value,
      "Only floating point Type is implemented for: template<Type> class DoubleOut<Type>");

    // TODO : optimize
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(doubleout.Precision()) << doubleout.Value();
    std::string str = stream.str();
    size_t capacity = last - first;
    if (str.size() > capacity)
    {
      return {last, std::errc::value_too_large};
    }
    memcpy(first, str.data(), str.size());
    return {first + str.size(), std::errc()};
  }

  template<typename Type>
  std::string
  to_string(const Stream::MemoryStream::DoubleOut<Type>& doubleout)
    /*throw (eh::Exception) */
  {
    // TODO optimize
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(doubleout.Precision()) << doubleout.Value();
    return stream.str();
  }

  //
  // to_chars floating_point overload
  //

  template<typename FloatType>
  std::enable_if<std::is_floating_point<FloatType>::value, size_t>::type
  to_chars_len(FloatType value) /*throw (eh::Exception)*/
  {
    // TODO optimize
    std::ostringstream stream;
    stream << value;
    return stream.str().size();
  }

  template<typename FloatType>
  std::enable_if<std::is_floating_point<FloatType>::value, std::to_chars_result>::type
  to_chars(char* first, char* last, FloatType value) /*throw (eh::Exception)*/
  {
    // TODO optimize
    std::ostringstream stream;
    stream << value;
    std::string str = stream.str();
    size_t capacity = last - first;
    if (str.size() > capacity)
    {
      return {last, std::errc::value_too_large};
    }
    memcpy(first, str.data(), str.size());
    return {first + str.size(), std::errc()};
  }

  template<typename FloatType>
  std::enable_if<std::is_floating_point<FloatType>::value, std::string>::type
  to_string(FloatType value) /*throw (eh::Exception)*/
  {
    // TODO optimize
    std::ostringstream stream;
    stream << value;
    return stream.str();
  }

  //
  // integral types to_chars_len
  //

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, size_t>::type
  to_chars_len(IntType value) /*throw (eh::Exception)*/
  {
    bool is_neg = value < 0;
    if (is_neg) {
      value = -value;
    }
    return value == 0 ? 1 : trunc(log10(value)) + 1 + is_neg;
  }

  template<typename IntType>
  std::enable_if<std::is_enum<IntType>::value, size_t>::type
  to_chars_len(IntType value) /*throw (eh::Exception)*/
  {
    // TODO optimize
    return std::to_string(value).size();
  }

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, size_t>::type
  to_chars_len(const volatile std::atomic<IntType>& value) /*throw (eh::Exception)*/
  {
    return to_chars_len(value.load());
  }
}
