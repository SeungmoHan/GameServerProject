#include "ObjectStar.h"
#include <iostream>


void OneStar::Action()
{
	xPos++;
	if (xPos >= 70)
		removeFlag = true;
}

void OneStar::Draw()
{
	for (size_t i = 0; i < xPos; i++)
	{
		printf(" ");
	}
	printf("*");
}


void TwoStar::Action()
{
	xPos+=2;
	if (xPos >= 70)
		removeFlag = true;
}
void TwoStar::Draw()
{
	for (size_t i = 0; i < xPos; i++)
	{
		printf(" ");
	}
	printf("**");
}

void ThreeStar::Action()
{
	xPos+=3;
	if (xPos >= 70)
		removeFlag = true;
}
void ThreeStar::Draw()
{
	for (size_t i = 0; i < xPos; i++)
	{
		printf(" ");
	}
	printf("***");
}