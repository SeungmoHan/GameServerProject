#pragma once
#ifndef __LINKED_LIST_HEADER__
#define __LINKED_LIST_HEADER__
#include "node.h"

typedef struct _list
{
	Node* head, * tail;
	int size;
}List;

void ListInit(List* l);
bool ListIsEmpty(List* l);
void ListInsert(List* l, void* data, int pos);
void* ListRemove(List* l, int pos);
int FindPos(List* l,const void* data, int dataSize);
void PrintList(List* l);
#endif // !__LINKED_LIST_HEADER__
