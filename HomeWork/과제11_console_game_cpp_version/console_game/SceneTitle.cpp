#include "ObjectManager"
#include "Scene"
#include "SceneManager"
#include <Windows.h>



SceneTitle::SceneTitle()
{
	//���⼭ ���� Ÿ��Ʋ ���� �� ������Ʈ�� �����ؼ� ����ְ�...
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


