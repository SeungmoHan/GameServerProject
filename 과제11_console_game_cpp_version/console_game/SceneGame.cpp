#include "ObjectManager"
#include "Scene"
#include "SceneManager"
#include "Player"
#include "Enemy"
#include <Windows.h>


SceneGame::SceneGame()
{
	manager = new ObjectManager(this);
	player = new Player(40, 15, ObjectInterface::ObjectType::Player, manager);
	manager->PushObject(player);
	
	for (size_t i = 0; i < 10; i++)
	{
		manager->CreateObject(15 + (i*5), 4, ObjectInterface::ObjectType::Enemy, false);
		numOfEnemy++;
	}
	for (size_t i = 0; i < 5; i++)
	{
		manager->CreateObject(30 + (i*5), 7, ObjectInterface::ObjectType::Enemy, false);
		numOfEnemy++;
	}
}

void SceneGame::Update()
{
	if (!Enemy::currentEnemyNum)
	{
		SceneManager::GetInstance()->LoadScene(new SceneEnd(true));
	}
	if (GetAsyncKeyState(VK_LEFT)) player->EnqueueMessage(Player::Direction::Left);
	if (GetAsyncKeyState(VK_RIGHT)) player->EnqueueMessage(Player::Direction::Right);
	if (GetAsyncKeyState(VK_UP)) player->EnqueueMessage(Player::Direction::Up);
	if (GetAsyncKeyState(VK_DOWN)) player->EnqueueMessage(Player::Direction::Down);
	if (GetAsyncKeyState(VK_SPACE)) player->EnqueueMessage(Player::Direction::Shoot);
	
	manager->Update();
	manager->Render();
}

SceneGame::~SceneGame()
{
	delete manager;
}
