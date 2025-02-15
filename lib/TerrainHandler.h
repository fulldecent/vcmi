/*
 * TerrainHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/EntityService.h>
#include <vcmi/Entity.h>
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "Color.h"
#include "filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE TerrainPaletteAnimation
{
	/// index of first color to cycle
	int32_t start;
	/// total numbers of colors to cycle
	int32_t length;

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & start;
		h & length;
	}
};


class DLL_LINKAGE TerrainType : public EntityT<TerrainId>
{
	friend class TerrainTypeHandler;
	std::string identifier;
	std::string modScope;
	TerrainId id;
	ui8 passabilityType;

	enum PassabilityType : ui8
	{
		LAND = 1,
		WATER = 2,
		SURFACE = 4,
		SUBTERRANEAN = 8,
		ROCK = 16
	};

public:
	int32_t getIndex() const override { return id.getNum(); }
	int32_t getIconIndex() const override { return 0; }
	std::string getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override {}
	TerrainId getId() const override { return id;}
	void updateFrom(const JsonNode & data) {};

	std::string getNameTextID() const override;
	std::string getNameTranslated() const override;

	std::vector<BattleField> battleFields;
	std::vector<TerrainId> prohibitTransitions;
	ColorRGBA minimapBlocked;
	ColorRGBA minimapUnblocked;
	std::string shortIdentifier;
	AudioPath musicFilename;
	AnimationPath tilesFilename;
	std::string terrainViewPatterns;
	AudioPath horseSound;
	AudioPath horseSoundPenalty;

	std::vector<TerrainPaletteAnimation> paletteAnimation;

	TerrainId rockTerrain;
	RiverId river;
	int moveCost;
	bool transitionRequired;

	TerrainType() = default;

	bool isLand() const;
	bool isWater() const;
	bool isPassable() const;
	bool isSurface() const;
	bool isUnderground() const;
	bool isTransitionRequired() const;
	bool isSurfaceCartographerCompatible() const;
	bool isUndergroundCartographerCompatible() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & battleFields;
		h & prohibitTransitions;
		h & minimapBlocked;
		h & minimapUnblocked;
		h & modScope;
		h & identifier;
		h & musicFilename;
		h & tilesFilename;
		h & shortIdentifier;
		h & terrainViewPatterns;
		h & rockTerrain;
		h & river;
		h & paletteAnimation;

		h & id;
		h & moveCost;
		h & horseSound;
		h & horseSoundPenalty;
		h & passabilityType;
		h & transitionRequired;
	}
};

class DLL_LINKAGE TerrainTypeService : public EntityServiceT<TerrainId, TerrainType>
{
public:
};

class DLL_LINKAGE TerrainTypeHandler : public CHandlerBase<TerrainId, TerrainType, TerrainType, TerrainTypeService>
{
public:
	virtual TerrainType * loadFromJson(
		const std::string & scope,
		const JsonNode & json,
		const std::string & identifier,
		size_t index) override;

	virtual const std::vector<std::string> & getTypeNames() const override;
	virtual std::vector<JsonNode> loadLegacyData() override;
	virtual std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objects;
	}
};

VCMI_LIB_NAMESPACE_END
