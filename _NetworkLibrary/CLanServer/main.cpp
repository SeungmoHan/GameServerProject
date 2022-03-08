#include "IOCP_Echo_Server.h"


int main()
{
    unsigned short serverPort = 6000;
    int backlogQueueSize = SOMAXCONN;
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int threadPoolSize = info.dwNumberOfProcessors * 2;
    int runningThread = info.dwNumberOfProcessors;
    //threadPoolSize = 8;
    //runningThread = 8;
    printf("ThreadPool Size : ");
    scanf_s("%d", &threadPoolSize);
    printf("Num of Running Thread : ");
    scanf_s("%d", &runningThread);
    univ_dev::EchoServer server(serverPort, SOMAXCONN, threadPoolSize, runningThread, false, 10000);

    if (server.GetNetCoreInitializeFlag())
    {
        server.Run();
    }
    else
    {
        printf("Core -> Run Error UserError Code == %d\n", server.GetNetCoreErrorCode());
        printf("if api error == 0 no api error\n");
        printf("Core -> Run Error APIError Code == %d\n", server.GetLastAPIErrorCode());
    }

    return 0;
}