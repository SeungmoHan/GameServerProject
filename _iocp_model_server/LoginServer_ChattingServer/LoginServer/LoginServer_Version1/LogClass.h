#pragma once
#ifndef __LOG_CLASS__
#define __LOG_CLASS__
#define __UNIV_DEVELOPER_
#include <Windows.h>


namespace univ_dev
{
	class LogClass
	{
	public:
		constexpr static int MAX_LOG_SIZE = 2048;
		enum class LogLevel
		{
			LOG_LEVEL_ALL, 
			LOG_LEVEL_DEBUG, 
			LOG_LEVEL_WARNING, 
			LOG_LEVEL_SYSTEM,
			LOG_LEVEL_LIBRARY,
			LOG_LEVEL_ERROR,
		};
	public:
		LogClass();
		~LogClass();
		void LOG_SET_LEVEL(LogLevel level);
		void LOG_SET_DIRECTORY(const WCHAR* directory);
		void LOG(const WCHAR* message, LogLevel logLevel);

	private:
		LogLevel _LogLevel;
		WCHAR _Directory[MAX_PATH];
		int _LogCount;
	};
}





#endif
