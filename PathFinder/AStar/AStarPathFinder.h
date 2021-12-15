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
	
	// 한큐씩 돌리기 위해서... 만든 변수들
	PathNode* beginNode, * currentNode;
	bool beginFlag;
	bool alreadyStart;
	std::list<PathNode*> openList;
	std::list<PathNode*> GC;

	std::list<PathNode*> GetNeibor(PathNode* currentNode,TileMap& tileMap);

	PathNode* FindPath(TileMap& OUT tileMap, int IN beginXPoint, int IN endXPoint, int IN beginYPoint, int IN endYPoint);
	//return true이면 OUT ret에 마지막 노드의 포인터가 찍혀있음.
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
