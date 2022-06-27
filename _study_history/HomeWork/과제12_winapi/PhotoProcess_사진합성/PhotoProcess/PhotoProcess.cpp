#include <Windows.h>
#include <iostream>

int main()
{
	FILE* sample1;
	FILE* sample2;

	FILE* newFile;
	fopen_s(&sample1, "sample.bmp", "rb");
	if (sample1 == nullptr)
		return -1;
	fopen_s(&sample2, "sample2.bmp", "rb");
	if (sample2 == nullptr)
		return -2;
	fopen_s(&newFile, "newFile.bmp", "wb");
	if (newFile == nullptr)
		return -3;

	BITMAPFILEHEADER fileHeader;
	BITMAPINFOHEADER infoHeader;
	fread_s(&fileHeader, sizeof(fileHeader), sizeof(fileHeader), 1, sample1);
	fread_s(&fileHeader, sizeof(fileHeader), sizeof(fileHeader), 1, sample2);
	fread_s(&infoHeader, sizeof(infoHeader), sizeof(infoHeader), 1, sample1);
	fread_s(&infoHeader, sizeof(infoHeader), sizeof(infoHeader), 1, sample2);

	unsigned char* sample1Buffer = (unsigned char*)malloc(fileHeader.bfSize);
	unsigned char* sample2Buffer = (unsigned char*)malloc(fileHeader.bfSize);
	unsigned char* newFileBuffer = (unsigned char*)malloc(fileHeader.bfSize);
	if (sample1Buffer == nullptr)return -4;
	if (sample2Buffer == nullptr)return -5;
	if (newFileBuffer == nullptr)return -6;
	fread_s(sample1Buffer, fileHeader.bfSize, fileHeader.bfSize, 1, sample1);
	fread_s(sample2Buffer, fileHeader.bfSize, fileHeader.bfSize, 1, sample2);

	fwrite(&fileHeader, sizeof(fileHeader), 1, newFile);
	fwrite(&infoHeader, sizeof(infoHeader), 1, newFile);

	for (size_t i = 0; i < fileHeader.bfSize; i++)
	{
		newFileBuffer[i] = (sample1Buffer[i] / 2) + (sample2Buffer[i] / 2);
	}
	fwrite(newFileBuffer, fileHeader.bfSize, 1, newFile);

	return 0;
}