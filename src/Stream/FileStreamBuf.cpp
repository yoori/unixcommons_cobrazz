#include <iostream>
#include <cassert>

#include <Generics/Function.hpp>

#include <Stream/MemoryStream.hpp>
#include <Stream/FileStreamBuf.hpp>


namespace Stream
{
  namespace File
  {
    //
    // IO class
    //

    IO::~IO() throw ()
    {
    }


    //
    // InStreamBuf class
    //

    InStreamBuf::InStreamBuf(IO* io, size_t buffer_size,
      size_t put_back_size)
      /*throw (InvalidArgument, eh::Exception)*/
      : IO_(io), BUFFER_SIZE_(buffer_size), PUT_BACK_SIZE_(put_back_size)
    {
      if (BUFFER_SIZE_ <= PUT_BACK_SIZE_)
      {
        Stream::Error ostr;
        ostr << FNS << "Wrong buffer size parameters. "
          "Put back size parameter must be less than buffer size";
        throw InvalidArgument(ostr);
      }

      in_buffer_.reset(BUFFER_SIZE_);

      setg(in_buffer_.get() + PUT_BACK_SIZE_,
        in_buffer_.get() + PUT_BACK_SIZE_,
      in_buffer_.get() + PUT_BACK_SIZE_);
    }

    InStreamBuf::int_type
    InStreamBuf::underflow() /*throw (Underflow, eh::Exception)*/
    {
      assert (gptr() == egptr());

      size_t number_to_put_back =
        std::min(static_cast<size_t>(gptr() - eback()), PUT_BACK_SIZE_);
      if (number_to_put_back)
      {
        std::copy(gptr() - number_to_put_back, gptr(),
          in_buffer_.get() + (PUT_BACK_SIZE_ - number_to_put_back));
      }

      size_t bytes_read = IO_->read(in_buffer_.get() + PUT_BACK_SIZE_,
        BUFFER_SIZE_- PUT_BACK_SIZE_);
      if (bytes_read == 0)
      {
        return traits_type::eof();
      }

      setg(in_buffer_.get() + PUT_BACK_SIZE_ - number_to_put_back,
        in_buffer_.get() + PUT_BACK_SIZE_,
        in_buffer_.get() + PUT_BACK_SIZE_ + bytes_read);

      return traits_type::to_int_type(*gptr());
    }


    //
    // OutStreamBuf class
    //

    OutStreamBuf::OutStreamBuf(IO* io, size_t buffer_size)
      /*throw (InvalidArgument, eh::Exception)*/
      : IO_(io), BUFFER_SIZE_(buffer_size), out_buffer_(BUFFER_SIZE_)
    {
      setp(out_buffer_.get(), out_buffer_.get() + BUFFER_SIZE_);
    }

    OutStreamBuf::~OutStreamBuf() throw ()
    {
      if (pptr() != pbase())
      {
        try
        {
          IO_->write(out_buffer_.get(), pptr() - pbase());
        }
        catch (const eh::Exception& ex)
        {
          std::cerr << "Failed to write to file: " << ex.what() << "\n";
        }
      }
    }

    OutStreamBuf::int_type
    OutStreamBuf::overflow(int_type c) /*throw (Overflow, eh::Exception)*/
    {
      assert (pptr() != pbase());

      IO_->write(out_buffer_.get(), pptr() - pbase());

      setp(out_buffer_.get(), out_buffer_.get() + BUFFER_SIZE_);

      if (traits_type::not_eof(c))
      {
        *this->pptr() = c;
        this->pbump(1);
        return true;
      }

      return false;
    }
  }
}
