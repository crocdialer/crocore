// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __
//
// Copyright (C) 2012-2016, Fabian Schmidt <crocdialer@googlemail.com>
//
// It is distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
// __ ___ ____ _____ ______ _______ ________ _______ ______ _____ ____ ___ __

#include <iostream>
#include <limits>
#include <thread>
#include <fstream>
#include <cstdarg>
#include "crocore/filesystem.hpp"
#include "crocore/ThreadPool.hpp"
#include "crocore/Logger.hpp"
#include "crocore/Connection.hpp"

namespace crocore {

////////////////////////// ConnectionStreamBuf ////////////////////////////////////////////

class ConnectionStreamBuf : public std::streambuf
{
public:
    explicit ConnectionStreamBuf(ConnectionPtr the_con, size_t buff_sz = 1 << 10);

    const ConnectionPtr &connection() const { return m_connection; }

protected:

    // flush the characters in the buffer
    int flushBuffer();

    int overflow(int c) override;

    int sync() override;

private:
    ConnectionPtr m_connection;
    std::vector<char> m_buffer;
};

ConnectionStreamBuf::ConnectionStreamBuf(ConnectionPtr the_con, size_t buff_sz) :
        m_connection(std::move(the_con)),
        m_buffer(buff_sz + 1)
{
    //set putbase pointer and endput pointer
    char *base = m_buffer.data();
    setp(base, base + buff_sz);
}

// flush the characters in the buffer
int ConnectionStreamBuf::flushBuffer()
{
    int num = pptr() - pbase();

    // pass the flushed char sequence
    m_connection->write_bytes(pbase(), num);

    // reset put pointer
    pbump(-num);
    return num;
}

int ConnectionStreamBuf::overflow(int c)
{
    if(c != EOF)
    {
        *pptr() = c;
        pbump(1);
    }
    if(flushBuffer() == EOF){ return EOF; }
    return c;
}

int ConnectionStreamBuf::sync()
{
    // ERROR
    if(flushBuffer() == EOF){ return -1; }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

const std::string currentDateTime()
{
    time_t now = time(nullptr);
    struct tm tstruct = {};
    char buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Logger::Logger() noexcept : m_impl(std::make_unique<LoggerImpl>())
{
    clear_streams();
    add_outstream(&std::cout);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

struct LoggerImpl
{
    Severity global_severity = Severity::INFO;
    std::set<std::shared_ptr<std::ostream>> out_streams;
    std::mutex mutex;
    std::ofstream log_file_stream;
    crocore::ThreadPool thread_pool;
    bool use_timestamp = true;
    bool use_thread_id = false;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

Logger::Logger(Logger &&other) noexcept:
        Logger()
{
    std::swap(m_impl, other.m_impl);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Logger::~Logger()
{
    clear_streams();
    m_impl->log_file_stream.close();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Logger &Logger::operator=(Logger other)
{
    std::swap(m_impl, other.m_impl);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::set_severity(const Severity &theSeverity)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    // change state
    m_impl->global_severity = theSeverity;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

Severity Logger::severity() const
{
    return m_impl->global_severity;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool Logger::if_log(Severity theSeverity, const char *theModule, int theId)
{
    return theSeverity <= m_impl->global_severity;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::log(Severity theSeverity, const char *theModule, int theId,
                 const std::string &theText)
{
    std::stringstream stream;
    std::ostringstream postfix;
    postfix << theText;

    if(theSeverity > Severity::PRINT)
    {
        if(m_impl->use_timestamp){ stream << currentDateTime(); }
        postfix << " [" << fs::get_filename_part(theModule) << " at:" << theId << "]";
        if(m_impl->use_thread_id){ postfix << " [thread-id: " << std::this_thread::get_id() << "]"; }
    }

    switch(theSeverity)
    {
        case Severity::TRACE_1:
        case Severity::TRACE_2:
        case Severity::TRACE_3:
            stream << " TRACE: " << postfix.str();
            break;
        case Severity::DEBUG:
            stream << " DEBUG: " << postfix.str();
            break;
        case Severity::INFO:
            stream << " INFO: " << postfix.str();
            break;
        case Severity::WARNING:
            stream << " WARNING: " << postfix.str();
            break;
        case Severity::PRINT:
            stream << postfix.str();
            break;
        case Severity::ERROR:
            stream << " ERROR: " << postfix.str();
            break;
        default:
            throw std::runtime_error("Unknown logger severity");
            break;
    }

    std::string log_str = stream.str();

    // pass log string to outstreams
    m_impl->thread_pool.post([this, log_str{std::move(log_str)}]()
                             {
                                 std::lock_guard<std::mutex> lock(m_impl->mutex);
                                 for(auto &os : m_impl->out_streams){ *os << log_str << std::endl; }
                             });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::add_outstream(std::ostream *the_stream)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    // do not delete on deconstruction
    auto ptr = std::shared_ptr<std::ostream>(the_stream, [](std::ostream *p) {});

    // change state
    m_impl->out_streams.insert(ptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::remove_outstream(std::ostream *the_stream)
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    for(auto &ptr : m_impl->out_streams)
    {
        if(ptr.get() == the_stream){ m_impl->out_streams.erase(ptr); }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::add_outstream(const ConnectionPtr &the_con)
{
    if(the_con && the_con->is_open())
    {
        auto out_stream =
                std::shared_ptr<std::ostream>(new std::ostream(new ConnectionStreamBuf(the_con)),
                                              [](std::ostream *the_ost)
                                              {
                                                  delete the_ost->rdbuf();
                                                  delete the_ost;
                                              });

        the_con->set_disconnect_cb([this, out_stream](ConnectionPtr c)
                                   {
                                       LOG_DEBUG << "removing outstream: " << c->description();

                                       // avoid a cyclic ref after destruction
                                       c->set_disconnect_cb({});

                                       std::lock_guard<std::mutex> lock(m_impl->mutex);
                                       m_impl->out_streams.erase(out_stream);
                                   });
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        m_impl->out_streams.insert(out_stream);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::remove_outstream(const ConnectionPtr &the_con)
{
    for(auto &ptr : m_impl->out_streams)
    {
        auto stream_buf = dynamic_cast<ConnectionStreamBuf *>(ptr->rdbuf());

        if(stream_buf && stream_buf->connection() == the_con)
        {
            remove_outstream(ptr.get());
            return;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::clear_streams()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);

    // change state
    m_impl->out_streams.clear();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool Logger::use_time_stamp() const
{
    return m_impl->use_timestamp;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::set_use_time_stamp(bool b)
{
    m_impl->use_timestamp = b;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool Logger::use_thread_id() const
{
    return m_impl->use_thread_id;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::set_use_thread_id(bool b)
{
    m_impl->use_thread_id = b;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

bool Logger::use_log_file() const
{
    return m_impl->log_file_stream.is_open();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void Logger::set_use_log_file(bool b, const std::string &the_log_file)
{
    if(b)
    {
        m_impl->log_file_stream.open(the_log_file);
        add_outstream(&m_impl->log_file_stream);
    }else
    {
        remove_outstream(&m_impl->log_file_stream);
        m_impl->log_file_stream.close();
    }
}

}// namespace crocore
