#include <Enemy/SleepBossHanachan.hpp>
#include <Enemy/BossHanachan.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/MirrorActor.hpp>
#include <Strategic/ObjManager.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <System/FlagManager.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static const char* sleepBossHanachan_bastable[] = {
	"/scene/sleepBossHanachan/bas/demohanatyan_fall.bas",
	nullptr,
};

void TSleepBossHanachanManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "demohanatyan_model.bmd", 0x10010000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TSleepBossHanachan::init(TLiveManager* manager)
{
	TDemoBossHanachan::initBase(manager, 3);
	mSpine->initWith(&TNerveSBH_SleepContinue::theNerve());
	initHitActor(0x8000016, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	unk64 |= 0x1;
	initAnmSound();
	getMActor()->setBckFromIndex(1);
	setCurAnmSound();
	MsMtxSetXYZRPH(getModel()->getBaseTRMtx(), mPosition.x, mPosition.y,
	               mPosition.z, mRotation.x, mRotation.y, mRotation.z);
	mMirrorActor = new TMirrorActor("寝てるボスハナチャンin鏡");
	mMirrorActor->init(getModel(), 10);
}

void TSleepBossHanachan::calcRootMatrix() { }

const char** TSleepBossHanachan::getBasNameTable() const
{
	return sleepBossHanachan_bastable;
}

void TSleepBossHanachan::startFall(f32 x, f32 y, f32 z)
{
	TFlagManager* fm = TFlagManager::smInstance;
	fm->setBool(true, 0x5000B);
	mFallPos.x = x;
	mFallPos.y = y;
	mFallPos.z = z;
	getMActor()->setBckFromIndex(0);
	setCurAnmSound();
	mSpine->setNext(&TNerveSBH_Fall::theNerve());
}

DEFINE_NERVE(TNerveSBH_SleepContinue, TLiveActor) { return false; }

DEFINE_NERVE(TNerveSBH_Fall, TLiveActor)
{
	TSleepBossHanachan* self = (TSleepBossHanachan*)spine->getBody();
	if (self->getMActor()->curAnmEndsNext(0, nullptr)) {
		JGeometry::TVec3<f32> p = self->mFallPos;
		TShine* shine           = gpItemManager->makeShineAppearWithDemo(
		              "シャイン（ボス用）", "ボスシャインカメラ", p.x, p.y, p.z);
		shine->onMapObjFlag(0x20000000);
		self->mLiveFlag |= 0x1;
		TMirrorActor* mirror = self->mMirrorActor;
		mirror->unk1A |= 1;
		SMS_HideAllShapePacket(mirror->unk14);

		spine->pushAfterCurrent(&TNerveSBH_SleepContinue::theNerve());
		return true;
	}
	return false;
}
