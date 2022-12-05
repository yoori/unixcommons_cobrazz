/**
 * @file   Stream/BzlibStreams.hpp
 * @author Dmitry Trifilov
 */
#ifndef STREAM_BZLIB_STREAMS_HPP
#define STREAM_BZLIB_STREAMS_HPP

#include <istream>
#include <ostream>

#include <Stream/FileStreamBuf.hpp>


namespace Stream
{
  /**
   * Stream with std::istream interface to read bzip'ed files
   */
  class BzlibInStream :
    public std::basic_istream<char, std::char_traits<char> >
  {
  public:
    /**
     * Constructor transmit parameters to BzlibInStreambuf constructor
     * (aggregated member)
     * @param bzip_file_name File name to read and decompress data
     * @param buffer_size Memory size to be allocate for read data buffer
     * @param put_back_size The size of the data obtained in the previous
     * decompression query saved before new decompression. Each time buffer
     * will contain put_back_size bytes early loaded data.
     */
    explicit
    BzlibInStream(const char* bzip_file_name,
      size_t buffer_size = 64 * 1024, size_t put_back_size = 64)
      /*throw (eh::Exception)*/;

  protected:
    File::InStreamBuf buf_;
  };

  /**
   * Stream with std::ostream interface to write bzip'ed files
   */
  class BzlibOutStream :
    public std::basic_ostream<char, std::char_traits<char> >
  {
  public:
    /**
     * Constructor transmit parameters to BzlibOutStreambuf constructor
     * (aggregated member)
     * @param bzip_file_name File name to compress and write data
     * @param buffer_size Memory size to be allocate for write data buffer
     */
    explicit
    BzlibOutStream(const char* bzip_file_name,
      size_t buffer_size = 64 * 1024)
      /*throw (eh::Exception)*/;

  protected:
    File::OutStreamBuf buf_;
  };
}

#endif
