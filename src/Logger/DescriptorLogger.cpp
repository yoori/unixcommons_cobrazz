#include <eh/Errno.hpp>

#include <Logger/DescriptorLogger.hpp>


namespace Logging
{
  namespace Descriptor
  {
    namespace Helper
    {
      //
      // Handler class
      //

      void
      Handler::publish(const LogRecord& record)
        /*throw (Exception, eh::Exception)*/
      {
        FormatWrapper::Result line(formatter_.format(record));
        if (!line.get())
        {
          Stream::Error ostr;
          ostr << FNS << "failed to format message";
          throw Exception(ostr);
        }

        for (ssize_t offset = 0, length = strlen(line.get()); length;)
        {
          ssize_t res = write(fd_, line.get() + offset, length);
          if (res == 0)
          {
            Stream::Error ostr;
            ostr << FNS << "nothing has been written";
            throw Exception(ostr);
          }
          if (res < 0)
          {
            if (errno == EINTR)
            {
              continue;
            }
            eh::throw_errno_exception<Exception>(FNE,
              "Failed to write");
          }
          length -= res;
          offset += res;
        }
      }
    }
  }
}
