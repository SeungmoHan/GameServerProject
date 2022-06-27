#pragma comment(lib,"DbgHelp.lib")
#include <stdio.h>
#include "DumpClass.h"

namespace univ_dev
{
	volatile long MemoryDump::_DumpCount = 0;
	MemoryDump::MemoryDump()
	{
		_DumpCount = 0;
		_invalid_parameter_handler oldHandler, newHandler;

		newHandler = myInvalidParameterHandler;
		oldHandler = _set_invalid_parameter_handler(newHandler);

		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);

		_CrtSetReportHook(_custom_Report_hook);

		_set_purecall_handler(myPurecallHandler);

		SetHandlerDump();
	}

	LONG WINAPI MemoryDump::MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer)
	{
		long dumpCount = InterlockedIncrement(&_DumpCount);

		if (dumpCount == 1)
		{
			SYSTEMTIME time;
			WCHAR fileName[MAX_PATH];
			GetLocalTime(&time);
			wsprintf(fileName, L"Dump_%d%02d_%02d_%02d%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
			HANDLE dumpFile = CreateFile(fileName, GENERIC_ALL, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			wprintf(L"dumpFile Name : %s , dumpFile Handle value %p\n", fileName, dumpFile);

			if (dumpFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
				dumpInfo.ThreadId = GetCurrentThreadId();
				dumpInfo.ExceptionPointers = pExceptionPointer;
				dumpInfo.ClientPointers = true;

				MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), dumpFile, MiniDumpWithFullMemory, &dumpInfo, nullptr, nullptr);

				CloseHandle(dumpFile);
			}
		}
		Sleep(100);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	void MemoryDump::SetHandlerDump()
	{
		SetUnhandledExceptionFilter(MyExceptionFilter);
	}

	void MemoryDump::myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved)
	{
		Crash();
	}
	int MemoryDump::_custom_Report_hook(int ireposttype, char* message, int* returnvalue)
	{
		Crash();
		return true;
	}

	void MemoryDump::myPurecallHandler()
	{
		Crash();
	}

	void MemoryDump::Crash()
	{
		int* ptr = nullptr;
		*ptr = 100;
	}

	static MemoryDump dump;
}
