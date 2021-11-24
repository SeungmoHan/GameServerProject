

#include <Windows.h>
#include "SceneManager"
#include <ctime>

#if _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__,__LINE__)
#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK,__FILE__,__LINE__)
#endif

int main()
{
	SceneManager* sceneManager = SceneManager::GetInstance();

	while (true)
	{
		sceneManager->Run();
		Sleep(50);
#if _DEBUG
		_CrtDumpMemoryLeaks();
#endif
	}
}