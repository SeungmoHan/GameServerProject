#include "ObjectManager"
#include "Scene"
#include "SceneManager"
#include <Windows.h>



SceneTitle::SceneTitle()
{
	//여기서 이제 타이틀 씬에 들어갈 오브젝트들 생성해서 집어넣고...
	manager = new ObjectManager(this);
	manager->CreateObject(0, 0, ObjectInterface::ObjectType::TitleObject, false);
}

SceneTitle::~SceneTitle()
{
	delete manager;
}

void SceneTitle::Update()
{
	if (GetAsyncKeyState(VK_RETURN))
	{
		SceneManager::GetInstance()->LoadScene(new SceneGame());
	}
	manager->Update();
	manager->Render();
}


