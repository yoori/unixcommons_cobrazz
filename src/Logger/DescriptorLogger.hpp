#pragma once

#include <unistd.h>

#include <Logger/SimpleLogger.hpp>


namespace Logging::Descriptor
{
  namespace Helper
  {
    /**
     * Configuration for Descriptor Handler
     */
    struct Config
    {
      /**
       * Constructor
       */
      Config(const Formatter* formatter, int fd, size_t preallocated_size) noexcept;

      Formatter_var formatter;
      int fd;
      size_t preallocated_size;
    };

    /**
     * Logger handler allowing output to the specified file descriptor
     */
    class Handler :
      public Logging::Handler,
      public ReferenceCounting::AtomicImpl
    {
    public:
      /**
       * Constructor
       * @param config handler configuration
       */
      explicit Handler(Config&& config)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Message formatting and output function
       * @param record Log record
       */
      virtual void publish(const LogRecord& record)
        /*throw (Exception, eh::Exception)*/;

    protected:
      /**
       * Destructor
       */
      virtual ~Handler() noexcept;

      /**
       * Allows children to pass file descriptor later
       * @param fd File descriptor
       */
      void set_fd_(int fd) noexcept;

      /**
       * Closes stored file descriptor
       */
      void close_fd_() noexcept;

    private:
      FormatWrapper formatter_;
      int fd_;
    };
  }

  /**
   * Configuration for Descriptor Logger
   */
  struct Config :
    public Helper::Config,
    public Simple::Config
  {
    /**
     * Constructor
     * @param formatter formatter to use
     * @param fd file descriptor for output
     * @param preallocated_size preallocated memory size for formatting
     */
    explicit
    Config(const Formatter* formatter = 0, int fd = -1, size_t preallocated_size = 0) noexcept;
  };

  /**
   * Descriptor Logger
   */
  using Logger = DerivedLogger<Config, Helper::Handler>;
}

//
// INLINES
//

namespace Logging::Descriptor
{
  namespace Helper
  {
    //
    // Config class
    //

    inline Config::Config(const Formatter* formatter, int fd, size_t preallocated_size) noexcept
      : formatter(ReferenceCounting::add_ref(formatter)), fd(fd),
        preallocated_size(preallocated_size)
    {
    }


    //
    // Handler class
    //

    inline Handler::Handler(Config&& config) /*throw (Exception, eh::Exception)*/
      : formatter_(config.formatter, config.preallocated_size),
        fd_(config.fd)
    {
    }

    inline void Handler::close_fd_() noexcept
    {
      if (fd_ != -1)
      {
        close(fd_);
        fd_ = -1;
      }
    }

    inline Handler::~Handler() noexcept
    {
      close_fd_();
    }

    inline void Handler::set_fd_(int fd) noexcept
    {
      fd_ = fd;
    }
  }


  //
  // Config class
  //

  inline Config::Config(const Formatter* formatter, int fd, size_t preallocated_size) noexcept
    : Helper::Config(formatter, fd, preallocated_size)
  {
  }
}
