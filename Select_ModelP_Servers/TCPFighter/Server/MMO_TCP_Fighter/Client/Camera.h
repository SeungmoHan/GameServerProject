#pragma once
#ifndef __CAMERA_HEADER__
#define __CAMERA_HEADER__
#define __UNIV_DEVELOPER_
#include "BaseObject.h"


#define CAMERA_MIN_WIDTH RANGE_MOVE_LEFT
#define CAMERA_MIN_HEIGHT RANGE_MOVE_TOP
#define CAMERA_MAX_WIDTH RANGE_MOVE_RIGHT-SCREEN_WIDTH
#define CAMERA_MAX_HEIGHT RANGE_MOVE_BOTTOM-SCREEN_HEIGHT

namespace univ_dev
{
	class CameraObject : public BaseObject
	{
	public:
		virtual void Render(BYTE* pDest, int destWidth, int destHeight, int destPitch) {};
		virtual void Update() 
		{
			curX = pTargetObject->GetCurrentX() - (SCREEN_WIDTH / 2);
			curY = pTargetObject->GetCurrentY() - (SCREEN_HEIGHT / 2);

			if (curX > CAMERA_MAX_WIDTH)
				curX = CAMERA_MAX_WIDTH;
			else if (curX < CAMERA_MIN_WIDTH)
				curX = CAMERA_MIN_WIDTH;
			if (curY > CAMERA_MAX_HEIGHT)
				curY = CAMERA_MAX_HEIGHT;
			else if (curY < CAMERA_MIN_HEIGHT)
				curY = CAMERA_MIN_HEIGHT;
		};
		void SetTarget(BaseObject* newTarget)
		{
			if (newTarget == nullptr) return;
			pTargetObject = newTarget;
		}
		CameraObject() : BaseObject(0, 0, e_OBJECT_TYPE::TYPE_PLAYER, 0, 0, objectIDCounts++), pTargetObject(nullptr) {};
	private:
		BaseObject* pTargetObject;
	};
}



#endif // !__CAMERA_HEADER__
