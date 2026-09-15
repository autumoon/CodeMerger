#pragma once

#include <string>
#include <vector>
#include "Defines.h"
#include "Configure.h"

namespace InPlace
{
	struct Result
	{
		int nConverted;
		int nSkipped;
		int nFailed;
	};

	Result DoInPlaceConvert(const std::vector<_tstring>& vDirs,
		const config_s& _cfg,
		const _tstring& strReportPath);
}