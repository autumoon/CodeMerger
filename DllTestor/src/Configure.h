//author:autumoon
//mail:autumoon@vip.qq.com

#pragma once

#include <tchar.h>
#include "Defines.h"
#include "SimpleIni.h"
#include "StdStrFile.h"

typedef struct _config_s 
{
	bool bRemPath;
	int nWindowWidth;
	int nWindowHeight;
	int nMaxFileSizeMB;
	std::vector<_tstring> vDstPaths;
	std::vector<_tstring> vItemPaths;
	std::vector<_tstring> vSuffixs;
	bool bInPlaceConvert;
	std::vector<_tstring> vInPlaceSuffixs;
	bool bInPlaceUtf8Bom;
	bool bInPlaceDryRun;

	_config_s()
	{
		//内部初始化
		bRemPath = true;
		nWindowWidth = 640;
		nWindowHeight = 480;
		nMaxFileSizeMB = 2;
		//默认后缀白名单
		static const TCHAR* aszDefaultSuffix[] = {
			_T(".txt"), _T(".md"), _T(".h"), _T(".hpp"), _T(".c"), _T(".cpp"),
			_T(".cs"), _T(".py"), _T(".cc"), _T(".cxx"), _T(".inl"), _T(".js"),
			_T(".ts"), _T(".java"), _T(".go"), _T(".rs"), _T(".json"), _T(".xml"),
			_T(".yml"), _T(".yaml"), _T(".html"), _T(".htm"), _T(".css"), _T(".sh"),
			_T(".bat"), _T(".ps1"), _T(".sql"), _T(".ini"), _T(".cfg"), _T(".log"),
			_T(".csv")
		};
		const int nSuffixNum = sizeof(aszDefaultSuffix) / sizeof(aszDefaultSuffix[0]);
		for (int i = 0; i < nSuffixNum; ++i)
		{
			vSuffixs.push_back(aszDefaultSuffix[i]);
		}

		bInPlaceConvert = false;
		bInPlaceUtf8Bom = true;
		bInPlaceDryRun = false;
		vInPlaceSuffixs.push_back(_T(".c"));
		vInPlaceSuffixs.push_back(_T(".cpp"));
		vInPlaceSuffixs.push_back(_T(".h"));
		vInPlaceSuffixs.push_back(_T(".hpp"));
		vInPlaceSuffixs.push_back(_T(".cs"));
	}

}config_s;

int ReadIniFile(const _tstring& strIniPath, config_s& _cfg);
int WriteIniFile(const _tstring& strIniPath, const config_s& _cfg);

_tstring VectorToString(const std::vector<_tstring>& vStrings);
std::vector<_tstring> ParseExtensions(const _tstring& strSuffixs);
_tstring ExtensionsToString(const std::vector<_tstring>& vSuffixs);

//_tstring GetIniPath()
//{
//TCHAR chpath[MAX_PATH];
//GetModuleFileName(NULL, chpath, sizeof(chpath));

//_tstring strModulePath = CMfcStrFile::CString2string(chpath);
//_tstring strIniPath = CStdStr::ReplaceSuffix(strModulePath, _T(".ini"));

//return strIniPath;
//}