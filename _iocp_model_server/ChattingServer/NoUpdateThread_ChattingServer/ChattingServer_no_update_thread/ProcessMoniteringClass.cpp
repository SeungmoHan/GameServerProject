#include"ProcessMoniteringClass.h"
#include <strsafe.h>
#include <Psapi.h>

namespace univ_dev
{
	ProcessMoniter::ProcessMoniter(HANDLE process) : _Process{ process }, _ProcessTotal{ 0 }, _ProcessUser{ 0 }, _ProcessKernel{ 0 }, _ProcessLastKernel{ 0 }, _ProcessLastUser{ 0 }, _ProcessLastTime{ 0 }
	{
		if (_Process == INVALID_HANDLE_VALUE)
			_Process = GetCurrentProcess();
		//if (_PrivateMemoryQuery == nullptr)
		//	PdhOpenQuery(NULL, NULL, &_PrivateMemoryQuery);

		GetModuleBaseName(_Process, nullptr, _ProcessName, MAX_PATH - 1);

		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		_NumOfProcessor = SystemInfo.dwNumberOfProcessors;

		UpdateProcessTime();
	}

	void ProcessMoniter::UpdateProcessTime()
	{
		ULARGE_INTEGER _;
		ULARGE_INTEGER nowTime;

		ULARGE_INTEGER kernel;
		ULARGE_INTEGER user;
		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);
		GetProcessTimes(_Process, (LPFILETIME)&_, (LPFILETIME)&_, (LPFILETIME)&kernel, (LPFILETIME)&user);

		ULONGLONG timeDiff;
		ULONGLONG userDiff;
		ULONGLONG kernelDiff;
		ULONGLONG total;
		timeDiff = nowTime.QuadPart - _ProcessLastTime.QuadPart;
		userDiff = user.QuadPart - _ProcessLastUser.QuadPart;
		kernelDiff = kernel.QuadPart - _ProcessLastKernel.QuadPart;
		total = kernelDiff + userDiff;
		_ProcessTotal = (float)(total / (double)_NumOfProcessor / (double)timeDiff * 100.0f);
		_ProcessKernel = (float)(kernelDiff / (double)_NumOfProcessor / (double)timeDiff * 100.0f);
		_ProcessUser = (float)(userDiff / (double)_NumOfProcessor / (double)timeDiff * 100.0f);
		_ProcessLastTime = nowTime;
		_ProcessLastKernel = kernel;
		_ProcessLastUser = user;


		PROCESS_MEMORY_COUNTERS_EX  counter;
		if (GetProcessMemoryInfo(_Process, (PROCESS_MEMORY_COUNTERS*)&counter, sizeof(counter)))
			_PrivateMemoryBytes = counter.PrivateUsage;
	}

}
