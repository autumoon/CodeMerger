#pragma once

#include <windows.h>
#include <string>
#include <vector>

namespace TextMerge
{
	std::string		WideToUtf8(const std::wstring& wstr);
	std::wstring	MultiByteToWide(const std::string& str, UINT codePage);
	bool			IsValidUtf8(const std::string& str);
	std::string		ConvertToUtf8(const std::string& data, std::wstring& encName);
	bool			IsPathExcluded(const std::wstring& strFullPath,
						const std::vector<std::wstring>& vExcludeDirNames,
						const std::vector<std::wstring>& vExcludeFileNames);
}