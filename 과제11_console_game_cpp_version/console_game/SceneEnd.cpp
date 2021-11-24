#include "Scene"
#include "SceneManager"
#include "ObjectInterface"
#include "ObjectManager"
#include <Windows.h>

SceneEnd::SceneEnd(bool gameWin)
{
	//���⼭ ���� Ÿ��Ʋ ���� �� ������Ʈ�� �����ؼ� ����ְ�...
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