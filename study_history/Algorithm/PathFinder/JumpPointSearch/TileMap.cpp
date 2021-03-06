#include "TileMap.h"

///__univ_developer_tile_map_

namespace univ_dev
{
	bool TileMap::BlockTile(int x, int y, bool what)
	{
		if (x < 0 || x >= TileMapSize::Width) return false;
		if (y < 0 || y >= TileMapSize::Height) return false;
		if (blockTile[y][x] == what) return false;
		blockTile[y][x] = what;
		return true;
	}
	void TileMap::ResetBlockTile()
	{
		for (int i = 0; i < TileMapSize::Height; i++)
			for (int j = 0; j < TileMapSize::Width; j++)
				blockTile[i][j] = false;
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
		for (int i = 0; i < TileMapSize::Height; i++)
			for (int j = 0; j < TileMapSize::Width; j++)
				openList[i][j] = false;
	}
	void TileMap::ResetCloseList()
	{
		for (int i = 0; i < TileMapSize::Height; i++)
			for (int j = 0; j < TileMapSize::Width; j++)
				closeList[i][j] = false;
	}
	void TileMap::ResetBlockColor()
	{
		for (int i = 0; i < TileMapSize::Height; i++)
			for (int j = 0; j < TileMapSize::Width; j++)
				blockColor[i][j] = 0;
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
	void TileMap::SetCloseList(int x, int y, bool what)
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
}
























