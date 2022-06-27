#pragma once
#ifndef __CONFIG_PARSER__
#define __CONFIG_PARSER__

#include <unordered_map>
#include <string>
#include <Windows.h>




namespace univ_dev
{
	class ConfigReader
	{
	private:
		constexpr static WCHAR SECTION_FLAG = L':';
		constexpr static WCHAR SECTION_BEGIN_FLAG = L'{';
		constexpr static WCHAR SECTION_END_FLAG = L'}';
		struct SectionBlock
		{
			std::wstring _SectionName;
			std::unordered_map<std::wstring, std::wstring> _ValueMap;
		};
	public:
		ConfigReader();
		bool init(const WCHAR* filename);
		void ReadSection(WCHAR* sectionStart);
		bool SetCurrentSection(const WCHAR* blockName);
		bool Find(const WCHAR* itemName);
		std::wstring Get(const WCHAR* itemName);
		int Get(const WCHAR* itemName, int);

	private:
		FILE* _File;

		std::unordered_map<std::wstring, SectionBlock*> _SectionMap;
		SectionBlock* _Cursor;
	};
	extern ConfigReader g_ConfigReader;
}

#endif // !__CONFIG_PARSER__



