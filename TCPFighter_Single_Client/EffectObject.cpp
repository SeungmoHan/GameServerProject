#include "EffectObject.h"

namespace univ_dev
{
		void EffectObject::Render(BYTE* pDest, int destWidth, int destHeight, int destPitch)
		{
			g_SpriteDibBuffer.DrawSprite(GetSprite(), GetCurrentX(), GetCurrentY(), pDest, destWidth, destHeight, destPitch);
		}

		void EffectObject::Update()
		{
			if (IsEndFrame()) return;
			startFrame--;
			if (startFrame < 0) effectStart = true;
			if (effectStart)
			{
				NextFrame();
			}
		}
		std::vector<univ_dev::BaseObject*> g_ObjectList;
}





