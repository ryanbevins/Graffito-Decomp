#include <Enemy/ElecNokonoko.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <Strategic/Strategy.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

// ------------------------------------------------------------------ nerves

DEFINE_NERVE(TNerveElecCarapaceReturn, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecCarapaceWait, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecCarapaceMove, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoFreeze, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoCollect, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoAttack, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoRebirth, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoTurn, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveElecNokonokoShoot, TLiveActor)
{
	return false;
}

// --------------------------------------------------------------- bas table

static const char* dennoko_bastable[] = {
	"/scene/dennoko/bas/dennoko_catch1.bas",        // 0
	"/scene/dennoko/bas/dennoko_down1.bas",         // 1
	"/scene/dennoko/bas/dennoko_elec_down1.bas",    // 2
	"/scene/dennoko/bas/dennoko_hit1.bas",          // 3
	nullptr,                                        // 4
	nullptr,                                        // 5
	"/scene/dennoko/bas/dennoko_mogaki1_loop.bas",  // 6
	"/scene/dennoko/bas/dennoko_mogaki1_start.bas", // 7
	nullptr,                                        // 8
	nullptr,                                        // 9
	"/scene/dennoko/bas/dennoko_run1_loop.bas",     // 10
	nullptr,                                        // 11
	"/scene/dennoko/bas/dennoko_shoot1.bas",        // 12
	"/scene/dennoko/bas/dennoko_supply1.bas",       // 13
	nullptr,                                        // 14
	"/scene/dennoko/bas/dennoko_turn1_loop.bas",    // 15
	nullptr,                                        // 16
	nullptr,                                        // 17
};

const char** TElecNokonoko::getBasNameTable() const { return dennoko_bastable; }

u8 TElecNokonoko::mReflectSw = 1;

// ------------------------------------------------------ TElecNokonoko: anims

void TElecNokonoko::setDeadAnm() { setBckAnm(1); }

void TElecNokonoko::setWaitAnm()
{
	unk198 = 0;
	setBckAnm(0x11);
}

void TElecNokonoko::setWalkAnm()
{
	if (mCurrentBckAnm != 0xa) {
		setBckAnm(0xb);
	}
}

void TElecNokonoko::setRunAnm()
{
	if (mCurrentBckAnm != 0xa) {
		setBckAnm(0xb);
	}
}

void TElecNokonoko::setMeltAnm()
{
	setBckAnm(2);
	mLiveFlag |= 8;
	unk18C = 3;

	f32 zero = 0.0f;
	Vec zeroVec;
	zeroVec.x             = zero;
	zeroVec.y             = zero;
	zeroVec.z             = zero;
	mCarapace->mScaling.x = zeroVec.x;
	mCarapace->mScaling.y = zeroVec.y;
	mCarapace->mScaling.z = zeroVec.z;

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0xCA, (MtxPtr)((u8*)mMActor->getModel()->mNodeMatrices + 0x58), 0,
	    nullptr);
	if (emitter) {
		emitter->unk154.x = zeroVec.x;
		emitter->unk154.y = zeroVec.y;
		emitter->unk154.z = zeroVec.z;
		emitter->unk174.x = zeroVec.x;
		emitter->unk174.y = zeroVec.y;
		emitter->unk174.z = zeroVec.z;
	}
}

// ----------------------------------------------------- TElecNokonoko: stubs

void TElecNokonoko::load(JSUMemoryInputStream& stream) { TWalkerEnemy::load(stream); }

BOOL TElecNokonoko::receiveMessage(THitActor* sender, u32 message) { return FALSE; }

void TElecNokonoko::init(TLiveManager* manager) { TWalkerEnemy::init(manager); }

void TElecNokonoko::calcRootMatrix()
{
	TSpineEnemy::calcRootMatrix();

	bool isReady = unk1A4 == 0;
	if (isReady
	    && mSpine->getCurrentNerve() != &TNerveElecNokonokoFreeze::theNerve()
	    && mSpine->getCurrentNerve() != &TNerveSmallEnemyDie::theNerve()
	    && mSpine->getCurrentNerve()
	           != &TNerveElecNokonokoCollect::theNerve()) {
		if (gpMSound->gateCheck(0x20BB)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x20BB, (const Vec*)&mPosition, 0, nullptr, 0, 4);
		}

		JPABaseEmitter* emitter1 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17A, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x150), 1, this);
		if (emitter1) {
			emitter1->setScale(getScaling());
		}

		JPABaseEmitter* emitter2 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17B, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x150), 1, this);
		if (emitter2) {
			emitter2->setScale(getScaling());
		}

		JPABaseEmitter* emitter3 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17C, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x150), 1, this);
		if (emitter3) {
			emitter3->setScale(getScaling());
		}
	}

	if (mCurrentBckAnm == 2) {
		JPABaseEmitter* emitter4 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x17D, (MtxPtr)mMActor->unk4->mNodeMatrices, 1, this);
		if (emitter4) {
			emitter4->setScale(getScaling());
		}

		Mtx* nodeMtx
		    = (Mtx*)((u8*)mMActor->unk4->mNodeMatrices + 0x180);
		((JGeometry::TVec3<f32>*)&unk1A8)
		    ->set((*nodeMtx)[0][3], (*nodeMtx)[1][3], (*nodeMtx)[2][3]);
		JPABaseEmitter* emitter5 = gpMarioParticleManager->emitAndBindToPosPtr(
		    0x17E, (JGeometry::TVec3<f32>*)&unk1A8, 1, this);
		if (emitter5) {
			emitter5->setScale(getScaling());
		}

		if (mMActor->getFrameCtrl(0)->checkPass(72.0f)) {
			JPABaseEmitter* emitter6
			    = gpMarioParticleManager->emitAndBindToPosPtr(
			        0x17F, (JGeometry::TVec3<f32>*)&unk1A8, 1, this);
			if (emitter6) {
				emitter6->setScale(getScaling());
			}
		}
	}
}

void TElecNokonoko::moveObject() { }

void TElecNokonoko::genRandomItem() { }

void TElecNokonoko::behaveToWater(THitActor* water) { }

void TElecNokonoko::attackToMario() { }

void TElecNokonoko::setMActorAndKeeper() { }

void TElecNokonoko::sendAttackMsgToMario() { }

void TElecNokonoko::behaveToFindMario() { }

bool TElecNokonoko::isResignationAttack() { return false; }

void TElecNokonoko::rest() { }

TElecNokonoko::TElecNokonoko(const char* name)
    : TWalkerEnemy(name)
    , mCarapace(nullptr)
    , unk198(0)
    , unk1A4(0)
{
}

// --------------------------------------------------------- TElecCarapace

void TElecCarapace::perform(u32 flags, JDrama::TGraphics* graphics) { }
BOOL TElecCarapace::receiveMessage(THitActor* sender, u32 message) { return FALSE; }
void TElecCarapace::calcRootMatrix()
{
	MsMtxSetXYZRPH(mMActor->unk4->getBaseTRMtx(), mPosition.x, mPosition.y,
	               mPosition.z, mRotation.x, mRotation.y + unk188, mRotation.z);
	mMActor->unk4->setBaseScale(mScaling);

	if (gpMSound->gateCheck(0x20bc)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x20bc, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	}

	JPABaseEmitter* emitter0 = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17a, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x60), 1, this);
	if (emitter0) {
		emitter0->setScale(unk16C->getScaling());
	}

	JPABaseEmitter* emitter1 = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17b, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x60), 1, this);
	if (emitter1) {
		emitter1->setScale(unk16C->getScaling());
	}

	JPABaseEmitter* emitter2 = gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x17c, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x60), 1, this);
	if (emitter2) {
		emitter2->setScale(unk16C->getScaling());
	}
}
void TElecCarapace::bind() { }
void TElecCarapace::kill() { }
void TElecCarapace::loadInit(TSpineEnemy* host, const char* name) { }
void TElecCarapace::appear() { }
void TElecCarapace::rebirth() { }
void TElecCarapace::sendMessage()
{
	for (int i = 0; i < mColCount; ++i) {
		THitActor* col = mCollisions[i];
		if (col->isActorTypeOf(0x80000000)) {
			if (SMS_SendMessageToMario(this, 9)) {
				unk64 |= 1;
				if (mSpine->getCurrentNerve()
				    != &TNerveElecCarapaceWait::theNerve()) {
					mSpine->pushNerve(&TNerveElecCarapaceWait::theNerve());
				}
			}
			continue;
		}

		if (col == unk16C) {
			unk64 &= ~1;
			continue;
		}

		if (col->isActorTypeOf(0x1000000)) {
			// TODO: fabricated stack slots standing in for an inlined helper
			// (probably an electric-spark effect) that was DCE'd down to its
			// rand() side effects; remove once the real inline is identified.
			volatile s32 sparkData[2];
			sparkData[0] = 0;
			sparkData[1] = 0x168;
			for (int j = 0; j < 5; ++j) {
				rand();
				rand();
				rand();
			}
		} else if (TElecNokonoko::mReflectSw) {
			reflect(col);
		}
	}
}
void TElecCarapace::behaveToHitGround()
{
	if (unk176)
		unk184 = 1;

	u16 type = mGroundPlane->mBGType;
	bool isSlope;
	if (type == 0x100 || type == 0x101
	    || (u16)(type - 0x102) <= 3 || type == 0x4104) {
		isSlope = true;
	} else {
		isSlope = false;
	}

	if (isSlope)
		behaveToHost();

	unk176       = 0;
	unk168       = 1;
	mLiveFlag &= ~0x80;
	mVelocity.x = 0.0f;
	mVelocity.y = 0.0f;
	mVelocity.z = 0.0f;
}
void TElecCarapace::behaveToHitWall(const TBGCheckData* wall)
{
	if (unk180 > 0)
		return;
	if (!TElecNokonoko::mReflectSw)
		return;

	unk180 = 1;
	unk184 = 0;
	unk176 = 1;
	unk175 = 1;

	f32 dot = mLinearVelocity.y * wall->mNormal.y
	          + mLinearVelocity.x * wall->mNormal.x
	          + mLinearVelocity.z * wall->mNormal.z;
	dot *= -1.5f;
	mVelocity.x = dot * wall->mNormal.x;
	mVelocity.y = 3.0f;
	mVelocity.z = dot * wall->mNormal.z;
	mPosition.y = 2.0f + mGroundHeight;

	setGoalPath(TPathNode(unk16C->getPosition()));
}
void TElecCarapace::setBehavior() { }
void TElecCarapace::recoverScale() { }
f32 TElecCarapace::getNowGravity() { return mGravity; }
f32 TElecCarapace::getPhaseShift() const { return 0.0f; }
void TElecCarapace::shoot()
{
	unk180 = 0;
	if (unk150 == 2)
		return;

	JGeometry::TVec3<f32> dir = *gpMarioPos;
	dir.sub(mPosition);

	JGeometry::TVec3<f32> target = dir;
	MsVECNormalize((Vec*)&target, (Vec*)&target);

	f32 flyDist
	    = ((TElecNokonoko*)unk16C)->mSaveParams->mSLCarapaceFlyDist.get();
	target.x *= flyDist;
	target.y *= mPosition.y;
	target.z *= flyDist;
	target.x += mPosition.x;
	target.y += mPosition.y;
	target.z += mPosition.z;

	unk174 = (unk174 == 0);
	unk188 = 0.0f;
	unk150 = 2;
	unk176 = 0;
	unk168 = 0;
	unk184 = 0;
	unk175 = 0;
	offHitFlag(1);

	mSpine->initWith(&TNerveElecCarapaceMove::theNerve());
	setGoalPath(TPathNode(target));

	f32 speed = MsRandF(3.0f, 5.0f);

	JGeometry::TVec3<f32> toGoal = unk104.getPoint();
	toGoal.sub(mPosition);
	unk178 = speed * JGeometry::TUtil<f32>::sqrt(toGoal.dot(toGoal));

	unk17C = MsRandF(20.0f, 30.0f);
}
void TElecCarapace::reflect(THitActor* other)
{
	if ((THitActor*)unk170 == other)
		return;

	unk184 = 0;
	unk170 = (int)other;
	unk176 = 1;
	unk175 = 0;

	JGeometry::TVec3<f32> dir(other->mPosition.x - mPosition.x, 0.0f,
	                          other->mPosition.z - mPosition.z);

	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f)
		dir.x = dir.x + 1.0f;

	MsVECNormalize((Vec*)&dir, (Vec*)&dir);

	f32 xDir = 0.0f;
	f32 zDir = 0.0f;
	if (__fabsf(dir.z / dir.x) > 1.0f) {
		if (other->mPosition.z > mPosition.z)
			zDir = 1.0f;
		else
			zDir = -1.0f;
	} else {
		if (other->mPosition.x > mPosition.x)
			xDir = 1.0f;
		else
			xDir = -1.0f;
	}

	f32 dot = dir.y * 0.0f + dir.x * xDir + dir.z * zDir;
	f32 force = -7.0f * dot;
	mVelocity.x = dir.x * force;
	mVelocity.y = 2.0f;
	mVelocity.z = dir.z * force;
	mPosition.y = 2.0f + mGroundHeight;

	unk174 = 0;
	if ((mVelocity.x > 0.0f && mVelocity.z > 0.0f)
	    || (mVelocity.x < 0.0f && mVelocity.z < 0.0f))
		unk174 = 1;

	setGoalPath(TPathNode(unk16C->getPosition()));
}

TElecCarapace::TElecCarapace(const char* name)
    : TEnemyAttachment(name)
    , unk16C(0)
    , unk170(0)
    , unk174(1)
    , unk175(0)
    , unk176(0)
    , unk180(0)
    , unk184(0)
{
	unk178 = 0.0f;
	unk17C = 0.0f;
	unk188 = 0.0f;
	unk198 = 0.0f;
}

// ------------------------------------------------------------ params

TElecNokonokoSaveLoadParams::TElecNokonokoSaveLoadParams(const char* path)
    : TWalkerEnemyParams(path)
    , PARAM_INIT(mSLReadyTime, 300)
    , PARAM_INIT(mSLCarapaceGravity, 0.01f)
    , PARAM_INIT(mSLCarapaceSpeed, 5.0f)
    , PARAM_INIT(mSLCarapaceTurnSpeed, 2.0f)
    , PARAM_INIT(mSLCarapaceSpinSpeed, 2.0f)
    , PARAM_INIT(mSLCarapaceShootRange, 500.0f)
    , PARAM_INIT(mSLCarapaceFlyDist, 100.0f)
{
	load(mPrmPath);
}

// ------------------------------------------------------------ manager

TSpineEnemy* TElecNokonokoManager::createEnemyInstance()
{
	return new TElecNokonoko(
	    "\x93\x64\x8B\x43\x83\x6D\x83\x52\x83\x6D\x83\x52");
}

void TElecNokonokoManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "dennoko_model1.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TElecNokonokoManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TElecNokonokoSaveLoadParams("/enemy/elecNokonoko.prm");
	TSmallEnemyManager::load(stream);
}

void TElecNokonokoManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TEnemyManager::perform(flags, graphics);
}

void TElecNokonokoManager::clipEnemies(JDrama::TGraphics* graphics)
{
	TEnemyManager::clipEnemies(graphics);
}

void TElecNokonokoManager::initSetEnemies() { }

TElecNokonokoManager::TElecNokonokoManager(const char* name)
    : TSmallEnemyManager(name)
{
}
