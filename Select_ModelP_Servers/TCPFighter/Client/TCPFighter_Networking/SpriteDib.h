#pragma once
#ifndef __SPRITE_DIB_HEADER__
#define __SPRITE_DIB_HEADER__
#define __UNIV_DEVELOPER_
#include "framework.h"
namespace univ_dev
{
	enum class e_SPRITE
	{
		MAP,
		//Left Stand
		PLAYER_STAND_L01, PLAYER_STAND_L02, PLAYER_STAND_L03, PLAYER_STAND_L04, PLAYER_STAND_L05,
		PLAYER_STAND_L_MAX,
		//Right Stand
		PLAYER_STAND_R01, PLAYER_STAND_R02, PLAYER_STAND_R03, PLAYER_STAND_R04, PLAYER_STAND_R05,
		PLAYER_STAND_R_MAX,
		//Left Move
		PLAYER_MOVE_L01, PLAYER_MOVE_L02, PLAYER_MOVE_L03, PLAYER_MOVE_L04, PLAYER_MOVE_L05, PLAYER_MOVE_L06, PLAYER_MOVE_L07, PLAYER_MOVE_L08, PLAYER_MOVE_L09, PLAYER_MOVE_L10, PLAYER_MOVE_L11, PLAYER_MOVE_L12,
		PLAYER_MOVE_L_MAX,
		//Right Move
		PLAYER_MOVE_R01, PLAYER_MOVE_R02, PLAYER_MOVE_R03, PLAYER_MOVE_R04, PLAYER_MOVE_R05, PLAYER_MOVE_R06, PLAYER_MOVE_R07, PLAYER_MOVE_R08, PLAYER_MOVE_R09, PLAYER_MOVE_R10, PLAYER_MOVE_R11, PLAYER_MOVE_R12,
		PLAYER_MOVE_R_MAX,
		//Left Attack1
		PLAYER_ATTACK1_L01, PLAYER_ATTACK1_L02, PLAYER_ATTACK1_L03, PLAYER_ATTACK1_L04,
		PLAYER_ATTACK1_L_MAX,
		//Right Attack1
		PLAYER_ATTACK1_R01, PLAYER_ATTACK1_R02, PLAYER_ATTACK1_R03, PLAYER_ATTACK1_R04,
		PLAYER_ATTACK1_R_MAX,
		//Left Attack2
		PLAYER_ATTACK2_L01, PLAYER_ATTACK2_L02, PLAYER_ATTACK2_L03, PLAYER_ATTACK2_L04,
		PLAYER_ATTACK2_L_MAX,
		//Right Attack2
		PLAYER_ATTACK2_R01, PLAYER_ATTACK2_R02, PLAYER_ATTACK2_R03, PLAYER_ATTACK2_R04,
		PLAYER_ATTACK2_R_MAX,
		//Left Attack3
		PLAYER_ATTACK3_L01, PLAYER_ATTACK3_L02, PLAYER_ATTACK3_L03, PLAYER_ATTACK3_L04, PLAYER_ATTACK3_L05, PLAYER_ATTACK3_L06,
		PLAYER_ATTACK3_L_MAX,
		//Right Attack3
		PLAYER_ATTACK3_R01, PLAYER_ATTACK3_R02, PLAYER_ATTACK3_R03, PLAYER_ATTACK3_R04, PLAYER_ATTACK3_R05, PLAYER_ATTACK3_R06,
		PLAYER_ATTACK3_R_MAX,
		//Hit Effect
		EFFECT_SPARK_01, EFFECT_SPARK_02, EFFECT_SPARK_03, EFFECT_SPARK_04,
		EFFECT_SPARK_MAX,
		//Hp Guage
		GUAGE_HP,
		//Player Shadow
		SHADOW,
		//non used
		SPRITE_MAX
	};

	class SpriteDib
	{
	public:
		typedef struct Sprite
		{
			BYTE* image;
			int width;
			int height;
			int pitch;

			int centerPointX;
			int centerPointY;
		};

		SpriteDib(int maxSprite, DWORD colorKey);
		virtual ~SpriteDib();
		void ReleaseSprite(int spriteIndex);
		bool LoadDibSprite(int spriteIndex, const WCHAR* fileName, int centerPointX, int centerPointY);
		void DrawSprite(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch, int drawLen = 100);
		void DrawSprite50(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch);
		void DrawSpriteRed(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch);
		void DrawImage(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch, int drawLen = 100);
	protected:
		int _maxSprite;
		Sprite* _sprites;
		DWORD _colorKey;
	};
	extern SpriteDib g_SpriteDibBuffer;
}


#endif // !__SPRITE_DIB_HEADER__
