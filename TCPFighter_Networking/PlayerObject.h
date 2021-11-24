#pragma once
#ifndef __PLAYER_OBJECT_HEADER__
#define __PLAYER_OBJECT_HEADER__
#include "BaseObject.h"
#include <list>

class EffectObject;
class PlayerObject : public BaseObject
{
public:
	PlayerObject(int xPos, int yPos, e_OBJECT_TYPE type, bool isPlayerCharacter,int spriteStart,int spriteEnd,int objID, int hp) : BaseObject(xPos, yPos, type,spriteStart,spriteEnd,objID), isPlayerCharacter(isPlayerCharacter)
		, hp(hp), actionCur(-1), actionOld(-1), dirCur(ACTION_MOVE_RR), dirOld(-1)/*,hitEffect(nullptr)*/{};

	virtual ~PlayerObject();

	void ActionProc();
	void InputActionProc(DWORD actionType, int newDirection);
	int GetDirection();
	int GetHP();
	bool IsPlayer();
	void UpdateEffect();
	void CreateEffect(int startFrame,int xPos,int yPos);
	void SetActionAttack1();
	void SetActionAttack2();
	void SetActionAttack3();
	void SetActionMove();
	void SetActionStand();
	void SetDirection(int direction) { dirOld = dirCur; dirCur = direction; };
	void SetHP(int hp) { this->hp = hp; }

	virtual void Render(BYTE* pDest,int destWidth,int destHeight,int destPitch);
	virtual void Update();

 private:
	bool isPlayerCharacter;
	int hp;
	std::list<EffectObject*> hitEffects;
	//EffectObject* hitEffect;
	DWORD actionCur;
	DWORD actionOld;
	int dirCur;
	int dirOld;
};

#endif // !__PLAYER_OBJECT_HEADER__
