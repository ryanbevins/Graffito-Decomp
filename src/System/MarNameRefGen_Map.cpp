#include <System/MarNameRefGen.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <Map/Map.hpp>
#include <Map/MapDraw.hpp>
#include <Map/Sky.hpp>
#include <Map/Shimmer.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/PollutionTest.hpp>
#include <Camera/sunmgr.hpp>
#include <Camera/SunModel.hpp>
#include <Map/MarineSnow.hpp>
#include <Map/StickyStainManager.hpp>
#include <Map/BathWaterManager.hpp>

const char* cSunVolumeName    = "/scene/sun";
const char* cSunsetVolumeName = "/scene/sunset";

JDrama::TNameRef* TMarNameRefGen::getNameRef_Map(const char* name) const
{
	if (strcmp(name, "Map") == 0)
		return new TMap("Map");
	if (strcmp(name, "MapDrawWall") == 0)
		return new TMapDrawWall("MapDrawWall");
	if (strcmp(name, "Sky") == 0)
		return new TSky("Sky");
	if (strcmp(name, "Shimmer") == 0)
		return new TShimmer("<Shimmer>");
	if (strcmp(name, "Pollution") == 0)
		return new TPollutionManager("Pollution");
	if (strcmp(name, "PollutionTest") == 0)
		return new TPollutionTest("PollutionTest");
	if (strcmp(name, "SunMgr") == 0)
		return new TSunMgr("<TSunMgr>");
	if (strcmp(name, "SunModel") == 0)
		return new TSunModel(false, "<TSunModel>");
	if (strcmp(name, "SunsetModel") == 0)
		return new TSunModel(true, "<TSunModel>");
	if (strcmp(name, "MarineSnow") == 0)
		return new TMarineSnow("MarineSnow");
	if (strcmp(name, "StickyStain") == 0)
		return new TStickyStainManager;
	if (strcmp(name, "BathWater") == 0)
		return new TBathWaterManager;
	return nullptr;
}
