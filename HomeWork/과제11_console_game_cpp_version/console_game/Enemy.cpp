#include <random>

#include "Enemy"
#include "Bullet"
#include "ObjectManager"
#include "Scene"
#include "SceneManager"
#include "random"


int Enemy::currentEnemyNum = 0;

Enemy::Enemy(int x, int y, ObjectInterface::ObjectType type, ObjectManager* m) : ObjectInterface(x, y, type, m), rand(50) { currentEnemyNum++; }
Enemy::~Enemy() { currentEnemyNum--; }
void Enemy::OnCollision(ObjectInterface* other)
{
	if (other->GetObjectType() == ObjectInterface::ObjectType::Bullet)
	{
		Bullet* bullet = dynamic_cast<Bullet*>(other);
		if (bullet->IsPlayerBullet())
		{
			manager->Destroy(this);
		}
	}
	else if (other->GetObjectType() == ObjectInterface::ObjectType::Player)
	{
		manager->Destroy(this);

	}
}

void Enemy::Update()
{

	if (balance >= 5) movingLeft = true;
	else if (balance <= -5) movingLeft = false;
	if (movingLeft)
	{
		this->xPos--;
		this->balance--;
	}
	else
	{
		this->xPos++;
		this->balance++;
	}

	//2 ÃÑÀ» ½ò°ÇÁö ¸»°ÇÁö
	if (rand.GetNextRandomCase())
	{
		manager->CreateObject(xPos, yPos, ObjectInterface::ObjectType::Bullet, false);
	}

}

void Enemy::Render()
{
	ScreenBuffer::GetInstance()->DrawSprite(xPos, yPos, 'E');
}
