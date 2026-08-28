#pragma once

#include <Generics/ActiveObject.hpp>

#include <Logger/Logger.hpp>


namespace Logging
{
  /**
   * Adapter for logger into reference countable callback.
   * Forward error reports into logger.
   */
  class ActiveObjectCallbackImpl :
    public virtual Generics::ActiveObjectCallback,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    explicit
    ActiveObjectCallbackImpl(Logger* logger = 0,
      const char* message_prefix = "ActiveObject",
      const char* aspect = 0, const char* code = 0) noexcept;

    virtual
    void
    report_error(Severity severity, const String::SubString& description,
      const char* code = 0) noexcept;

    virtual Logger* logger() const noexcept;

    virtual const char* message_prefix() const noexcept;

    virtual const char* aspect() const noexcept;

    virtual const char* code(const char* error_code) const noexcept;

  protected:
    virtual ~ActiveObjectCallbackImpl() noexcept;

  private:
    Logger* logger_;
    const char* message_prefix_;
    const char* aspect_;
    const char* code_;
  };
  using ActiveObjectCallbackImpl_var = ReferenceCounting::QualPtr<ActiveObjectCallbackImpl>;

  /**
   * Simply represent linked pair of Logger & ActiveObjectCallback
   * with ability to change logger at runtime
   */
  class LoggerCallbackHolder
  {
  public:
    /*
     * Construct LoggerCallbackHolder
     * @param logger is initial logger will be used
     * @param message_prefix for callback
     * @param aspect for callback
     * @param code for callback
     */
    LoggerCallbackHolder(Logger* logger, const char* message_prefix,
      const char* aspect, const char* code) /*throw (eh::Exception)*/;

    /*
     * Get stored callback
     * @return stored callback
     */
    Generics::ActiveObjectCallback* callback() noexcept;
    /*
     * Get stored logger
     * @return stored logger
     */
    Logger* logger() const noexcept;
    /*
     * Set stored logger
     * @param new_logger is logger to store
     */
    void logger(Logger* new_logger) noexcept;

  protected:
    mutable LoggerHolder_var logger_holder_;
    Generics::ActiveObjectCallback_var callback_;
  };
}

//
// INLINES
//

namespace Logging
{
  //
  // ActiveObjectCallback class
  //

  inline
  ActiveObjectCallbackImpl::ActiveObjectCallbackImpl(Logging::Logger* logger,
    const char* message_prefix, const char* aspect, const char* code) noexcept
    : logger_(logger), message_prefix_(message_prefix), aspect_(aspect),
      code_(code)
  {
  }

  inline ActiveObjectCallbackImpl::~ActiveObjectCallbackImpl() noexcept
  {
  }


  //
  // LoggerCallbackHolder class
  //

  inline
  LoggerCallbackHolder::LoggerCallbackHolder(Logger* logger,
    const char* message_prefix, const char* aspect, const char* code)
    /*throw (eh::Exception)*/
    : logger_holder_(new LoggerHolder(logger)),
      callback_(new ActiveObjectCallbackImpl(logger_holder_, message_prefix, aspect, code))
  {
  }

  inline Generics::ActiveObjectCallback* LoggerCallbackHolder::callback() noexcept
  {
    return callback_;
  }

  inline Logger* LoggerCallbackHolder::logger() const noexcept
  {
    return logger_holder_;
  }

  inline void LoggerCallbackHolder::logger(Logger* new_logger) noexcept
  {
    logger_holder_->logger(new_logger);
  }
}
