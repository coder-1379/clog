#ifndef CLOG_LOGGER_H
#define CLOG_LOGGER_H

#include <string>
#include <thread>
#include <map>
#include <vector>
#include <mutex>
#include <sys/time.h>
#include <limits.h>

// %c logger name
// %C(class) class name
// %d DateTime
// %D Date
// %T Time
// %r(run time) milliseconds
// %p(priority) level
// %L line number
// %M method name
// %F file name
// %m(message) message
// %n(new line) "\r\n"

namespace clog
{


class LogManager;
class LogAppender;
class LogLocation;

class LogLevel
{
public:
	enum { LEVEL_OFF, LEVEL_FATAL, LEVEL_ERROR,
		LEVEL_WARN, LEVEL_INFO, LEVEL_DEBUG, LEVEL_ALL };
	static std::string ToString(int level);
};

class Logger
{
public:
	~Logger();

	void Append(int level, const std::string& msg, const LogLocation& location);
	bool LogEnable(int level);

	// 获取根Logger
	static Logger* GetLogger();
	// 获取非根Logger
	static Logger* GetLogger(const std::string& name);

	// 简单控制台输出
	static void Configure(const std::string& pattern);
	// 简单文件输出
	static void Configure(const std::string& logFileName, const std::string& pattern);
	// 通过配置文件配置输出
	//static void Configure(const std::string& configFileName);

private:
	friend class LogManager;

	Logger(const std::string& name);
	Logger* GetParent();
	void SetParent(Logger* logger);
	const std::string& GetName() const;

	void SetLevel(int level = LogLevel::LEVEL_WARN);
	void SetAdditive(bool additive = true); // 是否继承父logger的Appender

	void AddAppender(LogAppender* appender);
	void RemoveAppenders();

private:
	typedef std::vector<LogAppender*> Appenders;
	std::mutex mutex_;
	Logger* parent_;
	std::string name_;
	int level_;
	bool additive_;
	Appenders appenders_;
};

class LogLocation
{
public:
	LogLocation(const char* fileName, const char* methodName, int lineNumber)
		: _fileName(fileName)
		, _methodName(methodName)
		, _lineNumber(lineNumber)
	{}
	const char* GetFileName() const { return _fileName; }
	const std::string GetMethodName() const;
	const std::string GetClassName() const;
	int GetLineNumber() const { return _lineNumber; }

private:
	const char* _fileName;
	const char* _methodName;
	int _lineNumber;
};

} // namespace clog

#if defined(_MSC_VER)
#  if _MSC_VER >= 1300
#    define __CLOG_FUNC__ __FUNCSIG__
#  endif
#elif defined(__GNUC__)
#  define __CLOG_FUNC__ __PRETTY_FUNCTION__
#endif

#if !defined(__CLOG_FUNC__)
#  define __CLOG_FUNC__ ""
#endif

#define CLOG_LOCATION clog::LogLocation(__FILE__, __CLOG_FUNC__, __LINE__)

#if defined(CLOG_ENABLE_LOG)
#  define LOG_FATAL(logger, message)\
  {\
  if (logger != NULL && logger->LogEnable(clog::LogLevel::LEVEL_FATAL))\
	{\
	std::stringstream ss;\
	ss << message;\
	logger->Append(clog::LogLevel::LEVEL_FATAL, ss.str(), CLOG_LOCATION);\
}\
}
#  define LOG_ERROR(logger, message)\
  {\
	if (logger != NULL && logger->LogEnable(clog::LogLevel::LEVEL_ERROR))\
    {\
	  std::stringstream ss;\
	  ss << message;\
	  logger->Append(clog::LogLevel::LEVEL_ERROR, ss.str(), CLOG_LOCATION);\
    }\
  }
#  define LOG_WARN(logger, message)\
  {\
	if (logger != NULL && logger->LogEnable(clog::LogLevel::LEVEL_WARN))\
    {\
	  std::stringstream ss;\
	  ss << message;\
	  logger->Append(clog::LogLevel::LEVEL_WARN, ss.str(), CLOG_LOCATION);\
    }\
  }
#  define LOG_INFO(logger, message)\
  {\
	if (logger != NULL && logger->LogEnable(clog::LogLevel::LEVEL_INFO))\
    {\
	  std::stringstream ss;\
	  ss << message;\
	  logger->Append(clog::LogLevel::LEVEL_INFO, ss.str(), CLOG_LOCATION);\
    }\
  }
#  define LOG_DEBUG(logger, message)\
  {\
	if (logger != NULL && logger->LogEnable(clog::LogLevel::LEVEL_DEBUG))\
    {\
	  std::stringstream ss;\
	  ss << message;\
	  logger->Append(clog::LogLevel::LEVEL_DEBUG, ss.str(), CLOG_LOCATION);\
    }\
  }
#else 
#  define LOG_FATAL(logger, message)
#  define LOG_ERROR(logger, message)
#  define LOG_WARN(logger, message)
#  define LOG_INFO(logger, message)
#  define LOG_DEBUG(logger, message)
#endif

/* 配置文件例
<?xml version="1.0" encoding="utf-8" ?> 
<configuration>
  <log>
    <root level="WARN">
      <appender ref="fa" />
    </root>
    <logger name="Net" level="WARN"  additivity="false">
      <appender ref="ca" />
    </logger>
    <appender 
      name="fa" 
      type="FileAppender"
      file="log-file.txt"
	  maxlength="100"
      pattern="[%d] - %m%n"
    />
    <appender 
      name="ca" 
      type="ConsoleAppender"
      pattern="[%d] - %m%n"
    />
  </log>
</configuration>
*/

#endif // CLOG_LOGGER_H
