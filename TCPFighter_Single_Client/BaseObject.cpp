#include "BaseObject.h"

///__univ_developer_base_object_

namespace univ_dev
{
		int BaseObject::objectIDCounts = 0;

		void BaseObject::NextFrame()
		{
			if (spriteStart < 0) return;

			delayCount++;
			if (delayCount >= frameDelay)
			{
				delayCount = 0;
				spriteNow++;
				if (spriteNow > spriteEnd - 1)
				{
					if (this->objectType != e_OBJECT_TYPE::TYPE_EFFECT) spriteNow = spriteStart;
					endFrameFlag = true;
				}
			}
		}
		void BaseObject::SetSprite(int begin, int end, int frameDelay)
		{
			spriteNow = begin;
			spriteStart = begin;
			spriteEnd = end;

			this->frameDelay = frameDelay;
			delayCount = 0;
			endFrameFlag = false;
		}

		BaseObject::BaseObject(int curX, int curY, e_OBJECT_TYPE type, int spriteStart, int spriteEnd) : objectType(type), endFrameFlag(false), actionInput(-1), curX(curX), curY(curY), delayCount(0), frameDelay(DELAY_STAND),
			spriteStart(spriteStart), spriteEnd(spriteEnd), spriteNow(spriteStart), objectID(objectIDCounts++)
		{

		}


		BaseObject::~BaseObject()
		{

		}

		void BaseObject::Render(BYTE* pDest, int destWidth, int destHeight, int destPitch)
		{

		}
}
