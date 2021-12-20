#include "JumpPointSearch.h"
#include "TileMap.h"
#include <algorithm>
#include <functional>
#include <set>
#include "profiler.h"

int g_OperationCounting = 0;

int g_OpenListSize = 0;
int i = 0;
JumpPointSearch::PathNode* JumpPointSearch::GetShortestPath(JumpPointSearch::PathNode* nodeList, TileMap& tileMap)
{
	if (nodeList == nullptr) return nullptr;

	JumpPointSearch::PathNode* shortestPath = new JumpPointSearch::PathNode(*nodeList);
	shortestPath->parent = nullptr;
	JumpPointSearch::PathNode* tempNode = shortestPath;
	JumpPointSearch::PathNode* nextNode = nodeList->parent;
	JumpPointSearch::PathNode* lastHaveNode = nodeList->parent;

	while (nextNode != nullptr)
	{
		bool isConnected = true;
		DrawLine::Node* line = drawLiner.GetStraightLine(tempNode->x, nextNode->x, tempNode->y, nextNode->y);
		while (line != nullptr)
		{
			if (tileMap.IsBlocked(line->x, line->y))
			{
				isConnected = false;
				break;
			}
			line = line->next;
		}
		if (isConnected)
		{
			lastHaveNode = nextNode;
			if (nextNode->parent == nullptr)
			{
				JumpPointSearch::PathNode* newNode = new JumpPointSearch::PathNode();
				newNode->x = nextNode->x;
				newNode->y = nextNode->y;
				newNode->parent = nullptr;
				tempNode->parent = newNode;
				tempNode = newNode;
				break;
			}
			nextNode = nextNode->parent;
			continue;
		}
		JumpPointSearch::PathNode* newNode = new JumpPointSearch::PathNode();
		newNode->x = lastHaveNode->x;
		newNode->y = lastHaveNode->y;
		newNode->parent = nullptr;
		tempNode->parent = newNode;
		tempNode = newNode;
	}
	return shortestPath;
}
JumpPointSearch::PathNode* JumpPointSearch::FindPath(TileMap& OUT tileMap, int IN beginXPoint, int IN endXPoint, int IN beginYPoint, int IN endYPoint)
{
	ProFiler FindPath("JumpPointSearch::FindPath()");
	tileMap.ResetBlockColor();
	tileMap.ResetOpenList();
	tileMap.ResetCloseList();
	std::list <PathNode*>  openList, closeList;
	std::list<PathNode*> GC;
	PathNode* beginNode = new PathNode();
	GC.push_back(beginNode);
	beginNode->x = beginXPoint;
	beginNode->y = beginYPoint;
	beginNode->GValue = 0;
	beginNode->HValue = std::abs((endXPoint - beginXPoint) * 10 + std::abs(endYPoint - beginYPoint) * 10);
	beginNode->FValue = beginNode->GValue + beginNode->HValue;
	beginNode->parent = nullptr;
	tileMap.SetOpenList(beginNode->x, beginNode->y, true);
	openList.push_back(beginNode);


	while (!openList.empty())
	{
		ProFiler whileLoop("while loop");
		auto minIter = std::min_element(openList.begin(), openList.end(), [](const PathNode* lhs, const PathNode* rhs) {return lhs->FValue < rhs->FValue; });
		PathNode* currentNode = *minIter;

		openList.erase(minIter);
		g_OpenListSize = openList.size();

		tileMap.SetOpenList(currentNode->x, currentNode->y, false);
		tileMap.SetCloseList(currentNode->x, currentNode->y, true);
		if (currentNode->x == endXPoint && currentNode->y == endYPoint)
		{
			PathNode* ret = nullptr;
			PathNode* prev = nullptr;
			while (currentNode != nullptr)
			{
				ret = new PathNode(*currentNode);
				ret->parent = prev;
				prev = ret;
				currentNode = currentNode->parent;
			}
			for (auto iter = GC.begin(); iter != GC.end(); ++iter)
				delete* iter;
			return ret;
		}
		COLORREF color = RGB(rand() % 255, rand() % 255, rand() % 255);
		
		PathNode tempNode = *currentNode;
		tempNode.parent = currentNode;

		if (currentNode->parent == nullptr)
		{
			//8방향 다해줘야됨.
			JumpLeft(tileMap, currentNode, endXPoint, endYPoint,openList,color);
			JumpRight(tileMap, currentNode, endXPoint, endYPoint,openList, color);
			JumpUp(tileMap, currentNode, endXPoint, endYPoint,openList, color);
			JumpDown(tileMap, currentNode, endXPoint, endYPoint,openList, color);
			JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint,openList, color);
			JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint,openList, color);
			JumpRightUp(tileMap, currentNode,  endXPoint, endYPoint,openList, color);
			JumpRightDown(tileMap, currentNode, endXPoint,endYPoint,openList, color);
			continue;
		}
		int dx = currentNode->x - currentNode->parent->x;
		int dy = currentNode->y - currentNode->parent->y;
		// parent가 왼쪽에 있고 높이는 같을때  = 오른쪽으로 진행
		if (dx > 0 && dy == 0) 
		{
			JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			//오른쪽 위 코너라면
			if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
				JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			//오른쪽 아래 코너라면
			if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
				JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		// parent가 오른쪽에 있고 높이는 같을때  = 왼쪽으로 진행
		else if (dx < 0 && dy == 0)
		{
			JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			// 왼쪽 위 코너라면
			if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
				JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			// 왼쪽 아래 코너라면
			if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
				JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		//parent가 아래에 있고 좌우는 같을때  = 위로 진행
		else if (dy < 0 && dx == 0)
		{
			JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			// 오른쪽 위 코너일때
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
				JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			// 왼쪽 위 코너일때
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
				JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		//parent가 위에있고 좌우는 같을때 = 아래로 진행
		else if (dy > 0 && dx == 0)
		{
			JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			//오른쪽 아래 코너일때
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
				JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			//왼쪽 아래 코너일때
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
				JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		// parent가 오른쪽에 있고 parent가 아래에 있을때 = 왼쪽 위로 진행
		else if (dx < 0 && dy < 0)
		{
			JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);

			// 왼쪽 아래 코너일때
			if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
				JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			// 오른쪽 위 코너일때
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
				JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		// parent가 왼쪽에 있고 parent가 위에 있을때 
		else if (dx > 0 && dy > 0)
		{
			JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);

			// 오른쪽 위 코너일때
			if(tileMap.IsBlocked(currentNode->x,currentNode->y -1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y -1))
				JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			// 왼쪽 아래 코너일때
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
				JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		// parent가 왼쪽에 있고 parent가 아래에 있을때
		else if (dx > 0 && dy < 0)
		{
			JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);

			// 왼쪽 위 코너일때
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
				JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
				JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
		//parent 가 오른쪽에있고 parent가 위에 있을때
		else if (dx < 0 && dy > 0)
		{
			JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);

			if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
				JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
				JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		}
	}

	for (auto iter = GC.begin(); iter != GC.end(); ++iter)
		delete* iter;
	return nullptr;
}

void JumpPointSearch::JumpLeft(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	while (true)
	{
		tempNode.x--;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 10;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;

		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);

		if (tempNode.x == endXPoint && tempNode.y == endYPoint)
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}

		if ((tileMap.IsBlocked(tempNode.x, tempNode.y + 1) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y + 1)) ||
			(tileMap.IsBlocked(tempNode.x, tempNode.y - 1) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y - 1)))
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}
	}
}

void JumpPointSearch::JumpRight(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	while (true)
	{
		tempNode.x++;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 10;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;

		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		if (tempNode.x == endXPoint && tempNode.y == endYPoint)
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}
		if ((tileMap.IsBlocked(tempNode.x, tempNode.y + 1) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y + 1)) ||
			(tileMap.IsBlocked(tempNode.x, tempNode.y - 1) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y - 1)))
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}

	}
}

void JumpPointSearch::JumpUp(TileMap& OUT tileMap, PathNode* currentNode,int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	while (true)
	{
		tempNode.y--;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 10;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;

		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		if (tempNode.x == endXPoint && tempNode.y == endYPoint)
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}
		if ((tileMap.IsBlocked(tempNode.x+1, tempNode.y ) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y - 1)) ||
			(tileMap.IsBlocked(tempNode.x - 1, tempNode.y ) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y - 1)))
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}
	}
}

void JumpPointSearch::JumpDown(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	while (true)
	{
		tempNode.y++;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 10;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;

		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		if (tempNode.x == endXPoint && tempNode.y == endYPoint)
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}
		if ((tileMap.IsBlocked(tempNode.x + 1, tempNode.y) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y + 1)) ||
			(tileMap.IsBlocked(tempNode.x - 1, tempNode.y) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y + 1)))
		{
			if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
					{
						if ((*iter)->GValue > tempNode.GValue)
						{
							(*iter)->parent = tempNode.parent;
							(*iter)->GValue = tempNode.GValue;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							break;
						}
					}
				}
				break;
			}
			if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))break;
			PathNode* newNode = new PathNode(tempNode);
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			break;
		}
	}
}

void JumpPointSearch::JumpLeftUp(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	bool flag = false;
	while (true)
	{
		tempNode.x--;
		tempNode.y--;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 14;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;
		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		if (endXPoint == tempNode.x && endYPoint == tempNode.y)
		{
			flag = true;
			break;
		}
		if ((tileMap.IsBlocked(tempNode.x, tempNode.y + 1) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y + 1)) ||
			(tileMap.IsBlocked(tempNode.x + 1, tempNode.y) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y - 1)))
		{
			flag = true;
			break;
		}
		flag = CheckLeft(tileMap, &tempNode, endXPoint, endYPoint, color);
		flag = CheckUp(tileMap, &tempNode, endXPoint, endYPoint, color) || flag;
		if (flag)break;
	}
	if (flag)
	{
		if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
				{
					if ((*iter)->GValue > tempNode.GValue)
					{
						(*iter)->parent = tempNode.parent;
						(*iter)->GValue = tempNode.GValue;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						break;
					}
				}
			}
			return;
		}
		if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))return;
		PathNode* newNode = new PathNode(tempNode);
		tileMap.SetOpenList(newNode->x, newNode->y,true);
		openList.push_back(newNode);
	}
}

void JumpPointSearch::JumpLeftDown(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	bool flag = false;
	while (true)
	{
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		tempNode.x--;
		tempNode.y++;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 14;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;
		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		if (endXPoint == tempNode.x && endYPoint == tempNode.y)
		{
			flag = true;
			break;
		}
		if ((tileMap.IsBlocked(tempNode.x, tempNode.y - 1) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y - 1)) ||
			(tileMap.IsBlocked(tempNode.x + 1, tempNode.y) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y + 1)))
		{
			flag = true;
			break;
		}
		flag = CheckLeft(tileMap, &tempNode, endXPoint, endYPoint, color);
		flag = CheckDown(tileMap, &tempNode, endXPoint, endYPoint, color) || flag;
		if (flag) break;
	}
	if (flag)
	{
		if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
				{
					if ((*iter)->GValue > tempNode.GValue)
					{
						(*iter)->parent = tempNode.parent;
						(*iter)->GValue = tempNode.GValue;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						break;
					}
				}
			}
			return;
		}
		if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))return;
		PathNode* newNode = new PathNode(tempNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		openList.push_back(newNode);
	}
}

void JumpPointSearch::JumpRightUp(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	bool flag = false;
	while (true)
	{
		tempNode.x++;
		tempNode.y--;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 14;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;
		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		if (endXPoint == tempNode.x && endYPoint == tempNode.y)
		{
			flag = true;
			break;
		}
		if (tileMap.IsBlocked(tempNode.x, tempNode.y + 1) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y + 1) ||
			tileMap.IsBlocked(tempNode.x - 1, tempNode.y) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y - 1))
		{
			flag = true;
			break;
		}
		flag = CheckRight(tileMap, &tempNode, endXPoint, endYPoint, color);
		flag = CheckUp(tileMap, &tempNode, endXPoint, endYPoint, color) || flag;
		if (flag) break;
	}
	if (flag)
	{
		if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
				{
					if ((*iter)->GValue > tempNode.GValue)
					{
						(*iter)->parent = tempNode.parent;
						(*iter)->GValue = tempNode.GValue;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						break;
					}
				}
			}
			return;
		}
		if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))return;
		PathNode* newNode = new PathNode(tempNode);
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y,true);
	}
}

void JumpPointSearch::JumpRightDown(TileMap& OUT tileMap, PathNode* currentNode, int endXPoint, int endYPoint, std::list<PathNode*>& openList, DWORD color)
{
	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
	bool flag = false;
	while (true)
	{
		tempNode.x++;
		tempNode.y++;
		tempNode.HValue = (std::abs((endXPoint - tempNode.x) * 10) + (std::abs(endYPoint - tempNode.y) * 10));
		tempNode.GValue += 14;
		tempNode.FValue = tempNode.GValue + tempNode.HValue;
		if (tileMap.IsBlocked(tempNode.x, tempNode.y)) break;
		tileMap.SetBlockColor(tempNode.x, tempNode.y, color);
		if (endXPoint == tempNode.x && endYPoint == tempNode.y)
		{
			flag = true;
			break;
		}
		if (tileMap.IsBlocked(tempNode.x, tempNode.y - 1) && !tileMap.IsBlocked(tempNode.x + 1, tempNode.y - 1) ||
			tileMap.IsBlocked(tempNode.x - 1, tempNode.y) && !tileMap.IsBlocked(tempNode.x - 1, tempNode.y + 1))
		{
			flag = true;
			break;
		}
		flag = CheckRight(tileMap, &tempNode, endXPoint, endYPoint, color);
		flag = CheckDown(tileMap, &tempNode, endXPoint, endYPoint, color) || flag;
		if (flag) break;
	}
	if (flag)
	{
		if (tileMap.IsInOpenList(tempNode.x, tempNode.y))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if (tempNode.x == (*iter)->x && tempNode.y == (*iter)->y)
				{
					if ((*iter)->GValue > tempNode.GValue)
					{
						(*iter)->parent = tempNode.parent;
						(*iter)->GValue = tempNode.GValue;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						break;
					}
				}
			}
			return;
		}
		if (tileMap.hasBeenBefore(tempNode.x, tempNode.y))return;
		PathNode* newNode = new PathNode(tempNode);
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y,true);
	}
}

bool JumpPointSearch::CheckLeft(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color)
{
	int xForDiagonal = currentNode->x;
	int yForDiagonal = currentNode->y;
	while (true)
	{
		xForDiagonal--;
		if (tileMap.IsBlocked(xForDiagonal, yForDiagonal)) return false;
		if (endXPoint == xForDiagonal && endYPoint == yForDiagonal) return true;
		if (tileMap.IsInOpenList(xForDiagonal, yForDiagonal)) return false;
		if ((tileMap.IsBlocked(xForDiagonal, yForDiagonal + 1) && !tileMap.IsBlocked(xForDiagonal - 1, yForDiagonal + 1)) ||
			tileMap.IsBlocked(xForDiagonal, yForDiagonal - 1) && !tileMap.IsBlocked(xForDiagonal - 1, yForDiagonal - 1))
			return true;
		tileMap.SetBlockColor(xForDiagonal, yForDiagonal, color);
	}
	return false;
}

bool JumpPointSearch::CheckRight(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color)
{
	int xForDiagonal = currentNode->x;
	int yForDiagonal = currentNode->y;
	while (true)
	{
		xForDiagonal++;
		if (tileMap.IsBlocked(xForDiagonal, yForDiagonal)) return false;
		if (endXPoint == xForDiagonal && endYPoint == yForDiagonal) return true;
		if (tileMap.IsInOpenList(xForDiagonal, yForDiagonal)) return false;
		if ((tileMap.IsBlocked(xForDiagonal, yForDiagonal + 1) && !tileMap.IsBlocked(xForDiagonal + 1, yForDiagonal + 1)) ||
			tileMap.IsBlocked(xForDiagonal, yForDiagonal - 1) && !tileMap.IsBlocked(xForDiagonal + 1, yForDiagonal - 1))
			return true;
		tileMap.SetBlockColor(xForDiagonal, yForDiagonal, color);
	}
	return false;
}

bool JumpPointSearch::CheckUp(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color)
{
	int xForDiagonal = currentNode->x;
	int yForDiagonal = currentNode->y;
	while (true)
	{
		yForDiagonal--;
		if (tileMap.IsBlocked(xForDiagonal, yForDiagonal)) return false;
		if (endXPoint == xForDiagonal && endYPoint == yForDiagonal) return true;
		if (tileMap.IsInOpenList(xForDiagonal, yForDiagonal)) return false;
		if ((tileMap.IsBlocked(xForDiagonal + 1, yForDiagonal) && !tileMap.IsBlocked(xForDiagonal + 1, yForDiagonal - 1)) ||
			tileMap.IsBlocked(xForDiagonal - 1, yForDiagonal) && !tileMap.IsBlocked(xForDiagonal - 1, yForDiagonal - 1))
			return true;
		tileMap.SetBlockColor(xForDiagonal, yForDiagonal, color);
	}
	return false;
}

bool JumpPointSearch::CheckDown(TileMap& tileMap, PathNode* currentNode, int endXPoint, int endYPoint, DWORD color)
{
	int xForDiagonal = currentNode->x;
	int yForDiagonal = currentNode->y;
	while (true)
	{
		yForDiagonal++;
		if (tileMap.IsBlocked(xForDiagonal, yForDiagonal)) return false;
		if (endXPoint == xForDiagonal && endYPoint == yForDiagonal) return true;
		if (tileMap.IsInOpenList(xForDiagonal, yForDiagonal)) return false;
		if ((tileMap.IsBlocked(xForDiagonal + 1, yForDiagonal) && !tileMap.IsBlocked(xForDiagonal + 1, yForDiagonal + 1)) ||
			tileMap.IsBlocked(xForDiagonal - 1, yForDiagonal) && !tileMap.IsBlocked(xForDiagonal - 1, yForDiagonal + 1))
			return true;
		tileMap.SetBlockColor(xForDiagonal, yForDiagonal, color);
	}
	return false;
}

//위에함수는 ... 한번에 처리해서 뿅하고 path를 주는방식이라... 한큐씩 받으려면 이걸 써야됨.
bool JumpPointSearch::GetNextPoint(TileMap& OUT tileMap, PathNode*& OUT retNode, int IN currentXPoint, int IN endXPoint, int IN currentYPoint, int IN endYPoint)
{
	if (!beginFlag) return false;
	if (beginNode == nullptr)
	{
		openList.clear();
		GC.clear();
		tileMap.ResetCloseList();
		tileMap.ResetOpenList();
		beginNode = new PathNode();
		GC.push_back(beginNode);
		beginNode->x = currentXPoint;
		beginNode->y = currentYPoint;
		beginNode->GValue = 0;
		beginNode->HValue = std::abs((endXPoint - currentXPoint) + std::abs(endYPoint - currentYPoint));
		beginNode->FValue = beginNode->GValue + beginNode->HValue;
		beginNode->parent = nullptr;
		openList.push_back(beginNode);

		return false;
	}

	if (openList.empty())
	{
		PathNode* ret = nullptr;
		PathNode* prev = nullptr;
		while (currentNode != nullptr)
		{
			ret = new PathNode(*currentNode);
			ret->parent = prev;
			prev = ret;
			currentNode = currentNode->parent;
		}
		for (auto iter = GC.begin(); iter != GC.end(); ++iter)
			delete* iter;
		g_OpenListSize = openList.size();
		GC.clear();
		openList.clear();
		beginFlag = false;
		beginNode = nullptr;
		currentNode = nullptr;
		alreadyStart = false;
		retNode = nullptr;
		return true;
	}

	auto minIter = std::min_element(openList.begin(), openList.end(), [](const PathNode* lhs, const PathNode* rhs) {return lhs->FValue < rhs->FValue; });
	PathNode* currentNode = *minIter;

	openList.erase(minIter);
	g_OpenListSize = openList.size();

	tileMap.SetOpenList(currentNode->x, currentNode->y, false);
	tileMap.SetCloseList(currentNode->x, currentNode->y, true);
	if (currentNode->x == endXPoint && currentNode->y == endYPoint)
	{
		PathNode* ret = nullptr;
		PathNode* prev = nullptr;
		while (currentNode != nullptr)
		{
			ret = new PathNode(*currentNode);
			ret->parent = prev;
			prev = ret;
			currentNode = currentNode->parent;
		}
		for (auto iter = GC.begin(); iter != GC.end(); ++iter)
			delete* iter;
		g_OpenListSize = openList.size();
		GC.clear();
		openList.clear();
		beginFlag = false;
		beginNode = nullptr;
		currentNode = nullptr;
		alreadyStart = false;
		retNode = ret;
		return true;
	}
	COLORREF color = RGB(rand() % 255, rand() % 255, rand() % 255);

	PathNode tempNode = *currentNode;
	tempNode.parent = currentNode;

	if (currentNode->parent == nullptr)
	{
		//8방향 다해줘야됨.
		JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		return false;
	}
	int dx = currentNode->x - currentNode->parent->x;
	int dy = currentNode->y - currentNode->parent->y;
	// parent가 왼쪽에 있고 높이는 같을때  = 오른쪽으로 진행
	if (dx > 0 && dy == 0)
	{
		JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		//오른쪽 위 코너라면
		if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
			JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		//오른쪽 아래 코너라면
		if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
			JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	// parent가 오른쪽에 있고 높이는 같을때  = 왼쪽으로 진행
	else if (dx < 0 && dy == 0)
	{
		JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		// 왼쪽 위 코너라면
		if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
			JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		// 왼쪽 아래 코너라면
		if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
			JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	//parent가 아래에 있고 좌우는 같을때  = 위로 진행
	else if (dy < 0 && dx == 0)
	{
		JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		// 오른쪽 위 코너일때
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
			JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		// 왼쪽 위 코너일때
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
			JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	//parent가 위에있고 좌우는 같을때 = 아래로 진행
	else if (dy > 0 && dx == 0)
	{
		JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		//오른쪽 아래 코너일때
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
			JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		//왼쪽 아래 코너일때
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
			JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	// parent가 오른쪽에 있고 parent가 아래에 있을때 = 왼쪽 위로 진행
	else if (dx < 0 && dy < 0)
	{
		JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);

		// 왼쪽 아래 코너일때
		if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
			JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		// 오른쪽 위 코너일때
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
			JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	// parent가 왼쪽에 있고 parent가 위에 있을때 
	else if (dx > 0 && dy > 0)
	{
		JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);

		// 오른쪽 위 코너일때
		if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1))
			JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		// 왼쪽 아래 코너일때
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1))
			JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	// parent가 왼쪽에 있고 parent가 아래에 있을때
	else if (dx > 0 && dy < 0)
	{
		JumpRight(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpRightUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);

		// 왼쪽 위 코너일때
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
			JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
			JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}
	//parent 가 오른쪽에있고 parent가 위에 있을때
	else if (dx < 0 && dy > 0)
	{
		JumpDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpLeft(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		JumpLeftDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);

		if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1) && !tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1))
			JumpLeftUp(tileMap, currentNode, endXPoint, endYPoint, openList, color);
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y) && !tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1))
			JumpRightDown(tileMap, currentNode, endXPoint, endYPoint, openList, color);
	}

	retNode = currentNode;
	return false;
}

