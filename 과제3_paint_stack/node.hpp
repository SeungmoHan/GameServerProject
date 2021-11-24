#pragma once
#ifndef __NODE_HEADER__
#define __NODE_HEADER__

template<typename T>
class Node
{
public:
	Node<T>(T data);
	Node<T>(T data, Node<T>* prev, Node<T>* next);
	T GetData()const;
	void SetData(T data);

	Node<T>* GetPrevNode()const;
	Node<T>* GetNextNode()const;

	void SetPrevNode(Node<T>* prev);
	void SetNextNode(Node<T>* next);
private:
	Node<T>* prev;
	T data;
	Node<T>* next;
};

template<typename T>
Node<T>::Node<T>(T data) :data(data), prev(nullptr), next(nullptr) {};

template<typename T>
Node<T>::Node<T>(T data, Node<T>* prev, Node<T>* next) : data(data), prev(prev), next(next) {};

template<typename T>
T Node<T>::GetData()const { return data; }

template<typename T>
void Node<T>::SetData(T data) { this->data = data; }


template<typename T>
void Node<T>::SetNextNode(Node<T>* next) { this->next = next; }
template<typename T>
void Node<T>::SetPrevNode(Node<T>* prev) { this->prev = prev; }

template<typename T>
Node<T>* Node<T>::GetNextNode()const { return next; }
template<typename T>
Node<T>* Node<T>::GetPrevNode()const { return prev; }

#endif // !__NODE_HEADER__
