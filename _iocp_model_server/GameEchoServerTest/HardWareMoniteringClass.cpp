#include "HardWareMoniteringClass.h"
#include <strsafe.h>
#include <Psapi.h>
namespace univ_dev
{
	HardWareMoniter::HardWareMoniter(HANDLE hProcess) : _EthernetStruct{ 0 }, _Process(hProcess), _EthernetQuery(nullptr), _NonPagedPoolQuery(nullptr), _NetworkRecvBytes(0), _NetworkSendBytes(0), _NonPagedPoolBytes(0), _AvailableMemoryQuery(nullptr), _AvailableMemoryBytes(0)
	{
		//------------------------------------------------------------------
		// ���μ��� �ڵ� �Է��� ���ٸ� �ڱ� �ڽ��� �������...
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
		// ���μ��� ������ Ȯ���Ѵ�.
		//
		// ���μ��� (exe) ����� ���� cpu ������ �����⸦ �Ͽ� ���� ������ ����.
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
		// PDH enum Object �� ����ϴ� ���.
		// ��� �̴��� �̸��� �������� ���� ������� �̴���, �����̴��� ����� Ȯ�κҰ� ��.
		//---------------------------------------------------------------------------------------
		// PdhEnumObjectItems �� ���ؼ� "NetworkInterface" �׸񿡼� ���� �� �ִ�
		// �����׸�(Counters) / �������̽� �׸�(Interfaces) �� ����. �׷��� �� ������ ���̸� �𸣱� ������
		// ���� ������ ���̸� �˱� ���ؼ� Out Buffer ���ڵ��� NULL �����ͷ� �־ ����� Ȯ��.
		//---------------------------------------------------------------------------------------
		PdhEnumObjectItems(NULL, NULL, L"Network Interface", counterList, &counterSize, interfaceList, &instanceSize, PERF_DETAIL_WIZARD, 0);
		counterList = new WCHAR[counterSize];
		interfaceList = new WCHAR[instanceSize];
		//---------------------------------------------------------------------------------------
		// ������ �����Ҵ� �� �ٽ� ȣ��!
		// 
		// szCounters �� szInterfaces ���ۿ��� �������� ���ڿ��� ������ ���´�. 2���� �迭�� �ƴϰ�,
		// �׳� NULL �����ͷ� ������ ���ڿ����� dwCounterSize, dwInterfaceSize ���̸�ŭ ������ �������.
		// �̸� ���ڿ� ������ ��� ������ Ȯ�� �ؾ� ��. aaa\0bbb\0ccc\0ddd �̵� ��
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
		// szInterfaces ���� ���ڿ� ������ �����鼭 , �̸��� ����޴´�.
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
	// ���μ��� ������ �����Ѵ�.
	//
	// ������ ��� ����ü�� FILETIME ������, ULARGE_INTEGER �� ������ �����Ƿ� �̸� �����.
	// FILETIME ����ü�� 100 ���뼼���� ������ �ð� ������ ǥ���ϴ� ����ü��.
	//---------------------------------------------------------
		ULARGE_INTEGER idleTime;
		ULARGE_INTEGER kernelTime;
		ULARGE_INTEGER userTime;
		//---------------------------------------------------------
		// �ý��� ��� �ð��� ���Ѵ�.
		//
		// ���̵� Ÿ�� / Ŀ�� ��� Ÿ�� (���̵�����) / ���� ��� Ÿ��
		//---------------------------------------------------------
		if (GetSystemTimes((PFILETIME)&idleTime, (PFILETIME)&kernelTime, (PFILETIME)&userTime) == false)
		{
			return;
		}
		// Ŀ�� Ÿ�ӿ��� ���̵� Ÿ���� ���Ե�.
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
			// Ŀ�� Ÿ�ӿ� ���̵� Ÿ���� �����Ƿ� ���� ���.
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

