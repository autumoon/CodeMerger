#pragma once

#include <windows.h>
#include <string>

namespace TextMerge
{
	std::string		WideToUtf8(const std::wstring& wstr);
	std::wstring	MultiByteToWide(const std::string& str, UINT codePage);
	bool			IsValidUtf8(const std::string& str);
	std::string		ConvertToUtf8(const std::string& data, std::wstring& encName);
}