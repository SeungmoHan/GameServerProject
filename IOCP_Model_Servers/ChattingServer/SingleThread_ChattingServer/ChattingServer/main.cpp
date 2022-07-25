#include "ChatServer.h"
#include <iostream>
#include <time.h>
#include <random>
#include <algorithm>
int a = 20;

int main()
{
    //alignas(64) 
        ULONGLONG f = __rdtsc(); 
    InterlockedExchange((unsigned long*)&a, 10);
    //alignas(64) 
        //ULONGLONG e = __rdtsc();
    //printf("%llu", e - f);
    //alignas(64) 
        //ULONGLONG 
        f = __rdtsc();
    InterlockedExchange((unsigned long*)&a, 10);
    //alignas(64) 
        ULONGLONG 
        e = __rdtsc();
    printf("%llu", e - f);

    return 0;

    univ_dev::InitProfile();
    if (!univ_dev::g_ConfigReader.init(L"_ChattingServerConfig.ini")) return -1;

    univ_dev::ChatServer server;
    server.ChatServerInit();

    return 0;
}