#pragma once
#include <format>
namespace DofusDB
{
	struct DBInfos
	{
		int x, y, direction, exit;
		::std::string hint;

	};
	::std::string DBInfos2json(DBInfos infos);
}