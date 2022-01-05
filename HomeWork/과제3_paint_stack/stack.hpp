#pragma once
#ifndef __STACK_HEADER__
#define __STACK_HEADER__
#include "SimpleList.hpp"

template <typename T>
class Stack
{
public:
	void Push(T data);
	bool Pop(T& ret);
private:
	bool IsEmpty();
	LinkedList<T> list;
};

template <typename T>
void Stack<T>::Push(T data)
{
	return list.InsertBack(data);
}

template <typename T>
bool Stack<T>::Pop(T& ret)
{
	return list.RemoveBack(ret);
}

template <typename T>
bool Stack<T>::IsEmpty()
{
	return list.IsEmpty();
}
#endif // !__STACK_HEADER__
