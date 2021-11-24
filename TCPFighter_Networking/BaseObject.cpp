#include "BaseObject.h"

int BaseObject::objectIDCounts = 0;

void BaseObject::NextFrame()
{
	if (spriteStart < 0) return;
	delayCount++;
	if (delayCount >= frameDelay)
	{
		delayCount = 0;
		spriteNow++;
		if (spriteNow > spriteEnd-1)
		{
			if(this->objectType != e_OBJECT_TYPE::TYPE_EFFECT) spriteNow = spriteStart;
			endFrameFlag = true;
		}
	}
}
void BaseObject::SetSprite(int begin, int end, int frameDelay)
{
	if (begin != spriteStart || end != spriteEnd)
	{
		spriteNow = begin;
		spriteStart = begin;
		spriteEnd = end;

		this->frameDelay = frameDelay;
		delayCount = 0;
		endFrameFlag = false;
	}
}

BaseObject::BaseObject(int curX, int curY, e_OBJECT_TYPE type, int spriteStart, int spriteEnd,int objID) :objectID(objID), objectType(type), endFrameFlag(false), actionInput(-1), curX(curX), curY(curY), delayCount(0), frameDelay(DELAY_STAND),
spriteStart(spriteStart), spriteEnd(spriteEnd), spriteNow(spriteStart)
{

}


BaseObject::~BaseObject()
{

}

void BaseObject::Render(BYTE* pDest, int destWidth, int destHeight, int destPitch)
{
	//if (objectType != e_OBJECT_TYPE::TYPE_EFFECT)
	//{
	//	g_SpriteDibBuffer.DrawSprite50((int)e_SPRITE::SHADOW, curX, curY, pDest, destWidth, destHeight, destPitch);
	//	g_SpriteDibBuffer.DrawSprite((int)e_SPRITE::GUAGE_HP, curX, curY, pDest, destWidth, destHeight, destPitch);
	//}
	//g_SpriteDibBuffer.DrawSprite(GetSprite(), curX, curY, pDest, destWidth, destHeight, destPitch);
}
