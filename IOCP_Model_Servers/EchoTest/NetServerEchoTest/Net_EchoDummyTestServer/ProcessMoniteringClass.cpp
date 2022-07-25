#include"ProcessMoniteringClass.h"
#include <strsafe.h>
#include <Psapi.h>

namespace univ_dev
{
	ProcessMoniter::ProcessMoniter(HANDLE process) : _Process{ process }, _ProcessTotal{ 0 }, _ProcessUser{ 0 }, _ProcessKernel{ 0 }, _ProcessLastKernel{ 0 }, _ProcessLastUser{ 0 }, _ProcessLastTime{ 0 }
	{
		if (this->_Process == INVALID_HANDLE_VALUE)
			this->_Process = GetCurrentProcess();
		//if (_PrivateMemoryQuery == nullptr)
		//	PdhOpenQuery(NULL, NULL, &_PrivateMemoryQuery);

		GetModuleBaseName(this->_Process, nullptr, this->_ProcessName, MAX_PATH - 1);

		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		this->_NumOfProcessor = SystemInfo.dwNumberOfProcessors;

		UpdateProcessTime();
	}

	void ProcessMoniter::UpdateProcessTime()
	{
		ULARGE_INTEGER _;
		ULARGE_INTEGER nowTime;

		ULARGE_INTEGER kernel;
		ULARGE_INTEGER user;
		GetSystemTimeAsFileTime((LPFILETIME)&nowTime);
		GetProcessTimes(this->_Process, (LPFILETIME)&_, (LPFILETIME)&_, (LPFILETIME)&kernel, (LPFILETIME)&user);

		ULONGLONG timeDiff;
		ULONGLONG userDiff;
		ULONGLONG kernelDiff;
		ULONGLONG total;
		timeDiff = nowTime.QuadPart - this->_ProcessLastTime.QuadPart;
		userDiff = user.QuadPart - this->_ProcessLastUser.QuadPart;
		kernelDiff = kernel.QuadPart - this->_ProcessLastKernel.QuadPart;
		total = kernelDiff + userDiff;
		this->_ProcessTotal = (float)(total / (double)this->_NumOfProcessor / (double)timeDiff * 100.0f);
		this->_ProcessKernel = (float)(kernelDiff / (double)this->_NumOfProcessor / (double)timeDiff * 100.0f);
		this->_ProcessUser = (float)(userDiff / (double)this->_NumOfProcessor / (double)timeDiff * 100.0f);
		this->_ProcessLastTime = nowTime;
		this->_ProcessLastKernel = kernel;
		this->_ProcessLastUser = user;


		PROCESS_MEMORY_COUNTERS_EX  counter;
		if (GetProcessMemoryInfo(this->_Process, (PROCESS_MEMORY_COUNTERS*)&counter, sizeof(counter)))
			this->_PrivateMemoryBytes = counter.PrivateUsage;
	}

}
