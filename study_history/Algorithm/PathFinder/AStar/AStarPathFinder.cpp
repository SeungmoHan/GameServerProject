#include "AStarPathFinder.h"
#include "TileMap.h"
#include <algorithm>
#include <functional>
#include <set>
#include "profiler.h"

//__univ_developer_AStar_

int g_OperationCounting = 0;

int g_OpenListSize = 0;


PathFinder::PathNode* PathFinder::FindPath(TileMap& OUT tileMap, int IN beginXPoint, int IN endXPoint, int IN beginYPoint, int IN endYPoint)
{
	ProFiler TempFindPath("FindPath");
	tileMap.ResetOpenList();
	tileMap.ResetCloseList();
	std::list <PathNode*>  openList, closeList;
	std::list<PathNode*> GC;
	PathNode* beginNode = new PathNode();
	GC.push_back(beginNode);
	beginNode->x = beginXPoint;
	beginNode->y = beginYPoint;
	beginNode->GValue = 0;
	beginNode->HValue = std::abs((endXPoint - beginXPoint) + std::abs(endYPoint - beginYPoint));
	beginNode->FValue = beginNode->GValue + beginNode->HValue;
	beginNode->parent = nullptr;
	tileMap.SetOpenList(beginNode->x, beginNode->y, true);
	openList.push_back(beginNode);


	while (!openList.empty())
	{
		ProFiler whileLoop("while loop");
		auto minIter = std::min_element(openList.begin(), openList.end(), [](const PathNode* lhs, const PathNode* rhs) {return lhs->FValue < rhs->FValue; });
		PathNode* currentNode = *minIter;

		g_OpenListSize = openList.size();
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
			g_OpenListSize = openList.size();
			for (auto iter = GC.begin(); iter != GC.end(); ++iter)
				delete* iter;
			return ret;
		}
		openList.erase(minIter);
		tileMap.SetCloseList(currentNode->x, currentNode->y,true);
		tileMap.SetOpenList(currentNode->x, currentNode->y,false);
		do
		{
			//오른쪽
			ProFiler rightLoop("right loop");
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y)) break;
			if (tileMap.IsInCloseList(currentNode->x + 1, currentNode->y))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x + 1, currentNode->y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x + 1 && (*iter)->y == currentNode->y)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 10)
						{
							(*iter)->GValue = currentNode->GValue + 10;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
							(*iter)->parent = currentNode;
						}
						break;
					}
				}
			}
			if (openListHas) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->x++;
			newNode->GValue += 10;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);

		} while (0);

		do
		{
			//오른쪽 위
			ProFiler rightLoop("right up loop");
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1)) break;
			if (tileMap.IsInCloseList(currentNode->x + 1, currentNode->y - 1))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x + 1, currentNode->y - 1))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x + 1 && (*iter)->y == currentNode->y - 1)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 14)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 14;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}
			if (openListHas) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->x++;
			newNode->y--;
			newNode->GValue+=14;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);

		do
		{
			//위
			ProFiler rightLoop("up loop");
			if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1)) break;
			if (tileMap.IsInCloseList(currentNode->x, currentNode->y - 1))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x, currentNode->y - 1))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x && (*iter)->y == currentNode->y - 1)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 10)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 10;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}
			if (openListHas ) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->y--;
			newNode->GValue+=10;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);

		do
		{
			//왼쪽 위
			ProFiler rightLoop("left up loop");
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1)) break;
			if (tileMap.IsInCloseList(currentNode->x - 1, currentNode->y - 1))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x - 1, currentNode->y - 1))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x - 1 && (*iter)->y == currentNode->y - 1)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 14)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 14;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}
			if (openListHas) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->x--;
			newNode->y--;
			newNode->GValue+=14;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);

		do
		{
			//왼쪽
			ProFiler rightLoop("left loop");
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y)) break;
			if (tileMap.IsInCloseList(currentNode->x - 1, currentNode->y))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x - 1, currentNode->y))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x - 1 && (*iter)->y == currentNode->y)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 10)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 10;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}

			if (openListHas) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->x--;
			newNode->GValue+=10;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);

		do
		{
			//왼쪽 아래
			ProFiler rightLoop("left down loop");
			if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1)) break;
			if (tileMap.IsInCloseList(currentNode->x - 1, currentNode->y + 1))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x - 1, currentNode->y + 1))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x - 1 && (*iter)->y == currentNode->y + 1)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 14)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 14;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}

			if (openListHas ) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->x--;
			newNode->y++;
			newNode->GValue+=14;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);

		do
		{
			//아래
			ProFiler rightLoop("down loop");
			if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1)) break;
			if (tileMap.IsInCloseList(currentNode->x, currentNode->y + 1))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x, currentNode->y + 1))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x && (*iter)->y == currentNode->y + 1)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 10)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 10;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}
			if (openListHas) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->y++;
			newNode->GValue+=10;
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);

		do
		{
			//오른쪽 아래
			ProFiler rightLoop("right down loop");
			if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1)) break;
			if (tileMap.IsInCloseList(currentNode->x + 1, currentNode->y + 1))break;
			bool openListHas = false;
			if (tileMap.IsInOpenList(currentNode->x + 1, currentNode->y + 1))
			{
				for (auto iter = openList.begin(); iter != openList.end(); ++iter)
				{
					if ((*iter)->x == currentNode->x + 1 && (*iter)->y == currentNode->y + 1)
					{
						openListHas = true;
						if ((*iter)->GValue > currentNode->GValue + 14)
						{
							(*iter)->parent = currentNode;
							(*iter)->GValue = currentNode->GValue + 14;
							(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
						}
						break;
					}
				}
			}
			if (openListHas ) break;
			PathNode* newNode = new PathNode(*currentNode);
			newNode->y++;
			newNode->x++;
			newNode->GValue+=14;
			
			newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
			newNode->FValue = newNode->GValue + newNode->HValue;
			newNode->parent = currentNode;
			openList.push_back(newNode);
			tileMap.SetOpenList(newNode->x, newNode->y, true);
			GC.push_back(newNode);
		} while (0);
	}

	for (auto iter = GC.begin(); iter != GC.end(); ++iter)
		delete* iter;
	return nullptr;
}


//위에함수는 ... 한번에 처리해서 뿅하고 path를 주는방식이라... 한큐씩 받으려면 이걸 써야됨.
bool PathFinder::GetNextPoint(TileMap& OUT tileMap, PathNode*& OUT retNode, int IN currentXPoint, int IN endXPoint, int IN currentYPoint, int IN endYPoint)
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

	auto minIter = std::min_element(openList.begin(), openList.end(), [](const PathNode* lhs, const PathNode* rhs) {return lhs->FValue < rhs->FValue;});
	if (openList.size() == 0)
	{
		GC.clear();
		openList.clear();
		beginFlag = false;
		beginNode = nullptr;
		currentNode = nullptr;
		alreadyStart = false;
		return true;
	}
	currentNode = *minIter;
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
	//printf("openList.size : %d\t", openList.size());
	g_OpenListSize = openList.size();
	openList.erase(minIter);
	//printf("openList.size : %d\n", openList.size());
	tileMap.SetCloseList(currentNode->x, currentNode->y, true);
	tileMap.SetOpenList(currentNode->x, currentNode->y, false);

	do
	{
		//오른쪽
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y)) break;
		if (tileMap.hasBeenBefore(currentNode->x + 1, currentNode->y))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x + 1, currentNode->y))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x + 1 && (*iter)->y == currentNode->y)
				{
					if ((*iter)->GValue > currentNode->GValue + 10)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 10;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					openListHas = true;
					break;
				}
			}
		}

		if (openListHas) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->x++;
		newNode->GValue += 10;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//오른쪽 위
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y - 1)) break;
		if (tileMap.hasBeenBefore(currentNode->x + 1, currentNode->y - 1))break;

		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x + 1, currentNode->y - 1))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x + 1 && (*iter)->y == currentNode->y - 1)
				{
					if ((*iter)->GValue > currentNode->GValue + 14)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 14;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					openListHas = true;
					break;
				}
			}
		}
		if (openListHas) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->x++;
		newNode->y--;
		newNode->GValue += 14;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//위
		if (tileMap.IsBlocked(currentNode->x, currentNode->y - 1)) break;
		if (tileMap.hasBeenBefore(currentNode->x, currentNode->y - 1))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x, currentNode->y - 1))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x && (*iter)->y == currentNode->y - 1)
				{
					openListHas = true;
					if ((*iter)->GValue > currentNode->GValue + 10)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 10;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					break;
				}
			}
		}

		if (openListHas ) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->y--;
		newNode->GValue += 10;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//왼쪽 위
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y - 1)) break;
		if (tileMap.hasBeenBefore(currentNode->x - 1, currentNode->y - 1))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x - 1, currentNode->y - 1))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x - 1 && (*iter)->y == currentNode->y - 1)
				{
					openListHas = true;
					if ((*iter)->GValue > currentNode->GValue + 14)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 14;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					break;
				}
			}
		}
		if (openListHas) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->x--;
		newNode->y--;
		newNode->GValue += 14;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//왼쪽
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y)) break;
		if (tileMap.hasBeenBefore(currentNode->x - 1, currentNode->y))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x - 1, currentNode->y))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x - 1 && (*iter)->y == currentNode->y)
				{
					openListHas = true;
					if ((*iter)->GValue > currentNode->GValue + 10)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 10;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					break;
				}
			}
		}
		if (openListHas) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->x--;
		newNode->GValue += 10;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//왼쪽 아래
		if (tileMap.IsBlocked(currentNode->x - 1, currentNode->y + 1)) break;
		if (tileMap.hasBeenBefore(currentNode->x - 1, currentNode->y + 1))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x - 1, currentNode->y + 1))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x - 1 && (*iter)->y == currentNode->y + 1)
				{
					openListHas = true;
					if ((*iter)->GValue > currentNode->GValue + 14)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 14;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					break;
				}
			}
		}
		if (openListHas ) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->x--;
		newNode->y++;
		newNode->GValue += 14;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//아래
		if (tileMap.IsBlocked(currentNode->x, currentNode->y + 1)) { break; }
		if (tileMap.hasBeenBefore(currentNode->x, currentNode->y + 1))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x, currentNode->y + 1))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x && (*iter)->y == currentNode->y + 1)
				{
					openListHas = true;
					if ((*iter)->GValue > currentNode->GValue + 10)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 10;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					break;
				}
			}
		}
		if (openListHas) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->y++;
		newNode->GValue += 10;
		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	do
	{
		//오른쪽 아래
		if (tileMap.IsBlocked(currentNode->x + 1, currentNode->y + 1)) break;
		if (tileMap.hasBeenBefore(currentNode->x + 1, currentNode->y + 1))break;
		bool openListHas = false;
		if (tileMap.IsInOpenList(currentNode->x + 1, currentNode->y + 1))
		{
			for (auto iter = openList.begin(); iter != openList.end(); ++iter)
			{
				if ((*iter)->x == currentNode->x + 1 && (*iter)->y == currentNode->y + 1)
				{
					openListHas = true;
					if ((*iter)->GValue > currentNode->GValue + 14)
					{
						(*iter)->parent = currentNode;
						(*iter)->GValue = currentNode->GValue + 14;
						(*iter)->FValue = (*iter)->GValue + (*iter)->HValue;
					}
					break;
				}
			}
		}

		if (openListHas) break;
		PathNode* newNode = new PathNode(*currentNode);
		newNode->y++;
		newNode->x++;
		newNode->GValue += 14;

		newNode->HValue = std::abs(endXPoint - newNode->x) * 10 + std::abs(endYPoint - newNode->y) * 10;
		newNode->FValue = newNode->GValue + newNode->HValue;
		newNode->parent = currentNode;
		openList.push_back(newNode);
		tileMap.SetOpenList(newNode->x, newNode->y, true);
		GC.push_back(newNode);
	} while (0);

	retNode = currentNode;
	return false;
}


