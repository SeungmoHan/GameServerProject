#include "ScreenDib.h"

namespace univ_dev
{
	ScreenDib g_ScreenDibBuffer(640, 480, 32);
	ScreenDib::ScreenDib(int width, int height, int colorBit) : _buffer(nullptr), _height(0), _width(0), _pitch(0), _colorBit(0)
	{
		memset(&_dibInfo, 0, sizeof(BITMAPINFO));
		CreateDibBuffer(width, height, colorBit);
	}

	ScreenDib::~ScreenDib()
	{
		ReleaseDibBuffer();
	}

	void ScreenDib::CreateDibBuffer(int width, int height, int colorBit)
	{
		_width = width;
		_height = height;
		_colorBit = colorBit;
		_pitch = ((_width * (_colorBit / 8)) + 3) & ~3;
		_bufferSize = _pitch * _height;

		BITMAPINFOHEADER& header = _dibInfo.bmiHeader;
		header.biSize = sizeof(BITMAPINFOHEADER);
		header.biWidth = _width;
		header.biHeight = -_height;
		header.biPlanes = 1;
		header.biBitCount = _colorBit;
		header.biCompression = 0;
		header.biSizeImage = _bufferSize;
		header.biXPelsPerMeter = 0;
		header.biYPelsPerMeter = 0;
		header.biClrUsed = 0;
		header.biClrImportant = 0;

		_buffer = new BYTE[_bufferSize];
		memset(_buffer, 0xff, _bufferSize);
	}

	void ScreenDib::ReleaseDibBuffer()
	{
		_width = 0;
		_height = 0;
		_pitch = 0;
		_bufferSize = 0;

		memset(&_dibInfo, 0x00, sizeof(BITMAPINFO));
		if (_buffer != nullptr)
			delete[] _buffer;
		_buffer = nullptr;
	}

	void ScreenDib::DrawBuffer(HWND hWnd, int x, int y)
	{
		if (_buffer == nullptr)return;

		RECT rect;
		HDC hdc = GetDC(hWnd);
		GetWindowRect(hWnd, &rect);

		int i = SetDIBitsToDevice(hdc, 0, 0, _width, _height,
			0, 0, 0, _height,
			_buffer, &_dibInfo, DIB_RGB_COLORS);
		ReleaseDC(hWnd, hdc);
	}

	BYTE* ScreenDib::GetDibBuffer()
	{
		return _buffer;
	}

	int ScreenDib::GetWidth()
	{
		return _width;
	}

	int ScreenDib::GetHeight()
	{
		return _height;
	}

	int ScreenDib::GetPitch()
	{
		return _pitch;
	}

}

