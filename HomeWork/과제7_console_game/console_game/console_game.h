#pragma once
#ifndef __CONSOLE_GAME__
#define __CONSOLE_GAME__

enum State
{
	START=1,PLAYING,END,LOAD
};

struct Player
{
	int hp;
	int x, y;
};


struct Enemy
{
	bool aliveFlag;
	bool goingLeft;
	int hp;
	int x, y;
};

struct Bullet
{
	bool isMyBullet;
	int x;
	int y;
};

void Load();
State GetCurrentState();
void Start();
bool GameInit();
void Play();
void Render();
void ClearBuffer();
void AdjustFrame();
void End();

#endif // !__CONSOLE_GAME__
