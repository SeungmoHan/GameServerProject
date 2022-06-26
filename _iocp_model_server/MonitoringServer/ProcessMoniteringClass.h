#pragma once
#ifndef __PROCESS_MonitorING_CLASS__
#define __PROCESS_MonitorING_CLASS__
#define __UNIV_DEVELOPER_

#pragma comment(lib,"Pdh.lib")
#include <Pdh.h>

namespace univ_dev
{
	class ProcessMonitor
	{
	public:
		ProcessMonitor(HANDLE process = INVALID_HANDLE_VALUE);

		void UpdateProcessTime();

		inline float ProcessTotal() { return _ProcessTotal; }
		inline float ProcessUser() { return _ProcessUser; }
		inline float ProcessKernel() { return _ProcessKernel; }

		inline unsigned long long PrivateMemoryBytes() { return _PrivateMemoryBytes; }
		inline unsigned long long PrivateMemoryKBytes() { return _PrivateMemoryBytes >> 10; }
		inline unsigned long long PrivateMemoryMBytes() { return _PrivateMemoryBytes >> 20; }

	private:
		HANDLE _Process;
		WCHAR _ProcessName[MAX_PATH];
		int _NumOfProcessor;

		alignas(64)
		float _ProcessTotal;
		float _ProcessUser;
		float _ProcessKernel;

		ULARGE_INTEGER _ProcessLastKernel;
		ULARGE_INTEGER _ProcessLastUser;
		ULARGE_INTEGER _ProcessLastTime;
		unsigned long long _PrivateMemoryBytes;
	};
}



#endif // !__PROCESS_MonitorING_CLASS__
