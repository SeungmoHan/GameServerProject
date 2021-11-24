#include "Bullet"
#include "ObjectManager"




Bullet::Bullet(int x, int y, ObjectInterface::ObjectType type,ObjectManager* m,bool isPlayerBullet) : ObjectInterface(x, y, type,m), isPlayerBullet(isPlayerBullet){};

void Bullet::OnCollision(ObjectInterface* other)
{
	if (other->GetObjectType() == ObjectInterface::ObjectType::Enemy && isPlayerBullet)
	{
		manager->Destroy(this);
	}
	else if (other->GetObjectType() == ObjectInterface::ObjectType::Player && !isPlayerBullet)
	{
		manager->Destroy(this);
	}
	else if (other->GetObjectType() == ObjectInterface::ObjectType::Bullet)
	{
		Bullet* bullet = dynamic_cast<Bullet*>(other);
		if (bullet->isPlayerBullet != isPlayerBullet)
		{
			manager->Destroy(this);
		}
	}
}

bool Bullet::IsPlayerBullet()
{
	return isPlayerBullet;
}

void Bullet::Update()
{
	if (isPlayerBullet) yPos--;
	else yPos++;

	if (yPos >= bufferHeight || yPos <= 0)
	{
		manager->Destroy(this);
	}
}

void Bullet::Render()
{
	ScreenBuffer::GetInstance()->DrawSprite(xPos, yPos, 'B');
}
