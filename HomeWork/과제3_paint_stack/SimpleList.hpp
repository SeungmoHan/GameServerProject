#ifndef __SIMPLE_LIST_HEADER__
#define __SIMPLE_LIST_HEADER__
#include "node.hpp"
template<typename T>
//스택만을 위한 리스트임.
//뒤에서 넣고 뒤에서 꺼내고 밖에없는 구조
class LinkedList
{
public:
	void InsertBack(T data);
	bool RemoveBack(T& ret);
	bool IsEmpty();
	LinkedList();
private:
	Node<T>* head;
	Node<T>* tail;
};

template<typename T>
LinkedList<T>::LinkedList() : head(nullptr), tail(nullptr) {};
;
template<typename T>
void LinkedList<T>::InsertBack(T data)
{
	if (IsEmpty())
	{
		head = tail = new Node<T>(data);
		return;
	}
	tail->SetNextNode(new Node<T>(data, tail, nullptr));
	tail = tail->GetNextNode();
	Node<T>* temp = tail;
	while (temp != nullptr)
	{
		temp = temp->GetPrevNode();
	}
}

template<typename T>
bool LinkedList<T>::RemoveBack(T& ret)
{
	if (IsEmpty()) return false;
	Node<T>* removeNode = tail;
	ret = removeNode->GetData();
	if (head == tail)
	{
		head = tail = nullptr;
	}
	else
	{
		tail = tail->GetPrevNode();
	}
	delete removeNode;
	return true;
}

template<typename T>
bool LinkedList<T>::IsEmpty()
{
	return head == nullptr && tail == nullptr;
}
#endif // !__SIMPLE_LIST_HEADER__