#pragma once
#ifndef __SCREEN_DIB_BUFFER_HEADER__
#define __SCREEN_DIB_BUFFER_HEADER__
#define __UNIV_DEVEOPER_
#include "framework.h"

namespace univ_dev
{
	class ScreenDib
	{
	public:
		ScreenDib(int width, int height, int colorBit);
		virtual ~ScreenDib();
	protected:
		void CreateDibBuffer(int width, int height, int colorBit);
		void ReleaseDibBuffer();
	public:
		void DrawBuffer(HWND hWnd, int x = 0, int y = 0);
		BYTE* GetDibBuffer();
		int GetWidth();
		int GetHeight();
		int GetPitch();
	protected:
		BITMAPINFO _dibInfo;
		BYTE* _buffer;
		int _width;
		int _height;
		int _pitch;
		int _colorBit;
		int _bufferSize;
	};
	extern ScreenDib g_ScreenDibBuffer;
}


#endif // !__SCREEN_DIB_BUFFER_HEADER__
