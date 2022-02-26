#pragma once
#ifndef __NET_CORE_ERROR_DEFINE__
#define __NET_CORE_ERROR_DEFINE__
#define __UNIV_DEVEOPER_



/// Univ_Developer NetCore Error Define
#define ACCEPT_THREAD_COUNT (size_t)1
#define MONITERING_THREAD_COUNT (size_t)1


//---------------------------------------------------------------
// NetCore Initialize Failed Error Code

#define dfNCINIT_NET_CORE_ALREADY_EXIST 8000

#define dfNCINIT_WSASTARTUP_FAILED 8001

#define dfNCINIT_IOCOMPLETIONPORT_CREATE_FAILED 8002

#define dfNCINIT_LOG_THREAD_CREATE_FAILED 8003

#define dfNCINIT_WORKER_THREAD_CREATE_FAILED_0 7000

#define dfNCINIT_LISTEN_SOCKET_CREATE_FAILED 8004

#define dfNCINIT_SOCKET_BIND_FAILED 8005

#define dfNCINIT_SOCKET_LISTEN_FAILED 8006

#define dfNCINIT_ACCEPT_THREAD_CREATE_FAILED 8007

#define dfNCINIT_RUNNING_EVENT_CREATE_FAILED 8008

//---------------------------------------------------------------





#endif // !__NET_CORE_ERROR_DEFINE__
