#include "ChatServer.h"
#include <iostream>
#include <time.h>
#include <random>
#include <algorithm>

#define CHAT_SERVER_PORT 10445
#define MAX_SESSION_COUNT 20000
#define TIME_OUT_CLOCK_TIME 40000


int main()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    
    int threadPoolSize = info.dwNumberOfProcessors * 2;
    int runningThread = info.dwNumberOfProcessors;

    univ_dev::InitProfile();
    printf("ThreadPool Size : ");
    scanf_s("%d", &threadPoolSize);
    printf("Num of Running Thread : ");
    scanf_s("%d", &runningThread);

    

    univ_dev::ChatServer server(CHAT_SERVER_PORT, SOMAXCONN, threadPoolSize, runningThread, false, MAX_SESSION_COUNT, TIME_OUT_CLOCK_TIME);
    return 0;
}