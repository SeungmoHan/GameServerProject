#include "Scene"
#include "SceneManager"
#include "ObjectInterface"
#include "ObjectManager"
#include <Windows.h>

SceneEnd::SceneEnd(bool gameWin)
{
	//여기서 이제 타이틀 씬에 들어갈 오브젝트들 생성해서 집어넣고...
	manager = new ObjectManager(this);
	manager->CreateObject(0, 0, ObjectInterface::ObjectType::EndingObject, false, gameWin);
}

SceneEnd::~SceneEnd()
{
	delete manager;
}

void SceneEnd::Update()
{
	if (GetAsyncKeyState(VK_RSHIFT))
	{
		SceneManager::GetInstance()->LoadScene(new SceneTitle());
	}
	manager->Update();
	manager->Render();
}