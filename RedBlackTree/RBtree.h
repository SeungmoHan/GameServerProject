#pragma once
#pragma once
#ifndef __RED_BLACK_TREE_HEADER__
#define __RED_BLACK_TREE_HEADER__
#include <Windows.h>
namespace univ_dev
{
	class RedBlackTree
	{
	private:
		enum class NodeColor { BLACK, RED };
		struct Node
		{
			Node* pParentNode;
			Node* pLeftNode;
			Node* pRightNode;
			NodeColor eNodeColor;
			int key;
		};
	public:
		void Print(HDC hdc);
		RedBlackTree();
		~RedBlackTree();
		bool Insert(int data);
		bool Remove(int data);
		void Traverse();
		void Release();
		int GetNodeCount() { return nodeCount; }
	private:
		void Print(Node* root, int& depth, int& x, HDC hdc);
		bool Insert(Node* root, int key);
		int NumOfChild(Node* root);
		int GetRightChildCount(Node* root);
		int GetLeftChildCount(Node* root);
		bool Remove(Node* root, int key);
		void Release(Node* root);
		//void Traverse(Node* root, int depth);
		void RotateRight(Node* root);
		void RotateLeft(Node* root);
		void BalanceTree(Node* root);
		void ReBalanceTree(Node* root);
	private:
		Node m_NilNode;
		Node* root;
		int nodeCount;
	};
}



#endif // !__RED_BLACK_TREE_HEADER__
