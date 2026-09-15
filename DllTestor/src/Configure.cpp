//author:autumoon
//mail:autumoon@vip.qq.com
#include "Configure.h"

inline _tstring VectorToString(const std::vector<_tstring>& vStrings)
{
	_tstring strRes;

	const size_t num = vStrings.size();

	if (num > 0)
	{
		for (size_t i = 0; i < num - 1; ++i)
		{
			const _tstring& strCurItems = vStrings[i];
			strRes += strCurItems + _T('|');
		}

		strRes += vStrings[vStrings.size() - 1];
	}

	return strRes;
}

//解析后缀白名单字符串，支持逗号/分号分隔，自动补点并统一转小写
std::vector<_tstring> ParseExtensions(const _tstring& strSuffixs)
{
	std::vector<_tstring> vRes;

	if (strSuffixs.empty())
	{
		return vRes;
	}

	_tstring strCur = CStdStr::ReplaceAllDistinct(strSuffixs, _T(";"), _T(","));
	std::vector<_tstring> vTokens = CStdStr::Split(strCur, _T(","));

	for (size_t i = 0; i < vTokens.size(); ++i)
	{
		_tstring strItem = CStdStr::Trim(vTokens[i]);
		if (strItem.empty())
		{
			continue;
		}

		if (strItem[0] != _T('.'))
		{
			strItem = _T(".") + strItem;
		}

		strItem = CStdStr::ToUpperLower(strItem);

		if (!CStdTpl::VectorContains(vRes, strItem))
		{
			vRes.push_back(strItem);
		}
	}

	return vRes;
}

//后缀白名单转字符串（逗号分隔）
_tstring ExtensionsToString(const std::vector<_tstring>& vSuffixs)
{
	_tstring strRes;

	const size_t nSuffixNum = vSuffixs.size();
	for (size_t i = 0; i < nSuffixNum; ++i)
	{
		if (i > 0)
		{
			strRes += _T(",");
		}
		strRes += vSuffixs[i];
	}

	return strRes;
}

int ReadIniFile(const _tstring& strIniPath, config_s& _cfg)
{
	bool bRes = CStdFile::IfAccessFile(strIniPath.c_str());

	CSimpleIni Ini;
	Ini.SetUnicode();
	if (bRes)
	{
		//读取信息
		Ini.LoadFile(strIniPath.c_str());
		_cfg.bRemPath = Ini.GetBoolValue(INI_PRESUFFIX, INI_REMPATH, _cfg.bRemPath);

		_tstring strDstDirs = VectorToString(_cfg.vDstPaths);
		strDstDirs = Ini.GetValue(INI_PRESUFFIX, INI_DST_DIRS, strDstDirs.c_str());
		_cfg.vDstPaths = CStdStr::Split(strDstDirs, _T("|"));

		_tstring strAllItems = VectorToString(_cfg.vItemPaths);
		strAllItems = Ini.GetValue(INI_PRESUFFIX, INI_ALL_ITEMS, strAllItems.c_str());
		_cfg.vItemPaths = CStdStr::Split(strAllItems, _T("|"));

		//后缀白名单（Extensions），无该键时使用默认列表
		_tstring strSuffixs = Ini.GetValue(INI_PRESUFFIX, INI_EXTENSIONS, _T(""));
		std::vector<_tstring> vParsedSuffixs = ParseExtensions(strSuffixs);
		if (!vParsedSuffixs.empty())
		{
			_cfg.vSuffixs = vParsedSuffixs;
		}

		//单文件大小上限（MB），默认2
		_cfg.nMaxFileSizeMB = (int)Ini.GetLongValue(INI_PRESUFFIX, INI_MAX_FILE_SIZE_MB, _cfg.nMaxFileSizeMB);
		if (_cfg.nMaxFileSizeMB <= 0)
		{
			_cfg.nMaxFileSizeMB = 2;
		}

		//原地转码设置
		_cfg.bInPlaceConvert = Ini.GetBoolValue(INI_PRESUFFIX, INI_ENABLE_INPLACE_CONVERT, _cfg.bInPlaceConvert);
		_cfg.bInPlaceUtf8Bom = Ini.GetBoolValue(INI_PRESUFFIX, INI_INPLACE_UTF8_BOM, _cfg.bInPlaceUtf8Bom);
		_cfg.bInPlaceDryRun = Ini.GetBoolValue(INI_PRESUFFIX, INI_INPLACE_DRYRUN, _cfg.bInPlaceDryRun);

		_tstring strInPlaceExt = Ini.GetValue(INI_PRESUFFIX, INI_INPLACE_EXTENSIONS, _T(""));
		std::vector<_tstring> vParsedInPlaceExt = ParseExtensions(strInPlaceExt);
		if (!vParsedInPlaceExt.empty())
		{
			_cfg.vInPlaceSuffixs = vParsedInPlaceExt;
		}

		_cfg.nWindowWidth = Ini.GetLongValue(INI_PRESUFFIX, INI_WIN_WIDTH, _cfg.nWindowWidth);
		_cfg.nWindowHeight = Ini.GetLongValue(INI_PRESUFFIX, INI_WIN_HEIGHT, _cfg.nWindowHeight);
	}
	else
	{
		WriteIniFile(strIniPath, _cfg);
	}
	Ini.Reset();

	return 0;
}

int WriteIniFile(const _tstring& strIniPath, const config_s& _cfg)
{
	CSimpleIni Ini;
	Ini.SetUnicode();

	Ini.SetBoolValue(INI_PRESUFFIX, INI_REMPATH, _cfg.bRemPath);
	Ini.SetValue(INI_PRESUFFIX, INI_DST_DIRS, VectorToString(_cfg.vDstPaths).c_str());
	Ini.SetValue(INI_PRESUFFIX, INI_ALL_ITEMS, VectorToString(_cfg.vItemPaths).c_str());
	Ini.SetValue(INI_PRESUFFIX, INI_EXTENSIONS, ExtensionsToString(_cfg.vSuffixs).c_str());
	Ini.SetLongValue(INI_PRESUFFIX, INI_MAX_FILE_SIZE_MB, _cfg.nMaxFileSizeMB);
	Ini.SetBoolValue(INI_PRESUFFIX, INI_ENABLE_INPLACE_CONVERT, _cfg.bInPlaceConvert);
	Ini.SetValue(INI_PRESUFFIX, INI_INPLACE_EXTENSIONS, ExtensionsToString(_cfg.vInPlaceSuffixs).c_str());
	Ini.SetBoolValue(INI_PRESUFFIX, INI_INPLACE_UTF8_BOM, _cfg.bInPlaceUtf8Bom);
	Ini.SetBoolValue(INI_PRESUFFIX, INI_INPLACE_DRYRUN, _cfg.bInPlaceDryRun);
	Ini.SetLongValue(INI_PRESUFFIX, INI_WIN_WIDTH, _cfg.nWindowWidth);
	Ini.SetLongValue(INI_PRESUFFIX, INI_WIN_HEIGHT, _cfg.nWindowHeight);

	Ini.SaveFile(strIniPath.c_str());
	Ini.Reset();

	return 0;
}
