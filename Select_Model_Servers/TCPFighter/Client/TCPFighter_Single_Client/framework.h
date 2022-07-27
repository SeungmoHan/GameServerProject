// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

enum class e_OBJECT_TYPE
{
	TYPE_PLAYER,
	TYPE_EFFECT
};



#define ACTION_MOVE_LL 0
#define ACTION_MOVE_LU 1
#define ACTION_MOVE_UU 2
#define ACTION_MOVE_RU 3
#define ACTION_MOVE_RR 4
#define ACTION_MOVE_RD 5
#define ACTION_MOVE_DD 6
#define ACTION_MOVE_LD 7
#define ACTION_ATTACK1 8
#define ACTION_ATTACK2 9
#define ACTION_ATTACK3 10
