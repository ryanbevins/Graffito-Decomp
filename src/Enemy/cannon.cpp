#include <Enemy/Cannon.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <dolphin/mtx.h>
#include <stdlib.h>

static const char* cannon_bastable[] = {
	nullptr,
	nullptr,
	"/scene/cannon/bas/CannonDom_break.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/cannon/bas/tyorobe_appear1.bas",
	"/scene/cannon/bas/tyorobe_damage1.bas",
	"/scene/cannon/bas/tyorobe_down1.bas",
	"/scene/cannon/bas/tyorobe_hyde1.bas",
	"/scene/cannon/bas/tyorobe_throw1.bas",
	nullptr,
	nullptr,
	nullptr,
};

static const char* sCannonDomPartsJointTable[] = {
	"nullC",
	"nullB",
	"nullA",
};

u8 TCannon::mChorobeiJntIdx     = 4;
u8 TCannon::mChorobeiHandJntIdx = 4;
f32 TCannon::mVelocityRate      = 0.62f;
f32 TCannon::mSearchRate        = 0.02f;

DEFINE_NERVE(TNerveCannonObject, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonDamageDemo, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonDamage, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonClose, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonForceBombShoot, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonShoot, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonSearch, TLiveActor) { return FALSE; }

DEFINE_NERVE(TNerveCannonOpen, TLiveActor) { return FALSE; }

void TCannon::startChorobeiShout() { }

bool TCannon::isObject()
{
	return mCurrentBckAnm == 2;
}

void TCannon::setKillerGoalPoint() { }

void TCannon::killerShoot() { }

void TCannon::bombShoot() { }

void TCannon::bombSet() { }

bool TCannon::isHitVallid(u32)
{
	return false;
}

MtxPtr TCannon::getTakingMtx()
{
	if (checkLiveFlag(LIVE_FLAG_UNK4000))
		return unk1A8->unk6C->getMActor()->getModel()->getBaseTRMtx();

	MtxPtr takingMtx = unk1A8->unk6C->getMActor()->getModel()->getAnmMtx(
	    mChorobeiHandJntIdx);
	unk1E4.identity33();
	unk1E4.mMtx[0][3] = unk1A8->mPosition.x;
	unk1E4.mMtx[1][3] = takingMtx[1][3];
	unk1E4.mMtx[2][3] = unk1A8->mPosition.z;
	return unk1E4.mMtx;
}

void TCannon::calcRootMatrix()
{
	TSmallEnemy::calcRootMatrix();
}

void TCannon::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
}

const char** TCannon::getBasNameTable() const
{
	return cannon_bastable;
}

BOOL TCannon::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->mActorType == 0x40000235 && message == HIT_MESSAGE_TAKE
	    && mHolder == nullptr) {
		mHolder = (TTakeActor*)sender;
		return TRUE;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER)
		return TRUE;

	return FALSE;
}

void TCannon::moveObject()
{
	TSmallEnemy::moveObject();
}

void TCannon::reset()
{
	TSmallEnemy::reset();
	unk1A0 = nullptr;
	unk1A8 = nullptr;
	unk1B8 = nullptr;
	unk1E0 = nullptr;
	unk214 = nullptr;
	unk218 = nullptr;
	unk21C = false;
	unk220 = 0.0f;
	unk224 = 0.0f;
	unk228 = 0.0f;
	unk22C = 0.0f;
	unk230 = true;
	unk238 = false;
	unk239 = true;
	unk254 = nullptr;
	unk258 = nullptr;
	unk290 = false;
	unk2AC = 0.0f;
}

void TCannon::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
}

void TCannon::loadAfter()
{
	TSmallEnemy::loadAfter();
}

void TCannon::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
}

TCannon::TCannon(const char* name)
    : TSmallEnemy(name)
    , unk1A0(nullptr)
    , unk1A8(nullptr)
    , unk1B8(nullptr)
    , unk1E0(nullptr)
    , unk214(nullptr)
    , unk218(nullptr)
    , unk21C(false)
    , unk220(0.0f)
    , unk230(true)
    , unk238(false)
    , unk239(true)
    , unk254(nullptr)
    , unk258(nullptr)
    , unk290(false)
    , unk2AC(0.0f)
{
	unk224 = 0.0f;
	unk228 = 0.0f;
	unk22C = 0.0f;
}

void TCannonDom::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((unk10->mLiveFlag & 0x7) != 0)
		return;

	if (flags == 2) {
		if (unk1C != nullptr && unk20 != nullptr) {
			J3DFrameCtrl* ctrl = unk18->getFrameCtrl(0);
			unk1C->animeLoop((Vec*)&unk10->mPosition, ctrl->getFrame(),
			                 ctrl->getRate(), 0, 4);
		}

		Mtx rot;
		MtxPtr connectedMtx = getConnectedMtx();
		MsMtxSetRotRPH(rot, unk28, unk2C, 0.0f);
		PSMTXConcat(connectedMtx, rot, connectedMtx);
		PSMTXCopy(connectedMtx, unk18->getModel()->getBaseTRMtx());
	}

	unk18->perform(flags, graphics);
}

TCannonDom::TCannonDom(TLiveActor* owner, int jointIndex, SDLModelData* data,
                       u32 flags, const char* name)
    : TSharedParts(owner, jointIndex, data, flags, name)
    , unk1C(nullptr)
    , unk20(nullptr)
    , unk24(0)
    , unk28(0.0f)
    , unk2C(0.0f)
    , unk30(0.0f)
{
	f32 min = 0.0f;
	f32 max = 360.0f;
	unk30  = min + (max - min) * ((f32)rand() * (1.0f / 32768.0f));

	if (unk1C != nullptr)
		return;

	unk1C = new MAnmSound(gpMSound);
	unk1C->initAnmSound(nullptr, 1, 0.0f);
}

BOOL TChorobei::receiveMessage(THitActor*, u32)
{
	return FALSE;
}

BOOL TChorobei::checkHit()
{
	return FALSE;
}

void TChorobei::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk6C != nullptr)
		unk6C->perform(flags, graphics);

	if (unk74 != nullptr && unk78 != nullptr) {
		J3DFrameCtrl* ctrl = unk6C->getMActor()->getFrameCtrl(0);
		unk74->animeLoop(&mPosition, ctrl->getFrame(), ctrl->getRate(), 0, 4);
	}
}

TChorobei::TChorobei(TCannon* cannon, int jointIndex, const char* name)
    : THitActor(name)
    , unk68(cannon)
    , unk6C(nullptr)
    , unk70(0.0f)
    , unk74(nullptr)
    , unk78(nullptr)
    , unk7C(300.0f)
{
	unk6C = new TSharedParts(cannon, jointIndex,
	                         "/scene/cannon/tyorobe_model1.bmd", 0x10020000,
	                         3, "<TSharedParts>");
	if (unk74 != nullptr)
		return;

	unk74 = new MAnmSound(gpMSound);
	unk74->initAnmSound(nullptr, 1, 0.0f);
}

TSmallEnemy* TCannonManager::createEnemyInstance()
{
	return new TCannon("砲台");
}

void TCannonManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TCannonSaveLoadParams("/enemy/cannon.prm");
}

TCannonManager::TCannonManager(const char* name)
    : TSmallEnemyManager(name)
{
}

TCannonSaveLoadParams::TCannonSaveLoadParams(const char* path)
    : TSmallEnemyParams(path)
    , PARAM_INIT(mSLHideDist, 300.0f)
    , PARAM_INIT(mSLBombDist, 2000.0f)
    , PARAM_INIT(mSLKillerDist, 10000.0f)
    , PARAM_INIT(mSLBombInterval, 100)
    , PARAM_INIT(mSLKillerInterval, 50)
    , PARAM_INIT(mSLShootInterval, 100)
    , PARAM_INIT(mSLChorobeiAttackRadius, 100.0f)
    , PARAM_INIT(mSLChorobeiAttackHeight, 100.0f)
    , PARAM_INIT(mSLChorobeiDamageRadius, 100.0f)
    , PARAM_INIT(mSLChorobeiDamageHeight, 100.0f)
    , PARAM_INIT(mSLKillerTransYOffset, -50.0f)
    , PARAM_INIT(mSLBombHeiGenerateRate, 0.7f)
    , PARAM_INIT(mSLThrowXZSpeed, 12.0f)
{
	TParams::load(mPrmPath);
}
