#include "stdafx.h"
#include "TextMergeHelper.h"
#include "StdStrFile.h"

namespace TextMerge
{
	std::string WideToUtf8(const std::wstring& wstr)
	{
		if (wstr.empty())
		{
			return std::string();
		}

		const int nUtf8Len = ::WideCharToMultiByte(CP_UTF8, 0,
			wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
		if (nUtf8Len <= 0)
		{
			return std::string();
		}

		std::string strUtf8;
		strUtf8.resize(nUtf8Len);
		::WideCharToMultiByte(CP_UTF8, 0,
			wstr.c_str(), (int)wstr.length(), &strUtf8[0], nUtf8Len, NULL, NULL);

		return strUtf8;
	}

	std::wstring MultiByteToWide(const std::string& str, UINT codePage)
	{
		if (str.empty())
		{
			return std::wstring();
		}

		const int nWideLen = ::MultiByteToWideChar(codePage, 0,
			str.c_str(), (int)str.length(), NULL, 0);
		if (nWideLen <= 0)
		{
			return std::wstring();
		}

		std::wstring strWide;
		strWide.resize(nWideLen);
		::MultiByteToWideChar(codePage, 0,
			str.c_str(), (int)str.length(), &strWide[0], nWideLen);

		return strWide;
	}

	bool IsValidUtf8(const std::string& str)
	{
		//含NUL字节的通常不是普通UTF-8文本（更可能是UTF-16或二进制）
		if (str.find('\0') != std::string::npos)
		{
			return false;
		}

		const unsigned char* pData = (const unsigned char*)str.c_str();
		const size_t nLen = str.length();
		size_t nPos = 0;

		while (nPos < nLen)
		{
			const unsigned char cLead = pData[nPos];

			if (cLead < 0x80)
			{
				++nPos;
				continue;
			}

			int nRemain = 0;
			unsigned int uCode = 0;

			if ((cLead & 0xE0) == 0xC0)
			{
				//2字节序列：110xxxxx 10xxxxxx
				nRemain = 1;
				uCode = cLead & 0x1F;
				if (uCode < 0x02)		//禁止过长（0x00~0x7F）
				{
					return false;
				}
			}
			else if ((cLead & 0xF0) == 0xE0)
			{
				//3字节序列：1110xxxx 10xxxxxx 10xxxxxx
				nRemain = 2;
				uCode = cLead & 0x0F;
			}
			else if ((cLead & 0xF8) == 0xF0)
			{
				//4字节序列：11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
				nRemain = 3;
				uCode = cLead & 0x07;
			}
			else
			{
				return false;
			}

			//后续字节是否足够
			if (nPos + (size_t)nRemain >= nLen)
			{
				return false;
			}

			for (int i = 1; i <= nRemain; ++i)
			{
				const unsigned char cCont = pData[nPos + i];
				if ((cCont & 0xC0) != 0x80)
				{
					return false;
				}
				uCode = (uCode << 6) | (cCont & 0x3F);
			}

			//统一检查最小长度，禁止过长编码
			if (nRemain == 1 && uCode < 0x80)		//2字节过长（本应≤7位）
			{
				return false;
			}
			if (nRemain == 2 && uCode < 0x800)		//3字节过长（本应≤11位）
			{
				return false;
			}
			if (nRemain == 3 && uCode < 0x10000)	//4字节过长（本应≤16位）
			{
				return false;
			}

			//禁止代理区以及超出U+10FFFF
			if ((uCode >= 0xD800 && uCode <= 0xDFFF) || uCode > 0x10FFFF)
			{
				return false;
			}

			nPos += (size_t)nRemain + 1;
		}

		return true;
	}

	//按UTF-16编码页将字节内容转换为宽字符（绝不返回空：失败时手工拼wchar_t）
	static std::wstring UTF16ToWide(const std::string& str, bool bBigEndian)
	{
		const UINT nCodePage = bBigEndian ? 1201 : 1200;
		const int nBytes = (int)(str.length() & ~(size_t)1);
		if (nBytes <= 0)
		{
			return std::wstring();
		}

		const int nWideLen = ::MultiByteToWideChar(nCodePage, 0,
			str.c_str(), nBytes, NULL, 0);

		std::wstring strWide;
		if (nWideLen > 0)
		{
			strWide.resize(nWideLen);
			::MultiByteToWideChar(nCodePage, 0, str.c_str(), nBytes,
				&strWide[0], nWideLen);
		}

		//兜底：若 MultiByteToWideChar 失败或没填满，手工按 2 字节一对拼 wchar_t
		const size_t nExpected = (size_t)nBytes / 2;
		if (strWide.size() < nExpected)
		{
			strWide.clear();
			strWide.reserve(nExpected);
			for (size_t i = 0; i + 1 < (size_t)nBytes; i += 2)
			{
				wchar_t wc;
				if (!bBigEndian)
				{
					wc = (wchar_t)((unsigned char)str[i]
						| ((unsigned char)str[i + 1] << 8));
				}
				else
				{
					wc = (wchar_t)(((unsigned char)str[i] << 8)
						| (unsigned char)str[i + 1]);
				}
				strWide.push_back(wc);
			}
		}

		return strWide;
	}

	std::string ConvertToUtf8(const std::string& data, std::wstring& encName)
	{
		encName.clear();

		if (data.empty())
		{
			encName = L"UTF-8";
			return std::string();
		}

		//UTF-8 BOM
		if (data.length() >= 3
			&& (unsigned char)data[0] == 0xEF
			&& (unsigned char)data[1] == 0xBB
			&& (unsigned char)data[2] == 0xBF)
		{
			encName = L"UTF-8 (BOM)";
			return data.substr(3);
		}

		//UTF-16 LE BOM
		if (data.length() >= 2
			&& (unsigned char)data[0] == 0xFF
			&& (unsigned char)data[1] == 0xFE)
		{
			if (data.length() == 2)
			{
				encName = L"UTF-16 LE";
				return std::string();
			}

			std::wstring strWide = UTF16ToWide(data.substr(2), false);
			if (!strWide.empty())
			{
				encName = L"UTF-16 LE";
				return WideToUtf8(strWide);
			}

			encName = L"未知编码";
			return data;
		}

		//UTF-16 BE BOM
		if (data.length() >= 2
			&& (unsigned char)data[0] == 0xFE
			&& (unsigned char)data[1] == 0xFF)
		{
			if (data.length() == 2)
			{
				encName = L"UTF-16 BE";
				return std::string();
			}

			std::wstring strWide = UTF16ToWide(data.substr(2), true);
			if (!strWide.empty())
			{
				encName = L"UTF-16 BE";
				return WideToUtf8(strWide);
			}

			encName = L"未知编码";
			return data;
		}

		//无BOM的UTF-16启发式检测
		size_t nSample = data.length() > 4096 ? 4096 : data.length();
		size_t nZero = 0;
		for (size_t i = 0; i < nSample; ++i)
		{
			if ((unsigned char)data[i] == 0x00)
			{
				++nZero;
			}
		}
		//0x00占比：nZero*100/nSample>=20 满足“>=20%”，>=30 满足“>=30%”

		const bool bEvenLen = (data.length() & 1) == 0;
		if (data.length() >= 4 && bEvenLen && nZero * 100 / nSample >= 20)
		{
			const size_t nCheckPairs = data.length() / 2;
			const size_t nStatPairs = nCheckPairs > 512 ? 512 : nCheckPairs;
			size_t nEvenZero = 0;
			size_t nOddZero = 0;

			for (size_t i = 0; i < nStatPairs; ++i)
			{
				if ((unsigned char)data[i * 2] == 0x00)
				{
					++nEvenZero;
				}
				if ((unsigned char)data[i * 2 + 1] == 0x00)
				{
					++nOddZero;
				}
			}

#ifdef _DEBUG
			OutputDebugStringW(
				(L"[TextMerge] UTF16 heuristic: nStatPairs=" +
					std::to_wstring(nStatPairs) +
					L", nEvenZero=" + std::to_wstring(nEvenZero) +
					L", nOddZero=" + std::to_wstring(nOddZero) +
					L"\n").c_str());
#endif // _DEBUG

			//UTF-16 LE（无BOM）：低字节在前，奇数位置是 0x00
			if (nOddZero * 10 >= nStatPairs * 9 && nEvenZero * 10 <= nStatPairs * 1)
			{
				std::wstring strWide = UTF16ToWide(data, false);
				if (!strWide.empty())
				{
					encName = L"UTF-16 LE (无BOM)";
					return WideToUtf8(strWide);
				}
			}

			//UTF-16 BE（无BOM）：高字节在前，偶数位置是 0x00
			if (nEvenZero * 10 >= nStatPairs * 9 && nOddZero * 10 <= nStatPairs * 1)
			{
				std::wstring strWide = UTF16ToWide(data, true);
				if (!strWide.empty())
				{
					encName = L"UTF-16 BE (无BOM)";
					return WideToUtf8(strWide);
				}
			}

			//兜底：0x00占比>=30%且长度偶数，直接按 UTF-16 LE 试一次
			if (nZero * 100 / nSample >= 30)
			{
				std::wstring strWide = UTF16ToWide(data, false);
				if (!strWide.empty())
				{
					encName = L"UTF-16 LE (无BOM)";
					return WideToUtf8(strWide);
				}
			}
		}

		//无BOM：严格校验是否为合法UTF-8
		if (IsValidUtf8(data))
		{
			encName = L"UTF-8";
			return data;
		}

		//否则按ANSI（CP_ACP / GBK）转换
		std::wstring strWide = MultiByteToWide(data, CP_ACP);
		if (!strWide.empty())
		{
			encName = L"ANSI/GBK";
			return WideToUtf8(strWide);
		}

		encName = L"未知编码";
		return data;
	}

	bool IsPathExcluded(const std::wstring& strFullPath,
		const std::vector<std::wstring>& vExcludeDirNames,
		const std::vector<std::wstring>& vExcludeFileNames)
	{
		if (vExcludeDirNames.empty() && vExcludeFileNames.empty())
		{
			return false;
		}

		// 1) 提取文件名（最后一段）
		size_t nLastSlash = strFullPath.find_last_of(L"\\/");
		std::wstring strName = (nLastSlash == std::wstring::npos)
			? strFullPath
			: strFullPath.substr(nLastSlash + 1);

		// 2) 文件名匹配
		if (!vExcludeFileNames.empty() && !strName.empty())
		{
			std::wstring strNameLower = CStdStr::ToUpperLower(strName);
			for (size_t i = 0; i < vExcludeFileNames.size(); ++i)
			{
				if (strNameLower == vExcludeFileNames[i])
				{
					return true;
				}
			}
		}

		// 3) 目录段匹配：对最后一段（文件名）之前的所有路径段逐一比对
		if (!vExcludeDirNames.empty())
		{
			size_t nEnd = (nLastSlash == std::wstring::npos)
				? strFullPath.size()
				: nLastSlash;

			size_t nStart = 0;
			while (nStart < nEnd)
			{
				size_t nSep = strFullPath.find_first_of(L"\\/", nStart);
				if (nSep == std::wstring::npos || nSep >= nEnd)
				{
					nSep = nEnd;
				}

				if (nSep > nStart)
				{
					std::wstring strSeg = strFullPath.substr(nStart, nSep - nStart);
					std::wstring strSegLower = CStdStr::ToUpperLower(strSeg);
					for (size_t i = 0; i < vExcludeDirNames.size(); ++i)
					{
						if (strSegLower == vExcludeDirNames[i])
						{
							return true;
						}
					}
				}

				nStart = nSep + 1;
			}
		}

		return false;
	}
}