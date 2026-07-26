#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapModel.hpp>
#include <Map/MapWarp.hpp>
#include <Map/MapXlu.hpp>
#include <Map/MapCollisionEntry.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapEventMare.hpp>
#include <Map/MapStaticObject.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/MapObjOption.hpp>
#include <MoveBG/MapObjWater.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JDrama/JDRViewObjPtrList.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

TMap* gpMap;

#pragma dont_inline on
static void initMonte()
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	const char* indirectSceneName = "インダイレクトシーン";
	JDrama::TNameRef* indirectScene = root->searchF(
	    JDrama::TNameRef::calcKeyCode(indirectSceneName), indirectSceneName);

	TMapStaticObj* waterIndirect = new TMapStaticObj("水インダイレクト");
	waterIndirect->init("SeaIndirect");
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group
	    = (JDrama::TViewObjPtrListT<JDrama::TViewObj>*)indirectScene;
	group->getChildren().push_back(waterIndirect);

	if (gpMarDirector->unk7D == 0 || gpMarDirector->unk7D == 2
	    || gpMarDirector->unk7D == 5 || gpMarDirector->unk7D == 6) {
		SMS_LoadParticle("/scene/map/pollution/ms_newfire_b.jpa", 0x1DC);
		SMS_LoadParticle("/scene/map/pollution/ms_newfire_a.jpa", 0x65);
	}

	if (gpMarDirector->unk7D == 1 || gpMarDirector->unk7D == 3
	    || gpMarDirector->unk7D == 5 || gpMarDirector->unk7D == 7)
		SMS_LoadParticle("/scene/map/map/ms_monte_yuge.jpa", 0x156);
}

static void initMare()
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	const char* mapGroupName = "マップグループ";
	JDrama::TNameRef* mapGroup
	    = root->searchF(JDrama::TNameRef::calcKeyCode(mapGroupName),
	                    mapGroupName);
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group
	    = (JDrama::TViewObjPtrListT<JDrama::TViewObj>*)mapGroup;

	if (gpMarDirector->unk7D == 5) {
		TMapStaticObj* gate = new TMapStaticObj("マーレ５ＥＸゲート");
		gate->init("Mare5ExGate");
		group->getChildren().push_back(gate);
	}

	if (gpMarDirector->unk7D == 0) {
		SMS_LoadParticle("/scene/map/map/ms_mare_objup_a.jpa", 0x69);
		SMS_LoadParticle("/scene/map/map/ms_mare_objup_b.jpa", 0x1E5);
	}

	if (gpMarDirector->unk7D != 0) {
		for (int i = 1; i < 8; ++i) {
			TMapCollisionWarp* warp
			    = TMapObjBase::newAndInitBuildingCollisionWarp(i, nullptr);
			warp->setUp();
		}
	}

	TMareEventDepressWall* first
	    = new TMareEventDepressWall("イベント（マーレへこむ壁）");
	first->init1stEvent();
	group->getChildren().push_back(first);

	TMareEventDepressWall* second
	    = new TMareEventDepressWall("イベント（マーレへこむ壁）");
	second->init2ndEvent();
	group->getChildren().push_back(second);

	TMareEventDepressWall* third
	    = new TMareEventDepressWall("イベント（マーレへこむ壁）");
	third->init3rdEvent();
	group->getChildren().push_back(third);
}

static void initPinnaParco()
{
	J3DModel* model = new J3DModel(
	    gpMap->getModelManager()->getJointModel(0)->getModelData(), 0, 1);
	MActor* actor = new MActor(gpMap->getModelManager()->getMActorAnmData());
	actor->setModel(model, 0);

	TMapModelActor* modelActor
	    = new TMapModelActor("ピンナ鏡用地形モデル");
	modelActor->unk68 = actor;
	TMapObjBase::joinToGroup("鏡シーン", modelActor);
}

static void initStageCommon()
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	const char* indirectSceneName = "インダイレクトシーン";
	JDrama::TNameRef* indirectScene = root->searchF(
	    JDrama::TNameRef::calcKeyCode(indirectSceneName), indirectSceneName);
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* indirectGroup
	    = (JDrama::TViewObjPtrListT<JDrama::TViewObj>*)indirectScene;

	root = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	const char* mapGroupName = "マップグループ";
	root->searchF(JDrama::TNameRef::calcKeyCode(mapGroupName), mapGroupName);

	u8 map = gpMarDirector->mMap;
	if (map == 4 || map == 3 || map == 13 || map == 9 || map == 5 || map == 6
	    || map == 20 || map <= 1) {
		TMapStaticObj* waveFar = new TMapStaticObj("波（遠景）");
		waveFar->init("sea");

		TMapStaticObj* indirectWave = new TMapStaticObj("インダイレクト波");
		indirectWave->init("SeaIndirect");
		indirectGroup->getChildren().push_back(indirectWave);

		TMapObjWaterFilter* waterFilter
		    = new TMapObjWaterFilter("水中カメラフィルタ");
		waterFilter->init();
		indirectGroup->getChildren().push_back(waterFilter);

		TMapObjSeaIndirect* waterIndirect
		    = new TMapObjSeaIndirect("水中カメラインダイレクト");
		waterIndirect->init();
		indirectGroup->getChildren().push_back(waterIndirect);
	}

	if (map == 2) {
		TMapObjSeaIndirect* waterIndirect
		    = new TMapObjSeaIndirect("水中カメラインダイレクト");
		waterIndirect->init();
		indirectGroup->getChildren().push_back(waterIndirect);
	}
}

static void initStage()
{
	if (gpMarDirector->unk7D > 9)
		return;

	initStageCommon();

	switch (gpMarDirector->mMap) {
	case 1:
		if (gpMarDirector->unk7D != 5 && gpMarDirector->unk7D != 9) {
			TMapCollisionWarp* warp
			    = TMapObjBase::newAndInitBuildingCollisionWarp(1, nullptr);
			warp->setUp();
			warp = TMapObjBase::newAndInitBuildingCollisionWarp(2, nullptr);
			warp->setUp();
		}
		break;
	case 2:
		if (gpMarDirector->unk7D != 0) {
			TMapCollisionWarp* warp
			    = TMapObjBase::newAndInitBuildingCollisionWarp(1, nullptr);
			warp->setUp();
			warp = TMapObjBase::newAndInitBuildingCollisionWarp(2, nullptr);
			warp->setUp();
		}
		break;
	case 9:
		initMare();
		break;
	case 8:
		initMonte();
		break;
	case 6:
		if (gpMarDirector->unk7D != 0) {
			TMapCollisionWarp* warp
			    = TMapObjBase::newAndInitBuildingCollisionWarp(1, nullptr);
			warp->setUp();
		}
		break;
	case 5:
		SMS_LoadParticle("/scene/mapObj/SandSteam.jpa", 0x6A);
		break;
	case 13:
		initPinnaParco();
		break;
	case 15: {
		TMapObjOptionWall* wall = new TMapObjOptionWall("オプション用壁");
		wall->init();
		TMapObjBase::joinToGroup("マップグループ", wall);
		break;
	}
	default:
		break;
	}
}
#pragma dont_inline off

void TMap::update()
{
	static Vec pos;
	static s8 init;

	u8 map = gpMarDirector->mMap;

	switch (map) {
	case 3:
		if (!init) {
			pos.x = 1815.0f;
			pos.y = 1500.0f;
			pos.z = 1550.0f;
			init  = true;
		}

		if (gpMSound->gateCheck(0x3000))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x3000, &pos, 0, nullptr, 0, 4);
		break;
	case 8:
		if (gpMarDirector->unk7D == 1 || gpMarDirector->unk7D == 3
		    || gpMarDirector->unk7D == 5 || gpMarDirector->unk7D == 7)
			gpMarioParticleManager->emit(
			    0x156, (JGeometry::TVec3<f32>*)&gpMapObjManager->unk44, 1,
			    this);
		break;
	case 7: {
		TMapWarp* warp = mWarp;
		int warpArea   = warp->unk8;
		int areaNo = gpCubeArea->unk1C;
		if (areaNo != warpArea) {
			if (areaNo != -1)
				warp->changeModel(areaNo);
			else if (gpMarDirector->unk7D != 0)
				warp->changeModel(3);
		}
		break;
	}
	default:
		break;
	}

	if (gpMarDirector->unk124 != 0)
		return;

	CPolarSubCamera* camera = gpCamera;
	bool demoCamera         = true;
	if (!camera->isSimpleDemoCamera()) {
		bool modeDemo = camera->mMode == 0x49 ? demoCamera : false;
		if (!modeDemo)
			demoCamera = false;
	}

	bool isDemoCamera = demoCamera ? true : false;
	if (!isDemoCamera) {
		map = gpMarDirector->mMap;
		if (map == 0x39)
			return;

		if (map == 0x10)
			return;

		if (SMS_CheckMarioFlag(0x2) == false) {
			JGeometry::TVec3<f32>* cameraPos = &gpCamera->unk124;
			f32 cameraX                      = cameraPos->x;
			f32 waterHeight
			    = gpMapObjWave->getHeight(cameraX, cameraPos->y,
			                              cameraPos->z);
			f32 cameraY = gpCamera->unk124.y;
			if (waterHeight == cameraY || cameraY > waterHeight) {
				if (unk20 == 0) {
					unk20 = 1;
					MSSeCallBack::setWaterCameraFir(false);
				}
			} else {
				if (unk20 != 0) {
					unk20 = 0;
					MSSeCallBack::setWaterCameraFir(true);
				}
			}
		}
	}
}

TBGCheckData* TMap::getIllegalCheckData()
{
	return &TMapCollisionData::mIllegalCheckData;
}

bool TMap::isInArea(f32 param_1, f32 param_2) const
{
	if (-mCollisionData->mGridExtentX < param_1
	    && param_1 < mCollisionData->mGridExtentX
	    && -mCollisionData->mGridExtentY < param_2
	    && param_2 < mCollisionData->mGridExtentY)
		return true;

	return false;
}

const TBGCheckData* TMap::intersectLine(const JGeometry::TVec3<f32>& param_1,
                                        const JGeometry::TVec3<f32>& param_2,
                                        bool param_3,
                                        JGeometry::TVec3<f32>* param_4) const
{
	return mCollisionData->intersectLine(param_1, param_2, param_3, param_4);
}

bool TMap::isTouchedOneWall(f32 x, f32 y, f32 z, f32 radius) const
{
	bool result = isTouchedOneWallAndMoveXZ(&x, y, &z, radius);
	return result;
}

bool TMap::isTouchedOneWallAndMoveXZ(f32* x, f32 y, f32* z, f32 radius) const
{
	TBGWallCheckRecord record(*x, y, *z, radius, 1, 0);

	int r = mCollisionData->checkWalls(&record);
	if (r != 0 ? true : false) {
		*x = record.mCenter.x;
		*z = record.mCenter.z;
		return true;
	} else {
		return false;
	}
}

bool TMap::isTouchedWallsAndMoveXZ(TBGWallCheckRecord* record) const
{
	return mCollisionData->checkWalls(record) != 0 ? true : false;
}

f32 TMap::checkRoofIgnoreWaterThrough(f32 x, f32 y, f32 z,
                                      const TBGCheckData** result) const
{
	return mCollisionData->checkRoof(
	    x, y, z, TMapCollisionData::IGNORE_WATER_THROUGH, result);
}

f32 TMap::checkRoof(f32 x, f32 y, f32 z, const TBGCheckData** result) const
{
	return mCollisionData->checkRoof(x, y, z, 0, result);
}

f32 TMap::checkRoof(const JGeometry::TVec3<f32>& pos,
                    const TBGCheckData** param_2) const
{
	return mCollisionData->checkRoof(pos.x, pos.y, pos.z, 0, param_2);
}

f32 TMap::checkGroundIgnoreWaterThrough(f32 x, f32 y, f32 z,
                                        const TBGCheckData** result) const
{
	return mCollisionData->checkGround(
	    x, y, z, TMapCollisionData::IGNORE_WATER_THROUGH, result);
}

f32 TMap::checkGroundIgnoreWaterSurface(f32 x, f32 y, f32 z,
                                        const TBGCheckData** result) const
{
	return mCollisionData->checkGround(
	    x, y, z, TMapCollisionData::IGNORE_WATER_SURFACE, result);
}

f32 TMap::checkGroundIgnoreWaterSurface(const JGeometry::TVec3<f32>& pos,
                                        const TBGCheckData** result) const
{
	return mCollisionData->checkGround(
	    pos.x, pos.y, pos.z, TMapCollisionData::IGNORE_WATER_SURFACE, result);
}

f32 TMap::checkGroundExactY(f32 x, f32 y, f32 z,
                            const TBGCheckData** result) const
{
	return mCollisionData->checkGround(x, y - -78.0f, z, 0, result);
}

f32 TMap::checkGround(const JGeometry::TVec3<f32>& pos,
                      const TBGCheckData** result) const
{
	return mCollisionData->checkGround(pos.x, pos.y, pos.z, 0, result);
}

f32 TMap::checkGround(f32 x, f32 y, f32 z, const TBGCheckData** result) const
{
	return mCollisionData->checkGround(x, y, z, 0, result);
}

void TMap::changeModel(s16 param_1) const { mWarp->changeModel(param_1); }

void TMap::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 1) {
		update();
		mCollisionData->initMoveCollision();
		mWarp->watchToWarp();
	}

	if (param_1 & 0x200) {
		if ((param_1 & 0x2000000)) {
			if (!mXlu->changeXluJoint(1))
				return;
		} else if ((param_1 & 0x4000000)) {
			if (!mXlu->changeXluJoint(0))
				return;
		} else {
			mXlu->changeNormalJoint();
		}
	}

	if (param_1 & 8)
		draw(param_1, param_2);

	mModelManager->perform(param_1, param_2);
}

void TMap::loadAfter()
{
	JDrama::TViewObj::loadAfter();
	initStage();
}

void TMap::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);
	mXlu->init(stream);
	mModelManager->init();
	mCollisionData->init(stream);
	mWarp->initModel();
	mWarp->init(stream);
	mModelManager->mCollision->setUp();
}

TMap::TMap(const char* name)
    : JDrama::TViewObj(name)
{
	mCollisionData = new TMapCollisionData;
	mModelManager  = new TMapModelManager("地形モデル管理");
	mWarp          = new TMapWarp;
	mXlu           = new TMapXlu;
	unk20          = 0;

	gpMap = this;
}

TMap::~TMap() { }
