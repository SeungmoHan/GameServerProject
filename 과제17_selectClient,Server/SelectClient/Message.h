#pragma once
#ifndef __MESSAGE_HEADER__
#define __MESSAGE_HEADER__

#define MESSAGE_ID 0
#define MESSAGE_CREATE_STAR 1
#define MESSAGE_DELETE_STAR 2
#define MESSAGE_MOVE 3
struct Header
{
	int type;
};
struct MsgID
{
	int id;
private:
	int not_used[2];
};
struct MsgCreateStar
{
	int id;
	int x;
	int y;
};
struct MsgDeleteStar
{
	int id;
private:
	int not_used[2];
};
struct MsgMove
{
	int id;
	int x;
	int y;
};

#endif // !__MESSAGE_HEADER__
