#include <stdio.h>
#include <stdlib.h>
#include "linkedlist.h"

void ListInit(List* l)
{
	l->tail = l->head = NULL;
	l->size = 0;
}

bool ListIsEmpty(List* l)
{
	return l->size == 0;
}
int FindPos(List* l, const void* data, int dataSize)
{
	Node* current = l->head;
	char* dataPtr = (char*)data;
	char* compPtr = nullptr;
	int idx = 0;
	bool dataFound;
	while (current != nullptr)
	{
		dataFound = true;
		compPtr = (char*)current->data;
		for (size_t i = 0; i < dataSize; i++)
		{
			if (compPtr[i] != dataPtr[i])
			{
				dataFound = false;
				break;
			}
		}
		if (dataFound) return idx;
		idx++;
		current = current->next;
	}
	return -1;
}
void ListInsert(List* l, void* data, int pos)
{
	if (pos > l->size)
	{
		printf("index overflow\n");
		return;
	}
	Node* newNode = (Node*)malloc(sizeof(Node));
	if (!newNode)
	{
		printf("memory allocation failed\n");
		return;
	}
	newNode->next = newNode->prev = NULL;
	newNode->data = data;

	//ó�� �����ϴ� ���
	if(ListIsEmpty(l))
	{
		l->head = l->tail = newNode;
		l->size++;
		return;
	}
	//head�տ� �ִ°��
	if (pos == 0)
	{
		l->head->prev = newNode;
		newNode->next = l->head;
		l->head = newNode;
		l->size++;
		return;
	}
	//tail�ڿ� �ִ°��
	else if (pos == l->size)
	{
		l->tail->next = newNode;
		newNode->prev = l->tail;
		l->tail = newNode;
		l->size++;
		return;
	}
	//�߰��� �����ϴ� ���
	Node* current = l->head;
	for (int i = 0; i < pos-1; i++)
		current = current->next;
	newNode->prev = current;
	newNode->next = current->next;
	current->next->prev = newNode;
	current->next = newNode;
	l->size++;
}
void* ListRemove(List* l, int pos)
{
	if (ListIsEmpty(l))
	{
		return nullptr;
	}
	if (pos >= l->size || pos < 0)
	{
		return nullptr;
	}
	
	Node* temp;
	void* backup;
	if (l->size == 1)
	{
		temp = l->head;
		backup = temp->data;
		l->head = l->tail = NULL;
		free(temp);
		l->size--;
		return backup;
	}
	//head�� ����� case
	if (pos == 0)
	{
		temp = l->head;
		l->head = l->head->next;
		l->head->prev = NULL;
		backup = temp->data;
		free(temp);
		l->size--;
		return backup;
	}
	//tail�� ����� case
	else if (pos == l->size-1)
	{
		temp = l->tail;
		l->tail = l->tail->prev;
		l->tail->next = NULL;
		backup = temp->data;
		free(temp);
		l->size--;
		return backup;
	}
	//�߰��� ���Ե� ��� ���� case
	temp = l->head;
	for (int i = 0; i < pos; i++)
		temp = temp->next;
	//temp == �����Ϸ����ϴ� pos���ִ� ���
	backup = temp->data;
	temp->prev->next = temp->next;
	temp->next->prev = temp->prev;
	free(temp);
	l->size--;
	return backup;
}