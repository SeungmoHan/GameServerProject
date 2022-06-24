#include <GameServer.h>
#include <process.h>




int main()
{
	
	univ_dev::InitProfile();
	if (!univ_dev::g_ConfigReader.init(L"_GameServerConfig.ini")) return -1;;

	univ_dev::GameServer server;

	if (server.SetServerConfig())
	{
		server.AttachAuthBlock();
		server.AttachGameBlock();
		server.WaitForServerEnd();
		return 0;
	}
	
	printf("Server Setting Failed\n");
	system("pause");
	return 0;
}