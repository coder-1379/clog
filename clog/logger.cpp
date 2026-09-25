#include <clog/logger.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <cstdarg>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/syscall.h>

namespace clog
{

///////////////////////////////////////////////////////////////////////////////
// event

struct event
{
    int level;
    const char* file;
    int line;
    const char* func;
    std::chrono::system_clock::time_point time;
    pid_t tid;
    const char* message;
    int message_len;

    event(int _level, const char* _file, int _line, const char* _func, const char* _msg, int _len)
        : level(_level)
        , file(_file)
        , line(_line)
        , func(_func)
        , time(std::chrono::system_clock::now())
        , tid(static_cast<pid_t>(::syscall(SYS_gettid)))
        , message(_msg)
        , message_len(_len)
    {
    }
};

///////////////////////////////////////////////////////////////////////////////
// appenders

class appender
{
public:
    virtual ~appender() { }

    virtual void append(const std::string& output)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << output << std::endl;
    }

protected:
    std::mutex mutex_;
};

class file_appender : public appender
{
public:
    file_appender(const std::string& file_name)
    {
        file_.open(file_name, std::ios::app);
        if (!file_.is_open())
            std::cerr << "Failed to open log file: " << file_name << std::endl;
    }

    virtual ~file_appender() override
    {
        if (file_.is_open())
            file_.close();
    }

    virtual void append(const std::string& output) override
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open())
            file_ << output << std::endl;
        else
            appender::append(output);
    }

private:
    std::ofstream file_;
};

///////////////////////////////////////////////////////////////////////////////
// converters

class converter
{
public:
    virtual ~converter() {}
    virtual void format(std::string& output, const event& event) = 0;
};

class literal_pattern_converter : public converter
{
    std::string text_;
public:
    explicit literal_pattern_converter(std::string t) : text_(std::move(t)) {}
    void format(std::string& out, const event&) override { out += text_; }
};
class thread_id_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override
    {
        char tid[32];
        snprintf(tid, sizeof(tid), "%d", e.tid);
        out += tid;
    }
};
class date_time_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override
    {
        auto time = std::chrono::system_clock::to_time_t(e.time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                e.time.time_since_epoch()) % 1000;
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);
        out += time_str;
        snprintf(time_str, sizeof(time_str), ".%d", int(ms.count()));
        out += time_str;
    }
};

class date_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override
    {
        auto time = std::chrono::system_clock::to_time_t(e.time);
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%Y-%m-%d", &tm);
        out += time_str;
    }
};

class time_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override
    {
        auto time = std::chrono::system_clock::to_time_t(e.time);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                e.time.time_since_epoch()) % 1000;
        std::tm tm;
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%H:%M:%S", &tm);
        out += time_str;
        snprintf(time_str, sizeof(time_str), ".%d", int(ms.count()));
        out += time_str;
    }
};

class level_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override
    {
        static const char* names[] = {"DEBUG","INFO ","WARN ","ERROR","FATAL"};
        out += names[e.level];
    }
};

class file_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override { out += e.file; }
};

class line_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", e.line);
        out += buf;
    }
};

class method_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override { out += e.func; }
};

class message_pattern_converter : public converter
{
public:
    void format(std::string& out, const event& e) override { out += e.message; }
};

class new_line_pattern_converter : public converter
{
public:
    void format(std::string& out, const event&) override { out += '\n'; }
};

///////////////////////////////////////////////////////////////////////////////
// converter parser

converter* make_converter(char escape)
{
    switch (escape)
    {
    case '%': return new literal_pattern_converter("%");
    case 't': return new thread_id_pattern_converter;
    case 'd': return new date_time_pattern_converter;
    case 'D': return new date_pattern_converter;
    case 'T': return new time_pattern_converter;
    case 'l': return new level_pattern_converter;
    case 'F': return new file_pattern_converter;
    case 'L': return new line_pattern_converter;
    case 'M': return new method_pattern_converter;
    case 'm': return new message_pattern_converter;
    case 'n': return new new_line_pattern_converter;
    default:  return nullptr;
    }
}

void parse_pattern(std::vector<converter*>& result, const std::string& pattern)
{
    std::string literal;

    auto flush_literal = [&]()
    {
        if (!literal.empty())
        {
            result.push_back(new literal_pattern_converter(literal));
            literal.clear();
        }
    };

    for (size_t i = 0; i < pattern.size(); ++i)
    {
        char c = pattern[i];

        if (c != '%')
        {
            literal.push_back(c);
            continue;
        }

        if (i + 1 >= pattern.size())
        {
            literal.push_back('%');
            break;
        }

        char esc = pattern[++i];
        auto conv = make_converter(esc);
        if (!conv)
        {
            literal.push_back('%');
            literal.push_back(esc);
            continue;
        }

        flush_literal();
        result.push_back(conv);
    }

    flush_literal();
}

///////////////////////////////////////////////////////////////////////////////
// logger

logger& logger::instance()
{
    static logger logger;
    return logger;
}

logger::~logger()
{
    for (auto f: converters_)
        delete f;
    if (appender_)
        delete appender_;
}

void logger::configure(int level, const std::string& pattern)
{
    instance().level_ = level;
    instance().appender_ = new appender;
    parse_pattern(instance().converters_, pattern);
}

void logger::configure(const std::string& file_name, int level, const std::string& pattern)
{
    instance().level_ = level;
    instance().appender_ = new file_appender(file_name);
    parse_pattern(instance().converters_, pattern);
}

void logger::log(int level, const char* file, int line, const char* func,
            const char* format, ...)
{
    if (level >= instance().level_ && instance().appender_)
    {
        va_list args;
        va_start(args, format);

        va_list args_copy;
        va_copy(args_copy, args);
        int size = std::vsnprintf(nullptr, 0, format, args_copy);
        va_end(args_copy);
        if (size < 0)
        {
            va_end(args);
            return;
        }

        std::vector<char> buffer(size + 1);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);
        va_end(args);

        event event(level, file, line, func, buffer.data(), size);

        std::string output;
        for (auto f: instance().converters_)
        {
            f->format(output, event);
        }
        instance().appender_->append(output);
    }
}

} // namespace clog
