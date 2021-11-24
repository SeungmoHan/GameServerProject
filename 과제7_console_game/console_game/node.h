#pragma once
#ifndef __NODE_HEADER__
#define __NODE_HEADER__
#define NO_MATCHING_VALUE 987654321

typedef struct _node
{
	struct _node* prev;
	void* data;
	struct _node* next;
}Node;

#endif // !__NODE_HEADER__
