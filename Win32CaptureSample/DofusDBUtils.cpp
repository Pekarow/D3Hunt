#include "DofusDBUtils.h"

::std::string DofusDB::DBInfos2json(DofusDB::DBInfos infos)
{
	return ::std::format("{{"
		"\"x\":{},"
		"\"y\":{},"
		"\"direction\":{},"
		"\"exit\":{},"
		"\"hint\":\"{}\""
		"}}", infos.x, infos.y, infos.direction, infos.exit, infos.hint);
}