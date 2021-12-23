#pragma once
#ifndef __DRAW_STRAIGHT_LINE_HEADER__
#define __DRAW_STRAIGHT_LINE_HEADER__
#define __UNIV_DEVELOPER_

namespace univ_dev
{
	class DrawLine
	{
	public:
		struct Node
		{
			int x;
			int y;
			Node* next;
		};
		Node* DrawStraightLine(int beginXPoint, int endXPoint, int beginYPoint, int endYPoint);
		Node* GetStraightLine(int beginXPoint, int endXPoint, int beginYPoint, int endYPoint);
		void StartDrawLine()
		{
			startFlag = true;
		}
		void EndDrawLine()
		{
			startFlag = false;
		}
		bool IsStartFlagOn()
		{
			return startFlag;
		}
		void RemoveLine(DrawLine::Node*& node)
		{
			while (node != nullptr)
			{
				DrawLine::Node* next = node->next;
				delete node;
				node = next;
			}
			node = nullptr;
		}
	private:
		bool startFlag = false;
	};
}



#endif // !__DRAW_STRAIGHT_LINE_HEADER__
