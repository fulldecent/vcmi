/*
* ShrineInstanceConstructor.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "CDefaultObjectTypeHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGShrine;

class ShrineInstanceConstructor final : public CDefaultObjectTypeHandler<CGShrine>
{
	JsonNode parameters;

protected:
	void initTypeData(const JsonNode & config) override;
	void randomizeObject(CGShrine * object, CRandomGenerator & rng) const override;

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<AObjectTypeHandler&>(*this);
		h & parameters;
	}
};

VCMI_LIB_NAMESPACE_END
