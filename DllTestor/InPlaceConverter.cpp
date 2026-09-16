#include "stdafx.h"
#include "InPlaceConverter.h"
#include "TextMergeHelper.h"
#include <fstream>

namespace InPlace
{
	static _tstring GetFileExtensionLower(const _tstring& strPath)
	{
		size_t nDot = strPath.rfind(_T('.'));
		if (nDot == _tstring::npos || nDot == 0)
		{
			return _tstring();
		}

		_tstring strExt = strPath.substr(nDot);
		return CStdStr::ToUpperLower(strExt);
	}

	static bool IsTargetSuffix(const _tstring& strExt,
		const std::vector<_tstring>& vSuffixs)
	{
		for (size_t i = 0; i < vSuffixs.size(); ++i)
		{
			if (strExt == vSuffixs[i])
			{
				return true;
			}
		}
		return false;
	}

	static bool HasHiddenOrSystem(const _tstring& strPath)
	{
		DWORD dwAttr = ::GetFileAttributesW(strPath.c_str());
		if (dwAttr == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
		return (dwAttr & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) != 0;
	}

	static bool IsReadOnly(const _tstring& strPath)
	{
		DWORD dwAttr = ::GetFileAttributesW(strPath.c_str());
		if (dwAttr == INVALID_FILE_ATTRIBUTES)
		{
			return false;
		}
		return (dwAttr & FILE_ATTRIBUTE_READONLY) != 0;
	}

	static bool ReadAllBytes(const _tstring& strPath, std::string& out)
	{
		HANDLE hFile = ::CreateFileW(strPath.c_str(),
			GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		LARGE_INTEGER liSize;
		if (!::GetFileSizeEx(hFile, &liSize))
		{
			::CloseHandle(hFile);
			return false;
		}

		DWORD dwSize = (DWORD)liSize.QuadPart;
		out.resize(dwSize);

		DWORD dwTotal = 0;
		while (dwTotal < dwSize)
		{
			DWORD dwRead = 0;
			if (!::ReadFile(hFile, &out[dwTotal], dwSize - dwTotal, &dwRead, NULL) || dwRead == 0)
			{
				break;
			}
			dwTotal += dwRead;
		}
		::CloseHandle(hFile);

		return dwTotal == dwSize;
	}

	static bool WriteAllBytes(const _tstring& strPath, const std::string& data)
	{
		HANDLE hFile = ::CreateFileW(strPath.c_str(),
			GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			return false;
		}

		DWORD dwWritten = 0;
		DWORD dwTotal = 0;
		const char* pData = data.c_str();
		DWORD dwSize = (DWORD)data.size();

		while (dwTotal < dwSize)
		{
			if (!::WriteFile(hFile, pData + dwTotal, dwSize - dwTotal, &dwWritten, NULL))
			{
				break;
			}
			dwTotal += dwWritten;
		}
		::FlushFileBuffers(hFile);
		::CloseHandle(hFile);

		return dwTotal == dwSize;
	}

	enum ConvertResult { CR_CONVERTED, CR_SKIPPED, CR_FAILED };

	static ConvertResult ConvertOneFile(const _tstring& src,
		const config_s& _cfg,
		std::ofstream& report)
	{
		std::string data;
		if (!ReadAllBytes(src, data))
		{
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (open/read failed)" << std::endl;
			return CR_FAILED;
		}

		std::wstring wEncName;
		std::string utf8 = TextMerge::ConvertToUtf8(data, wEncName);
		if (wEncName == L"未知编码")
		{
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (unknown encoding)" << std::endl;
			return CR_FAILED;
		}

		std::string target = utf8;
		if (_cfg.bInPlaceUtf8Bom)
		{
			target = std::string("\xEF\xBB\xBF", 3) + target;
		}

		if (target == data)
		{
			report << "  [SKIP] " << TextMerge::WideToUtf8(src) << " (already target encoding)" << std::endl;
			return CR_SKIPPED;
		}

		if (_cfg.bInPlaceDryRun)
		{
			std::string bomStr = _cfg.bInPlaceUtf8Bom ? "yes" : "no";
			report << "  [DRY-RUN] would convert: "
				<< TextMerge::WideToUtf8(wEncName)
				<< " -> UTF-8(BOM: " << bomStr << ")"
				<< "  " << TextMerge::WideToUtf8(src) << std::endl;
			return CR_CONVERTED;
		}

		if (IsReadOnly(src))
		{
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (readonly)" << std::endl;
			return CR_FAILED;
		}

		_tstring strBak;
		for (int idx = 0; idx < 100; ++idx)
		{
			_tstring strTry;
			if (idx == 0)
			{
				strTry = src + L".bak";
			}
			else
			{
				strTry = src + L".bak." + std::to_wstring(idx);
			}

			DWORD dwAttr = ::GetFileAttributesW(strTry.c_str());
			if (dwAttr == INVALID_FILE_ATTRIBUTES)
			{
				strBak = strTry;
				break;
			}
		}

		if (strBak.empty())
		{
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (backup index exhausted)" << std::endl;
			return CR_FAILED;
		}

		if (!::CopyFileW(src.c_str(), strBak.c_str(), FALSE))
		{
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (backup copy failed)" << std::endl;
			return CR_FAILED;
		}

		_tstring strTmp = src + L".tmp";
		::DeleteFileW(strTmp.c_str());

		if (!WriteAllBytes(strTmp, target))
		{
			::DeleteFileW(strTmp.c_str());
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (write tmp failed)" << std::endl;
			return CR_FAILED;
		}

		if (!::MoveFileExW(strTmp.c_str(), src.c_str(),
			MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
		{
			::DeleteFileW(strTmp.c_str());
			report << "  [FAIL] " << TextMerge::WideToUtf8(src) << " (move failed)" << std::endl;
			return CR_FAILED;
		}

		std::string bomStr = _cfg.bInPlaceUtf8Bom ? "yes" : "no";
		report << "  [OK] " << TextMerge::WideToUtf8(wEncName)
			<< " -> UTF-8 (BOM: " << bomStr << ")"
			<< ", size " << data.size() << " -> " << target.size()
			<< ", backup: " << TextMerge::WideToUtf8(strBak) << std::endl;

		return CR_CONVERTED;
	}

	Result DoInPlaceConvert(const std::vector<_tstring>& vDirs,
		const config_s& _cfg,
		const _tstring& strReportPath)
	{
		Result r = { 0, 0, 0 };

		std::ofstream report(strReportPath.c_str(), std::ios::binary | std::ios::trunc);
		if (!report.is_open())
		{
			return r;
		}

		std::string sHeader = TextMerge::WideToUtf8(L"# InPlaceConvertReport\r\n");
		sHeader += TextMerge::WideToUtf8(L"# Mode: DryRun=");
		sHeader += _cfg.bInPlaceDryRun ? "1" : "0";
		sHeader += TextMerge::WideToUtf8(L", TargetBOM=");
		sHeader += _cfg.bInPlaceUtf8Bom ? "1" : "0";
		sHeader += "\r\n";
		sHeader += TextMerge::WideToUtf8(L"# Extensions: ");
		sHeader += TextMerge::WideToUtf8(ExtensionsToString(_cfg.vInPlaceSuffixs));
		sHeader += "\r\n";
		sHeader += TextMerge::WideToUtf8(L"# 开始时间: ");
		{
			SYSTEMTIME st;
			::GetLocalTime(&st);
			wchar_t szTime[64];
			swprintf_s(szTime, L"%04d-%02d-%02d %02d:%02d:%02d",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			sHeader += TextMerge::WideToUtf8(szTime);
		}
		sHeader += "\r\n\r\n";
		report.write(sHeader.c_str(), (std::streamsize)sHeader.size());

		std::vector<_tstring> vFailed;

		for (size_t d = 0; d < vDirs.size(); ++d)
		{
			_tstring strDir = CStdStr::AddSlashIfNeeded(vDirs[d]);

			std::vector<_tstring> vStack;
			vStack.push_back(strDir);

			while (!vStack.empty())
			{
				_tstring strCur = vStack.back();
				vStack.pop_back();

				_tstring strSearch = strCur + L"*";

				WIN32_FIND_DATAW fd;
				HANDLE hFind = ::FindFirstFileW(strSearch.c_str(), &fd);
				if (hFind == INVALID_HANDLE_VALUE)
				{
					continue;
				}

				do
				{
					_tstring strName = fd.cFileName;

					if (strName == L"." || strName == L"..")
					{
						continue;
					}

					_tstring strFull = strCur + strName;

					if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						//排除指定目录，不进入递归
						_tstring strDirNameLow = CStdStr::ToUpperLower(strName);
						if (CStdTpl::VectorContains(_cfg.vExcludeDirNames, strDirNameLow))
						{
							continue;
						}
						vStack.push_back(CStdStr::AddSlashIfNeeded(strFull));
						continue;
					}

					if (HasHiddenOrSystem(strFull))
					{
						++r.nSkipped;
						continue;
					}

					//按名称排除文件（不计入任何计数）
					if (TextMerge::IsPathExcluded(strFull,
							_cfg.vExcludeDirNames, _cfg.vExcludeFileNames))
					{
						continue;
					}

					_tstring strExt = GetFileExtensionLower(strFull);
					if (!IsTargetSuffix(strExt, _cfg.vInPlaceSuffixs))
					{
						continue;
					}

					ConvertResult cr = ConvertOneFile(strFull, _cfg, report);

					switch (cr)
					{
					case CR_CONVERTED:
						++r.nConverted;
						break;
					case CR_SKIPPED:
						++r.nSkipped;
						break;
					case CR_FAILED:
						++r.nFailed;
						vFailed.push_back(strFull);
						break;
					}

				} while (::FindNextFileW(hFind, &fd));

				::FindClose(hFind);
			}
		}

		std::string sFooter = "\r\n";
		sFooter += TextMerge::WideToUtf8(L"# 结束时间: ");
		{
			SYSTEMTIME st;
			::GetLocalTime(&st);
			wchar_t szTime[64];
			swprintf_s(szTime, L"%04d-%02d-%02d %02d:%02d:%02d",
				st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
			sFooter += TextMerge::WideToUtf8(szTime);
		}
		sFooter += "\r\n";
		sFooter += TextMerge::WideToUtf8(L"# 汇总: converted=");
		sFooter += std::to_string(r.nConverted);
		sFooter += TextMerge::WideToUtf8(L", skipped=");
		sFooter += std::to_string(r.nSkipped);
		sFooter += TextMerge::WideToUtf8(L", failed=");
		sFooter += std::to_string(r.nFailed);
		sFooter += "\r\n";

		if (!vFailed.empty())
		{
			sFooter += TextMerge::WideToUtf8(L"\r\n# 失败清单:\r\n");
			for (size_t i = 0; i < vFailed.size(); ++i)
			{
				sFooter += "  " + TextMerge::WideToUtf8(vFailed[i]) + "\r\n";
			}
		}

		report.write(sFooter.c_str(), (std::streamsize)sFooter.size());
		report.close();

		return r;
	}
}