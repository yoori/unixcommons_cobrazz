#ifndef LOGGER_SYSLOG_ADAPTER_HPP
#define LOGGER_SYSLOG_ADAPTER_HPP

#include <syslog.h>

static const auto SYSLOG_LOG_DEBUG = LOG_DEBUG;
static const auto SYSLOG_LOG_INFO = LOG_INFO;
static const auto SYSLOG_LOG_WARNING = LOG_WARNING;

#undef LOG_DEBUG
#undef LOG_INFO
#undef LOG_WARNING

#endif
