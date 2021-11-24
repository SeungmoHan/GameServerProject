#pragma once
#ifndef __BASE_OBJECT_HEADER__
#define __BASE_OBJECT_HEADER__
#include "ScreenDib.h"
#include "SpriteDib.h"
#include <vector>
#define RANGE_MOVE_TOP	50
#define RANGE_MOVE_LEFT	10
#define RANGE_MOVE_RIGHT	630
#define RANGE_MOVE_BOTTOM	470

#define DELAY_STAND	5
#define DELAY_MOVE	4
#define DELAY_ATTACK1	3
#define DELAY_ATTACK2	4
#define DELAY_ATTACK3	4
#define DELAY_EFFECT	3

class BaseObject
{
public:
	//base properties
	int GetCurrentX() { return curX; }
	int GetCurrentY() { return curY; }
	e_OBJECT_TYPE GetObjectType() { return objectType; }
	int GetObjectID() { return objectID; }
	void SetPosition(int x, int y) { curX = x; curY = y; }
	void NextFrame();
	bool IsEndFrame()
	{
		if (spriteNow >= spriteEnd) endFrameFlag = true;
		return endFrameFlag;
	}
	void SetAction(DWORD actionType) { actionInput = actionType; }
	DWORD GetAction() { DWORD ret = actionInput; actionInput = -1; return ret; }
	int GetSprite() { return spriteNow; }
	void SetSprite(int begin, int end, int frameDelay);
	BaseObject(int curX, int curY, e_OBJECT_TYPE type, int spriteStart, int spriteEnd);
	virtual ~BaseObject();

	//framework
	virtual void Render(BYTE* pDest, int destWidth, int destHeight, int destPitch);
	virtual void Update() { NextFrame(); };
private:
	static int objectIDCounts;
	e_OBJECT_TYPE objectType;
	int objectID;
	DWORD actionInput;
	int curY;
	int curX;
	bool endFrameFlag;
	int frameDelay;
	int delayCount;
	int spriteStart;
	int spriteEnd;
	int spriteNow;
};
extern std::vector<BaseObject*> g_ObjectList;
#endif // !__BASE_OBJECT_HEADER__
