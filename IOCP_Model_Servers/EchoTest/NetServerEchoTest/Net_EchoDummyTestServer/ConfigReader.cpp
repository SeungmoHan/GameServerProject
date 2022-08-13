#include "ConfigReader.h"
#include <iostream>

#define CRASH() do{int *ptr = nullptr; *ptr = 100;} while(0)

namespace univ_dev
{
	ConfigReader g_ConfigReader;

	ConfigReader::ConfigReader() : _File(nullptr), _Cursor(nullptr) {};
	bool ConfigReader::init(const WCHAR* filename)
	{
		WCHAR line[512];


		while (this->_File == nullptr)
			_wfopen_s(&this->_File, filename, L"r,ccs=UTF-16LE");

		while (!feof(this->_File))
		{
			fgetws(line, 512, this->_File);
			
			WCHAR* curPos = line;
			while (*curPos)
			{
				if (*curPos == this->SECTION_FLAG)
				{
					this->ReadSection(curPos + 1);
					curPos++;
				}
				else if (*curPos == L'\t' || *curPos == L' ')
				{
					curPos++;
					continue;
				}
				else if (*curPos == L'\\' && *(curPos + 1) == L'\\')
				{
					break;
				}
				else
					break;
			}
		}
		fclose(this->_File);
		this->_File = nullptr;
		return true;
	}
	void ConfigReader::ReadSection(WCHAR* sectionStart)
	{
		SectionBlock* block = new SectionBlock();
		WCHAR line[256];
		WCHAR* curPos;
		WCHAR* keyString = nullptr;
		WCHAR* valueString = nullptr;

		curPos = sectionStart;
		while (*curPos)
		{
			if (*curPos == L' ' || *curPos == L'\t' || *curPos == L'\n')
			{
				*curPos = L'\0';
				break;
			}
			curPos++;
		}
		block->_SectionName += sectionStart;

		fgetws(line, 256, this->_File);
		curPos = line;

		if (*curPos != this->SECTION_BEGIN_FLAG)
			return;

		while (true)
		{
			fgetws(line, 256, this->_File);
			curPos = line;

			if (*curPos == this->SECTION_END_FLAG)
				break;

			while (*curPos)
			{
				if (*curPos == L' ' || *curPos == L'\t')
				{
					curPos++;
					continue;
				}
				break;
			}

			keyString = curPos;
			valueString = nullptr;

			while (*curPos)
			{
				if (*curPos == L'=')
				{
					*curPos = L'\0';
					valueString = curPos + 1;
					break;
				}
				else if (*curPos == L' ' || *curPos == L'\t')
				{
					*curPos = L'\0';
				}
				else if (*curPos == L'/')
				{
					if (*(curPos + 1) == L'/')
					{
						valueString = nullptr;
						break;
					}
				}
				curPos++;
			}

			if (valueString != nullptr)
			{
				while (*valueString)
				{
					if (*valueString == L' ' || *valueString == L'\t' || *valueString == L'/')
					{
						*valueString = L'\0';
						valueString++;
						continue;
					}
					break;
				}
				curPos = valueString;

				while (*curPos)
				{
					if (*curPos == L' ' || *curPos == L'\t' || *curPos == L'\n')
					{
						*curPos = L'\0';
						break;
					}
					else if (*curPos == L'/')
					{
						if (*(curPos + 1) == L'/')
						{
							*curPos = L'\0';
							break;
						}
					}
					curPos++;
				}
				block->_ValueMap.insert(std::make_pair(keyString, valueString));
			}
		}
		this->_SectionMap.insert(std::make_pair(block->_SectionName, block));
	}

	bool ConfigReader::SetCurrentSection(const WCHAR* blockName)
	{
		auto iter = this->_SectionMap.find(blockName);

		if (iter == this->_SectionMap.end())
			return false;

		this->_Cursor = iter->second;

		return true;
	}
	bool ConfigReader::Find(const WCHAR* itemName)
	{
		if (this->_Cursor == NULL)
			return false;

		auto iter = this->_Cursor->_ValueMap.find(itemName);

		if (iter == this->_Cursor->_ValueMap.end())
			return false;

		return true;
	}
	int ConfigReader::Get(const WCHAR* itemName, int)
	{
		if (this->_Cursor == NULL)
		{
			CRASH();
			return -1;
		}
		auto iter = this->_Cursor->_ValueMap.find(itemName);
		if (iter == this->_Cursor->_ValueMap.end())
			CRASH();

		int ret = std::stoi(iter->second);

		return ret;
	}
	std::wstring ConfigReader::Get(const WCHAR* itemName)
	{
		if (this->_Cursor == NULL)
		{
			CRASH();
			return L"";
		}
		auto iter = this->_Cursor->_ValueMap.find(itemName);
		if (iter == this->_Cursor->_ValueMap.end())
			CRASH();

		return iter->second;
	}
}