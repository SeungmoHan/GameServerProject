#include <set>

#include "list"
#include "Player"
#include "Bullet"
#include "Enemy"
#include "ObjectManager"
#include "SceneManager"
#include "TitleObject"
#include "Scene"
#include "EndingObject"
ObjectManager::ObjectManager(SceneInterface* currentScene) : currentScene(currentScene) {};
ObjectManager::~ObjectManager()
{
	auto iter = objects.begin();
	auto endIter = objects.end();
	while (endIter != iter)
	{
		delete (*iter);
		++iter;
	}
}
void ObjectManager::Update()
{
	//���������� ���۶� ť�� �����ִ� ���ŵǾ�ߵ� ��� ������Ʈ�� �ٻ���
	//����Ʈ���ѵ� ���� ťó�� ���� �װ� ť��...
	std::set<ObjectInterface*> s;
	while (!destroyTargetInNextFrame.empty())
	{
		auto destroyTarget = destroyTargetInNextFrame.begin();
		objects.remove(*destroyTarget);
		s.insert(*destroyTarget);
		destroyTargetInNextFrame.erase(destroyTarget);
	}
	for (auto obj : s)
		delete obj;
	auto iter = objects.begin();
	auto endIter = objects.end();

	while (iter != endIter)
	{
		(*iter)->Update();
		iter++;
	}

	//2���̻��� �ε����� ��쿡�� �ѿ��� ���� OnCollision�� ȣ������ߵȴ�.
	for (auto originIter = objects.begin(); originIter != endIter; ++originIter)
	{
		auto compIter = originIter;
		for (++compIter; compIter != endIter; ++compIter)
		{
			if ((*originIter)->xPos == (*compIter)->xPos && (*originIter)->yPos == (*compIter)->yPos)
			{
				(*originIter)->OnCollision(*compIter);
				(*compIter)->OnCollision(*originIter);
			}
		}
	}
}

void ObjectManager::Render()
{
	auto iter = objects.begin();
	auto endIter = objects.end();

	while (iter != endIter)
	{
		(*iter)->Render();
		iter++;
	}
	//flip�� ������Ʈ �Ŵ����� �ϴ°ź��� scene���� ���ִ°� �´°Ű��������ε�...
	//ScreenBuffer::GetInstance()->Flip();
}

void ObjectManager::Destroy(ObjectInterface* target,bool enemy)
{
	destroyTargetInNextFrame.push_back(target);
}

void ObjectManager::CreateObject(int x,int y,ObjectInterface::ObjectType type,bool isPlayerBullet, bool win)
{
	ObjectInterface* newObject;

	switch (type)
	{
	case ObjectInterface::ObjectType::Player:
		newObject = new Player(x, y, type, this);
		break;
	case ObjectInterface::ObjectType::Enemy:
		newObject = new Enemy(x, y, type, this);
		break;
	case ObjectInterface::ObjectType::Bullet:
		newObject = new Bullet(x, y, type, this, isPlayerBullet);
		break;
	case ObjectInterface::ObjectType::TitleObject:
		newObject = new TitleObject(x, y, ObjectInterface::ObjectType::TitleObject, this);
		break;
	case ObjectInterface::ObjectType::EndingObject:
		newObject = new EndingObject(x, y, ObjectInterface::ObjectType::EndingObject, this, win);
		break;
	default:
		newObject = nullptr;
		break;
	}
	objects.push_back(newObject);
}

void ObjectManager::PushObject(ObjectInterface* newObject)
{
	objects.push_back(newObject);
}
