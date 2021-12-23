#pragma once
#ifndef __TILE_MAP_HEADER__
#define __TILE_MAP_HEADER__
#define __UNIV_DEVELOPER_
#define TILE_MAP_WIDTH 60
#define TILE_MAP_HEIGHT 40

#ifdef __PRINT_WITH_WINAPI
#include <Windows.h>
#endif 

namespace univ_dev
{
	using DWORD = unsigned long;
	class TileMap
	{
	private:
		enum TileMapSize { Width = TILE_MAP_WIDTH, Height = TILE_MAP_HEIGHT };
		bool blockTile[TileMapSize::Height][TileMapSize::Width]{ 0 };
		bool openList[TileMapSize::Height][TileMapSize::Width]{ 0 };
		bool closeList[TileMapSize::Height][TileMapSize::Width]{ 0 };
		DWORD blockColor[TileMapSize::Height][TileMapSize::Width]{ 0 };
	public:
#ifdef __PRINT_WITH_WINAPI
		friend void PrintTile(TileMap& tile, HDC hdc);
#endif
		bool BlockTile(int x, int y, bool what);
		void ResetBlockTile();
		bool hasBeenBefore(int x, int y);
		bool IsBlocked(int x, int y);
		void ResetOpenList();
		void ResetCloseList();
		void ResetBlockColor();

		void SetOpenList(int x, int y, bool what);
		void SetBlockColor(int x, int y, DWORD color);
		void SetCloseList(int x, int y, bool what);

		bool IsInOpenList(int x, int y);
		bool IsInCloseList(int x, int y);

		void ResetTileMap()
		{
			ResetBlockTile();
			ResetOpenList();
			ResetCloseList();
			ResetBlockColor();
		}
	};
}


#endif // !__TILE_MAP_HEADER__
