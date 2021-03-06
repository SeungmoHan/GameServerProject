#include "PlayerObject.h"
#include "EffectObject.h"

///__univ_developer_player_object_

namespace univ_dev
{
	//#define RANGE_MOVE_TOP	50
//#define RANGE_MOVE_LEFT	10
//#define RANGE_MOVE_RIGHT	630
//#define RANGE_MOVE_BOTTOM	470

	PlayerObject::~PlayerObject()
	{
		auto iter = g_ObjectList.begin();
		for (; iter != g_ObjectList.end();)
		{
			bool find = false;
			for (auto effectIter = hitEffects.begin(); effectIter != hitEffects.end(); ++effectIter)
			{
				if (*iter == *effectIter)
				{
					iter = g_ObjectList.erase(iter);
					find = true;
					break;
				}
			}
			if (!find) iter++;
		}
	}

	void PlayerObject::ActionProc()
	{
		actionOld = GetOldAction();
		//if (isPlayerCharacter)
		actionCur = GetAction();
		//else actionCur = GetActionOther();
		if (isPlayerCharacter && (actionOld >= ACTION_ATTACK1 && actionOld <= ACTION_ATTACK3) && !IsEndFrame())
			return;
		if (!isPlayerCharacter && (actionCur == ACTION_ATTACK1 || actionCur == ACTION_ATTACK2 || actionCur == ACTION_ATTACK3))
		{
			if (IsEndFrame())
			{
				SetActionStand();
				SetAction(12345);
				return;
			}
		}
		switch (actionCur)
		{
		case ACTION_MOVE_LL:
			SetDirection(ACTION_MOVE_LL);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentX() <= RANGE_MOVE_LEFT) break;
			if (GetCurrentY() <= RANGE_MOVE_TOP) break;
			if (GetCurrentY() >= RANGE_MOVE_BOTTOM) break;
			SetPosition(GetCurrentX() - 3, GetCurrentY());
			break;
		case ACTION_MOVE_LU:
			SetDirection(ACTION_MOVE_LL);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentX() <= RANGE_MOVE_LEFT || GetCurrentY() <= RANGE_MOVE_TOP)break;

			SetPosition(GetCurrentX() - 3, GetCurrentY() - 2);
			break;
		case ACTION_MOVE_UU:
			SetDirection(dirOld);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentY() <= RANGE_MOVE_TOP || (GetCurrentX() <= RANGE_MOVE_LEFT || GetCurrentX() >= RANGE_MOVE_RIGHT)) break;
			SetPosition(GetCurrentX(), GetCurrentY() - 2);
			break;
		case ACTION_MOVE_RU:
			SetDirection(ACTION_MOVE_RR);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentX() >= RANGE_MOVE_RIGHT || GetCurrentY() <= RANGE_MOVE_TOP) break;
			SetPosition(GetCurrentX() + 3, GetCurrentY() - 2);
			break;
		case ACTION_MOVE_RR:
			SetDirection(ACTION_MOVE_RR);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentX() >= RANGE_MOVE_RIGHT) break;
			if (GetCurrentY() <= RANGE_MOVE_TOP) break;
			if (GetCurrentY() >= RANGE_MOVE_BOTTOM) break;
			SetPosition(GetCurrentX() + 3, GetCurrentY());
			break;
		case ACTION_MOVE_RD:
			SetDirection(ACTION_MOVE_RR);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentX() >= RANGE_MOVE_RIGHT || GetCurrentY() >= RANGE_MOVE_BOTTOM) break;
			SetPosition(GetCurrentX() + 3, GetCurrentY() + 2);
			break;
		case ACTION_MOVE_DD:
			SetDirection(dirOld);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentY() >= RANGE_MOVE_BOTTOM || (GetCurrentX() <= RANGE_MOVE_LEFT || GetCurrentX() >= RANGE_MOVE_RIGHT)) break;
			SetPosition(GetCurrentX(), GetCurrentY() + 2);
			break;
		case ACTION_MOVE_LD:
			SetDirection(ACTION_MOVE_LL);
			if (actionOld != actionCur) SetActionMove();
			if (GetCurrentX() <= RANGE_MOVE_LEFT || GetCurrentY() >= RANGE_MOVE_BOTTOM) break;
			SetPosition(GetCurrentX() - 3, GetCurrentY() + 2);
			break;
		case ACTION_ATTACK1:
			SetDirection(dirOld);
			if (actionOld != actionCur) SetActionAttack1();
			break;
		case ACTION_ATTACK2:
			SetDirection(dirOld);
			if (actionOld != actionCur) SetActionAttack2();
			break;
		case ACTION_ATTACK3:
			SetDirection(dirOld);
			if (actionOld != actionCur) SetActionAttack3();
			break;
		default:
			if (actionOld != actionCur) SetActionStand();
			break;
		}
	}

	void PlayerObject::InputActionProc(DWORD actionType, int newDirection)
	{
		if (actionCur >= ACTION_ATTACK1 && actionCur <= ACTION_ATTACK3)
			if (!IsEndFrame()) return;
		SetAction(actionType);
		SetDirection(newDirection);
	}

	int PlayerObject::GetDirection()
	{
		return dirCur;
	}

	int PlayerObject::GetHP()
	{
		return hp;
	}

	bool PlayerObject::IsPlayer()
	{
		return isPlayerCharacter;
	}

	void PlayerObject::UpdateEffect()
	{
		for (auto iter = hitEffects.begin(); iter != hitEffects.end();)
		{
			if ((*iter)->IsEndFrame())
			{
				g_ObjectList.erase(std::find(g_ObjectList.begin(), g_ObjectList.end(), *iter));
				iter = hitEffects.erase(iter);
			}
			else ++iter;
		}
	}

	void PlayerObject::CreateEffect(int startFrame, int xPos, int yPos)
	{
		EffectObject* effect = new EffectObject(xPos, yPos, e_OBJECT_TYPE::TYPE_EFFECT, GetObjectID(), startFrame, (int)e_SPRITE::EFFECT_SPARK_01, (int)e_SPRITE::EFFECT_SPARK_MAX);
		hitEffects.push_back(effect);
		g_ObjectList.push_back(effect);
		//if (hitEffect != nullptr) return;
		//hitEffect = new EffectObject(xPos, yPos, e_OBJECT_TYPE::TYPE_EFFECT, GetObjectID(), startFrame, (int)e_SPRITE::EFFECT_SPARK_01, (int)e_SPRITE::EFFECT_SPARK_MAX);
		//g_ObjectList.push_back(hitEffect);
	}

	void PlayerObject::SetActionAttack1()
	{
		//if (!isPlayerCharacter)
		//{
		//	DWORD actionCur = GetActionOther();
		//	DWORD actionOld = GetOldAction();
		//	if (actionCur == actionOld) return;
		//	if (actionCur != actionOld) actionOld = actionCur;
		//}
		int begin = -1, end = -1, frameDelay;

		if (dirCur == ACTION_MOVE_LL)
		{
			begin = (int)e_SPRITE::PLAYER_ATTACK1_L01;
			end = (int)e_SPRITE::PLAYER_ATTACK1_L_MAX;
		}
		else if (dirCur == ACTION_MOVE_RR)
		{
			begin = (int)e_SPRITE::PLAYER_ATTACK1_R01;
			end = (int)e_SPRITE::PLAYER_ATTACK1_R_MAX;
		}
		frameDelay = DELAY_ATTACK1;
		SetSprite(begin, end, frameDelay);
	}

	void PlayerObject::SetActionAttack2()
	{
		//if (!isPlayerCharacter)
		//{
		//	DWORD actionCur = GetActionOther();
		//	DWORD actionOld = GetOldAction();
		//	if (actionCur == actionOld) return;
		//	if (actionCur != actionOld) actionOld = actionCur;
		//}
		int begin = -1, end = -1, frameDelay;

		if (dirCur == ACTION_MOVE_LL)
		{
			begin = (int)e_SPRITE::PLAYER_ATTACK2_L01;
			end = (int)e_SPRITE::PLAYER_ATTACK2_L_MAX;
		}
		else if (dirCur == ACTION_MOVE_RR)
		{
			begin = (int)e_SPRITE::PLAYER_ATTACK2_R01;
			end = (int)e_SPRITE::PLAYER_ATTACK2_R_MAX;
		}
		frameDelay = DELAY_ATTACK2;
		SetSprite(begin, end, frameDelay);
	}

	void PlayerObject::SetActionAttack3()
	{
		//if (!isPlayerCharacter)
		//{
		//	DWORD actionCur = GetActionOther();
		//	DWORD actionOld = GetOldAction();
		//	if (actionCur == actionOld) return;
		//	if (actionCur != actionOld) actionOld = actionCur;
		//}
		int begin = -1, end = -1, frameDelay;

		if (dirCur == ACTION_MOVE_LL)
		{
			begin = (int)e_SPRITE::PLAYER_ATTACK3_L01;
			end = (int)e_SPRITE::PLAYER_ATTACK3_L_MAX;
		}
		else if (dirCur == ACTION_MOVE_RR)
		{
			begin = (int)e_SPRITE::PLAYER_ATTACK3_R01;
			end = (int)e_SPRITE::PLAYER_ATTACK3_R_MAX;
		}
		frameDelay = DELAY_ATTACK3;
		SetSprite(begin, end, frameDelay);
	}

	void PlayerObject::SetActionMove()
	{
		//if (!isPlayerCharacter)
		//{
		//	DWORD actionCur = GetActionOther();
		//	DWORD actionOld = GetOldAction();
		//	if (actionCur == actionOld) return;
		//	if (actionCur != actionOld) actionOld = actionCur;
		//}
		int begin = -1, end = -1, frameDelay;
		if (dirCur == ACTION_MOVE_LL)
		{
			begin = (int)e_SPRITE::PLAYER_MOVE_L01;
			end = (int)e_SPRITE::PLAYER_MOVE_L_MAX;
		}
		else if (dirCur == ACTION_MOVE_RR)
		{
			begin = (int)e_SPRITE::PLAYER_MOVE_R01;
			end = (int)e_SPRITE::PLAYER_MOVE_R_MAX;
		}
		frameDelay = DELAY_MOVE;
		if (begin == -1 || end == -1) return;
		SetSprite(begin, end, frameDelay);
	}

	void PlayerObject::SetActionStand()
	{
		//if (!isPlayerCharacter)
		//{
		//	DWORD actionCur = GetActionOther();
		//	DWORD actionOld = GetOldAction();
		//	if (actionCur == actionOld) return;
		//	if (actionCur != actionOld) actionOld = actionCur;
		//}
		int begin = -1, end = -1, frameDelay;
		if (dirCur == ACTION_MOVE_LL)
		{
			begin = (int)e_SPRITE::PLAYER_STAND_L01;
			end = (int)e_SPRITE::PLAYER_STAND_L_MAX;
		}
		else if (dirCur == ACTION_MOVE_RR)
		{
			begin = (int)e_SPRITE::PLAYER_STAND_R01;
			end = (int)e_SPRITE::PLAYER_STAND_R_MAX;
		}
		frameDelay = DELAY_STAND;
		if (begin == -1 || end == -1) return;
		SetSprite(begin, end, frameDelay);
	}

	void PlayerObject::Render(BYTE* pDest, int destWidth, int destHeight, int destPitch)
	{
		g_SpriteDibBuffer.DrawSprite50((int)e_SPRITE::SHADOW, GetCurrentX(), GetCurrentY(), pDest, destWidth, destHeight, destPitch);
		if (isPlayerCharacter) g_SpriteDibBuffer.DrawSpriteRed(GetSprite(), GetCurrentX(), GetCurrentY(), pDest, destWidth, destHeight, destPitch);
		else g_SpriteDibBuffer.DrawSprite(GetSprite(), GetCurrentX(), GetCurrentY(), pDest, destWidth, destHeight, destPitch);
		g_SpriteDibBuffer.DrawSprite((int)e_SPRITE::GUAGE_HP, GetCurrentX(), GetCurrentY(), pDest, destWidth, destHeight, destPitch, hp);
	}

	void PlayerObject::Update()
	{
		NextFrame();

		ActionProc();
		UpdateEffect();
		//if (hitEffect != nullptr && hitEffect->IsEndFrame())
		//{
		//	g_ObjectList.erase(std::find(g_ObjectList.begin(), g_ObjectList.end(), hitEffect));
		//	delete hitEffect;
		//	hitEffect = nullptr;
		//}
	}

}
