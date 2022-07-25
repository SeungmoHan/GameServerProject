#include "Player.h"

Player::Player(int id, int x, int y) : id(id),x(x),y(y){}

void Player::Move(Direction dir)
{
	switch (dir)
	{
	case Player::Direction::LL:
		if (x > 1)x--;//
		break;
	case Player::Direction::LU:
		if (x > 1)x--;
		if (y > 1)y--;
		break;
	case Player::Direction::UU:
		if (y > 1)y--;
		break;
	case Player::Direction::RU:
		if (x < 79)x++;
		if (y > 1)y--;
		break;
	case Player::Direction::RR:
		if (x < 79)x++;
		break;
	case Player::Direction::RD:
		if (x < 79)x++; 
		if (y < 23)y++;
		break;
	case Player::Direction::DD:
		if (y < 23)y++;
		break;
	case Player::Direction::LD:
		if (x > 1)x--;
		if (y < 23)y++;
		break;
	}
}

void Player::Move(int x, int y)
{
	this->x = x;
	this->y = y;
	if (this->x <= 0) this->x = 0;
	if (this->x >= 80) this->x = 80;
	if (this->y <= 0) this->y = 0;
	if (this->y >= 23) this->y = 23;
}


