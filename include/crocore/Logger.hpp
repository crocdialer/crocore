// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#pragma once

#include <sstream>
#include <memory>
#include <set>

namespace crocore {

class Connection;

enum class Severity
{
    DISABLED = 0,
    PRINT = 1,
    FATAL = 2,
    ERROR_ = 3,
    WARNING = 4,
    INFO = 5,
    DEBUG = 6,
    TRACE_1 = 7,
    TRACE_2 = 8,
    TRACE_3 = 9,
    TRACE = TRACE_1
};

class Logger
{

public:

    Logger() noexcept;

    Logger(Logger &&other) noexcept;

    Logger(const Logger &) = delete;

    ~Logger();

    Logger &operator=(Logger other);

    /**
    Used to detect if a message should be logged depending on its severity and the logger severity settings.
    returns true if theSeverity is higher (numerically smaller) than the verbosity setting
    a different verbosity can be defined for any id range in any module; if there are different
    verbosity settings for an overlapping id region in the same module, the setting for the
    smallest id-range takes precedence.
    */
    bool if_log(Severity the_severity, const char *theModule, int theId);

    void log(Severity the_severity, const char *theModule, int theId, const std::string &theText);

    void set_severity(const Severity &the_severity);

    Severity severity() const;

    void add_outstream(std::ostream *the_stream);

    void remove_outstream(std::ostream *the_stream);

    void add_outstream(const std::shared_ptr<Connection> &the_con);

    void remove_outstream(const std::shared_ptr<Connection> &the_con);

    void clear_streams();

    bool use_time_stamp() const;

    void set_use_time_stamp(bool b);

    bool use_log_file() const;

    void set_use_log_file(bool b, const std::string &the_log_file = "kinski.log");

    bool use_thread_id() const;

    void set_use_thread_id(bool b);

private:

    std::unique_ptr<struct LoggerImpl> m_impl;
};

// global instance
extern Logger g_logger;

/**
This class is used to collect the output and deliver it to the Logger on destruction
*/
class MessagePort
{
public:
    MessagePort(Logger *logger, Severity theSeverity, const char *theModule, int theId)
            : m_logger(logger), m_severity(theSeverity), m_module(theModule), m_Id(theId) {}

    ~MessagePort()
    {
        m_logger->log(m_severity, m_module, m_Id, m_stream.str());
    }

    inline std::ostringstream &stream() { return m_stream; }

private:

    Logger *m_logger;
    std::ostringstream m_stream;
    const Severity m_severity;
    const char *m_module;
    const int m_Id;
};

// convenience xprintf style log-function
template<typename ... Args>
void log(Severity the_severity, const std::string &the_format_text, Args ... args)
{
    Logger &l = g_logger;
    if(the_severity > l.severity()){ return; }
    int size = snprintf(nullptr, 0, the_format_text.c_str(), args ...) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, the_format_text.c_str(), args ...);
    l.log(the_severity, __FILE__, __LINE__, buf.get());
}

#define CROCORE_LOG_CHECK(LOGGER, SEVERITY, MODULE, MSGID) LOGGER.if_log(SEVERITY, MODULE, MSGID) \
    && (crocore::MessagePort(&LOGGER, SEVERITY, MODULE, MSGID).stream())

#define LOG_INFO CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::INFO, __FILE__, __LINE__)
#define LOG_TRACE CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::TRACE, __FILE__, __LINE__)
#define LOG_TRACE_1 CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::TRACE_1, __FILE__, __LINE__)
#define LOG_TRACE_2 CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::TRACE_2, __FILE__, __LINE__)
#define LOG_TRACE_3 CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::TRACE_3, __FILE__, __LINE__)
#define LOG_DEBUG CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::DEBUG, __FILE__, __LINE__)
#define LOG_PRINT CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::PRINT, __FILE__, __LINE__)
#define LOG_ERROR CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::ERROR_, __FILE__, __LINE__)
#define LOG_WARNING CROCORE_LOG_CHECK(crocore::g_logger, crocore::Severity::WARNING, __FILE__, __LINE__)

#define LOG_INFO_IF(b) b && LOG_INFO
#define LOG_TRACE_IF(b) b && LOG_TRACE
#define LOG_DEBUG_IF(b) b && LOG_DEBUG
#define LOG_PRINT_IF(b) b && LOG_PRINT
#define LOG_ERROR_IF(b) b && LOG_ERROR
#define LOG_WARNING_IF(b) b && LOG_WARNING

}//namespace
