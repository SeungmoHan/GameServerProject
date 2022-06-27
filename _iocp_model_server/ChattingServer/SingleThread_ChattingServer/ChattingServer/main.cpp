#include "ChatServer.h"
#include <iostream>
#include <time.h>
#include <random>
#include <algorithm>
#define CHAT_SERVER_PORT 11335

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
    univ_dev::ChatServer server(CHAT_SERVER_PORT, SOMAXCONN, threadPoolSize, runningThread, false, 20000);
    return 0;
}