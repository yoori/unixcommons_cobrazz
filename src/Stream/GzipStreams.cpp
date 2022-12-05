/**
 * @file   Stream/GzipStreams.cpp
 * @author Dmitry Trifilov
 */

#include <zlib.h>

#include <eh/Errno.hpp>

#include <Generics/Function.hpp>

#include <Stream/GzipStreams.hpp>
#include <Stream/MemoryStream.hpp>


namespace
{
  template <typename InvalidArgument, typename IOError>
  class FileHandleAdapter : public Stream::File::IO
  {
  public:
    FileHandleAdapter(const char* file_name, const char* mode)
      /*throw (InvalidArgument, eh::Exception)*/;

    ~FileHandleAdapter() throw ();

    size_t
    read(void* buf, size_t size) /*throw (eh::Exception)*/;

    void
    write(const void* buf, size_t size) /*throw (eh::Exception)*/;

  private:
    gzFile gzip_handle_;
  };

  template <typename InvalidArgument, typename IOError>
  FileHandleAdapter<InvalidArgument, IOError>::FileHandleAdapter(
    const char* file_name, const char* mode)
    /*throw (InvalidArgument, eh::Exception)*/
  {
    if (!file_name)
    {
      Stream::Error ostr;
      ostr << FNS << "file_name is NULL";
      throw InvalidArgument(ostr);
    }

    gzip_handle_ = ::gzopen(file_name, mode);
    if (gzip_handle_ == 0)
    {
      if (errno)
      {
        eh::throw_errno_exception<InvalidArgument>(FNE,
          "::gzopen(\"", file_name, "\", \"", mode, "\") failed");
      }
      else
      {
        Stream::Error ostr;
        ostr << FNS << "::gzopen(\"" << file_name << "\", \"" << mode <<
          "\") failed. Error code is Z_MEM_ERROR";
        throw InvalidArgument(ostr);
      }
    }
  }

  template <typename InvalidArgument, typename IOError>
  FileHandleAdapter<InvalidArgument, IOError>::~FileHandleAdapter() throw ()
  {
    ::gzclose(gzip_handle_);
  }

  template <typename InvalidArgument, typename IOError>
  size_t
  FileHandleAdapter<InvalidArgument, IOError>::read(void* buf, size_t size)
    /*throw (eh::Exception)*/
  {
    int bytes_read = ::gzread(gzip_handle_, buf, size);
    if (bytes_read < 0)
    {
      int gz_errnum = 0;
      ::gzerror(gzip_handle_, &gz_errnum);

      Stream::Error ostr;
      ostr << FNS << "::gzread has returned error. Error code = " <<
        gz_errnum;
      throw IOError(ostr);
    }
    return bytes_read;
  }

  template <typename InvalidArgument, typename IOError>
  void
  FileHandleAdapter<InvalidArgument, IOError>::write(
    const void* buf, size_t size) /*throw (eh::Exception)*/
  {
    int bytes_written = ::gzwrite(gzip_handle_, buf, size);
    if (bytes_written <= 0)
    {
      int gz_errnum = 0;
      ::gzerror(gzip_handle_, &gz_errnum);

      Stream::Error ostr;
      ostr << FNS << "::gzwrite has returned error. Error code = " <<
        gz_errnum;
      throw IOError(ostr);
    }
  }
}

namespace Stream
{
  GzipInStream::GzipInStream(const char* gzip_file_name,
    size_t buffer_size, size_t put_back_size)
    /*throw (eh::Exception)*/
    : std::basic_istream<char, std::char_traits<char> >(0),
      buf_(new FileHandleAdapter<
        File::InStreamBuf::InvalidArgument,
        File::InStreamBuf::Underflow>(gzip_file_name, "rb"),
        buffer_size, put_back_size)
  {
    init(&buf_);
  }

  GzipOutStream::GzipOutStream(const char* gzip_file_name,
    size_t buffer_size)
    /*throw (eh::Exception)*/
    : std::basic_ostream<char, std::char_traits<char> >(0),
      buf_(new FileHandleAdapter<
        File::OutStreamBuf::InvalidArgument,
        File::OutStreamBuf::Overflow>(gzip_file_name, "wb"), buffer_size)
  {
    init(&buf_);
  }
}
