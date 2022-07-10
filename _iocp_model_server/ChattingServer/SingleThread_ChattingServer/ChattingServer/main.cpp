#include "ChatServer.h"
#include <iostream>
#include <time.h>
#include <random>
#include <algorithm>

int main()
{
    univ_dev::InitProfile();
    if (!univ_dev::g_ConfigReader.init(L"_ChattingServerConfig.ini")) return -1;;

    univ_dev::ChatServer server;
    server.ChatServerInit();

    return 0;
}