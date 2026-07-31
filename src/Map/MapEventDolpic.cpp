#include <Map/MapEventDolpic.hpp>
#include <Map/Map.hpp>
#include <Map/MapModel.hpp>
#include <Map/JointModel.hpp>
#include <Map/JointModelManager.hpp>
#include <Map/JointObj.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/PollutionCount.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <Player/MarioAccess.hpp>
#include <Camera/CameraShake.hpp>
#include <Enemy/Enemy.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/Particles.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGeometry/JGMatrix34.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSoundBGM.hpp>

// rogue includes needed for matching sinit & rodata
#include <MSound/MSSetSound.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

// Minimal local TBiancoGateKeeper decl - the real class lives in
// MarNameRefGen_Enemy.cpp but we only need its TLiveActor/TSpineEnemy
// vtable layout for the kill() call (vtbl[57]).
class TBiancoGateKeeper : public TSpineEnemy {
public:
	TBiancoGateKeeper(const char*);
};

// infectious dummies (TVec3 zero/ones rodata constants @2585/@2587)
inline static void dummy(Vec* v) { *v = (Vec) { 0.0f, 0.0f, 0.0f }; }
inline static void dummy2(Vec* v) { *v = (Vec) { 1.0f, 1.0f, 1.0f }; }

// =====================================================================
// TDolpicEventBiancoGate (defined first; -inline deferred reverses order)
// =====================================================================

bool TDolpicEventBiancoGate::isFinishedAll() const
{
	return stateIs(0) ? true : false;
}

bool TDolpicEventBiancoGate::control()
{
	unk20->mPosition.y += unk24;
	gpCameraShake->keepShake((EnumCamShakeMode)5, 1.0f);
	SMSRumbleMgr->start(0, (f32*)nullptr);
	JGeometry::TVec3<f32>* position = &unk20->mPosition;
	gpPollution->clean(position->x, position->y, position->z, 10000.0f);
	TMapObjBase* gate = (TMapObjBase*)unk20;
	if (gate->mPosition.y >= gate->mInitialPosition.y) {
		gate->mPosition.y = gate->mInitialPosition.y;
		gate->setUpMapCollision(0);
		unk18 = 0;
		return true;
	} else {
		return false;
	}
}

bool TDolpicEventBiancoGate::watch()
{
	TFlagManager* fm = TFlagManager::smInstance;
	if (fm->getBool(0x10384)) {
		unk20->reset();
		((TMapObjBase*)unk20)->setUpMapCollision(1);
		return true;
	} else {
		return false;
	}
}

void TDolpicEventBiancoGate::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	unk20 = (TBiancoGateKeeper*)JDrama::TNameRefGen::getInstance()
	            ->getRootNameRef()
	            ->search("dptKing");
	unk20->kill();
	unk20->mPosition.y -= 1800.0f;
}

TDolpicEventBiancoGate::TDolpicEventBiancoGate(const char* name)
    : TMapEvent(name)
{
	unk20 = nullptr;
	unk24 = 3.0f;
}

// =====================================================================
// TDolpicEventRiccoMammaGate
// =====================================================================

bool TDolpicEventRiccoMammaGate::isFinishedAll() const
{
	return stateIs(0) ? true : false;
}

bool TDolpicEventRiccoMammaGate::control()
{
	if (unk44 < unk38 - unk3C && unk44 > unk40) {
		f32 scale = TMapObjBase::getJointScaleY(unk20) + unk34;
		JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > mtx;
		mtx.identity();
		mtx.mMtx[1][1] = scale;
		unk24->moveMtx((MtxPtr)mtx.mMtx);
		gpCameraShake->keepShake((EnumCamShakeMode)5, 1.0f);
		SMSRumbleMgr->start(0, (f32*)nullptr);
		TMapObjBase::setJointScaleY(unk20, scale);
		TMapObjBase::setJointTransY(unk20, 300.0f * (1.0f - scale));
		gpMap->getModelManager()->getJointModel(0)->getModel()->calc();
	}

	if (unk44 > unk40) {
		SMSRumbleMgr->start(0x13, (f32*)nullptr);
		if (gpMSound->gateCheck(0x3008)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x3008, (Vec*)&mPos, 0,
			                                          nullptr, 0, 4);
		}
	}

	if (unk44 > 0) {
		--unk44;
		return false;
	} else {
		TMapObjBase::setJointScaleY(unk20, 1.0f);
		unk24->remove();
		unk28->setUp();
		gpMarDirector->fireEndDemoCamera();
		MSBgm::setTrackVolume(0, 1.0f, 15, 0);
		unk18 = 0;
		return true;
	}
}

bool TDolpicEventRiccoMammaGate::watch()
{
	if (!TFlagManager::smInstance->getBool(unk2C)) {
		SMS_ShowJoint(unk20->getMesh(), true);
		TMapObjBase::setJointScaleY(unk20, unk34);

		JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > mtx;
		mtx.identity();
		mtx.mMtx[1][1] = unk34;

		unk24->setUp();
		unk44 = unk38;

		if (unk2C == 0x50001) {
			JDrama::TFlagT<u16> flag(0);
			gpMarDirector->fireStartDemoCamera("マニ屋上げデモカメラ", &mPos, -1,
			                                   0.0f, false, nullptr, 0, nullptr,
			                                   flag);
			gpMarioParticleManager->emit(0x66, &mPos, 0, this);
			gpMarioParticleManager->emit(0x1E2, &mPos, 2, this);
			gpPollution->getLayer(0)->unk32 |= 2;
		} else {
			JDrama::TFlagT<u16> flag(0);
			gpMarDirector->fireStartDemoCamera("灯台上げデモカメラ", &mPos, -1,
			                                   0.0f, false, nullptr, 0, nullptr,
			                                   flag);
			gpMarioParticleManager->emit(0x67, &mPos, 0, this);
			gpMarioParticleManager->emit(0x1E3, &mPos, 2, this);
			gpPollution->getLayer(1)->unk32 |= 2;
		}

		SMS_MarioWarpRequest(mWarpPos, mWarpY);
		return true;
	} else {
		return false;
	}
}

void TDolpicEventRiccoMammaGate::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	unk38 = 720;
	unk3C = 120;
	unk40 = 120;
	unk34 = 0.008f / (f32)(unk38 - unk3C - unk40);

	if (unk30) {
		unk28->setUp();
		unk18 = 0;
		if (unk2C == 0x50001) {
			gpPollution->getCounterLayer().offLayer(0);
		} else {
			gpPollution->getCounterLayer().offLayer(1);
		}
	}
}

void TDolpicEventRiccoMammaGate::load(JSUMemoryInputStream& stream)
{
	TMapEvent::load(stream);
	stream.readString();
	stream.read(&mWarpPos.x, 4);
	stream.read(&mWarpPos.y, 4);
	stream.read(&mWarpPos.z, 4);
	int dummy;
	stream.read(&dummy, 4);
	stream.read(&mWarpY, 4);

	int level;
	if (strcmp("イベント（リコゲート）", getName()) == 0) {
		unk2C = 0x50001;
		level = 0;
	} else {
		unk2C = 0x50002;
		level = 1;
	}
	unk24 = TMapObjBase::newAndInitBuildingCollisionMove(level + 1, nullptr);
	unk28 = TMapObjBase::newAndInitBuildingCollisionWarp(level + 1, nullptr);

	u32 flagId = unk2C;
	if (TFlagManager::smInstance->getBool(flagId)) {
		unk20 = getBuilding(level + 1)->getJoint();
		TMapObjBase::setJointScaleY(unk20, 0.008f);
		TMapObjBase::setJointTransY(unk20, 295.0f);
		gpMap->getModelManager()->getJointModel(0)->getModel()->calc();

		if (unk2C == 0x50001) {
			mPos.x = -10500.0f;
			mPos.y = 300.0f;
			mPos.z = 2003.0f;
			SMS_LoadParticle("/scene/map/map/ms_objup_maniya_a.jpa", 0x66);
			SMS_LoadParticle("/scene/map/map/ms_objup_maniya_b.jpa", 0x1E2);
		} else {
			mPos.x = 9915.0f;
			mPos.y = 300.0f;
			mPos.z = -7565.0f;
			SMS_LoadParticle("/scene/map/map/ms_objup_toudai_a.jpa", 0x67);
			SMS_LoadParticle("/scene/map/map/ms_objup_toudai_b.jpa", 0x1E3);
		}
		unk30 = false;
	} else {
		unk30 = true;
	}
}

TDolpicEventRiccoMammaGate::TDolpicEventRiccoMammaGate(const char* name)
    : TMapEvent(name)
{
	unk20  = nullptr;
	unk24  = nullptr;
	unk28  = nullptr;
	unk2C  = 0;
	unk30  = false;
	unk34  = 0.0f;
	unk38  = 0;
	unk3C  = 0;
	unk40  = 0;
	unk44  = 0;
	mWarpY = 0.0f;
	mPos.zero();
	mWarpPos.zero();
}
