#include "packing.h"
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>



bool Packing()
{
	char fileName[128]{ 0 };
	printf("저장할 파일 이름 입력 : ");
	scanf_s("%s", fileName, sizeof(fileName));
	FILE* package;
	fopen_s(&package, fileName, "wb");
	if (package == nullptr) return false;
	unsigned int numOfFiles;
	printf("입력할 파일 수\n");
	scanf_s("%u", &numOfFiles);

	PackHeader packHeader;
	packHeader.fileNum = numOfFiles;

	fwrite(&packHeader, sizeof(PackHeader), 1, package);

	FileHeader* fileHeaderPtr = (FileHeader*)malloc(sizeof(FileHeader) * numOfFiles);
	char** fileDatas = (char**)malloc(sizeof(char*) * numOfFiles);
	if (fileHeaderPtr == nullptr || fileDatas == nullptr)
	{
		printf("fileHeaderPtr or fileDatas == nullptr. memory allocation failed\n");
		fclose(package);
		remove(fileName);
		return false;
	}
	for (size_t i = 0; i < numOfFiles; i++)
	{
		printf("%d번째 파일 이름을 입력하세요 최대 64자\n", (unsigned int)i + 1);
		scanf_s("%s", fileHeaderPtr[i].fileName, 64);
		FILE* f;
		fopen_s(&f, fileHeaderPtr[i].fileName, "rb");
		if (f == nullptr)
		{
			printf("파일이 존재하지 않습니다.\n");
			system("pause");
			fclose(package);
			remove(fileName);
			return false;
		}
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f);
		rewind(f);
		fileHeaderPtr[i].size = size;
		printf("this file name : %s\n", fileHeaderPtr[i].fileName);
		printf("this file size : %d\n", fileHeaderPtr[i].size);
		
		fileDatas[i] = (char*)malloc(size);
		if (fileDatas[i] == nullptr)
		{
			printf("fileDatas[i] == nullptr. memory allocation failed\n");
			fclose(f);
			fclose(package);
			remove(fileName);
			return false;
		}
		fread_s(fileDatas[i], size, size, 1, f);

		fwrite(fileHeaderPtr + i, sizeof(FileHeader), 1, package);
		fclose(f);
	}

	for (size_t i = 0; i < numOfFiles; i++)
	{
		printf("file size : %d\n", fileHeaderPtr[i].size);
		fwrite(fileDatas[i], fileHeaderPtr[i].size, 1, package);
	}
	for (size_t i = 0; i < numOfFiles; i++)
	{
		free(fileDatas[i]);
	}
	free(fileDatas);
	free(fileHeaderPtr);
	fclose(package);
	return true;
}

bool UnPacking()
{
	char packageName[64]{ 0 };
	printf("언패킹 할 파일 이름을 입력하세요.\n");
	scanf_s("%s", packageName, sizeof(packageName));

	FILE* readFile;
	fopen_s(&readFile, packageName, "rb");
	if (readFile == nullptr)
	{
		printf("존재하지 않는 파일 %d\n", __LINE__);
		return false;
	}
	fseek(readFile, 0, SEEK_END);
	size_t fileSize = ftell(readFile);
	rewind(readFile);
	void* buffer = malloc(fileSize);
	if (buffer == nullptr)
	{
		printf("memory allocation failed %d\n", __LINE__);
		return false;
	}
	fread_s(buffer, fileSize, fileSize, 1, readFile);
	
	PackHeader* tempHeader = (PackHeader*)buffer;
	size_t numOfFiles = tempHeader->fileNum;
	char* currentPos = (char*)buffer;
	currentPos += sizeof(PackHeader);
	if (tempHeader->type != 0x19970404)
	{
		printf("wrong pkg\n");
		return false;
	}

	FileHeader* fileHeaderPtr = (FileHeader*)malloc(sizeof(FileHeader) * numOfFiles);
	if (fileHeaderPtr == nullptr)
	{
		printf("memory allocation failed %d\n",__LINE__);
		fclose(readFile);
		return false;
	}
	memcpy_s(fileHeaderPtr, sizeof(FileHeader) * numOfFiles, currentPos, sizeof(FileHeader) * numOfFiles);
	currentPos += sizeof(FileHeader) * numOfFiles;
	for (size_t i = 0; i < numOfFiles; i++)
	{
		FILE* tempFile;
		fopen_s(&tempFile, fileHeaderPtr[i].fileName, "wb");
		if (tempFile == nullptr)
		{
			printf("파일 열기 실패 %d\n", __LINE__);
			fclose(readFile);
			free(fileHeaderPtr);
			return false;
		}
		void* fileBuffer= malloc(fileHeaderPtr[i].size);
		if (fileBuffer == nullptr)
		{
			printf("memory allocation failed %d\n", __LINE__);
			fclose(tempFile);
			fclose(readFile);
			free(fileHeaderPtr);
			return false;
		}
		memcpy_s(fileBuffer, fileHeaderPtr[i].size, currentPos, fileHeaderPtr[i].size);
		currentPos += fileHeaderPtr[i].size;
		fwrite(fileBuffer, fileHeaderPtr[i].size, 1, tempFile);
		free(fileBuffer);
		fclose(tempFile);
	}
	return true;
}
