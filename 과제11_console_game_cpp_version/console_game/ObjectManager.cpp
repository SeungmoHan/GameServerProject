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
	//다음프레임 시작때 큐에 들어와있는 제거되어야될 모든 오브젝트들 다삭제
	//리스트긴한데 내가 큐처럼 쓰면 그게 큐지...
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

	//2개이상이 부딪혔을 경우에는 둘에게 각각 OnCollision을 호출해줘야된다.
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
	//flip을 오브젝트 매니저가 하는거보다 scene에서 해주는게 맞는거같은느낌인데...
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
