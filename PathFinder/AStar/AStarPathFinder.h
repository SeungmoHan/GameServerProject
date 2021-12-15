#pragma once
#ifndef __PATHFINDER_HEADER__
#define __PATHFINDER_HEADER__
#include <list>
#define OUT
#define IN
class TileMap;

class PathFinder
{
public:
	struct PathNode
	{
		int x;
		int y;
		int GValue;
		int HValue;
		int FValue;
		PathNode* parent;
	};
	
	// ��ť�� ������ ���ؼ�... ���� ������
	PathNode* beginNode, * currentNode;
	bool beginFlag;
	bool alreadyStart;
	std::list<PathNode*> openList;
	std::list<PathNode*> GC;

	std::list<PathNode*> GetNeibor(PathNode* currentNode,TileMap& tileMap);

	PathNode* FindPath(TileMap& OUT tileMap, int IN beginXPoint, int IN endXPoint, int IN beginYPoint, int IN endYPoint);
	//return true�̸� OUT ret�� ������ ����� �����Ͱ� ��������.
	bool GetNextPoint(TileMap& OUT tileMap, PathNode*& OUT ret, int IN currentXPoint, int IN endXPoint, int IN currentYPoint, int IN endYPoint);
	bool StartPathFinding()
	{
		if (alreadyStart) return false;
		beginFlag = true;
		alreadyStart = true;
		return true;
	}
	bool IsFindingPath()
	{
		return alreadyStart;
	}
};
extern int g_OpenListSize;
extern int g_OperationCounting ;

#endif // !__ASTAR_HEADER__
