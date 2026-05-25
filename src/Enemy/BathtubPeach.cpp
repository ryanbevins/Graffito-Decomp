#include <Enemy/BathtubPeach.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Camera/cameralib.hpp>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

#include <dolphin/mtx.h>

extern "C" float fmodf(float, float);

static const char* bathtubpeach_bastable[] = {
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr,
	"/scene/bathtubpeach/bas/peach_wait.bas",
	nullptr,
};

// reverse-order definitions for -inline deferred TU

DEFINE_NERVE(TNervePeachStagger, TLiveActor)
{
	TBathtubPeach* peach = (TBathtubPeach*)spine->getBody();
	MActor* mactor       = peach->getMActor();
	if (!mactor->checkCurBckFromIndex(0)) {
		mactor->setBckFromIndex(0);
		const char** table = peach->getBasNameTable();
		peach->setAnmSound(table ? table[0] : nullptr);
	}
	if (mactor->getCurAnmIdx(3) != 0)
		mactor->setBtpFromIndex(0);
	J3DFrameCtrl* frameCtrl = mactor->getFrameCtrl(0);
	frameCtrl->setRate(0.5f * (2.0f * SMSGetAnmFrameRate()));
	return mactor->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNervePeachEscape, TLiveActor)
{
	TBathtubPeach* peach = (TBathtubPeach*)spine->getBody();
	TBathtub* bathtub    = (TBathtub*)JDrama::TNameRefGen::instance->mRootNameRef
	                       ->searchF(JDrama::TNameRef::calcKeyCode("バスタブ"),
	                                 "バスタブ");
	if (bathtub->getUnk29A())
		return FALSE;

	if (!(spine->getTime() & 4)) {
		if (bathtub->getUnk1D4())
			spine->pushNerve(&TNervePeachStagger::theNerve());
		return FALSE;
	}

	MActor* mactor = peach->getMActor();
	if (!mactor->checkCurBckFromIndex(1)) {
		mactor->setBckFromIndex(1);
		const char** table = peach->getBasNameTable();
		peach->setAnmSound(table ? table[1] : nullptr);
	}
	if (mactor->getCurAnmIdx(3) != 1)
		mactor->setBtpFromIndex(1);
	J3DFrameCtrl* frameCtrl = mactor->getFrameCtrl(0);
	frameCtrl->setRate(0.5f * (2.0f * SMSGetAnmFrameRate()));

	(void)bathtub; // remainder of escape logic not yet decompiled
	return FALSE;
}

TBathtubPeach::TBathtubPeach(const char* name)
    : TSpineEnemy(name)
{
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	offLiveFlag(LIVE_FLAG_UNK100);
	offLiveFlag(LIVE_FLAG_UNK10);
}

const char** TBathtubPeach::getBasNameTable() const
{
	return bathtubpeach_bastable;
}

void TBathtubPeach::init(TLiveManager* manager)
{
	TSpineEnemy::init(manager);
	mSpine->initWith(&TNervePeachEscape::theNerve());
	initAnmSound();
	reset();
	mScaling.x = 2.0f;
	mScaling.y = 2.0f;
	mScaling.z = 2.0f;
}

void TBathtubPeach::reset()
{
	mPosition.x *= 0.21f;
	mPosition.y *= 0.21f;
	mPosition.z *= 0.21f;
	mBinder.init(50.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	unk130     = 0;
	mScaling.x = 1.5f;
	mScaling.y = 1.5f;
	mScaling.z = 1.5f;
	TLiveActor::mBinder = &mBinder;
	TSpineEnemy::reset();

	if (!mMActor->checkCurBckFromIndex(1)) {
		mMActor->setBckFromIndex(1);
		const char** table = getBasNameTable();
		setAnmSound(table ? table[1] : nullptr);
	}
	if (mMActor->getCurAnmIdx(3) != 1)
		mMActor->setBtpFromIndex(1);
	J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
	frameCtrl->setRate(0.5f * (2.0f * SMSGetAnmFrameRate()));
}

void TBathtubPeach::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSpineEnemy::perform(flags, graphics);
}

Mtx* TBathtubPeach::getRootJointMtx() const
{
	return (Mtx*)((u8*)getModel() + 0x20);
}

BOOL TBathtubPeach::receiveMessage(THitActor* sender, u32 message)
{
	return TSpineEnemy::receiveMessage(sender, message);
}

void TBathtubPeach::calcRootMatrix()
{
	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	TBathtub* bathtub      = (TBathtub*)root->searchF(
        JDrama::TNameRef::calcKeyCode("バスタブ"), "バスタブ");
	if (bathtub && bathtub->getUnk29A()) {
		Mtx* dst = (Mtx*)((u8*)getModel() + 0x20);
		PSMTXCopy(bathtub->getPeachMtxInDemo(), *dst);
	} else {
		TLiveActor::calcRootMatrix();
	}
}

TBathtubPeachManager::TBathtubPeachManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TBathtubPeachManager::createEnemyInstance() { return nullptr; }

void TBathtubPeachManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "ahiru_peach.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBathtubPeachManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TBathtubPeachParams("/enemy/bathtubpeach.prm");
}

TBathtubPeachParams::TBathtubPeachParams(const char* prm)
    : TSpineEnemyParams(prm)
    , PARAM_INIT(mTurnSpeed, 8.0f)
    , PARAM_INIT(mTurnSpeed2, 1.0f)
    , PARAM_INIT(mSpeed, 16.0f)
    , PARAM_INIT(mAngle, 72.0f)
    , PARAM_INIT(mRange, 100.0f)
    , PARAM_INIT(mRadius, 2200.0f)
{
	TParams::load(mPrmPath);
}
