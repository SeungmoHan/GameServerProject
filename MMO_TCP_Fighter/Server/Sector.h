#pragma once
#ifndef __SECTOR_HEADER__
#define __SECTOR_HEADER__
#define __UNIV_DEVELOPER_


namespace univ_dev
{
	constexpr int SECTOR_WIDTH = 64 * 5;
	constexpr int SECTOR_HEIGHT = 64 * 5;
	constexpr int SECTOR_MAX_X = 6400 / SECTOR_WIDTH;
	constexpr int SECTOR_MAX_Y = 6400 / SECTOR_HEIGHT;
	struct Sector
	{
		int x;
		int y;
	};

	struct SectorAround
	{
		int sectorCount;
		Sector aroundSector[9];
	};
}



#endif // !__SECROTR_HEADER__
