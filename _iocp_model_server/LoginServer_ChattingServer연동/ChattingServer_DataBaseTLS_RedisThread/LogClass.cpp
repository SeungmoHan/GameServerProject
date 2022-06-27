#include "LogClass.h"
#include <direct.h>
#include <iostream>

namespace univ_dev
{
	LogClass::LogClass() : _LogLevel(LogLevel::LOG_LEVEL_DEBUG), _Directory{ 0 }, _LogCount(1) {};
	void LogClass::LOG_SET_LEVEL(LogLevel level)
	{
		_LogLevel = level;
	}

	LogClass::~LogClass() {};

	void LogClass::LOG_SET_DIRECTORY(const WCHAR* directory)
	{
		wcscpy_s(_Directory, MAX_PATH, directory);
	}

	void LogClass::LOG(const WCHAR* message, LogLevel logLevel)
	{
		if ((int)logLevel < (int)_LogLevel)	return;

		int result = _wmkdir(_Directory);
		time_t curTime = time(nullptr);
		tm curTm;

		WCHAR curPath[MAX_PATH];

		FILE* logFile = nullptr;
		
		localtime_s(&curTm, &curTime);


		swprintf_s(curPath, L"%s\\%d_%d_%d_LOG.txt",_Directory, curTm.tm_year + 1900, curTm.tm_mon + 1, curTm.tm_mday);


		while (logFile == nullptr)
			_wfopen_s(&logFile, curPath, L"ab");
		
		this->_LogCount++;
		switch (logLevel)
		{
		case univ_dev::LogClass::LogLevel::LOG_LEVEL_DEBUG:
			fwprintf_s(logFile, L"LOG %.2d:%.2d:%.2d // DEBUG // %s\n", curTm.tm_hour, curTm.tm_min, curTm.tm_sec, message);
			break;
		case univ_dev::LogClass::LogLevel::LOG_LEVEL_WARNING:
			fwprintf_s(logFile, L"LOG %.2d:%.2d:%.2d // WARNING // %s\n", curTm.tm_hour, curTm.tm_min, curTm.tm_sec, message);
			break;
		case univ_dev::LogClass::LogLevel::LOG_LEVEL_SYSTEM:
			fwprintf_s(logFile, L"LOG %.2d:%.2d:%.2d // SYSTEM // %s\n", curTm.tm_hour, curTm.tm_min, curTm.tm_sec, message);
			break;
		case univ_dev::LogClass::LogLevel::LOG_LEVEL_ERROR:
			fwprintf_s(logFile, L"LOG %.2d:%.2d:%.2d // ERROR // %s\n", curTm.tm_hour, curTm.tm_min, curTm.tm_sec, message);
			break;
		case univ_dev::LogClass::LogLevel::LOG_LEVEL_LIBRARY:
			fwprintf_s(logFile, L"Log %.2d:%.2d:%.2d // LIBRARY // %s\n", curTm.tm_hour, curTm.tm_min, curTm.tm_sec, message);
		}
		fclose(logFile);
	}
}

