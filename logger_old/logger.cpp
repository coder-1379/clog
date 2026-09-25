#include "logger.h"
#include "fstream"
//#include "tinyxml.h"

namespace clog
{

class LogEvent;

class LogLayout
{
public:
	virtual ~LogLayout() {}
	virtual void Format(std::string& output, const LogEvent& event) = 0;
};

class PatternConverter
{
public:
	virtual ~PatternConverter() {}
	virtual void Format(std::string& output, const LogEvent& event) = 0;
};

typedef std::vector<PatternConverter*> PatternConverterList;

class PatternLayout : public LogLayout
{
public:
	PatternLayout(const std::string& pattern);
	virtual ~PatternLayout();
	virtual void Format(std::string& output, const LogEvent& event);

protected:
	PatternConverterList _patternConverters;
};

// 控制台输出
class LogAppender
{
public:
	LogAppender(LogLayout* layout);
	virtual ~LogAppender();
	void Append(const LogEvent& event);

protected:
	virtual void DoAppend(const std::string& output);
	LogLayout* layout_;
};

// 文件输出，文件达到最大长度后重新生成新文件
class FileAppender : public LogAppender
{
public:
	static const unsigned DEF_FILE_LEN = 100; // K
	static const unsigned MIN_FILE_LEN = 2;
	static const unsigned MAX_FILE_LEN = 500;
	FileAppender(LogLayout* layout, const std::string& fileName, unsigned maxLength = DEF_FILE_LEN);
	virtual ~FileAppender();

private:
	bool Open();
	void Close();
	virtual void DoAppend(const std::string& output);

private:
	bool opend_;
	std::string	fileName_;
	uint64_t	maxLength_;
	uint64_t	fileLength_;
	std::ofstream	file_;
};

// 日志管理器
class LogManager
{
	friend class Logger;
	friend class LogConfigurator;
public:
	~LogManager();

private:
	LogManager();
	static LogManager& Instance();

	static void Configure(const std::string& pattern);
	static void Configure(const std::string& logFileName, const std::string& pattern);
	//static void Configure(const std::string& configFileName);

	static Logger* GetLogger();
	static Logger* GetLogger(const std::string& name);

	static bool Configured();
	static void Configured(bool configured);

	typedef std::map<std::string, Logger*> LoggerMap;
	typedef std::vector<Logger*> ProvisionNode;
	typedef std::map<std::string, ProvisionNode> ProvisionNodeMap;

	static void UpdateChildren(ProvisionNode& pn, Logger* logger);
	static void UpdateParents(Logger* logger);
	static void RemoveLoggers();

private:
	std::mutex mutex_;
	bool configured_;
	Logger* root_;
	LoggerMap loggers_;
	ProvisionNodeMap provisionNodes_;
};

struct LogEvent
{
	std::string loggerName;
	const LogLocation& location;
	int level;
	const std::string& message;
	long timeStamp;

	LogEvent(const std::string& loggerName, const LogLocation& location, 
		int level, const std::string& message)
		: loggerName(loggerName)
		, location(location)
		, level(level)
		, message(message)
	{
		timeStamp = time(0);
	}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// LogManager

LogManager& LogManager::Instance()
{
	static LogManager manager;
	return manager;
}

LogManager::LogManager()
{
	root_ = new Logger("root");
}

LogManager::~LogManager()
{
	delete root_;
	RemoveLoggers();
}

void LogManager::RemoveLoggers()
{
	for (LoggerMap::iterator it = Instance().loggers_.begin();
		it != Instance().loggers_.end(); ++it)
	{
		delete it->second;
	}
	Instance().loggers_.clear();
}

Logger* LogManager::GetLogger()
{
	return Instance().root_;
}

Logger* LogManager::GetLogger(const std::string& name)
{
	std::lock_guard<std::mutex> lock(Instance().mutex_);
	LoggerMap::iterator it = Instance().loggers_.find(name);
	if (it != Instance().loggers_.end())
	{
		return it->second;
	}
	else
	{
		Logger* logger = new Logger(name);
		Instance().loggers_.insert(LoggerMap::value_type(name, logger));
		ProvisionNodeMap::iterator it2 = Instance().provisionNodes_.find(name);

		if (it2 != Instance().provisionNodes_.end())
		{
			Instance().UpdateChildren(it2->second, logger);
			Instance().provisionNodes_.erase(it2);
		}

		Instance().UpdateParents(logger);
		return logger;
	}
}

bool LogManager::Configured()
{
	return Instance().configured_;
}

void LogManager::Configured(bool configured)
{
	Instance().configured_ = configured;
}

void LogManager::UpdateParents(Logger* logger)
{
	const std::string& name = logger->GetName();
	size_t length = name.size();
	bool parentFound = false;

	for(size_t i = name.find_last_of('.', length-1);
		i != std::string::npos; i = name.find_last_of('.', i-1))
	{
		std::string substr = name.substr(0, i);
		LoggerMap::iterator it = Instance().loggers_.find(substr);
		if (it != Instance().loggers_.end())
		{
			parentFound = true;
			logger->parent_ = it->second;
			break;
		}
		else
		{
			ProvisionNodeMap::iterator it2 = Instance().provisionNodes_.find(substr);
			if (it2 != Instance().provisionNodes_.end())
			{
				it2->second.push_back(logger);
			}
			else
			{
				ProvisionNode node(1, logger);
				Instance().provisionNodes_.insert(ProvisionNodeMap::value_type(substr, node));
			}
		}
	}

	if (!parentFound)
		logger->SetParent(Instance().root_);
	logger->level_ = logger->GetParent()->level_;
}

void LogManager::UpdateChildren(ProvisionNode& pn, Logger* logger)
{
	for(ProvisionNode::iterator it = pn.begin();
		it != pn.end(); it++)
	{
		const std::string& parentName = (*it)->GetParent()->GetName();
		if (parentName.compare(0, logger->GetName().length(), logger->GetName()) != 0)
		{
			logger->SetParent((*it)->GetParent());
			(*it)->SetParent(logger);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Configure

void LogManager::Configure(const std::string& pattern)
{
	Logger* root = LogManager::GetLogger();
	if (LogManager::Configured())
	{
		root->RemoveAppenders();
		LogManager::RemoveLoggers();
	}
	LogLayout* layout = new PatternLayout(pattern);
	LogAppender* appender = new LogAppender(layout);
	root->AddAppender(appender);
	LogManager::Configured(true);
}

void LogManager::Configure(const std::string& logFileName, const std::string& pattern)
{
	Logger* root = LogManager::GetLogger();
	if (LogManager::Configured())
	{
		root->RemoveAppenders();
		LogManager::Instance().RemoveLoggers();
	}
	LogLayout* layout = new PatternLayout(pattern);
	LogAppender* appender = new FileAppender(layout, logFileName);
	root->AddAppender(appender);
	LogManager::Configured(true);
}
/*
void LogManager::Configure(const std::string& configFileName)
{
	class Help
	{
	public:
		static void SetLevel(TiXmlElement* elm, Logger* logger)
		{
			const char* level = elm->Attribute("level");
			if (level == NULL)
				return;
			if (std::string(level) == "DEBUG")
				logger->SetLevel(LogLevel::LEVEL_DEBUG);
			else if (std::string(level) == "INFO")
				logger->SetLevel(LogLevel::LEVEL_INFO);
			else if (std::string(level) == "WARN")
				logger->SetLevel(LogLevel::LEVEL_WARN);
			else if (std::string(level) == "ERROR")
				logger->SetLevel(LogLevel::LEVEL_ERROR);
			else if (std::string(level) == "FATAL")
				logger->SetLevel(LogLevel::LEVEL_FATAL);
		}
		static void SetAppender(TiXmlElement* logElm, TiXmlElement* loggerElm, Logger* logger)
		{
			TiXmlElement* apdElm = loggerElm->FirstChildElement("appender");
			while (apdElm != NULL)
			{
				const char* name = apdElm->Attribute("ref");
				if (name != NULL)
					SetAppender(logElm, logger, name);
				apdElm = apdElm->NextSiblingElement();
			}
		}
		static void SetAppender(TiXmlElement* logElm, Logger* logger, const std::string& appenderName)
		{
			TiXmlElement* apdElm = logElm->FirstChildElement("appender");
			while (apdElm != NULL)
			{
				const char* name = apdElm->Attribute("name");
				if (name != NULL && appenderName == name)
				{
					const char* type = apdElm->Attribute("type");
					if (type != NULL)
					{
						const char* pattern = apdElm->Attribute("pattern");
						if (pattern == NULL)
							pattern = "[d] - %m%n";
						LogLayout* layout = new PatternLayout(pattern);
						if (std::string(type) == "FileAppender")
						{
							LogManager::Configured(true);
							const char* filename = apdElm->Attribute("filename");
							int len;
							apdElm->Attribute("filelength", &len);
							if (len == 0)
								len = FileAppender::DEF_FILE_LEN;
							if (filename != NULL)
							{
								logger->AddAppender(new FileAppender(layout, filename, len));
							}
						}
						else if (std::string(type) == "ConsoleAppender")
						{
							LogManager::Configured(true);
							logger->AddAppender(new LogAppender(layout));
						}
						else
						{
							delete layout;
						}
					}
					break;
				}
				apdElm = apdElm->NextSiblingElement();
			}
		}
	};

	TiXmlDocument doc;
	if (!doc.LoadFile(configFileName.c_str())
		return;
	TiXmlElement* cfgElm = doc.RootElement();
	if (cfgElm == NULL)
		return;
	TiXmlElement* logElm = cfgElm->FirstChildElement("log");
	if (logElm == NULL)
		return;
	TiXmlElement* rootElm = logElm->FirstChildElement("root");
	if (rootElm != NULL)
	{
		Logger* root = LogManager::GetLogger();
		Help::SetLevel(rootElm, root);
		Help::SetAppender(logElm, rootElm, root);
	}
	TiXmlElement* loggerElm = logElm->FirstChildElement("logger");
	while (loggerElm != NULL)
	{
		const char* name = loggerElm->Attribute("name");
		if (name != NULL)
		{
			Logger* logger = LogManager::GetLogger(name);
			Help::SetLevel(loggerElm, logger);
			const char* additivity = loggerElm->Attribute("additivity");
			if (additivity != NULL && std::string(additivity) == "false")
				logger->SetAdditive(false);
			Help::SetAppender(logElm, loggerElm, logger);
		}
		loggerElm = loggerElm->NextSiblingElement();
	}
}
*/
//////////////////////////////////////////////////////////////////////////
// layout

#include "logpattern.h"

PatternLayout::PatternLayout(const std::string& pattern)
{
	std::string patn = pattern;
	if (patn.empty())
		patn = "%m%n";
	PatternParser::Parse(_patternConverters, patn);
}

PatternLayout::~PatternLayout()
{
	for (PatternConverterList::iterator it = _patternConverters.begin();
		it != _patternConverters.end(); ++it)
	{
		delete (*it);
	}
}

void PatternLayout::Format(std::string& output, const LogEvent& event)
{
	for (PatternConverterList::iterator it = _patternConverters.begin();
		it != _patternConverters.end(); ++it)
	{
		(*it)->Format(output, event);
	}
}

//////////////////////////////////////////////////////////////////////////
// appender

LogAppender::LogAppender(LogLayout* layout)
	: layout_(layout)
{
}

LogAppender::~LogAppender()
{
	delete layout_;
}

void LogAppender::Append(const LogEvent& event)
{
	std::string output;
	layout_->Format(output, event);
	DoAppend(output);
}

void LogAppender::DoAppend(const std::string& output)
{
	printf(output.c_str());
}

FileAppender::FileAppender(LogLayout* layout, 
	const std::string& fileName, unsigned maxLength)
	: LogAppender(layout)
	, fileName_(fileName)
	, fileLength_(0)
	, opend_(false)
{
	if (maxLength < MIN_FILE_LEN)
		maxLength = MIN_FILE_LEN;
	if (maxLength > MAX_FILE_LEN)
		maxLength = MAX_FILE_LEN;
	maxLength_ = maxLength * 1024;
}

FileAppender::~FileAppender()
{
	Close();
}

bool FileAppender::Open()
{
/*	if (!file_.Open(fileName_, File::FLAG_READ | File::FLAG_WRITE | 
		File::FLAG_APPEND | File::FLAG_CREATE, 0))
	{
		return false;
	}
	if (!file_.GetSize(fileLength_))
		fileLength_ = 0;
    */
    file_.open(fileName_);
	opend_ = true;
	return true;
}

void FileAppender::Close()
{
	if (opend_)
	{
		file_.close();
		opend_ = false;
	}
}

void FileAppender::DoAppend(const std::string& output)
{
	if (opend_ || Open())
	{
		if (fileLength_ + output.length() >= maxLength_)
		{
			Close();
			/*time_t tmt = time(NULL);
			tm* ptm = localtime(&tmt);
			TCHAR addname[64] = {0};
			_stprintf(addname, _T(".%02d%02d%02d%02d%02d%02d"),
				(ptm->tm_year+1900)%100, ptm->tm_mon+1, ptm->tm_mday, 
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			std::string oldName = fileName_ + addname;
			file_.Rename(fileName_, oldName);
			fileLength_ = 0;*/

			DoAppend(output);
			return;
		}
		file_.write(output.c_str(), (int)output.length());
	}
}

//////////////////////////////////////////////////////////////////////////
// LogLevel

std::string LogLevel::ToString(int level)
{
	std::string result = "";
	switch(level)
	{
	case LEVEL_FATAL:
		result = "FATAL"; break;
	case LEVEL_ERROR:
		result = "ERROR"; break;
	case LEVEL_WARN:
		result = "WARN"; break;
	case LEVEL_INFO:
		result = "INFO"; break;
	case LEVEL_DEBUG:
		result = "DEBUG"; break;
	default:
		break;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////
// Location

const std::string LogLocation::GetMethodName() const
{
	std::string result(_methodName);
	size_t colonPos = result.rfind("::");
	if (colonPos != std::string::npos)
	{
		result.erase(0, colonPos + 2);
	}
	else
	{
		size_t spacePos = result.rfind(' ');
		if (spacePos != std::string::npos)
			result.erase(0, spacePos + 1);
	}
	size_t parenPos = result.find('(');
	if (parenPos != std::string::npos)
		result.erase(parenPos);
	return result;
}

const std::string LogLocation::GetClassName() const
{
	std::string result(_methodName);
	size_t colonPos = result.rfind("::");
	if (colonPos != std::string::npos)
	{
		result.erase(colonPos);
		size_t spacePos = result.find_last_of(' ');
		if (spacePos != std::string::npos)
			result.erase(0, spacePos + 1);
		return result;
	}
	result.erase(0, result.length() );
	return result;
}

//////////////////////////////////////////////////////////////////////////
// Logger

Logger::Logger(const std::string& name)
: parent_(NULL)
, name_(name)
, level_(LogLevel::LEVEL_WARN)
, additive_(true)
{
}

Logger::~Logger()
{
	RemoveAppenders();
}

Logger* Logger::GetParent()
{
	return parent_;
}

void Logger::SetParent(Logger* logger)
{
	parent_ = logger;
}

const std::string& Logger::GetName() const
{
	return name_;
}

void Logger::Append(int level, const std::string& msg, const LogLocation& location)
{
	LogEvent logevent(name_, location, level, msg);
	for (Logger* logger = this; logger != NULL; logger = logger->parent_)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (Appenders::iterator it = logger->appenders_.begin();
			it != logger->appenders_.end(); ++it)
		{
			(*it)->Append(logevent);
		}
		if (!logger->additive_)
			break;
	}
}

bool Logger::LogEnable(int level)
{
	return (level <= level_) && LogManager::Configured(); 
}

Logger* Logger::GetLogger()
{
	return LogManager::GetLogger();
}

Logger* Logger::GetLogger(const std::string& name)
{
	return LogManager::GetLogger(name);
}

void Logger::Configure(const std::string& pattern)
{
	LogManager::Configure(pattern);
}

void Logger::Configure(const std::string& logFileName, const std::string& pattern)
{
	LogManager::Configure(logFileName, pattern);
}

/*void Logger::Configure(const std::string& configFileName)
{
	LogManager::Configure(configFileName);
}*/

void Logger::SetLevel(int level)
{
	level_ = level;
}

void Logger::SetAdditive(bool additive)
{
	additive_ = additive;
}

void Logger::AddAppender(LogAppender* appender)
{
	std::lock_guard<std::mutex> lock(mutex_);
	appenders_.push_back(appender);
}

void Logger::RemoveAppenders()
{
	std::lock_guard<std::mutex> lock(mutex_);;
	for (Appenders::iterator it = appenders_.begin();
		it != appenders_.end(); ++it)
	{
		delete *it;
	}
	appenders_.clear();
}

} // namespace clog
