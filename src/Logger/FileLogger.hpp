#pragma once

#include <sys/stat.h>

#include <ReferenceCounting/List.hpp>

#include <Logger/SimpleLogger.hpp>


namespace Logging::File
{
  namespace Helper
  {
    class Handler;
  }

  namespace Policies
  {
    /**
     * Policy base class.
     * Log file rotating policy interface.
     */
    class Policy : public virtual ReferenceCounting::Interface
    {
    public:
      DECLARE_EXCEPTION(Exception, LoggerException);

      /**
       * Checks rotate condition.
       * @param file_handler handler containing information about log
       * @return whether or not log should be rotated
       * @exception Exception Errors
       * @exception eh::Exception std::exception
       */
      virtual bool need_rotation(const Helper::Handler& file_handler)
        /*throw (Exception, eh::Exception)*/ = 0;

    protected:
      /**
       * Destructor
       */
      ~Policy() noexcept;
    };

    /**
     * Time span policy.
     * Rotates log after every given period of time.
     */
    class TimeSpanPolicy :
      public virtual Policy,
      public virtual ReferenceCounting::DefaultImpl<>
    {
    public:
      /**
       * Constructor.
       * @param rotation_time recreate at least as often as it is
       * @exception Exception Errors
       * @exception eh::Exception std::exception
       */
      explicit TimeSpanPolicy(const Generics::Time& rotation_time)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Checks rotate condition.
       * @param file_handler handler containing information about log
       * @return whether or not time exceeded and log should be rotated
       * @exception Exception Errors.
       * @exception eh::Exception std::exception
       */
      virtual bool need_rotation(const Helper::Handler& file_handler)
        /*throw (Exception, eh::Exception)*/;

    protected:
      /**
       * Destructor
       */
      virtual ~TimeSpanPolicy() noexcept;

    public:
      Generics::Time rotation_time;
    };

    /**
     * Size span policy.
     * Rotates log after exceeding of the given file size.
     */
    class SizeSpanPolicy :
      public virtual Policy,
      public virtual ReferenceCounting::DefaultImpl<>
    {
    public:
      /**
       * Constructor.
       * @param rotation_size recreate upon achieving this size
       * @exception Exception Errors
       * @exception eh::Exception std::exception
       */
      explicit SizeSpanPolicy(unsigned long long rotation_size)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Checks rotate condition.
       * @param file_handler handler containing information about log
       * @return whether or not size is achieved and log should be rotated
       * @exception Exception Errors.
       * @exception eh::Exception std::exception
       */
      virtual bool need_rotation(const Helper::Handler& file_handler)
        /*throw (Exception, eh::Exception)*/;

    protected:
      /**
       * Destructor
       */
      virtual ~SizeSpanPolicy() noexcept;

    public:
      unsigned long long rotation_size;
    };

    /**
     * Time events are calculated relative to supplied time, not for start
     */
    class AlignedTimeSpanPolicy :
      public virtual Policy,
      public virtual ReferenceCounting::DefaultImpl<>,
      private TimeSpanPolicy
    {
    public:
      /**
       * Constructor
       * @param start time (without date) to use as an anchor
       * @param rotation_time rotation time interval
       */
      explicit
      AlignedTimeSpanPolicy(const Generics::ExtendedTime& start,
        const Generics::Time& rotation_time)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Checks rotate condition.
       * @param file_handler handler containing information about log
       * @return whether or not time has come and log should be rotated
       * @exception Exception Errors.
       * @exception eh::Exception std::exception
       */
      virtual bool need_rotation(const Helper::Handler& file_handler)
        /*throw (Exception, eh::Exception)*/;

    protected:
      /**
       * Destructor
       */
      virtual ~AlignedTimeSpanPolicy() noexcept;

    private:
      Generics::ExtendedTime start_time_;
      Generics::Time last_create_time_;
      Generics::Time border_time_;
    };

    /**
     * Policy smart pointer
     */
    using Policy_var = ReferenceCounting::QualPtr<Policy>;

    /**
     * List of policies
     */
    using PolicyList = ReferenceCounting::List<Policy_var>;
  };

  struct Config;

  namespace Helper
  {
    /**
     * Configuration for File Handler
     */
    struct Config
    {
      /**
       * Constructor
       */
      Config(const char* file_name, Policies::PolicyList& policies,
        const Formatter* formatter, size_t preallocated_size)
        /*throw (eh::Exception)*/;

      std::string file_name;
      Policies::PolicyList policies;
      Formatter_var formatter;
      bool extended_name_format;
      unsigned from_num;
      unsigned order_num;
      size_t preallocated_size;
    };

    /**
     * File handler.
     * Writes formatted log line into file specified.
     */
    class Handler :
      public virtual Logging::Handler,
      public virtual ReferenceCounting::DefaultImpl<>
    {
    public:
      /**
       * Constructor
       * @param config configuration for handler
       */
      explicit Handler(File::Config&& config)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Writes record into the file, rotating file if needed.
       * @param record log record to publish
       */
      virtual void publish(const LogRecord& record)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Rotates a file.
       * @param time current time
       * @exception Exception Errors
       * @exception eh::Exception std::exception
       */
      void rotate(const Generics::ExtendedTime& time)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Rotates a file if required.
       * @param time current time
       * @return whether a file has been rotated or not
       * @exception Exception Errors
       * @exception eh::Exception std::exception
       */
      bool rotate_if_required(const Generics::ExtendedTime& time)
        /*throw (Exception, eh::Exception)*/;

      /**
       * Last file creation time
       * @return log creation time
       */
      Generics::Time log_create_time() const noexcept;

      /**
       * Last log time
       * @return last log time
       */
      Generics::Time log_time() const noexcept;

      /**
       * Current state of log file
       * @return current state of log file
       */
      const struct stat& file_stat() const noexcept;

      /**
       * Configured time zone
       * @return time zone to use
       */
      Generics::Time::TimeZone get_time_zone() const noexcept;

    protected:
      /**
       * Destructor
       */
      virtual ~Handler() noexcept;

    protected:
      using FileName = char[MAXPATHLEN];

      FileName file_name_;
      Policies::PolicyList POLICIES_;
      const Generics::Time::TimeZone TIME_ZONE_;
      const bool EXTENDED_NAME_FORMAT_;
      const unsigned FROM_NUM_;
      const unsigned ORDER_NUM_;

      FormatWrapper formatter_;

      FileName cur_file_name_;

      std::FILE* outfile_;
      Generics::Time log_create_time_;
      Generics::Time log_time_;
      struct stat file_stat_;
    };
  }

  /**
   * Configuration for File Logger
   */
  struct Config :
    public Helper::Config,
    public Simple::Config
  {
    /*
     * Constructor
     * @param file_name File write to (prefix)
     * @param policies File rotation policies to use
     * @param formatter Formatter to use
     * @param preallocated_size preallocated memory size for formatting
     */
    Config(const char* file_name,
      Policies::PolicyList& policies,
      unsigned long log_level = ::Logging::Logger::INFO,
      const Formatter* formatter = 0, size_t preallocated_size = 0)
      /*throw (eh::Exception)*/;
  };

  /**
   * File Logger
   */
  using Logger = DerivedLogger<Config, Helper::Handler>;
}

//
// INLINES
//

namespace Logging::File
{
  namespace Policies
  {
    //
    // Policy class
    //

    inline Policy::~Policy() noexcept
    {
    }


    //
    // TimeSpanPolicy class
    //

    inline TimeSpanPolicy::TimeSpanPolicy(const Generics::Time& rotation_tm)
      /*throw (Exception, eh::Exception)*/
      : rotation_time(rotation_tm)
    {
    }


    //
    // SizeSpanPolicy class
    //

    inline SizeSpanPolicy::SizeSpanPolicy(unsigned long long rotation_size)
      /*throw (Exception, eh::Exception)*/
      : rotation_size(rotation_size)
    {
    }


    //
    // AlignedTimeSpanPolicy class
    //

    inline
    AlignedTimeSpanPolicy::AlignedTimeSpanPolicy(
      const Generics::ExtendedTime& start,
      const Generics::Time& rotation_time)
      /*throw (Exception, eh::Exception)*/
      : TimeSpanPolicy(rotation_time), start_time_(start.get_time())
    {
    }
  }

  namespace Helper
  {
    //
    // Config class
    //

    inline
    Config::Config(const char* file_name,
      Policies::PolicyList& policies, const Formatter* formatter,
      size_t preallocated_size) /*throw (eh::Exception)*/
      : file_name(file_name), policies(policies),
        formatter(ReferenceCounting::add_ref(formatter)),
        extended_name_format(false), from_num(1), order_num(1),
        preallocated_size(preallocated_size)
    {
    }


    //
    // Handler class
    //

    inline Handler::~Handler() noexcept
    {
      if (outfile_)
      {
        ::fclose(outfile_);
        outfile_ = 0;
      }
    }

    inline Generics::Time Handler::log_create_time() const noexcept
    {
      return log_create_time_;
    }

    inline Generics::Time Handler::log_time() const noexcept
    {
      return log_time_;
    }

    inline const struct stat& Handler::file_stat() const noexcept
    {
      return file_stat_;
    }

    inline Generics::Time::TimeZone Handler::get_time_zone() const noexcept
    {
      return TIME_ZONE_;
    }
  }


  //
  // Config class
  //

  inline
  Config::Config(const char* file_name,
    Policies::PolicyList& policies, unsigned long log_level,
    const Formatter* formatter, size_t preallocated_size)
    /*throw (eh::Exception)*/
    : Helper::Config(file_name, policies, formatter, preallocated_size),
      Simple::Config(log_level)
  {
  }
}
