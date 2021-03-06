#pragma once
#ifndef __EFFECT_OBJECT_HEADER__
#define __EFFECT_OBJECT_HEADER__
#include "BaseObject.h"

namespace univ_dev
{
	class EffectObject : public BaseObject
	{
	public:
		EffectObject(int curX, int curY, e_OBJECT_TYPE type, DWORD attackID, int startFrame, int startSprite, int endSprite) : BaseObject(curX, curY, type, startSprite, endSprite, -1), effectStart(false), attackID(attackID), startFrame(startFrame) {};
		virtual ~EffectObject() {};

		void SetActive()
		{
			effectStart = true;
		}

		virtual void Render(BYTE*, int, int, int);
		virtual void Update();
	private:
		bool effectStart;
		int startFrame;
		DWORD attackID;
	};
}



#endif // !__EFFECT_OBJECT_HEADER__
