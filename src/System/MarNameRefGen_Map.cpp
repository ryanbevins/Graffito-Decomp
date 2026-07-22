#define JDRAMA_NO_INLINE_BASE_CTORS
#include <System/MarNameRefGen.hpp>
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

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[]
    = "MActorMtxCalcType_User ユーザー定義";

const char* cSunVolumeName    = "/scene/sun";
const char* cSunsetVolumeName = "/scene/sunset";

JDrama::TNameRef* TMarNameRefGen::getNameRef_Map(const char* name) const
{
	if (strcmp(name, "Map") == 0)
		return new TMap("\x83\x7d\x83\x62\x83\x76");
	if (strcmp(name, "MapDrawWall") == 0)
		return new TMapDrawWall("\x83\x4a\x83\x81\x83\x89\x90\x48\x82\xa2\x8d\x9e\x82\xdd\x95\xc7");
	if (strcmp(name, "Sky") == 0)
		return new TSky("\x8b\xf3");
	if (strcmp(name, "Shimmer") == 0)
		return new TShimmer("<Shimmer>");
	if (strcmp(name, "Pollution") == 0)
		return new TPollutionManager("\x97\x8e\x8f\x91\x82\xab\x8a\xc7\x97\x9d");
	if (strcmp(name, "PollutionTest") == 0)
		return new TPollutionTest("\x97\x8e\x8f\x91\x82\xab\x83\x65\x83\x58\x83\x67");
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
