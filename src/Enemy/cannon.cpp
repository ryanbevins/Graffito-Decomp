#include <Enemy/Cannon.hpp>
#include <Enemy/Bombhei.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Killer.hpp>
#include <Enemy/Popo.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MoveBG/MapObjDolpic.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/Particles.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
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

static const char* cannonParticleFiles[] = {
	"/scene/cannon/jpa/ms_cannon_a.jpa",
	"/scene/cannon/jpa/ms_cannon_b.jpa",
	"/scene/cannon/jpa/ms_cannon_c.jpa",
	"/scene/cannon/jpa/ms_cannon_d.jpa",
	"/scene/cannon/jpa/ms_cannon_e.jpa",
	"/scene/cannon/jpa/ms_cannon_smoke.jpa",
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
	return mCurrentBckAnm == 4 && checkCurAnmEnd(0);
}

void TCannon::setKillerGoalPoint()
{
	JGeometry::TVec3<f32> target;
	if (unk239) {
		target = *gpMarioPos;
		f32 angle = 0.0f
		            + (360000.0f - 0.0f)
		                * ((f32)rand() * (1.0f / 32768.0f));
		u16 angleShort = (u16)(s32)angle;
		u32 index      = angleShort >> jmaSinShift;
		target.x += 500.0f * jmaCosTable[index];
		target.z += 500.0f * jmaSinTable[index];
	} else {
		target = unk248;
	}

	TPathNode node(target);
	unkF4  = node;
	unk104 = node;
	unk114.clear();

	TCannonDom* dom = unk1AC[unk214];
	dom->getMActor()->setBckFromIndex(1);
	const char** bas = dom->unk10->getBasNameTable();
	dom->unk20       = bas ? bas[1] : nullptr;
	if (dom->unk20 != nullptr) {
		void* res = JKRFileLoader::getGlbResource(dom->unk20);
		dom->unk1C->initAnmSound(res, 1, 0.0f);
	} else {
		dom->unk1C->initAnmSound(nullptr, 1, 0.0f);
	}
}

void TCannon::killerShoot() { }

void TCannon::bombShoot()
{
	if (unk1A4 == nullptr)
		return;

	JGeometry::TVec3<f32> velocity;
	velocity.x = gpMarioPos->x - mPosition.x;
	velocity.y = 0.0f;
	velocity.z = gpMarioPos->z - mPosition.z;
	if (velocity.x == 0.0f && velocity.z == 0.0f)
		velocity.x = 1.0f;
	MsVECNormalize(&velocity, &velocity);

	Mtx rot;
	f32 angle = -30.0f
	            + (30.0f - -30.0f) * ((f32)rand() * (1.0f / 32768.0f));
	MsMtxSetRotRPH(rot, 0.0f, mRotation.y + angle, 0.0f);

	f32 speed = unk28C->mSLThrowXZSpeed.get();
	velocity.y = speed;
	velocity.x *= speed;
	velocity.z *= speed;

	unk1A4->mVelocity = velocity;
	if (unk21C) {
		unk1A4->offLiveFlag(LIVE_FLAG_UNK10);
	} else {
		unk1A4->onLiveFlag(LIVE_FLAG_AIRBORNE);
		unk1A4->getMActor()->setFrameRate(SMSGetAnmFrameRate(), 0);
	}

	unk1A4->mPosition.y += 2.0f;
	unk1A4->receiveMessage(this, HIT_MESSAGE_UNK6);
}

void TCannon::bombSet()
{
	unk21C = false;
	unk1A4 = nullptr;

	TSmallEnemy* spawned = nullptr;
	f32 rate = (f32)rand() * (1.0f / 32768.0f);
	if (rate < unk28C->mSLBombHeiGenerateRate.get()) {
		spawned = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
		    mPosition, "ボム兵マネージャー", 1);
	} else {
		int choice = (s32)(100.0f * ((f32)rand() * (1.0f / 32768.0f)));
		if (choice % 2 == 1) {
			spawned = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
			    mPosition, "ポポマネージャ", 1);
			if (spawned != nullptr)
				((TPopo*)spawned)->thrownByChorobei();
		} else {
			spawned = (TSmallEnemy*)gpConductor->makeOneEnemyAppear(
			    mPosition, "ハムクリマネージャ", 1);
		}
	}

	if (unk1A4 == nullptr) {
		unk1A4 = spawned;
		if (spawned != nullptr) {
			spawned->reset();
			spawned->getMActor()->setFrameRate(0.0f, 0);
		}
	}

	if (unk1A4 != nullptr) {
		unk1A4->mPosition = unk1A8->mPosition;
		unk220 = unk1A4->mScaling.x;
		unk1A4->mScaling.zero();
		unk1A4->mRotation = mRotation;
		if (unk1A4->receiveMessage(this, HIT_MESSAGE_TAKE))
			mHeldObject = unk1A4;
	}
}

bool TCannon::isHitVallid(u32)
{
	return false;
}

MtxPtr TCannon::getTakingMtx()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
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

	if (unk230 != 5 && unk230 != 9)
		return;

	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		unk1A8->mPosition = mPosition;
	} else {
		MtxPtr mtx = getModel()->getAnmMtx(mChorobeiJntIdx);
		unk1A8->mPosition.x = mtx[0][3];
		unk1A8->mPosition.y = mtx[1][3] - 100.0f;
		unk1A8->mPosition.z = mtx[2][3];
	}

	unk1A8->checkHit();

	JGeometry::TVec3<f32> velocity = mVelocity;
	mPosition.y += velocity.y;
	mVelocity.y -= getGravityY();
	if (mPosition.y < unk23C.y) {
		mVelocity.zero();
		mPosition.y = unk23C.y;
	}

	if (unk1A0 == nullptr)
		return;

	JGeometry::TVec3<f32> carriedPos = unk194;
	carriedPos.add(unk1A8->mPosition);
	unk1A0->mPosition = carriedPos;
	unk1A0->offLiveFlag(LIVE_FLAG_AIRBORNE);

	if (mSpine->getCurrentNerve() == &TNerveCannonClose::theNerve()) {
		unk1A0->kill();
		unk1A0 = nullptr;
		return;
	}

	if (!unk1A0->doKeepDistance()) {
		unk1A0 = nullptr;
		mSpine->pushNerve(&TNerveCannonDamage::theNerve());
	}
}

void TCannon::reset()
{
	TSmallEnemy::reset();
	TSpineEnemyParams* params = getSaveParam();
	mHitPoints = params ? params->mSLHitPointMax.get() : 1;
	unk64 |= HIT_FLAG_NO_COLLISION;
	unk214 = 1;
	mHeadHeight = 40.0f;
	mLiveFlag |= LIVE_FLAG_UNK10;
	unk2AC = mRotation.y;

	if (unk230 == 9) {
		unk239 = false;
		JGeometry::TVec3<f32> target(-565.0f, 8500.0f, 7675.0f);
		unkF4  = TPathNode(target);
		unk104 = TPathNode(target);
		unk114.clear();
	}
}

void TCannon::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x1000001c;
	unk150     = 0x11;
	unk28C     = getCannonParams();
	setBckAnm(3);

	void* res = JKRFileLoader::getGlbResource("/scene/cannon/cannon_Dom.bmd");
	SDLModelData* domData
	    = new SDLModelData(J3DModelLoaderDataBase::load(res, 0x10050000));
	unk234 = mRotation.y;
	unk230 = gpApplication.mCurrArea.unk0;

	if (unk230 == 5 || unk230 == 9) {
		mSpine->initWith(&TNerveCannonSearch::theNerve());

		if (unk230 == 9) {
			unk239 = false;
			JGeometry::TVec3<f32> target(-565.0f, 8500.0f, 7675.0f);
			unkF4  = TPathNode(target);
			unk104 = TPathNode(target);
			unk114.clear();
		} else {
			setGoalPath(TPathNode((THitActor*)gpMarioAddress));
		}

		unk1A8 = new TChorobei(this, 0, "チョロベー");
		unk1A8->initHitActor(
		    0x1000001d, 3, 0x90000000,
		    unk28C->mSLChorobeiAttackRadius.get(),
		    unk28C->mSLChorobeiAttackHeight.get(),
		    unk28C->mSLChorobeiDamageRadius.get(),
		    unk28C->mSLChorobeiDamageHeight.get());
		JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")->add(unk1A8);

		JUTNameTab* joints
		    = getMActor()->getModel()->getModelData()->getJointName();
		for (int i = 0; i < 3; ++i) {
			u16 index = joints->getIndex(sCannonDomPartsJointTable[i]);
			unk1AC[i] = new TCannonDom(this, index, domData, 3, "砲台");
			unk1C0[i] = new TMapCollisionMove();
			unk1C0[i]->init("/cannon/CannonDom", 2, this);
			unk1C0[i]->moveTrans(mPosition);
		}

		unk2B0 = new TMapCollisionMove();
		unk2B0->init("/cannon/CannonFuta", 2, this);
		unk2B0->moveTrans(mPosition);
	} else {
		mSpine->initWith(&TNerveCannonObject::theNerve());
		JUTNameTab* joints
		    = getMActor()->getModel()->getModelData()->getJointName();
		u16 index = joints->getIndex("nullA");
		unk1B8    = new TCannonDom(this, index, domData, 3, "砲台");
		unk1B8->unk24 = true;
	}

	void* marioRes
	    = JKRFileLoader::getGlbResource("/scene/cannon/hodai_mario.bmd");
	SDLModelData* marioData
	    = new SDLModelData(J3DModelLoaderDataBase::load(marioRes, 0x10010000));
	unk1BC = new TSharedParts(this, 0, marioData, 3, "<TSharedParts>");
	unk258 = new TMapCollisionMove();
	unk258->init(2, 0, 0, nullptr);
}

void TCannon::loadAfter()
{
	for (int i = 0; i < 5; ++i)
		SMS_LoadParticle(cannonParticleFiles[i], 0xe8 + i);

	SMS_LoadParticle(cannonParticleFiles[5], 0x166);

	if (unk230 == 5) {
		unk254 = JDrama::TNameRefGen::search<TMareGate>("efMareGate");
		if (unk254 != nullptr)
			((TMareGate*)unk254)->makeObjAppeared();
	}
}

void TCannon::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	setMActorAndKeeper();
	TSpineEnemyParams* params = getSaveParam();
	mHitPoints = params ? params->mSLHitPointMax.get() : 1;
	unk230 = gpApplication.mCurrArea.unk0;
	unk23C = mPosition;
}

TCannon::TCannon(const char* name)
    : TSmallEnemy(name)
    , unk1A0(nullptr)
    , unk1A8(nullptr)
    , unk1B8(nullptr)
    , unk1E0(nullptr)
    , unk214(0)
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
	for (int i = 0; i < mColCount; ++i) {
		THitActor* actor = mCollisions[i];
		if (actor->mActorType == 0x80000001)
			SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);

		if (actor->mActorType == 0x1000001e) {
			TBombHei* bomb = (TBombHei*)actor;
			if (unk68->mSpine->getCurrentNerve()
			        != &TNerveCannonDamage::theNerve()
			    && bomb->isDamageToCannon()) {
				unk68->mSpine->pushNerve(&TNerveCannonDamage::theNerve());
				bomb->kill();
			}
		}

		if (actor->mActorType == 0x1000001f) {
			TKiller* killer = (TKiller*)actor;
			if (killer->isRollFly()) {
				unk68->mSpine->pushNerve(&TNerveCannonDamage::theNerve());
				killer->kill();
			}
		}
	}

	return FALSE;
}

void TChorobei::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((unk68->mLiveFlag & 0x7) != 0)
		return;

	if (unk70 != 0.0f)
		return;

	if (flags & 2) {
		if (unk74 != nullptr && unk78 != nullptr) {
			J3DFrameCtrl* ctrl = unk6C->getMActor()->getFrameCtrl(0);
			unk74->animeLoop(&mPosition, ctrl->getFrame(), ctrl->getRate(), 0,
			                 4);
		}

		Mtx mtx;
		PSMTXCopy(unk6C->getConnectedMtx(), mtx);
		mtx[1][3] += unk7C;
		PSMTXCopy(mtx, unk6C->getMActor()->getModel()->getBaseTRMtx());
		mPosition.x = mtx[0][3];
		mPosition.y = mtx[1][3] - 150.0f;
		mPosition.z = mtx[2][3];
	}

	THitActor::perform(flags, graphics);
	unk6C->getMActor()->perform(flags, graphics);
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
