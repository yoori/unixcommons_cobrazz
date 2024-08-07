// @file Stream/MemoryStream.hpp
#ifndef STREAM_MEMORYSTREAM_HPP
#define STREAM_MEMORYSTREAM_HPP

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>

#include <sys/param.h>

#include <String/SubString.hpp>

//
// iomanip-like helpers
//

namespace Stream::MemoryStream
{
  //
  // Helper for std::setw and std::setfill
  //

  template<typename IntType>
  class WidthOut
  {
  public:
    /**
     * class to store state of std::setw + std::setfill
     * @param value - value to be stored
     * @param width - width value, width == 0 means width is not set
     * @param fill - setfill value, works only if width > 0
     */
    WidthOut(const IntType& value, size_t width = 0, char fill = ' ') noexcept;

    /**
     * @return value to be printed
     */
    IntType value() const noexcept;

    /**
     * @return output width value
     */
    size_t width() const noexcept;

    /**
     * @return fill character value
     */
    char fill() const noexcept;

  private:
    IntType value_;
    size_t width_;
    char fill_;
  };

  /**
   * iomanip-like helper for std::setw + std::setfill
   * @param value - value to be printed
   * @param width - width value
   * @param fill - setfill value
   * @return Widthout object
   */
  template<typename IntType>
  WidthOut<IntType>
  width_out(const IntType& value, size_t width = 0, char fill = ' ') noexcept;

  //
  // Helper for std::hex and std::uppercase
  //

  template<typename Type>
  class HexOut
  {
  public:
    /**
     * class to store state of std::hex + std::uppercase
     * @param value - value to be stored
     * @param upcase - uppercase value
     */
    HexOut(const Type& value, bool upcase = false, size_t width = 0, char fill = ' ') noexcept;

    /**
     * @return value to be printed
     */
    Type value() const noexcept;

    /**
     * @return whether uppercase letters must be used for value
     */
    bool upcase() const noexcept;

    /**
     * @return output width value
     */
    size_t width() const noexcept;

    /**
     * @return fill character value
     */
    char fill() const noexcept;

  private:
    Type value_;
    bool upcase_;
    size_t width_;
    char fill_;
  };

  /**
   * iomanip-like helper for std::hex + std::uppercase
   * @param value - value to be printed
   * @param upcase - uppercase flag
   * @return HexOut object
   */
  template<typename Type>
  HexOut<Type>
  hex_out(const Type& value, bool upcase = false, size_t width = 0, char fill = ' ') noexcept;

  //
  // Helper for std::setprecision and std::fixed
  //

  template<typename Type>
  class DoubleOut
  {
  public:
    /**
     * class to store state of std::setprecision (with obligatory std::fixed)
     * @param value - value to be stored
     * @param precision - fixed precision of value
     */
    DoubleOut(const Type& value, size_t precision = 0) noexcept;

    /**
     * @return value to be printed
     */
    Type value() const noexcept;

    /**
     * @return fixed precision
     */
    size_t precision() const noexcept;

  private:
    Type value_;
    size_t precision_;
  };

  /**
   * iomanip-like helper for std::setprecision (+ obligatory std::fixed)
   * @param value - value to be printed
   * @param precision - fixed precision of value
   * @return DoubleOut object
   */
  template<typename Type>
  DoubleOut<Type>
  double_out(const Type& value, size_t precision) noexcept;
}

namespace std
{

  //
  // Helper for std::setw and std::setfill
  //

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, size_t>::type
  to_chars_len(const Stream::MemoryStream::WidthOut<IntType>&)
    noexcept;

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, std::to_chars_result>::type
  to_chars(char*, char*, const Stream::MemoryStream::WidthOut<IntType>&)
    noexcept;

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, std::string>::type
  to_string(const Stream::MemoryStream::WidthOut<IntType>&)
    noexcept;

  //
  // Helper for std::hex and std::uppercase
  //

  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, size_t>::type
  to_chars_len(const Stream::MemoryStream::HexOut<Type>&)
    noexcept;

  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, std::to_chars_result>::type
  to_chars(char*, char*, const Stream::MemoryStream::HexOut<Type>&)
    noexcept;

  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, std::string>::type
  to_string(const Stream::MemoryStream::HexOut<Type>&)
    noexcept;

  //
  // Helper for std::setprecision and std::fixed
  //

  template<typename Type>
  std::enable_if<std::is_floating_point<Type>::value, size_t>::type
  to_chars_len(const Stream::MemoryStream::DoubleOut<Type>&)
    /*throw (eh::Exception) */;

  template<typename Type>
  std::enable_if<std::is_floating_point<Type>::value, std::to_chars_result>::type
  to_chars(char*, char*, const Stream::MemoryStream::DoubleOut<Type>&)
    /*throw (eh::Exception) */;

  template<typename Type>
  std::enable_if<std::is_floating_point<Type>::value, std::string>::type
  to_string(const Stream::MemoryStream::DoubleOut<Type>&)
    /*throw (eh::Exception) */;

  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, size_t>::type
  to_chars_len(const Stream::MemoryStream::DoubleOut<Type>&)
    noexcept;

  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, std::to_chars_result>::type
  to_chars(char*, char*, const Stream::MemoryStream::DoubleOut<Type>&)
    noexcept;

  template<typename Type>
  std::enable_if<std::is_integral<Type>::value, std::string>::type
  to_string(const Stream::MemoryStream::DoubleOut<Type>&)
    noexcept;

  //
  // integral and enum types to_chars_len
  //

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, size_t>::type
  to_chars_len(IntType) noexcept;

  template<typename IntType>
  std::enable_if<std::is_enum<IntType>::value, size_t>::type
  to_chars_len(IntType) noexcept;

  template<typename IntType>
  std::enable_if<std::is_integral<IntType>::value, size_t>::type
  to_chars_len(const volatile std::atomic<IntType>&) noexcept;
}

namespace Stream
{
  /**
   * These classes are designed to be "less costly" versions of ostringstream
   * and istringstream in terms of allocations and copying.
   * Based on Andrey Makarov's work.
   */
  namespace MemoryStream
  {
    /**
     * Input memory buffer
     * Using supplied memory region as a stream content
     * No allocations are performed
     */
    template <typename Elem, typename Traits>
    class InputMemoryBuffer : public std::basic_streambuf<Elem, Traits>
    {
    public:
      typedef typename Traits::int_type Int;
      typedef typename Traits::pos_type Position;
      typedef typename Traits::off_type Offset;

      typedef Elem* Pointer;
      typedef const Elem* ConstPointer;
      typedef size_t Size;

      /**
       * Constructor
       * @param ptr address of memory region
       * @param size size of memory region
       */
      InputMemoryBuffer(Pointer ptr, Size size)
        /*throw (eh::Exception)*/;

      /**
       * @return The pointer to data not read yet
       */
      ConstPointer
      data() const noexcept;

      /**
       * @return The size of data not read yet
       */
      Size
      size() const noexcept;

    protected:
      virtual
      Position
      seekoff(Offset off, std::ios_base::seekdir way,
        std::ios_base::openmode which) /*throw (eh::Exception)*/;

      virtual
      Position
      seekpos(Position pos, std::ios_base::openmode which)
        /*throw (eh::Exception)*/;

      virtual
      Int
      underflow() noexcept;
    };

    /**
     * Output memory buffer
     * Can preallocate memory region of desired size
     */
    template <typename Elem, typename Traits, typename Allocator,
      typename AllocatorInitializer = Allocator>
    class OutputMemoryBuffer
    {
    public:
      // for InputMemoryBuffer and MemoryBufferHolder compatibility
      typedef typename Traits::char_type char_type;

      typedef typename Traits::int_type Int;
      typedef typename Traits::pos_type Position;
      typedef typename Traits::off_type Offset;

      typedef std::allocator_traits<Allocator> AllocatorTraits;
      typedef typename AllocatorTraits::pointer Pointer;
      typedef typename AllocatorTraits::const_pointer ConstPointer;
      typedef typename AllocatorTraits::size_type Size;

      /**
       * Constructor
       * @param initial_size preallocated region size
       * @param args allocator initializer
       */
      OutputMemoryBuffer(Size initial_size = 0,
        const AllocatorInitializer& allocator_initializer =
          AllocatorInitializer()) /*throw (eh::Exception)*/;

      /**
       * Destructor
       * Frees allocated memory region
       */
      virtual
      ~OutputMemoryBuffer() noexcept;

      /**
       * @return The pointer to filled data
       */
      ConstPointer
      data() const noexcept;

      /**
       * @return The size of filled data
       */
      Size
      size() const noexcept;

    public:
      /**
       * @return The pointer to first elem of data region
       */
      Pointer
      begin() noexcept;

      /**
       * @return The pointer to one past last elem of data region
       */
      Pointer
      end() noexcept;

      /**
       * @return The pointer to one past last filled elem of data
       */
      Pointer
      ptr() noexcept;

      /**
       * Extends allocated data region
       * @return whether or not extension was successful
       */
      bool
      extend() /*throw (eh::Exception)*/;

      /**
       * advance ptr() off steps forward but not more than end()
       */
      void
      pbump(Offset off) noexcept;

    private:
      Allocator allocator_;
      Pointer begin_;
      Pointer ptr_;
      Pointer end_;
    };

    /**
     * Initializer of MemoryBuffer
     * Required because of order of construction
     */
    template <typename Buffer>
    class MemoryBufferHolder
    {
    public:
      typedef typename Buffer::char_type Elem;

      typedef typename String::BasicSubString<const Elem,
        String::CharTraits<typename std::remove_const<Elem>::type>,
          String::CheckerNone<Elem> > SubString;

      /**
       * Constructor
       * Constructs memory buffer without parameters
       */
      MemoryBufferHolder() /*throw (eh::Exception)*/;

      /**
       * Constructor
       * Constructs memory buffer with one parameter
       * @param var parameter for buffer's constructor
       */
      template <typename T>
      MemoryBufferHolder(T var) /*throw (eh::Exception)*/;

      /**
       * Constructor
       * Constructs memory buffer with two parameters
       * @param var1 the first parameter for buffer's constructor
       * @param var2 the second parameter for buffer's constructor
       */
      template <typename T1, typename T2>
      MemoryBufferHolder(T1 var1, T2 var2) /*throw (eh::Exception)*/;

      /**
       * Return SubString based on this buffer. Buffer can be without
       * zero at the end.
       * @return SubString spreads the buffer
       */
      SubString
      str() const /*throw (eh::Exception)*/;

      /**
       * Templatized version of string which allow get SubString
       * with any suitable Traits and Checker types
       * @return SubString spreads the buffer
       */
      template <typename Traits, typename Checker>
      String::BasicSubString<const Elem, Traits, Checker>
      str() const /*throw (eh::Exception)*/;

    protected:
      /**
       * @return pointer to holding buffer
       */
      Buffer*
      buffer() noexcept;

      /**
       * @return pointer to holding buffer
       */
      const Buffer*
      buffer() const noexcept;

    private:
      Buffer buffer_;
    };

    /**
     * Input memory stream. Uses InputMemoryBuffer for data access.
     */
    template <typename Elem, typename Traits = std::char_traits<Elem> >
    class InputMemoryStream :
      public MemoryBufferHolder<InputMemoryBuffer<Elem, Traits> >,
      public std::basic_istream<Elem, Traits>
    {
    private:
      typedef MemoryBufferHolder<InputMemoryBuffer<Elem, Traits> > Holder;
      typedef std::basic_istream<Elem, Traits> Stream;

    public:
      typedef Elem* Pointer;
      typedef const Elem* ConstPointer;
      typedef size_t Size;

      /**
       * Constructor
       * Passes data and Traits::length(data) to InputMemoryBlock's
       * constructor
       * @param data address of memory region
       */
      InputMemoryStream(ConstPointer data)
        /*throw (eh::Exception)*/;

      /**
       * Constructor
       * Passes parameters to InputMemoryBlock's constructor
       * @param data address of memory region
       * @param size size of memory region
       */
      InputMemoryStream(ConstPointer data, Size size)
        /*throw (eh::Exception)*/;

      /**
       * Constructor
       * Passes str.data() and str.size() to InputMemoryBlock's constructor
       * @param str memory region, should not be temporal
       */
      template <typename Char, typename STraits, typename Checker>
      InputMemoryStream(
        const String::BasicSubString<Char, STraits, Checker>& str)
        /*throw (eh::Exception)*/;

      /**
       * Constructor
       * Passes str.data() and str.size() to InputMemoryBlock's constructor
       * @param str memory region, should not be temporal
       */
      template <typename Char, typename STraits, typename Alloc>
      InputMemoryStream(const std::basic_string<Char, STraits, Alloc>& str)
        /*throw (eh::Exception)*/;
    };

    /**
     * Output memory stream. Uses OutputMemoryBuffer for data access.
     */
    template <typename Elem, typename Traits = std::char_traits<Elem>,
      typename Allocator = std::allocator<Elem>,
      typename AllocatorInitializer = Allocator, const size_t SIZE = 0>
    class OutputMemoryStream :
      public MemoryBufferHolder<
        OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer> >
    {
    private:
      typedef MemoryBufferHolder<
        OutputMemoryBuffer<Elem, Traits, Allocator, AllocatorInitializer> >
          Holder;

    public:
      /**
       * Constructor
       * Passes parameters to OutputMemoryBlock's constructor
       * @param initial_size preallocated region size
       * @param allocator_initializer allocator initializer
       */
      explicit
      OutputMemoryStream(typename Allocator::size_type initial_size = SIZE,
        const AllocatorInitializer& allocator_initializer =
          AllocatorInitializer()) /*throw (eh::Exception)*/;

      /**
       * append one character to filled part of memory region
       * set bad flag if char can not be appended
       * @param ch elemen to be appended
       */
      void append(Elem ch) /*throw (eh::Exception)*/;

      /**
       * append null terminated charater sequence after filled part of memory region
       * append as much of str as possible (try extend() if end() is reached)
       * if str is appended partially, then set bad flag
       * @param str null terminated character sequence
       */
      void append(const Elem* str) /*throw (eh::Exception)*/;

      /**
       * append size elements of str to filled part of memory region
       * copy block of data without checking null characters
       * @param str character sequence to be appended
       * @param size amount of characters to be appended
       */
      void write(const Elem* str, int len) /*throw (eh::Exception)*/;

      /**
       * @return true if last append failed because memory region capacity reached
       */
      bool bad() const noexcept;

      /**
       * if stream bad state is true, all stream write operations will do nothing
       * if stream bad state is true, calling bad(false) has no effect
       * @param value - set stream state to value
       */
      void bad(bool value) noexcept;

    private:
      bool bad_;

      template<typename HelperImplElem, typename HelperImplTraits, typename HelperImplAllocator,
        typename HelperImplAllocatorInitializer, const size_t HelperImplSIZE, typename HelperImplArgT,
        typename HelperImplToCharsLen, typename HelperImplToChars, typename HelperImplToString>
      friend
      OutputMemoryStream<HelperImplElem, HelperImplTraits, HelperImplAllocator,
        HelperImplAllocatorInitializer, HelperImplSIZE>&
      OutputMemoryStreamHelperImpl(OutputMemoryStream<HelperImplElem, HelperImplTraits, HelperImplAllocator,
        HelperImplAllocatorInitializer, HelperImplSIZE>& ostr, const HelperImplArgT& arg,
        HelperImplToCharsLen to_chars_len, HelperImplToChars to_chars, HelperImplToString to_string);
    };

    namespace Allocator
    {
      template <typename Elem, const size_t SIZE, typename Buffer,
        typename BufferInitializer = Buffer>
      class Simple : public std::allocator<Elem>
      {
      public:
        typedef std::allocator<Elem> Allocator;
        typedef std::allocator_traits<Allocator> AllocatorTraits;
        typedef AllocatorTraits::pointer Pointer;
        typedef AllocatorTraits::size_type Size;

        /**
         * Constructor without parameters
         */
        Simple() noexcept;

        /**
         * Constructor with buffer_ init value
         * @param buffer_initializer initializer for buffer_
         */
        Simple(BufferInitializer buffer_initializer) noexcept;

        /**
         * Allocation function
         * Allows to allocate SIZE bytes one time in a row
         * @param size should be equal to SIZE
         * @return pointer to size_ternal buffer
         */
        Pointer
        allocate(Size size, const void* = 0)
          noexcept;

        /**
         * Deallocation function
         * Deallocates previously allocated memory
         * @param ptr should be equal to the pointer returned by allocate()
         * @param size should be equal to SIZE
         */
        void
        deallocate(Pointer ptr, Size size) noexcept;

      private:
        Buffer buffer_;
        bool allocated_;
      };

      /**
       * Simple buffer allocator
       * Allows a single allocation on preallocated buffer
       */
      template <typename Elem, const size_t SIZE>
      class SimpleBuffer : public Simple<Elem, SIZE, Elem*>
      {
      public:
        /**
         * Constructor
         * @param buffer preallocated buffer of size not less than SIZE
         */
        explicit
        SimpleBuffer(Elem* buffer) noexcept;
      };

      template <typename Elem, const size_t SIZE, typename Initializer>
      class ArrayBuffer
      {
      public:
        explicit
        ArrayBuffer(Initializer initializer = Initializer()) noexcept;

        operator Elem*() noexcept;

      private:
        Elem buffer_[SIZE];
      };

      /**
       * Simple stack allocator
       * Required for disuse of heap for OutputStream
       */
      template <typename Elem, const size_t SIZE>
      class SimpleStack :
        public Simple<Elem, SIZE, ArrayBuffer<Elem, SIZE, size_t>, size_t>
      {
      public:
        /**
         * Constructor
         */
        explicit
        SimpleStack(size_t allocator_initializer) noexcept;
      };
    }
  }


  // Input memory streams working on external buffer
  typedef MemoryStream::InputMemoryStream<char> Parser;
  typedef MemoryStream::InputMemoryStream<wchar_t> WParser;

  // Dynamic memory output stream with preallocation
  typedef MemoryStream::OutputMemoryStream<char> Dynamic;


  /**
   * Output memory stream working on external memory buffer of size
   * not less than SIZE. No more than SIZE-1 chars are written size_to
   * the buffer and buffer is always zero terminated after the
   * destruction of the stream.
   *
   * Example:
   * char buf[10];
   * {
   *   Buffer<5> ostr(buf);
   *   ostr << something;
   *   // buf IS NOT required to be nul-terminated here
   * }
   * // buf IS nul-terminated here. strlen(buf) <= 4.
   */
  template <const size_t SIZE>
  class Buffer :
    public MemoryStream::OutputMemoryStream<char, std::char_traits<char>,
      MemoryStream::Allocator::SimpleBuffer<char, SIZE>,
      MemoryStream::Allocator::SimpleBuffer<char, SIZE>, SIZE - 1>
  {
  private:
    typedef MemoryStream::Allocator::SimpleBuffer<char, SIZE> Allocator;

  public:
    /**
     * Constructor
     * @param buffer buffer to make output to of size not less than SIZE
     */
    explicit
    Buffer(char* buffer) noexcept;

    /**
     * Destructor
     * Appends nul-terminating character to the buffer
     */
    ~Buffer() noexcept;
  };

  /**
   * Output memory stream holding memory buffer of size SIZE+1.
   */
  template <const size_t SIZE>
  class Stack :
    public MemoryStream::OutputMemoryStream<char, std::char_traits<char>,
      MemoryStream::Allocator::SimpleStack<char, SIZE + 1>, size_t, SIZE>
  {
  };

  /**
   * Default class for throwing DescriptiveException successors
   */
  class Error : public Stack<sizeof(eh::DescriptiveException)>
  {
  };

  /**
   * Helper class for forming of file names in preallocated buffers of
   * enough size
   */
  typedef Buffer<MAXPATHLEN> FileName;
}

namespace Stream::MemoryStream
{
  /**
   * Generalized template for operator <<(Stream::MemoryStream::OutputMemoryStream&, ...)
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const ArgT& arg) /*throw eh::Exception*/;

  /**
   * String::BasicSubString
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE,
    typename SElem, typename STraits, typename SChecker>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const String::BasicSubString<SElem, STraits, SChecker>& arg) /*throw eh::Exception*/;

  /**
   * String::string_view
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const std::string_view& arg) /*throw eh::Exception*/;

  /**
   * std::string
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const std::string& arg) /*throw eh::Exception*/;

  /**
   * const ArgT*, ArgT != char
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  std::enable_if<
    !std::is_same<Elem, ArgT>::value,
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>
  >::type&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const ArgT* arg) /*throw eh::Exception*/;

  /**
   * ArgT*, ArgT != char
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE, typename ArgT>
  std::enable_if<
    !std::is_same<Elem, ArgT>::value,
    OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>
  >::type&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    ArgT* arg) /*throw eh::Exception*/;

  /**
   * const char* + const char[n]
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    const Elem* arg) /*throw eh::Exception*/;

  /**
   * char* + char[n]
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    Elem* arg) /*throw eh::Exception*/;

  /**
   * char overload
   * decltype(std::to_chars(..., ArgT()), ...) actually takes char too
   * but we want char to be treated like char, do not apply to_chars
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    char arg) /*throw eh::Exception*/;

  /**
   * unsigned char overload
   * decltype(std::to_chars(..., ArgT()), ...) actually takes unsigned char too
   * but we want unsigned char to be treated like unsigned char, do not apply to_chars
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    unsigned char arg) /*throw eh::Exception*/;

  /**
   * signed char overload
   * decltype(std::to_chars(..., ArgT()), ...) actually takes unsigned char too
   * but we want signed char to be treated like signed char, do not apply to_chars
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    signed char arg) /*throw eh::Exception*/;

  /**
   * bool overload
   * do not use general overload for bool
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    bool arg) /*throw eh::Exception*/;

  /**
   * std::endl
   */
  template<typename Elem, typename Traits, typename Allocator,
    typename AllocatorInitializer, const size_t SIZE>
  OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>&
  operator <<(OutputMemoryStream<Elem, Traits, Allocator, AllocatorInitializer, SIZE>& ostr,
    std::basic_ostream<Elem, std::char_traits<Elem>>& (*)(std::basic_ostream<Elem, std::char_traits<Elem>>&))
    /*throw eh::Exception*/;
}

#include <Stream/MemoryStream.tpp>

#endif
