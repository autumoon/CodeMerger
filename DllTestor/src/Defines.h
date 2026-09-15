#pragma once

//author:autumoon
//2018.01.18
//xcopy /d /y .\src\*.dll $(TargetDir)

#define INI_PRESUFFIX		_T("INI_PRESUFFIX")
#define INI_REMPATH			_T("INI_REMPATH")
#define INI_ALL_ITEMS		_T("INI_ALL_ITEMS")
#define INI_DST_DIRS		_T("INI_DST_DIRS")
#define INI_EXTENSIONS		_T("Extensions")
#define INI_MAX_FILE_SIZE_MB	_T("MaxFileSizeMB")
#define INI_WIN_WIDTH		_T("INI_WIN_WIDTH")
#define INI_WIN_HEIGHT		_T("INI_WIN_HEIGHT")
#define INI_ENABLE_INPLACE_CONVERT	_T("EnableInPlaceConvert")
#define INI_INPLACE_EXTENSIONS		_T("InPlaceExtensions")
#define INI_INPLACE_UTF8_BOM		_T("InPlaceUtf8Bom")
#define INI_INPLACE_DRYRUN		_T("InPlaceDryRun")
#define INI_ISOLATE_PER_DIR	_T("IsolatePerDir")

#ifdef _UNICODE

#ifndef _tstring
#define _tstring std::wstring
#endif // _tstring

#else

#ifndef _tstring
#define _tstring std::string
#endif // _tstring

#endif
