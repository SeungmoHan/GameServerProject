#include <Windows.h>
#include "SpriteDib.h"

//__univ_developer_sprite_dib_

namespace univ_dev
{
	SpriteDib g_SpriteDibBuffer(100, 0x00ffffff);
	SpriteDib::SpriteDib(int maxSprite, DWORD colorKey) : _maxSprite(maxSprite), _colorKey(colorKey), _sprites(nullptr)
	{
		_sprites = new Sprite[_maxSprite];
		memset(_sprites, 0, sizeof(Sprite) * _maxSprite);
	}

	SpriteDib::~SpriteDib()
	{
		for (int i = 0; i < _maxSprite; i++)
		{
			ReleaseSprite(i);
		}
	}



	bool SpriteDib::LoadDibSprite(int spriteIndex, const WCHAR* fileName, int centerPointX, int centerPointY)
	{
		HANDLE file;
		DWORD read;
		int pitch;
		int imageSize;
		BITMAPFILEHEADER fileHeader;
		BITMAPINFOHEADER infoHeader;
		file = CreateFile(fileName, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (file == INVALID_HANDLE_VALUE) return false;
		ReleaseSprite(spriteIndex);
		ReadFile(file, &fileHeader, sizeof(BITMAPFILEHEADER), &read, NULL);
		if (fileHeader.bfType == 0x4d42)
		{
			ReadFile(file, &infoHeader, sizeof(BITMAPINFOHEADER), &read, NULL);
			if (infoHeader.biBitCount == 32)
			{
				pitch = (infoHeader.biWidth * 4) + 3 & ~3;

				_sprites[spriteIndex].width = infoHeader.biWidth;
				_sprites[spriteIndex].height = infoHeader.biHeight;
				_sprites[spriteIndex].pitch = pitch;

				imageSize = pitch * infoHeader.biHeight;
				_sprites[spriteIndex].image = new BYTE[imageSize];

				BYTE* tempBuffer = new BYTE[imageSize];
				BYTE* spriteTemp = _sprites[spriteIndex].image;
				BYTE* turnTemp;

				ReadFile(file, tempBuffer, imageSize, &read, NULL);

				turnTemp = tempBuffer + pitch * (infoHeader.biHeight - 1);

				for (int i = 0; i < infoHeader.biHeight; i++)
				{
					memcpy(spriteTemp, turnTemp, pitch);
					spriteTemp += pitch;
					turnTemp -= pitch;
				}
				delete[] tempBuffer;
				_sprites[spriteIndex].centerPointX = centerPointX;
				_sprites[spriteIndex].centerPointY = centerPointY;
				CloseHandle(file);
				return true;
			}
		}
		CloseHandle(file);
		return false;
	}
	void SpriteDib::ReleaseSprite(int spriteIndex)
	{
		if (_maxSprite <= spriteIndex)
			return;
		if (_sprites[spriteIndex].image != nullptr)
		{
			delete[] _sprites[spriteIndex].image;
			memset(&_sprites[spriteIndex], 0, sizeof(Sprite));
		}
	}
	void SpriteDib::DrawSprite(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch, int drawLen)
	{
		if (spriteIndex >= _maxSprite)return;
		if (_sprites[spriteIndex].image == nullptr)return;

		Sprite* pSprite = &_sprites[spriteIndex];

		int spriteWidth = pSprite->width;
		int spriteHeight = pSprite->height;
		int countX, countY;
		spriteWidth = spriteWidth * drawLen / 100;

		DWORD* pDest = (DWORD*)dest;
		DWORD* dwpSprite = (DWORD*)(pSprite->image);

		drawX = drawX - pSprite->centerPointX;
		drawY = drawY - pSprite->centerPointY;

		//upper clipping
		if (0 > drawY)
		{
			spriteHeight = spriteHeight - (-drawY);
			dwpSprite = (DWORD*)(pSprite->image + pSprite->pitch * (-drawY));
			drawY = 0;
		}
		//bottom clipping
		if (destHeight <= drawY + pSprite->height)
		{
			spriteHeight -= ((drawY + pSprite->height) - destHeight);
		}
		//left clipping
		if (drawX < 0)
		{
			spriteWidth = spriteWidth - (-drawX);
			dwpSprite = dwpSprite + (-drawX);
			drawX = 0;
		}
		//right clipping
		if (destWidth <= drawX + pSprite->width)
		{
			spriteWidth -= ((drawX + pSprite->width) - destWidth);
		}
		if (spriteWidth <= 0 || spriteHeight <= 0)return;

		pDest = (DWORD*)(((BYTE*)(pDest + drawX) + (drawY * destPitch)));

		BYTE* destOrigin = (BYTE*)pDest;
		BYTE* spriteOrigin = (BYTE*)dwpSprite;

		for (countY = 0; spriteHeight > countY; countY++)
		{
			for (countX = 0; spriteWidth > countX; countX++)
			{
				if (_colorKey != (*dwpSprite & 0x00ffffff))
				{
					*pDest = *dwpSprite;
				}
				pDest++;
				dwpSprite++;
			}

			destOrigin = destOrigin + destPitch;
			spriteOrigin = spriteOrigin + pSprite->pitch;

			pDest = (DWORD*)destOrigin;
			dwpSprite = (DWORD*)spriteOrigin;
		}
	}

	void SpriteDib::DrawSprite50(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch)
	{
		if (spriteIndex >= _maxSprite)return;
		if (_sprites[spriteIndex].image == nullptr)return;

		Sprite* pSprite = &_sprites[spriteIndex];

		int spriteWidth = pSprite->width;
		int spriteHeight = pSprite->height;
		int countX, countY;
		spriteWidth = spriteWidth * 100 / 100;

		DWORD* pDest = (DWORD*)dest;
		DWORD* dwpSprite = (DWORD*)(pSprite->image);

		drawX = drawX - pSprite->centerPointX;
		drawY = drawY - pSprite->centerPointY;

		//upper clipping
		if (0 > drawY)
		{
			spriteHeight = spriteHeight - (-drawY);
			dwpSprite = (DWORD*)(pSprite->image + pSprite->pitch * (-drawY));
			drawY = 0;
		}
		//bottom clipping
		if (destHeight <= drawY + pSprite->height)
		{
			spriteHeight -= ((drawY + pSprite->height) - destHeight);
		}
		//left clipping
		if (drawX < 0)
		{
			spriteWidth = spriteWidth - (-drawX);
			dwpSprite = dwpSprite + (-drawX);
			drawX = 0;
		}
		//right clipping
		if (destWidth <= drawX + pSprite->width)
		{
			spriteWidth -= ((drawX + pSprite->width) - destWidth);
		}
		if (spriteWidth <= 0 || spriteHeight <= 0)return;

		pDest = (DWORD*)(((BYTE*)(pDest + drawX) + (drawY * destPitch)));

		BYTE* destOrigin = (BYTE*)pDest;
		BYTE* spriteOrigin = (BYTE*)dwpSprite;

		for (countY = 0; spriteHeight > countY; countY++)
		{
			for (countX = 0; spriteWidth > countX; countX++)
			{
				if (_colorKey != (*dwpSprite & 0x00ffffff))
				{
					BYTE currentRed = GetRValue(*pDest);
					BYTE currentGreen = GetGValue(*pDest);
					BYTE currentBlue = GetBValue(*pDest);

					BYTE newRed = GetRValue(*dwpSprite);
					BYTE newGreen = GetGValue(*dwpSprite);
					BYTE newBlue = GetBValue(*dwpSprite);

					COLORREF newColor = RGB(currentRed / 2 + newRed / 2, currentGreen / 2 + newGreen / 2, currentBlue / 2 + newBlue / 2);
					*pDest = newColor;
				}
				pDest++;
				dwpSprite++;
			}

			destOrigin = destOrigin + destPitch;
			spriteOrigin = spriteOrigin + pSprite->pitch;

			pDest = (DWORD*)destOrigin;
			dwpSprite = (DWORD*)spriteOrigin;
		}
	}

	void SpriteDib::DrawSpriteRed(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch)
	{
		if (spriteIndex >= _maxSprite)return;
		if (_sprites[spriteIndex].image == nullptr)return;

		Sprite* pSprite = &_sprites[spriteIndex];

		int spriteWidth = pSprite->width;
		int spriteHeight = pSprite->height;
		int countX, countY;
		spriteWidth = spriteWidth * 100 / 100;

		DWORD* pDest = (DWORD*)dest;
		DWORD* dwpSprite = (DWORD*)(pSprite->image);

		drawX = drawX - pSprite->centerPointX;
		drawY = drawY - pSprite->centerPointY;

		//upper clipping
		if (0 > drawY)
		{
			spriteHeight = spriteHeight - (-drawY);
			dwpSprite = (DWORD*)(pSprite->image + pSprite->pitch * (-drawY));
			drawY = 0;
		}
		//bottom clipping
		if (destHeight <= drawY + pSprite->height)
		{
			spriteHeight -= ((drawY + pSprite->height) - destHeight);
		}
		//left clipping
		if (drawX < 0)
		{
			spriteWidth = spriteWidth - (-drawX);
			dwpSprite = dwpSprite + (-drawX);
			drawX = 0;
		}
		//right clipping
		if (destWidth <= drawX + pSprite->width)
		{
			spriteWidth -= ((drawX + pSprite->width) - destWidth);
		}
		if (spriteWidth <= 0 || spriteHeight <= 0)return;

		pDest = (DWORD*)(((BYTE*)(pDest + drawX) + (drawY * destPitch)));

		BYTE* destOrigin = (BYTE*)pDest;
		BYTE* spriteOrigin = (BYTE*)dwpSprite;

		for (countY = 0; spriteHeight > countY; countY++)
		{
			for (countX = 0; spriteWidth > countX; countX++)
			{
				if (_colorKey != (*dwpSprite & 0x00ffffff))
				{

					COLORREF newColor = *dwpSprite;
					newColor = RGB(
						GetBValue(newColor) / 2,
						GetGValue(newColor) / 2,
						GetRValue(newColor));
					*pDest = newColor;
				}
				pDest++;
				dwpSprite++;
			}

			destOrigin = destOrigin + destPitch;
			spriteOrigin = spriteOrigin + pSprite->pitch;

			pDest = (DWORD*)destOrigin;
			dwpSprite = (DWORD*)spriteOrigin;
		}
	}

	void SpriteDib::DrawImage(int spriteIndex, int drawX, int drawY, BYTE* dest, int destWidth, int destHeight, int destPitch, int drawLen)
	{
		if (spriteIndex >= _maxSprite) return;
		if (_sprites[spriteIndex].image == nullptr) return;

		Sprite* pSprite = &_sprites[spriteIndex];

		int spriteWidth = pSprite->width;
		int spriteHeight = pSprite->height;
		int countY;

		spriteWidth = spriteWidth * drawLen / 100;

		DWORD* dwpDest = (DWORD*)dest;
		DWORD* dwpSprite = (DWORD*)pSprite->image;


		//상부 클리핑
		if (drawY < 0)
		{
			spriteHeight = spriteHeight - (-drawY);
			dwpSprite = (DWORD*)(pSprite->image + pSprite->pitch * (-drawY));
			drawY = 0;
		}
		//하부 클리핑
		if (destHeight < drawY + pSprite->height)
		{
			spriteHeight -= ((drawY + pSprite->height) - destHeight);
		}

		//좌측클리핑
		if (drawX < 0)
		{
			spriteWidth = spriteWidth - (-drawX);
			dwpSprite = dwpSprite + (-drawX);
			drawX = 0;
		}

		if (destWidth < drawX + pSprite->width)
		{
			spriteWidth -= ((drawX + pSprite->width) - destWidth);
		}
		if (spriteWidth <= 0 || spriteHeight <= 0)return;

		dwpDest = (DWORD*)(((BYTE*)(dwpDest + drawX) + (drawY * destPitch)));

		for (countY = 0; spriteHeight > countY; countY++)
		{
			memcpy_s(dwpDest, spriteWidth * 4, dwpSprite, spriteWidth * 4);
			dwpDest = (DWORD*)((BYTE*)dwpDest + destPitch);
			dwpSprite = (DWORD*)((BYTE*)dwpSprite + pSprite->pitch);
		}
	}

}
