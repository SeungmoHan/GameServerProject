#pragma once
#ifndef __PLAYER_HEADER__
#define __PLAYER_HEADER__

class Player
{
public:
	enum class Direction { LL, LU, UU, RU, RR, RD, DD, LD,NON };
	Player(int id, int x, int y);
	~Player() {};

	void Move(Direction dir);
	void Move(int x, int y);
	void Render();
	int GetID()const { return id; }
	int GetXPos()const { return x; }
	int GetYPos()const { return y; }
private:
	int id;
	int x;
	int y;
};

#endif // !__PLAYER_HEADER__
