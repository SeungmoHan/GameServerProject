
/*---------------------------------------------------------------

��Ŷ������ ����.


�ڽ��� ĳ���Ϳ� ���� ��Ŷ�� �������� ���� ��, ��� �ڽ��� ����
�׼��� ���԰� ���ÿ� ��Ŷ�� ������ �����ֵ��� �Ѵ�.

- �̵� Ű �Է� �� �̵������� ���԰� ���ÿ� �̵� ��Ŷ�� �������� �Ѵ�.
- ����Ű �Է� �� ���� ������ ���ϸ鼭 ��Ŷ�� ������.
- �浹 ó�� �� �������� ���� ������ �������� ó�� �� �뺸�ϰ� �ȴ�.


---------------------------------------------------------------*/

#ifndef __PACKET_DEFINE__
#define __PACKET_DEFINE__

using BYTE = unsigned char;
#define UM_NETWORK (WM_USER +1)



//---------------------------------------------------------------
// ��Ŷ���.
//
//---------------------------------------------------------------
/*
	BYTE	byCode;			// ��Ŷ�ڵ� 0x89 ����.
	BYTE	bySize;			// ��Ŷ ������.
	BYTE	byType;			// ��ŶŸ��.
*/
#pragma pack(1)

struct PacketHeader
{
	BYTE code;
	BYTE payloadSize;
	BYTE packetType;
};

#define	dfPACKET_SC_CREATE_MY_CHARACTER			0
//---------------------------------------------------------------
// Ŭ���̾�Ʈ �ڽ��� ĳ���� �Ҵ�		Server -> Client
//
// ������ ���ӽ� ���ʷ� �ްԵǴ� ��Ŷ���� �ڽ��� �Ҵ���� ID ��
// �ڽ��� ���� ��ġ, HP �� �ް� �ȴ�. (ó���� �ѹ� �ް� ��)
// 
// �� ��Ŷ�� ������ �ڽ��� ID,X,Y,HP �� �����ϰ� ĳ���͸� �������Ѿ� �Ѵ�.
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------
struct SC_PacketCreateMyCharacter
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
	BYTE HP;
};

#define	dfPACKET_SC_CREATE_OTHER_CHARACTER		1
//---------------------------------------------------------------
// �ٸ� Ŭ���̾�Ʈ�� ĳ���� ���� ��Ŷ		Server -> Client
//
// ó�� ������ ���ӽ� �̹� ���ӵǾ� �ִ� ĳ���͵��� ����
// �Ǵ� �����߿� ���ӵ� Ŭ���̾�Ʈ���� ������ ����.
//
//
//	4	-	ID
//	1	-	Direction	(LL / RR)
//	2	-	X
//	2	-	Y
//	1	-	HP
//
//---------------------------------------------------------------
struct SC_PacketCreateOtherCharacter
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
	BYTE HP;
};

#define	dfPACKET_SC_DELETE_CHARACTER			2
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ						Server -> Client
//
// ĳ������ �������� �Ǵ� ĳ���Ͱ� �׾����� ���۵�.
//
//	4	-	ID
//
//---------------------------------------------------------------
struct SC_PacketDeleteCharacter
{
	unsigned int playerID;
};


#define	dfPACKET_CS_MOVE_START					10
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Client -> Server
//
// �ڽ��� ĳ���� �̵����۽� �� ��Ŷ�� ������.
// �̵� �߿��� �� ��Ŷ�� ������ ������, Ű �Է��� ����Ǿ��� ��쿡��
// ������� �Ѵ�.
//
// (���� �̵��� ���� �̵� / ���� �̵��� ���� ���� �̵�... ���)
//
//	1	-	Direction	( ���� ������ �� 8���� ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct CS_PacketMoveStart
{
	BYTE direction;
	unsigned short x;
	unsigned short y;
};

#define dfPACKET_MOVE_DIR_LL					0
#define dfPACKET_MOVE_DIR_LU					1
#define dfPACKET_MOVE_DIR_UU					2
#define dfPACKET_MOVE_DIR_RU					3
#define dfPACKET_MOVE_DIR_RR					4
#define dfPACKET_MOVE_DIR_RD					5
#define dfPACKET_MOVE_DIR_DD					6
#define dfPACKET_MOVE_DIR_LD					7




#define	dfPACKET_SC_MOVE_START					11
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Server -> Client
//
// �ٸ� ������ ĳ���� �̵��� �� ��Ŷ�� �޴´�.
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�� �̵�ó���� ���ֵ��� �Ѵ�.
// 
// ��Ŷ ���� �� �ش� Ű�� ����ؼ� ���������� �����ϰ�
// �ش� �������� ��� �̵��� �ϰ� �־�߸� �Ѵ�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ �� 8���� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------
struct SC_PacketMoveStart
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
};



#define	dfPACKET_CS_MOVE_STOP					12
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Client -> Server
//
// �̵��� Ű���� �Է��� ��� �����Ǿ��� ��, �� ��Ŷ�� ������ �����ش�.
// �̵��� ���� ��ȯ�ÿ��� ��ž�� ������ �ʴ´�.
//
//	1	-	Direction	( ���� ������ �� ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct CS_PacketMoveStop
{
	BYTE direction;
	unsigned short x;
	unsigned short y;
};

#define	dfPACKET_SC_MOVE_STOP					13
//---------------------------------------------------------------
// ĳ���� �̵����� ��Ŷ						Server -> Client
//
// ID �� �ش��ϴ� ĳ���Ͱ� �̵��� ������̹Ƿ� 
// ĳ���͸� ã�Ƽ� �����, ��ǥ�� �Է����ְ� ���ߵ��� ó���Ѵ�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_PacketMoveStop
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
};


#define	dfPACKET_CS_ATTACK1						20
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y	
//
//---------------------------------------------------------------

struct CS_PacketAttack1
{
	BYTE direction;
	unsigned short x;
	unsigned short y;
};


#define	dfPACKET_SC_ATTACK1						21
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����1�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_PacketAttack1
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
};

#define	dfPACKET_CS_ATTACK2						22
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct CS_PacketAttack2
{
	BYTE direction;
	unsigned short x;
	unsigned short y;
};

#define	dfPACKET_SC_ATTACK2						23
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����2�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_PacketAttack2
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
};


#define	dfPACKET_CS_ATTACK3						24
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Client -> Server
//
// ���� Ű �Է½� �� ��Ŷ�� �������� ������.
// �浹 �� �������� ���� ����� �������� �˷� �� ���̴�.
//
// ���� ���� ���۽� �ѹ��� �������� ������� �Ѵ�.
//
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct CS_PacketAttack3
{
	BYTE direction;
	unsigned short x;
	unsigned short y;
};


#define	dfPACKET_SC_ATTACK3						25
//---------------------------------------------------------------
// ĳ���� ���� ��Ŷ							Server -> Client
//
// ��Ŷ ���Ž� �ش� ĳ���͸� ã�Ƽ� ����3�� �������� �׼��� �����ش�.
// ������ �ٸ� ��쿡�� �ش� �������� �ٲ� �� ���ش�.
//
//	4	-	ID
//	1	-	Direction	( ���� ������ ��. ��/�츸 ��� )
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_PacketAttack3
{
	unsigned int playerID;
	BYTE direction;
	unsigned short x;
	unsigned short y;
};



#define	dfPACKET_SC_DAMAGE						30
//---------------------------------------------------------------
// ĳ���� ������ ��Ŷ							Server -> Client
//
// ���ݿ� ���� ĳ������ ������ ����.
//
//	4	-	AttackID	( ������ ID )
//	4	-	DamageID	( ������ ID )
//	1	-	DamageHP	( ������ HP )
//
//---------------------------------------------------------------

struct SC_PacketDamage
{
	unsigned int attackPlayerID;
	unsigned int damagePlayerID;
	BYTE damageHP;
};


// ������...
#define	dfPACKET_CS_SYNC						250
//---------------------------------------------------------------
// ����ȭ�� ���� ��Ŷ					Client -> Server
//
//
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct CS_PacketSync
{
	unsigned short x;
	unsigned short y;
};

#define	dfPACKET_SC_SYNC						251
//---------------------------------------------------------------
// ����ȭ�� ���� ��Ŷ					Server -> Client
//
// �����κ��� ����ȭ ��Ŷ�� ������ �ش� ĳ���͸� ã�Ƽ�
// ĳ���� ��ǥ�� �������ش�.
//
//	4	-	ID
//	2	-	X
//	2	-	Y
//
//---------------------------------------------------------------

struct SC_PacketSync
{
	unsigned int playerID;
	unsigned short x;
	unsigned short y;
};
#pragma pack()

#endif

