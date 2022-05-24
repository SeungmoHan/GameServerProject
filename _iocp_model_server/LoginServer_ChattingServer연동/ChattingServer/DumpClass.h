#pragma once
#ifndef __DUMP_CLASS_HEADER__
#define __DUMP_CLASS_HEADER__
#define __UNIV_DEVELOPER_
#include <Windows.h>
#include <DbgHelp.h>
#include <crtdbg.h>



namespace univ_dev
{
	class MemoryDump
	{
	public:

		MemoryDump();
		static LONG WINAPI MyExceptionFilter(__in PEXCEPTION_POINTERS pExceptionPointer);
		static void SetHandlerDump();
		static void myInvalidParameterHandler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t pReserved);
		static int _custom_Report_hook(int ireposttype, char* message, int* returnvalue);
		static void myPurecallHandler();
		static void Crash();
	private:
		static long _DumpCount;
	};
}



#endif // !__DUMP_CLASS_HEADER__
