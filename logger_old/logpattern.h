//////////////////////////////////////////////////////////////////////////
// PatternConverters

class FormattingInfo
{
public:
	FormattingInfo(bool leftAlign, const int minLength, const int maxLength)
	: _leftAlign(leftAlign), _minLength(minLength), _maxLength(maxLength) {}
	void Format(const int fieldStart, std::string& output)
	{
		int rawLength = (int)output.length() - fieldStart;
		if (rawLength > _maxLength)
		{
			output.erase(output.begin() + fieldStart,
				output.begin() + fieldStart + (rawLength - _maxLength));
		}
		else if (rawLength < _minLength)
		{
			if (_leftAlign)
				output.append(_minLength - rawLength, ' ');
			else
				output.insert(fieldStart, _minLength - rawLength, ' ');
		}
	}
private:
	const bool _leftAlign;
	const int _minLength;
	const int _maxLength;
};

class BasePatternConverter : public PatternConverter
{
public:
	BasePatternConverter() : _formattingInfo(NULL) {}
	void SetFormattingInfo(FormattingInfo* formattingInfo)
	{
		delete _formattingInfo;
		_formattingInfo = formattingInfo;
	}
	virtual ~BasePatternConverter()
	{
		delete _formattingInfo;
	}
	virtual void Format(std::string& output, const LogEvent& event)
	{
		int startField = (int)output.length();
		DoFormat(output, event);
		if (_formattingInfo != NULL)
			_formattingInfo->Format(startField, output);
	}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event) = 0;
	FormattingInfo* _formattingInfo;
};

class LiteralPatternConverter : public BasePatternConverter
{
public:
	LiteralPatternConverter(const std::string& literal)
		: _literal(literal) {}
	virtual ~LiteralPatternConverter() {}
private:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(_literal);
	}
	std::string _literal;
};

class LoggerNamePatternConverter : public BasePatternConverter
{
public:
	LoggerNamePatternConverter() {}
	virtual ~LoggerNamePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(event.loggerName);
	}
};

class ClassNamePatternConverter : public BasePatternConverter
{
public:
	ClassNamePatternConverter() {}
	virtual ~ClassNamePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(event.location.GetClassName());
	}
};

class DateTimePatternConverter : public BasePatternConverter
{
public:
	DateTimePatternConverter() {}
	virtual ~DateTimePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		time_t t = event.timeStamp;
		tm* ptm = localtime((time_t*)&t);
		if (ptm != NULL)
		{
			char addname[32] = {0};
			sprintf(addname, "%02d/%02d/%02d %02d-%02d-%02d",
				(ptm->tm_year+1900)%100, ptm->tm_mon+1, ptm->tm_mday,
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			output.append(addname);
		}
	}
};

class DatePatternConverter : public BasePatternConverter
{
public:
	DatePatternConverter() {}
	virtual ~DatePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		time_t t = event.timeStamp;
		tm* ptm = localtime((time_t*)&t);
		if (ptm != NULL)
		{
			char addname[16] = {0};
			sprintf(addname, "%02d/%02d/%02d",
				(ptm->tm_year+1900)%100, ptm->tm_mon+1, ptm->tm_mday);
			output.append(addname);
		}
	}
};

class TimePatternConverter : public BasePatternConverter
{
public:
	TimePatternConverter() {}
	virtual ~TimePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		time_t t = event.timeStamp;
		tm* ptm = localtime((time_t*)&t);
		if (ptm != NULL)
		{
			char addname[16] = {0};
			sprintf(addname, "%02d-%02d-%02d", 
				ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			output.append(addname);
		}
	}
};

class MilliSecondPatternConverter : public BasePatternConverter
{
public:
	MilliSecondPatternConverter() {}
	virtual ~MilliSecondPatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		int v = 0;//event.timeStamp.nsec/1000000;
		if (v > 999)
			v = 999;
		std::string ms = "0";
		output.append(ms);
	}
};

class LevelPatternConverter : public BasePatternConverter
{
public:
	LevelPatternConverter() {}
	virtual ~LevelPatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(LogLevel::ToString(event.level));
	}
};

class LinePatternConverter : public BasePatternConverter
{
public:
	LinePatternConverter() {}
	virtual ~LinePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
        char line[32];
        sprintf(line,"%d", event.location.GetLineNumber());
		output.append(line);
	}
};

class MethodNamePatternConverter : public BasePatternConverter
{
public:
	MethodNamePatternConverter() {}
	virtual ~MethodNamePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(event.location.GetMethodName());
	}
};

class FileNamePatternConverter : public BasePatternConverter
{
public:
	FileNamePatternConverter() {}
	virtual ~FileNamePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(event.location.GetFileName());
	}
};

class MessagePatternConverter : public BasePatternConverter
{
public:
	MessagePatternConverter() {}
	virtual ~MessagePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append(event.message);
	}
};

class NewLinePatternConverter : public BasePatternConverter
{
public:
	NewLinePatternConverter() {}
	virtual ~NewLinePatternConverter() {}
protected:
	virtual void DoFormat(std::string& output, const LogEvent& event)
	{
		output.append("\r\n");
	}
};

//////////////////////////////////////////////////////////////////////////
// PatternParser

class PatternParser
{
public:
	static void Parse(PatternConverterList& converters, const std::string& pattern);
	static PatternConverter* GetPatterConverter(char c);
};

PatternConverter* PatternParser::GetPatterConverter(char c)
{
	PatternConverter* pc = NULL;
	switch (c)
	{
	case 'c':
		pc = new LoggerNamePatternConverter;
		break;
	case 'C':
		pc = new ClassNamePatternConverter;
		break;
	case 'd':
		pc = new DateTimePatternConverter;
		break;
	case 'D':
		pc = new DatePatternConverter;
		break;
	case 'T':
		pc = new TimePatternConverter;
		break;
	case 'r':
		pc = new MilliSecondPatternConverter;
		break;
	case 'p':
		pc = new LevelPatternConverter;
		break;
	case 'L':
		pc = new LinePatternConverter;
		break;
	case 'M':
		pc = new MethodNamePatternConverter;
		break;
	case 'F':
		pc = new FileNamePatternConverter;
		break;
	case 'm':
		pc = new MessagePatternConverter;
		break;
	case 'n':
		pc = new NewLinePatternConverter;
		break;
	}
	return pc;
}

void PatternParser::Parse(PatternConverterList& converters, const std::string& pattern)
{
	enum { LITERAL_STATE, CONVERTER_STATE, DOT_STATE, MIN_STATE, MAX_STATE };

	std::string currentLiteral;

	bool useFormattingInfo;
	bool leftAlign;
	int minLength;
	int maxLength;

	int patternLength = (int)pattern.length();
	int state = LITERAL_STATE;
	char c;
	int i = 0;

	while (i < patternLength)
	{
		c = pattern[i++];
		switch (state)
		{
		case LITERAL_STATE:
			if (i == patternLength)
			{
				currentLiteral.append(1, c);
				continue;
			}
			if (c == '%')
			{
				if(pattern[i] == '%')
				{
					currentLiteral.append(1, c);
					i++;
				}
				else
				{
					if (!currentLiteral.empty())
					{
						converters.push_back(new LiteralPatternConverter(currentLiteral));
						currentLiteral.clear();
					}

					state = CONVERTER_STATE;

					useFormattingInfo = false;
					leftAlign = false;;
					minLength = 0;
					maxLength = INT_MAX;
				}
			}
			else
			{
				currentLiteral.append(1, c);
			}
			break;

		case CONVERTER_STATE:
			switch (c)
			{
			case '-':
				useFormattingInfo = true;
				leftAlign = true;
				break;
			case '.':
				state = DOT_STATE;
				break;
			default:
				if ((c >= '0') && (c <= '9'))
				{
					useFormattingInfo = true;
					minLength = c - '0';
					state = MIN_STATE;
				}
				else
				{
					PatternConverter* pc = GetPatterConverter(c);
					if (pc != NULL)
					{
						if (useFormattingInfo)
						{
							((BasePatternConverter*)pc)->SetFormattingInfo(
								new FormattingInfo(leftAlign, minLength, maxLength));
						}
						converters.push_back(pc);
					}
					state = LITERAL_STATE;
				}
				break;
			}
			break;

		case MIN_STATE:
			if ((c >= '0') && (c <= '9'))
			{
				useFormattingInfo = true;
				minLength = (minLength * 10) + (c - '0');
			}
			else if (c == '.')
			{
				state = DOT_STATE;
			}
			else
			{
				PatternConverter* pc = GetPatterConverter(c);
				if (pc != NULL)
				{
					if (useFormattingInfo)
					{
						((BasePatternConverter*)pc)->SetFormattingInfo(
							new FormattingInfo(leftAlign, minLength, maxLength));
					}
					converters.push_back(pc);
				}
				state = LITERAL_STATE;
			}

			break;

		case DOT_STATE:
			if ((c >= '0') && (c <= '9'))
			{
				useFormattingInfo = true;
				maxLength = c - '0';
				state = MAX_STATE;
			}
			else
			{
				// error
				state = LITERAL_STATE;
			}
			break;

		case MAX_STATE:
			if ((c >= '0') && (c <= '9'))
			{
				useFormattingInfo = true;
				maxLength = (maxLength * 10) + (c - '0');
			}
			else
			{
				PatternConverter* pc = GetPatterConverter(c);
				if (pc != NULL)
				{
					if (useFormattingInfo)
					{
						((BasePatternConverter*)pc)->SetFormattingInfo(
						new FormattingInfo(leftAlign, minLength, maxLength));
					}
					converters.push_back(pc);
				}
				state = LITERAL_STATE;
			}
			break;
		}
	}

	if (!currentLiteral.empty())
		converters.push_back(new LiteralPatternConverter(currentLiteral));
}
