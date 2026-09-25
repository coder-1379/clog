logger主题部分于2008年完成，但没有在实际项目中使用，最近翻出来重新编译了一下，因现在一些依赖缺失，我将其中编译有问题的注释了。
同时又整理了一个更简化的版本，简化版也许更适合日常使用

#header
```c++

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
```

#test
```c++
#include <iostream>
#include <clog/logger.h>

// g++ -std=c++11 -pthread -I.. logger_test.cpp ../clog/logger.cpp -o logger_test.bin

int main()
{
    clog::logger::configure(clog::logger::DEBUG, "%d %t %l %F:%L %M %m");
    //clog::logger::configure("logger.log", clog::logger::DEBUG, "%d %t %l %F:%L %M %m");
    LOG_INFO("hello %d", 100);
}
```
