#include "Player"
#include "ObjectManager"
#include "SceneManager"
#include "Scene"
#include "Bullet"

Player::Player(int x, int y, ObjectInterface::ObjectType type,ObjectManager*m) : ObjectInterface(x, y, type,m) {}

void Player::Update()
{
	//로직을 실행해서 자기위치 이동시키고
	while (!messages.empty())
	{
		Direction dir = *messages.begin();
		messages.erase(messages.begin());
		switch (dir)
		{
		case Player::Direction::Up:
			if (yPos <= 1) continue;
			this->yPos--;
			break;
		case Player::Direction::Down:
			if (yPos >= bufferHeight - 2) continue;
			this->yPos++;
			break;
		case Player::Direction::Left:
			if (xPos <= 1) continue;
			this->xPos--;
			break;
		case Player::Direction::Right:
			if (xPos > bufferWidth - 4)continue;
			this->xPos++;
			break;
		case Player::Direction::Shoot:
			manager->CreateObject(xPos, yPos, ObjectInterface::ObjectType::Bullet,true);
			break;
		}
	}
}

void Player::Render()
{
	//실제 렌더라기 보다는 플립할 버퍼에 점찍는 느낌으로다가 먼저 해놓자.
	//플립은 나중에 매니저가 호출해야될거같음
	ScreenBuffer::GetInstance()->DrawSprite(this->xPos, this->yPos, 'P');
}

void Player::OnCollision(ObjectInterface* other)
{
	bool gameOver = false;
	if (other->GetObjectType() == ObjectInterface::ObjectType::Enemy)
	{
		gameOver = true;
	}
	else if (other->GetObjectType() == ObjectInterface::ObjectType::Bullet)
	{
		Bullet* bullet = static_cast<Bullet*>(other);
		if (!bullet->IsPlayerBullet())
		{
			gameOver = true;
		}
	}
	if (gameOver) SceneManager::GetInstance()->LoadScene(new SceneEnd(false));
}
void Player::EnqueueMessage(Direction d)
{
	messages.push_back(d);
}