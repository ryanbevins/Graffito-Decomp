#include <Map/MapEventSink.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/PollutionLayer.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Player/MarioAccess.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

// infectious dummies — emit {0,0,0} and {1,1,1} Vec constants into rodata.
// Mirrors src/MoveBG/MapObjManager.cpp:33-34 and matches target's MapEventSink-
// family rodata layout (@2585 zero + @2587 ones at .rodata:0xE0/0xEC).
inline static void dummy(Vec* v) { *v = (Vec) { 0.0f, 0.0f, 0.0f }; }
inline static void dummy2(Vec* v) { *v = (Vec) { 1.0f, 1.0f, 1.0f }; }

bool TMapEventSirenaSink::watch()
{
	if (unk64) {
		gpPollution->getLayer(0)->onUnk32(0x2);
		unk28 = 0;
		TMarDirector* director = gpMarDirector;
		director->fireStartDemoCamera("ホテル上げカメラ", &mShinePos, -1,
		                              0.0f, true, nullptr, 0, nullptr,
		                              JDrama::TFlagT<u16>(0));
		gpItemManager->makeShineAppearWithDemo("シャイン（ホテル上げ用）",
		                                       "ホテル上げシャインカメラ",
		                                       mShinePos.x, mShinePos.y,
		                                       mShinePos.z);
		TFlagManager::smInstance->setBool(true, 0x50008);
		SMS_MarioWarpRequest(mWarpPos, mWarpY);
		gpMarioParticleManager->emit(
		    0x68, (const JGeometry::TVec3<f32>*)&gpMapObjManager->unk44, 0,
		    nullptr);
		gpMarioParticleManager->emit(
		    0x1E4, (const JGeometry::TVec3<f32>*)&gpMapObjManager->unk44, 2,
		    nullptr);
		return true;
	} else {
		return false;
	}
}

void TMapEventSirenaSink::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	TJointModel* model
	    = (TJointModel*)JDrama::TNameRefGen::getInstance()
	          ->getRootNameRef()
	          ->search("ホテル上げカメラ");
	unk40         = (int)model->mActor;
	unk44         = 240;
	unk48         = 240;
	unk38         = 3500.0f;
	mShinePos.x   = 0.0f;
	mShinePos.y   = 3300.0f;
	mShinePos.z   = -2570.0f;
}

void TMapEventSirenaSink::load(JSUMemoryInputStream& stream)
{
	TMapEventSink::load(stream);
	stream.readString();
	stream.read(&mWarpPos.x, 4);
	stream.read(&mWarpPos.y, 4);
	stream.read(&mWarpPos.z, 4);
	int dummy;
	stream.read(&dummy, 4);
	stream.read(&mWarpY, 4);

	SMS_LoadParticle("/scene/map/map/ms_objup_hotel_a.jpa", 0x68);
	SMS_LoadParticle("/scene/map/map/ms_objup_hotel_b.jpa", 0x1E4);
}

TMapEventSirenaSink::TMapEventSirenaSink(const char* name)
    : TMapEventSink(name)
{
	unk64  = 0;
	mWarpY = 0.0f;
	mShinePos.zero();
	mWarpPos.zero();
}

void TMapEventSirenaSink::makePollutionRecovered(u32) { }

void TMapEventSirenaSink::initPollution() { }
