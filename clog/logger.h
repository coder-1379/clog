#pragma once

#include <string>
#include <vector>

// patterns
// %t thread id
// %d DateTime
// %D Date
// %T Time
// %l level
// %F file name
// %L line number
// %M method name
// %m message

#define LOG_DEBUG(fmt, ...) \
    ::clog::logger::log(::clog::logger::DEBUG, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) \
    ::clog::logger::log(::clog::logger::INFO,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) \
    ::clog::logger::log(::clog::logger::WARN,  __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) \
    ::clog::logger::log(::clog::logger::ERROR, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) \
    ::clog::logger::log(::clog::logger::FATAL, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

namespace clog
{

class appender;
class converter;

class logger
{
public:
    enum level { DEBUG = 0, INFO, WARN, ERROR, FATAL };
    ~logger();

    static void configure(int level, const std::string& pattern);
    static void configure(const std::string& file_name, int level, const std::string& pattern);

    static void log(int level, const char* file, int line, const char* func,
            const char* format, ...);

private:
    static logger& instance();

    int level_;
    appender* appender_{};
    std::vector<converter*> converters_;
};

} // namespace clog
