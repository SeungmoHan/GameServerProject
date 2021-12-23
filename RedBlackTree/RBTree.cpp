#include "RBtree.h"
#include <iostream>
namespace univ_dev
{
	RedBlackTree::RedBlackTree() : m_NilNode{ &m_NilNode,&m_NilNode,&m_NilNode,NodeColor::BLACK,0 },root(&m_NilNode), nodeCount(0){root = &m_NilNode; };

	RedBlackTree::~RedBlackTree()
	{
		Release(root);
		root = &m_NilNode;
		nodeCount = 0;
	}

	bool RedBlackTree::Insert(int data)
	{
		if (root == &m_NilNode)
		{
			root = new Node();
			m_NilNode.eNodeColor = root->eNodeColor = NodeColor::BLACK;
			root->key = data;
			root->pParentNode = root->pLeftNode = root->pRightNode = &m_NilNode;
			nodeCount++;
			return true;
		}
		bool ret = Insert(root, data);
		root->pParentNode = &m_NilNode;
		return ret;
	}

	bool RedBlackTree::Remove(int data)
	{
		return Remove(root, data);
	}

	void RedBlackTree::Traverse()
	{
		//return Traverse(root, 1);
	}

	void RedBlackTree::Release()
	{
		Release(root);
		root = &m_NilNode;
	}

	bool RedBlackTree::Insert(Node* root, int key)
	{
		while (true)
		{
			if (root == &m_NilNode) break;
			if (root->key < key)
			{
				if (root->pRightNode == &m_NilNode)
				{
					Node* newNode = new Node();
					newNode->eNodeColor = NodeColor::RED;
					newNode->key = key;
					newNode->pLeftNode = newNode->pRightNode = &m_NilNode;
					newNode->pParentNode = root;
					root->pRightNode = newNode;
					nodeCount++;
					BalanceTree(newNode);
					return true;
				}
				root = root->pRightNode;
			}
			else if (root->key > key)
			{
				if (root->pLeftNode == &m_NilNode)
				{
					Node* newNode = new Node();
					newNode->eNodeColor = NodeColor::RED;
					newNode->key = key;
					newNode->pLeftNode = newNode->pRightNode = &m_NilNode;
					newNode->pParentNode = root;
					root->pLeftNode = newNode;
					nodeCount++;
					BalanceTree(newNode);
					return true;
				}
				root = root->pLeftNode;
			}
			else break;
		}
		return false;
	}

	int RedBlackTree::NumOfChild(Node* root)
	{
		if (root == &m_NilNode)return 0;
		int numOfChild = NumOfChild(root->pLeftNode) + NumOfChild(root->pRightNode);
		return numOfChild + 1;
	}

	int RedBlackTree::GetRightChildCount(Node* root)
	{
		if (root == &m_NilNode) return 0;
		return NumOfChild(root->pRightNode);
	}

	int RedBlackTree::GetLeftChildCount(Node* root)
	{
		if (root == &m_NilNode)return 0;
		return NumOfChild(root->pLeftNode);
	}

	bool RedBlackTree::Remove(Node* root, int key)
	{
		//일단 키를 찾고
		int a = 10;
		a++;
		while (root != &m_NilNode)
		{
			if (root->key == key) break;
			else if (root->key > key)
				root = root->pLeftNode;
			else
				root = root->pRightNode;
		}
		//닐노드면 없는거니 return false
		if (root == &m_NilNode) return false;
		//여기부턴 닐노드는 아니란의미고
		Node* pParentNode = root->pParentNode;
		Node* pSibling = pParentNode->pLeftNode;
		Node* pLeftNode = root->pLeftNode;
		Node* pRightNode = root->pRightNode;
		if (pSibling == root) pSibling = pParentNode->pRightNode;
		NodeColor removeNodeColor = root->eNodeColor;
		Node* param;
		//case 1 양쪽다 nil노드라면
		if (root->pLeftNode == &m_NilNode && root->pRightNode == &m_NilNode)
		{
			if (root == this->root)
			{
				this->root = &m_NilNode;
				nodeCount--;
				delete root;
				this->root->eNodeColor = NodeColor::BLACK;
				return true;
			}
			else if (root == pParentNode->pLeftNode)
			{
				pParentNode->pLeftNode = &m_NilNode;
				m_NilNode.pParentNode = pParentNode;
			}
			else
			{
				pParentNode->pRightNode = &m_NilNode;
				m_NilNode.pParentNode = pParentNode;
			}
			param = &m_NilNode;
		}
		//case 2 left만 nil노드라면
		else if (root->pLeftNode == &m_NilNode)
		{
			if (root == this->root)
			{
				this->root = root->pRightNode;
				nodeCount--;
				delete root;
				this->root->eNodeColor = NodeColor::BLACK;
				return true;
			}
			else if (pParentNode->pLeftNode == root)
			{
				pParentNode->pLeftNode = pRightNode;
				pRightNode->pParentNode = pParentNode;
			}
			else
			{
				pParentNode->pRightNode = pRightNode;
				pRightNode->pParentNode = pParentNode;
			}
			param = pRightNode;
		}
		//case 3 right만 nil노드라면
		else if (root->pRightNode == &m_NilNode)
		{
			if (root == this->root)
			{
				this->root = root->pLeftNode;
				nodeCount--;
				delete root;
				this->root->eNodeColor = NodeColor::BLACK;
				return true;
			}
			if (pParentNode->pLeftNode == root)
			{
				pParentNode->pLeftNode = pLeftNode;
				pLeftNode->pParentNode = pParentNode;
			}
			else
			{
				pParentNode->pRightNode = pLeftNode;
				pLeftNode->pParentNode = pParentNode;
			}
			param = pLeftNode;
		}
		//case 4 양쪽다 nil노드가 아니라면
		else
		{
			Node* temp = root->pLeftNode;
			while (temp->pRightNode != &m_NilNode)
				temp = temp->pRightNode;
			root->key = temp->key;
			removeNodeColor = temp->eNodeColor;
			Node* tempParent = temp->pParentNode;
			if (tempParent == root)
			{
				tempParent->pLeftNode = temp->pLeftNode;
				temp->pLeftNode->pParentNode = tempParent;
				param = tempParent->pLeftNode;
			}
			else
			{
				tempParent->pRightNode = temp->pLeftNode;
				temp->pLeftNode->pParentNode = tempParent;
				param = temp->pLeftNode;
			}
			root = temp;
		}
		if (removeNodeColor == NodeColor::BLACK)
			ReBalanceTree(param);

		this->root->eNodeColor = m_NilNode.eNodeColor = NodeColor::BLACK;
		this->root->pParentNode = &m_NilNode;
		m_NilNode.pLeftNode = m_NilNode.pParentNode = m_NilNode.pRightNode = &m_NilNode;
		nodeCount--;
		delete root;
		return true;
	}

	void RedBlackTree::Release(Node* root)
	{
		if (root == &m_NilNode) return;
		Release(root->pLeftNode);
		Release(root->pRightNode);
		delete root;
	}
	//void RedBlackTree::Traverse(Node* root, int depth)
	//{
	//	if (root == &m_NilNode) return;
	//	Traverse(root->pLeftNode, depth + 1);
	//	printf("%d ", root->key);
	//	Traverse(root->pRightNode, depth + 1);
	//}

	void RedBlackTree::RotateRight(Node* root)
	{
		Node* pParentNode = root->pParentNode;
		Node* pLeftNode = root->pLeftNode;
		Node* pRightNode = root->pRightNode;
		Node* pGrandChildNode = root->pLeftNode->pRightNode;

		if (pParentNode->pRightNode == root)
		{
			pParentNode->pRightNode = pLeftNode;
			pLeftNode->pParentNode = pParentNode;
			pGrandChildNode->pParentNode = root;
			root->pLeftNode = pGrandChildNode;
			pLeftNode->pRightNode = root;
			root->pParentNode = pLeftNode;
			if (this->root == root)
				this->root = pRightNode;
		}
		else
		{
			pParentNode->pLeftNode = pLeftNode;
			pLeftNode->pParentNode = pParentNode;
			root->pLeftNode = pGrandChildNode;
			pGrandChildNode->pParentNode = root;
			root->pParentNode = pLeftNode;
			pLeftNode->pRightNode = root;

			if (this->root == root)
				this->root = pLeftNode;
		}
	}

	void RedBlackTree::RotateLeft(Node* root)
	{
		Node* pParentNode = root->pParentNode;
		Node* pRightNode = root->pRightNode;
		Node* pLeftNode = root->pLeftNode;
		Node* pGrandChildNode = root->pRightNode->pLeftNode;
		if (pParentNode->pRightNode == root)
		{
			pParentNode->pRightNode = pRightNode;
			pRightNode->pParentNode = pParentNode;
			root->pParentNode = pRightNode;
			pRightNode->pLeftNode = root;
			root->pRightNode = pGrandChildNode;
			pGrandChildNode->pParentNode = root;

			if (this->root == root)
				this->root = pLeftNode;
		}
		else
		{
			pParentNode->pLeftNode = pRightNode;
			pRightNode->pParentNode = pParentNode;
			root->pParentNode = pRightNode;
			pRightNode->pLeftNode = root;
			root->pRightNode = pGrandChildNode;
			pGrandChildNode->pParentNode = root;

			if (this->root == root)
				this->root = pRightNode;
		}
	}

	void RedBlackTree::BalanceTree(Node* root)
	{
		while (true)
		{
			if (root->pParentNode->eNodeColor == NodeColor::BLACK) return;

			Node* pParentNode = root->pParentNode;
			Node* pGrandParentNode = root->pParentNode->pParentNode;
			Node* pUncleNode = pGrandParentNode->pLeftNode;
			Node* pSiblingNode = root->pParentNode->pLeftNode;
			bool isSiblingLeft = true;
			bool isUncleLeft = true;

			if (pSiblingNode == root)
			{
				isSiblingLeft = false;
				pSiblingNode = root->pParentNode->pRightNode;
			}
			if (pUncleNode == root->pParentNode)
			{
				isUncleLeft = false;
				pUncleNode = pGrandParentNode->pRightNode;
			}
			if (!isUncleLeft)
			{
				if (pUncleNode->eNodeColor == NodeColor::RED)
				{
					pUncleNode->eNodeColor = pParentNode->eNodeColor = NodeColor::BLACK;
					pGrandParentNode->eNodeColor = NodeColor::RED;
					root = pGrandParentNode;
					m_NilNode.eNodeColor = this->root->eNodeColor = NodeColor::BLACK;
					continue;
				}
				if (isSiblingLeft)
				{
					RotateLeft(pParentNode);
					pParentNode = root;
				}
				pParentNode->eNodeColor = NodeColor::BLACK;
				pGrandParentNode->eNodeColor = NodeColor::RED;
				RotateRight(pGrandParentNode);
				m_NilNode.eNodeColor = this->root->eNodeColor = NodeColor::BLACK;
				return;
			}
			else
			{
				if (pUncleNode->eNodeColor == NodeColor::RED)
				{
					pUncleNode->eNodeColor = pParentNode->eNodeColor = NodeColor::BLACK;
					pGrandParentNode->eNodeColor = NodeColor::RED;
					root = pGrandParentNode;
					m_NilNode.eNodeColor = this->root->eNodeColor = NodeColor::BLACK;
					continue;
				}
				if (!isSiblingLeft)
				{
					RotateRight(pParentNode);
					pParentNode = root;
				}
				pParentNode->eNodeColor = NodeColor::BLACK;
				pGrandParentNode->eNodeColor = NodeColor::RED;
				RotateLeft(pGrandParentNode);
				m_NilNode.eNodeColor = this->root->eNodeColor = NodeColor::BLACK;
				return;
			}
		}
	}

	void RedBlackTree::ReBalanceTree(Node* root)
	{
		while (root != this->root)
		{
			if (root->eNodeColor == NodeColor::RED)
			{
				root->eNodeColor = NodeColor::BLACK;
				return;
			}
			Node* pParentNode = root->pParentNode;
			Node* pSiblingNode = pParentNode->pLeftNode;
			if (pSiblingNode == root)
				pSiblingNode = pParentNode->pRightNode;
			Node* pSiblingLeftNode = pSiblingNode->pLeftNode;
			Node* pSiblingRightNode = pSiblingNode->pRightNode;

			if (pSiblingNode == pParentNode->pRightNode)
			{
				if (pSiblingNode->eNodeColor == NodeColor::RED)
				{
					pSiblingNode->eNodeColor = NodeColor::BLACK;
					pParentNode->eNodeColor = NodeColor::RED;
					RotateLeft(pParentNode);
					continue;
				}
				else if (pSiblingNode->eNodeColor == NodeColor::BLACK && pSiblingLeftNode->eNodeColor == NodeColor::BLACK && pSiblingRightNode->eNodeColor == NodeColor::BLACK)
				{
					pSiblingNode->eNodeColor = NodeColor::RED;
					root = pParentNode;
					continue;
				}
				else if (pSiblingNode->eNodeColor == NodeColor::BLACK && pSiblingLeftNode->eNodeColor == NodeColor::RED && pSiblingRightNode->eNodeColor == NodeColor::BLACK)
				{
					pSiblingLeftNode->eNodeColor = NodeColor::BLACK;
					pSiblingNode->eNodeColor = NodeColor::RED;
					RotateRight(pSiblingNode);
					pSiblingNode = pParentNode->pRightNode;
					pSiblingLeftNode = pSiblingNode->pLeftNode;
					pSiblingRightNode = pSiblingNode->pRightNode;
				}
				pSiblingNode->eNodeColor = pParentNode->eNodeColor;
				pParentNode->eNodeColor = NodeColor::BLACK;
				pSiblingRightNode->eNodeColor = NodeColor::BLACK;
				RotateLeft(pParentNode);
				break;
			}
			else
			{
				if (pSiblingNode->eNodeColor == NodeColor::RED)
				{
					pSiblingNode->eNodeColor = NodeColor::BLACK;
					pParentNode->eNodeColor = NodeColor::RED;
					RotateRight(pParentNode);
					continue;
				}
				else if (pSiblingNode->eNodeColor == NodeColor::BLACK && pSiblingRightNode->eNodeColor == NodeColor::BLACK && pSiblingLeftNode->eNodeColor == NodeColor::BLACK)
				{
					pSiblingNode->eNodeColor = NodeColor::RED;
					root = pParentNode;
					continue;
				}
				else if (pSiblingNode->eNodeColor == NodeColor::BLACK && pSiblingRightNode->eNodeColor == NodeColor::RED && pSiblingLeftNode->eNodeColor == NodeColor::BLACK)
				{
					pSiblingRightNode->eNodeColor = NodeColor::BLACK;
					pSiblingNode->eNodeColor = NodeColor::RED;
					RotateLeft(pSiblingNode);
					pSiblingNode = pParentNode->pLeftNode;
					pSiblingLeftNode = pSiblingNode->pLeftNode;
					pSiblingRightNode = pSiblingNode->pRightNode;
				}
				pSiblingNode->eNodeColor = pParentNode->eNodeColor;
				pParentNode->eNodeColor = NodeColor::BLACK;
				pSiblingLeftNode->eNodeColor = NodeColor::BLACK;
				RotateRight(pParentNode);
				break;
			}
		}
	}
}

