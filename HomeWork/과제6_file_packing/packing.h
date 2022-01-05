#pragma once
#ifndef __PACKING_HEADER__
#define __PACKING_HEADER__

struct FileHeader
{
	unsigned int size;
	char fileName[64]{0};
};

struct PackHeader
{
	const unsigned int type = 0x19970404;
	int fileNum;
};

bool Packing();
bool UnPacking();

#endif // !__PACKING_HEADER__
