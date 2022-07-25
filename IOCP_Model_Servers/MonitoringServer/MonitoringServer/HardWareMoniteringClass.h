#pragma once
#ifndef __HARD_WARE_MonitorING_CLASS__
#define __HARD_WARE_MonitorING_CLASS__
#define __UNIV_DEVELOPER_

#pragma comment(lib,"Pdh.lib")
#include <Pdh.h>


namespace univ_dev
{
	class HardWareMonitor
	{
	private:
		constexpr static int PDH_ETHERNET_MAX = 8;
		struct EthernetPDH
		{
			bool _Used;
			WCHAR _Name[128];
			PDH_HCOUNTER _CounterNetworkRecvBytes;
			PDH_HCOUNTER _CounterNetworkSendBytes;
		};
	public:
		//----------------------------------------------------------------------
		// 생성자, 확인대상 프로세스 핸들. 미입력시 자기 자신.
		//----------------------------------------------------------------------
		HardWareMonitor(HANDLE hProcess = INVALID_HANDLE_VALUE);
		void UpdateHardWareTime();
		inline double ProcessorTotal() { return this->_ProcessorTotal; }
		inline double ProcessorUser() { return this->_ProcessorUser; }
		inline double ProcessorKernel() { return this->_ProcessorKernel; }

		inline double EthernetSendBytes() { return this->_NetworkSendBytes; }
		inline double EthernetSendKBytes() { return this->_NetworkSendBytes / 1024; }
		inline double EthernetSendMBytes() { return this->_NetworkSendBytes / 1024 / 1024; }

		inline double EthernetRecvBytes() { return this->_NetworkRecvBytes; }
		inline double EthernetRecvKBytes() { return this->_NetworkRecvBytes / 1024; }
		inline double EthernetRecvMBytes() { return  this->_NetworkRecvBytes / 1024 / 1024; }

		inline unsigned long long NonPagedPoolBytes() { return this->_NonPagedPoolBytes; }
		inline unsigned long long NonPagedPoolKBytes() { return this->_NonPagedPoolBytes >> 10; }
		inline unsigned long long NonPagedPoolMBytes() { return this->_NonPagedPoolBytes >> 20; }

		inline unsigned long long AvailableMemoryBytes() { return this->_AvailableMemoryBytes; }
		inline unsigned long long AvailableMemoryKBytes() { return this->_AvailableMemoryBytes >> 10; }
		inline unsigned long long AvailableMemoryMBytes() { return this->_AvailableMemoryBytes >> 20; }
	private:
		HANDLE _Process;
		int _NumberOfProcessors;
		PDH_HQUERY _NonPagedPoolQuery;
		PDH_HQUERY _AvailableMemoryQuery;
		PDH_HQUERY _EthernetQuery;
		WCHAR _ProcessName[MAX_PATH];


		alignas(64)
		double _ProcessorTotal;
		double _ProcessorUser;
		double _ProcessorKernel;
		ULARGE_INTEGER _ProcessorLastKernel;
		ULARGE_INTEGER _ProcessorLastUser;
		ULARGE_INTEGER _ProcessorLastIdle;

		PDH_HCOUNTER _NonPagedPoolCounter;
		unsigned long long _NonPagedPoolBytes;

		PDH_HCOUNTER _AvailableMemoryCounter;
		unsigned long long _AvailableMemoryBytes;

		EthernetPDH _EthernetStruct[PDH_ETHERNET_MAX]; // 랜카드 별 PDH 정보
		double _NetworkRecvBytes; // 총 Recv Bytes 모든 이더넷의 Recv 수치 합산
		double _NetworkSendBytes; // 총 Send Bytes 모든 이더넷의 Send 수치 합
	};
}




#endif // !__HARD_WARE_MonitorING_CLASS__
