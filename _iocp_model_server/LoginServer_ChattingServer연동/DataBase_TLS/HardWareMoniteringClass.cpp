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
		if (_Process == INVALID_HANDLE_VALUE)
			_Process = GetCurrentProcess();
		if (_EthernetQuery == nullptr)
			PdhOpenQuery(NULL, NULL, &_EthernetQuery);
		if (_NonPagedPoolQuery == nullptr)
			PdhOpenQuery(NULL, NULL, &_NonPagedPoolQuery);
		if (_AvailableMemoryQuery == nullptr)
			PdhOpenQuery(NULL, NULL, &_AvailableMemoryQuery);

		GetModuleBaseName(_Process, nullptr, _ProcessName, MAX_PATH - 1);

		//------------------------------------------------------------------
		// ���μ��� ������ Ȯ���Ѵ�.
		//
		// ���μ��� (exe) ����� ���� cpu ������ �����⸦ �Ͽ� ���� ������ ����.
		//------------------------------------------------------------------
		SYSTEM_INFO SystemInfo;
		GetSystemInfo(&SystemInfo);
		_NumberOfProcessors = SystemInfo.dwNumberOfProcessors;
		_ProcessorTotal = 0;
		_ProcessorUser = 0;
		_ProcessorKernel = 0;
		_ProcessorLastKernel.QuadPart = 0;
		_ProcessorLastUser.QuadPart = 0;
		_ProcessorLastIdle.QuadPart = 0;

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
		for (int i = 0; *curInterface != L'\0' && i < PDH_ETHERNET_MAX; curInterface += wcslen(curInterface) + 1, i++)
		{
			_EthernetStruct[i]._Used = true;
			_EthernetStruct[i]._Name[0] = L'\0';
			wcscpy_s(_EthernetStruct[i]._Name, curInterface);
			queryString[0] = L'\0';
			StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Received/sec", curInterface);
			PdhAddCounter(_EthernetQuery, queryString, NULL, &_EthernetStruct[i]._CounterNetworkRecvBytes);
			queryString[0] = L'\0';
			StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Network Interface(%s)\\Bytes Sent/sec", curInterface);
			PdhAddCounter(_EthernetQuery, queryString, NULL, &_EthernetStruct[i]._CounterNetworkSendBytes);
		}
		queryString[0] = L'\0';
		StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Memory\\Pool Nonpaged Bytes");
		PdhAddCounter(_NonPagedPoolQuery, queryString, NULL, &_NonPagedPoolCounter);


		queryString[0] = L'\0';
		StringCbPrintf(queryString, sizeof(WCHAR) * 1024, L"\\Memory\\Available Bytes");
		PdhAddCounter(_AvailableMemoryQuery, queryString, NULL, &_AvailableMemoryCounter);

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
		ULONGLONG KernelDiff = kernelTime.QuadPart - _ProcessorLastKernel.QuadPart;
		ULONGLONG UserDiff = userTime.QuadPart - _ProcessorLastUser.QuadPart;
		ULONGLONG IdleDiff = idleTime.QuadPart - _ProcessorLastIdle.QuadPart;
		ULONGLONG Total = KernelDiff + UserDiff;
		ULONGLONG TimeDiff;
		if (Total == 0)
		{
			_ProcessorUser = 0.0f;
			_ProcessorKernel = 0.0f;
			_ProcessorTotal = 0.0f;
		}
		else
		{
			// Ŀ�� Ÿ�ӿ� ���̵� Ÿ���� �����Ƿ� ���� ���.
			_ProcessorTotal = (double)((double)(Total - IdleDiff) / Total * 100.0f);
			_ProcessorUser = (double)((double)UserDiff / Total * 100.0f);
			_ProcessorKernel = (double)((double)(KernelDiff - IdleDiff) / Total * 100.0f);
		}
		_ProcessorLastKernel = kernelTime;
		_ProcessorLastUser = userTime;
		_ProcessorLastIdle = idleTime;


		PDH_STATUS status;
		PDH_FMT_COUNTERVALUE counterValue;
		PdhCollectQueryData(_EthernetQuery);

		for (int iCnt = 0; iCnt < PDH_ETHERNET_MAX; iCnt++)
		{
			if (_EthernetStruct[iCnt]._Used)
			{
				status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._CounterNetworkRecvBytes,
					PDH_FMT_DOUBLE, nullptr, &counterValue);
				if (status == ERROR_SUCCESS) _NetworkRecvBytes += counterValue.doubleValue;
				status = PdhGetFormattedCounterValue(_EthernetStruct[iCnt]._CounterNetworkSendBytes,
					PDH_FMT_DOUBLE, nullptr, &counterValue);
				if (status == ERROR_SUCCESS) _NetworkSendBytes += counterValue.doubleValue;
			}
		}

		PdhCollectQueryData(_NonPagedPoolQuery);
		status = PdhGetFormattedCounterValue(_NonPagedPoolCounter, PDH_FMT_LARGE, nullptr, &counterValue);
		if (status == ERROR_SUCCESS) _NonPagedPoolBytes = counterValue.largeValue;

		PdhCollectQueryData(_AvailableMemoryQuery);
		status = PdhGetFormattedCounterValue(_AvailableMemoryCounter, PDH_FMT_LARGE, nullptr, &counterValue);
		if (status == ERROR_SUCCESS) _AvailableMemoryBytes = counterValue.largeValue;

	}
}

