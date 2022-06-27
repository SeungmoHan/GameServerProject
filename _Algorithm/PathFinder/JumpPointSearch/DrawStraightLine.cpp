#include "DrawStraightLine.h"
#include <algorithm>

//__univ_developer_draw_straight_line_

namespace univ_dev
{
	DrawLine::Node* DrawLine::DrawStraightLine(int beginXPoint, int endXPoint, int beginYPoint, int endYPoint)
	{
		if (!startFlag) return nullptr;
		int dx = endXPoint - beginXPoint;
		int dy = endYPoint - beginYPoint;
		bool xIsBigger = std::abs(dx) > std::abs(dy);
		int biggerNumber = xIsBigger ? std::abs(dx) : std::abs(dy);
		int lowerNumber = xIsBigger ? std::abs(dy) : std::abs(dx);
		if (dx != 0)
			dx /= std::abs(dx);
		if (dy != 0)
			dy /= std::abs(dy);
		int cumerativeNumber = 0;
		int currentXPoint = beginXPoint;
		int currentYPoint = beginYPoint;
		Node* currentNode = new Node();
		currentNode->next = nullptr;
		currentNode->x = currentXPoint;
		currentNode->y = currentYPoint;
		while (true)
		{
			if (currentXPoint == endXPoint && currentYPoint == endYPoint)
				return currentNode;
			if (xIsBigger)
			{
				currentXPoint += dx;

				cumerativeNumber += std::abs(lowerNumber);
				if ((cumerativeNumber - (biggerNumber / 2 + 1)) >= 0)
				{
					currentYPoint += dy;
					cumerativeNumber -= biggerNumber;
				}
			}
			else
			{
				currentYPoint += dy;

				cumerativeNumber += std::abs(lowerNumber);
				if ((cumerativeNumber - (biggerNumber / 2 + 1)) >= 0)
				{
					currentXPoint += dx;
					cumerativeNumber -= biggerNumber;
				}
			}

			Node* newNode = new Node();
			newNode->x = currentXPoint;
			newNode->y = currentYPoint;
			newNode->next = currentNode;
			currentNode = newNode;
		}
		return currentNode;
	}
	DrawLine::Node* DrawLine::GetStraightLine(int beginXPoint, int endXPoint, int beginYPoint, int endYPoint)
	{
		int dx = endXPoint - beginXPoint;
		int dy = endYPoint - beginYPoint;
		bool xIsBigger = std::abs(dx) > std::abs(dy);
		int biggerNumber = xIsBigger ? std::abs(dx) : std::abs(dy);
		int lowerNumber = xIsBigger ? std::abs(dy) : std::abs(dx);
		if (dx != 0)
			dx /= std::abs(dx);
		if (dy != 0)
			dy /= std::abs(dy);
		int cumerativeNumber = 0;
		int currentXPoint = beginXPoint;
		int currentYPoint = beginYPoint;
		Node* currentNode = new Node();
		currentNode->next = nullptr;
		currentNode->x = currentXPoint;
		currentNode->y = currentYPoint;
		while (true)
		{
			if (currentXPoint == endXPoint && currentYPoint == endYPoint)
				return currentNode;
			if (xIsBigger)
			{
				currentXPoint += dx;

				cumerativeNumber += std::abs(lowerNumber);
				if ((cumerativeNumber - (biggerNumber / 2 + 1)) >= 0)
				{
					currentYPoint += dy;
					cumerativeNumber -= biggerNumber;
				}
			}
			else
			{
				currentYPoint += dy;

				cumerativeNumber += std::abs(lowerNumber);
				if ((cumerativeNumber - (biggerNumber / 2 + 1)) >= 0)
				{
					currentXPoint += dx;
					cumerativeNumber -= biggerNumber;
				}
			}

			Node* newNode = new Node();
			newNode->x = currentXPoint;
			newNode->y = currentYPoint;
			newNode->next = currentNode;
			currentNode = newNode;
		}
		return currentNode;
	}

}


