#pragma once
#ifndef __TILE_MAP_HEADER__
#define __TILE_MAP_HEADER__

#define TILE_MAP_WIDTH 60
#define TILE_MAP_HEIGHT 40

#include <Windows.h>
class TileMap
{
private:
	enum TileMapSize { Width = TILE_MAP_WIDTH, Height = TILE_MAP_HEIGHT};
	bool blockTile[TileMapSize::Height][TileMapSize::Width]{ 0 };
	bool openList[TileMapSize::Height][TileMapSize::Width]{ 0 };
	bool closeList[TileMapSize::Height][TileMapSize::Width]{ 0 };
public:
	friend void PrintTile(TileMap& tile,HDC hdc);
	bool BlockTile(int x, int y,bool what);
	void ResetBlockTile();
	bool hasBeenBefore(int x, int y);
	bool IsBlocked(int x, int y);
	void ResetOpenList();
	void ResetCloseList();

	void SetOpenList(int x, int y,bool what);
	void SetCloseList(int x, int y, bool what);

	bool IsInOpenList(int x, int y);
	bool IsInCloseList(int x, int y);

	void ResetTileMap()
	{
		ResetBlockTile();
		ResetOpenList();
		ResetCloseList();
	}
};


#endif // !__TILE_MAP_HEADER__
