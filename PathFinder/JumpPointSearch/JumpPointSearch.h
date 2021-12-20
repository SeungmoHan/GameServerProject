#pragma once
#ifndef __JUMP_POINT_SEARCH_HEADER__
#define __JUMP_POINT_SEARCH_HEADER__
#include <list>
#include "DrawStraightLine.h"

#define OUT
#define IN

typedef unsigned long DWORD;
class TileMap;

class JumpPointSearch
{
private:
	enum class Direction { LL, RR, UU, DD, LD, LU, RD, RU };
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
	void ClearPath(JumpPointSearch::PathNode*& pathNode)
	{
		JumpPointSearch::PathNode* temp = pathNode;
		while (pathNode != nullptr)
		{
			pathNode = pathNode->parent;
			delete temp;
			temp = pathNode;
		}
		pathNode = nullptr;
	}
	JumpPointSearch::PathNode* GetShortestPath(JumpPointSearch::PathNode* nodeList, TileMap& tileMap);
private:
	void JumpLeft(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpRight(TileMap& OUT tileMap, PathNode* currentNode,  int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpUp(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpDown(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpLeftUp(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpLeftDown(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpRightUp(TileMap& OUT tileMap, PathNode* currentNode,  int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	void JumpRightDown(TileMap& OUT tileMap, PathNode* currentNode,int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color);
	bool CheckLeft(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint,DWORD color);
	bool CheckRight(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color);
	bool CheckUp(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color);
	bool CheckDown(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color);
	DrawLine drawLiner;
};

extern int g_OpenListSize;
extern int g_OperationCounting;

#endif // !__JUMP_POINT_SEARCH_HEADER__
