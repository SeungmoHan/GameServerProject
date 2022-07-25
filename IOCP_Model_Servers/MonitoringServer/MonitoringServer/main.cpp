#include "MonitoringServer.h"



int main()
{
    if(!univ_dev::g_ConfigReader.init(L"_MonitoringServerConfig.ini")) CRASH();

    univ_dev::MonitoringServer server;
    
    server.InitMonitoringServer();
    return 0;
}