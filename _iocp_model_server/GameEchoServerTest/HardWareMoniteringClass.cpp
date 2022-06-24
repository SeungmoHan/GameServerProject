#include "HardWareMoniteringClass.h"
#include <strsafe.h>
#include <Psapi.h>
namespace univ_dev
{
	HardWareMoniter::HardWareMoniter(HANDLE hProcess) : _EthernetStruct{ 0 }, _Process(hProcess), _EthernetQuery(nullptr), _NonPagedPoolQuery(nullptr), _NetworkRecvBytes(0), _NetworkSendBytes(0), _NonPagedPoolBytes(0), _AvailableMemoryQuery(nullptr), _AvailableMemoryBytes(0)
	{
		//------------------------------------------------------------------
		// 프로세스 핸들 입력이 없다면 자기 자신을 대상으로...
		//------------------------------------------------------------------
		if (this->_Process == INVALID_HANDLE_VALUE)
			this->_Process = GetCurrentProcess();
		if (this->_EthernetQuery == nullptr)
			PdhOpenQuery(NULL, NULL, &this->_EthernetQuery);
		if (this->_NonPagedPoolQuery == nullptr)
			PdhOpenQuery(NULL, NULL, &this->_NonPagedPoolQuery);
		if (this->_AvailableMemoryQuery == nullptr)
			PdhOpenQuery(NULL, NULL, &this->_AvailableMemoryQuery);

		GetModuleBaseName(this->_Process, nullptr, this->_ProcessName, MAX_PATH - 1);

		//------------------------------------------------------------------
		// 프로세서 개수를 확인한다.
		//
		// 프로세스 (exe) 실행률 계산시 cpu 개수로 나누기를 하여 실제 사용률을 구함.
		//------------------------------------------------------------------
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		this->_NumberOfProcessors = SystemInfo.dwNumberOfProcessors;
		this->_ProcessorTotal = 0;
		this->_ProcessorUser = 0;
		this->_ProcessorKernel = 0;
		this->_ProcessorLastKernel.QuadPart = 0;
		this->_ProcessorLastUser.QuadPart = 0;
		this->_ProcessorLastIdle.QuadPart = 0;

		WCHAR* curInterface = NULL;
		WCHAR* counterList = NULL;
		WCHAR* interfaceList = NULL;
		DWORD counterSize = 0, instanceSize = 0;
		WCHAR queryString[1024] = { 0, };
		// PDH enum Object 를 사용하는 방법.
		// 모든 이더넷 이름이 나오지만 실제 사용중인 이더넷, 가상이더넷 등등을 확인불가 함.
		//---------------------------------------------------------------------------------------
		// PdhEnumObjectItems 을 통해서 "NetworkInterface" 항목에서 얻을 수 있는
		// 측성항목(Counters) / 인터페이스 항목(Interfaces) 를 얻음. 그런데 그 개수나 길이를 모르기 때문에
		// 먼저 버퍼의 길이를 알기 위해서 Out Buffer 인자들을 NULL 포인터로 넣어서 사이즈만 확인.
		//---------------------------------------------------------------------------------------
		PdhEnumObjectItems(NULL, NULL, L"Network Interface", counterList, &counterSize, interfaceList, &instanceSize, PERF_DETAIL_WIZARD, 0);
		counterList = new WCHAR[counterSize];
		interfaceList = new WCHAR[instanceSize];
		//---------------------------------------------------------------------------------------
		// 버퍼의 동적할당 후 다시 호출!
		// 
		// szCounters 와 szInterfaces 버퍼에는 여러개의 문자열이 쭉쭉쭉 들어온다. 2차원 배열도 아니고,
		// 그냥 NULL 포인터로 끝나는 문자열들이 dwCounterSize, dwInterfaceSize 길이만큼 줄줄이 들어있음.
		// 이를 문자열 단위로 끊어서 개수를 확인 해야 함. aaa\0bbb\0ccc\0ddd 이딴 식
		//---------------------------------------------------------------------------------------
		if (PdhEnumObjectItems(NULL, NULL, L"Network Interface", counterList, &counterSize, interfaceList, &instanceSize, PERF_DETAIL_WIZARD,
			0) != ERROR_SUCCESS)
		{
			delete[] counterList;
			delete[] interfaceList;
			return;
		}
		curInterface = interfaceList;
		//---------------------------------------------------------
		// szInterfaces 에서 문자열 단위로 끊으면서 , 이름을 복사받는다.
		//---------------------------------------------------------
		for (int i = 0; *curInterface != L'\0' && i < this->PDH_ETHERNET_MAX; curInterface += wcslen(curInterface) + 1, i++)
		{
			this->_EthernetStruct[i]._Used = true;
			this->_EthernetStruct[i]._Name[0] = L'\0';
			wcscpy_s(this->_EthernetStruct[i]._Name, curInterface);
			queryString[0] = L'\0';
			StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", curInterface);
			PdhAddCounter(this->_EthernetQuery, queryString, NULL, &this->_EthernetStruct[i]._CounterNetworkRecvBytes);
			queryString[0] = L'\0';
			StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", curInterface);
			PdhAddCounter(this->_EthernetQuery, queryString, NULL, &this->_EthernetStruct[i]._CounterNetworkSendBytes);
		}
		queryString[0] = L'\0';
		StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Memory\\Pool Nonpaged Bytes");
		PdhAddCounter(this->_NonPagedPoolQuery, queryString, NULL, &this->_NonPagedPoolCounter);


		queryString[0] = L'\0';
		StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Memory\\Available Bytes");
		PdhAddCounter(this->_AvailableMemoryQuery, queryString, NULL, &this->_AvailableMemoryCounter);

		UpdateHardWareTime();
	}

	void HardWareMoniter::UpdateHardWareTime()
	{
		//---------------------------------------------------------
	// 프로세서 사용률을 갱신한다.
	//
	// 본래의 사용 구조체는 FILETIME 이지만, ULARGE_INTEGER 와 구조가 같으므로 이를 사용함.
	// FILETIME 구조체는 100 나노세컨드 단위의 시간 단위를 표현하는 구조체임.
	//---------------------------------------------------------
		ULARGE_INTEGER idleTime;
		ULARGE_INTEGER kernelTime;
		ULARGE_INTEGER userTime;
		//---------------------------------------------------------
		// 시스템 사용 시간을 구한다.
		//
		// 아이들 타임 / 커널 사용 타임 (아이들포함) / 유저 사용 타임
		//---------------------------------------------------------
		if (GetSystemTimes((PFILETIME)&idleTime, (PFILETIME)&kernelTime, (PFILETIME)&userTime) == false)
		{
			return;
		}
		// 커널 타임에는 아이들 타임이 포함됨.
		ULONGLONG KernelDiff = kernelTime.QuadPart - this->_ProcessorLastKernel.QuadPart;
		ULONGLONG UserDiff = userTime.QuadPart - this->_ProcessorLastUser.QuadPart;
		ULONGLONG IdleDiff = idleTime.QuadPart - this->_ProcessorLastIdle.QuadPart;
		ULONGLONG Total = KernelDiff + UserDiff;
		ULONGLONG TimeDiff;
		if (Total == 0)
		{
			this->_ProcessorUser = 0.0f;
			this->_ProcessorKernel = 0.0f;
			this->_ProcessorTotal = 0.0f;
		}
		else
		{
			// 커널 타임에 아이들 타임이 있으므로 빼서 계산.
			this->_ProcessorTotal = (double)((double)(Total - IdleDiff) / Total * 100.0f);
			this->_ProcessorUser = (double)((double)UserDiff / Total * 100.0f);
			this->_ProcessorKernel = (double)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		}
		this->_ProcessorLastKernel = kernelTime;
		this->_ProcessorLastUser = userTime;
		this->_ProcessorLastIdle = idleTime;


		PDH_STATUS status;
		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(this->_EthernetQuery);

		for (int iCnt = 0; iCnt < this->PDH_ETHERNET_MAX; iCnt++)
		{
			if (this->_EthernetStruct[iCnt]._Used)
			{
				status = PdhGetFormattedCounterValue(this->_EthernetStruct[iCnt]._CounterNetworkRecvBytes,
					PDH_FMT_DOUBLE, nullptr, &counterValue);
				if (status == ERROR_SUCCESS)
				{
					this->_NetworkRecvBytes += counterValue.doubleValue;
					this->_NetworkRecvBytesPerSec = counterValue.doubleValue;
				}
				status = PdhGetFormattedCounterValue(this->_EthernetStruct[iCnt]._CounterNetworkSendBytes,
					PDH_FMT_DOUBLE, nullptr, &counterValue);
				if (status == ERROR_SUCCESS)
				{
					this->_NetworkSendBytes += counterValue.doubleValue;
					this->_NetworkSendBytesPerSec = counterValue.doubleValue;
				}
			}
		}

		PdhCollectQueryData(this->_NonPagedPoolQuery);
		status = PdhGetFormattedCounterValue(this->_NonPagedPoolCounter, PDH_FMT_LARGE, nullptr, &counterValue);
		if (status == ERROR_SUCCESS) this->_NonPagedPoolBytes = counterValue.largeValue;

		PdhCollectQueryData(this->_AvailableMemoryQuery);
		status = PdhGetFormattedCounterValue(this->_AvailableMemoryCounter, PDH_FMT_LARGE, nullptr, &counterValue);
		if (status == ERROR_SUCCESS) this->_AvailableMemoryBytes = counterValue.largeValue;
	}
}

