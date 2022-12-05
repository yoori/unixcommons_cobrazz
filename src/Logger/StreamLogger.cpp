/**
* @file   StreamLogger.hpp
* @author Karen Aroutiounov
*/

#include <Generics/Function.hpp>

#include <Stream/MemoryStream.hpp>

#include <Logger/StreamLogger.hpp>


namespace Logging
{
  namespace OStream
  {
    namespace Helper
    {
      void
      Handler::publish(const LogRecord& record)
        /*throw (BadStream, Exception, eh::Exception)*/
      {
        FormatWrapper::Result line(formatter_.format(record));

        if (!line.get())
        {
          Stream::Error ostr;
          ostr << FNS << "failed to format message";
          throw Exception(ostr);
        }

        ostr_ << line.get() << std::flush;

        if (!ostr_.good())
        {
          Stream::Error ostr;
          ostr << FNS << "stream is dead";
          throw BadStream(ostr);
        }
      }
    }
  }
}
