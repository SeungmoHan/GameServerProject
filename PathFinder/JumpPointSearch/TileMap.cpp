#include "TileMap.h"
#include <memory.h>
bool TileMap::BlockTile(int x, int y,bool what)
{
	if (x < 0 || x >= TileMapSize::Width) return false;
	if (y < 0 || y >= TileMapSize::Height) return false;
	if (blockTile[y][x] == what) return false;
	blockTile[y][x] = what;
	return true;
}



void TileMap::ResetBlockTile()
{
	memset(blockTile, 0, TileMapSize::Height * TileMapSize::Width);
}

bool TileMap::hasBeenBefore(int x, int y)
{
	return closeList[y][x];
}

bool TileMap::IsBlocked(int x, int y)
{
	if (x < 0 || x >= TileMapSize::Width) return true;
	if (y < 0 || y >= TileMapSize::Height) return true;
	return blockTile[y][x];
}

void TileMap::ResetOpenList()
{
	memset(openList, 0, TileMapSize::Height * TileMapSize::Width);
}

void TileMap::ResetCloseList()
{
	memset(closeList, 0, TileMapSize::Height * TileMapSize::Width);
}

void TileMap::ResetBlockColor()
{
	memset(blockColor, 0, sizeof(DWORD) * TileMapSize::Height * TileMapSize::Width);
}

void TileMap::SetOpenList(int x, int y, bool what)
{
	if (x < 0 || x >= TileMapSize::Width) return;
	if (y < 0 || y >= TileMapSize::Height) return;
	openList[y][x] = what;
}

void TileMap::SetBlockColor(int x, int y, DWORD color)
{
	if (x < 0 || x >= TileMapSize::Width) return;
	if (y < 0 || y >= TileMapSize::Height) return;
	blockColor[y][x] = color;
}

void TileMap::SetCloseList(int x, int y,bool what)
{
	if (x < 0 || x >= TileMapSize::Width) return;
	if (y < 0 || y >= TileMapSize::Height) return;
	closeList[y][x] = what;
}

bool TileMap::IsInOpenList(int x, int y)
{
	return openList[y][x];
}

bool TileMap::IsInCloseList(int x, int y)
{
	return closeList[y][x];
}
