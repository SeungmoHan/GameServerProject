#pragma once
#ifndef __EFFECT_OBJECT_HEADER__
#define __EFFECT_OBJECT_HEADER__
#include "BaseObject.h"


class EffectObject : public BaseObject
{
public:
	EffectObject(int curX, int curY, e_OBJECT_TYPE type, DWORD attackID,int startFrame,int startSprite, int endSprite) : BaseObject(curX, curY, type,startSprite,endSprite), effectStart(false), attackID(attackID),startFrame(startFrame) {};
	~EffectObject() {};

	void SetActive()
	{
		effectStart = true;
	}

	virtual void Render(BYTE*,int,int,int);
	virtual void Update();
private:
	bool effectStart;
	int startFrame;
	DWORD attackID;
};


#endif // !__EFFECT_OBJECT_HEADER__
