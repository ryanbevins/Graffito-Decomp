#include <Enemy/Rocket.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Camera/Camera.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <Strategic/Spine.hpp>
#include <System/Particles.hpp>
#include <System/MarDirector.hpp>
#include <System/EmitterViewObj.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Player/Watergun.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <M3DUtil/MActor.hpp>
#include <Strategic/ObjModel.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JGeometry/JGRotation3.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <dolphin/mtx.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

f32 TRocket::mTestAng_y     = 90.0f;
f32 TRocket::mNozzleOffsetZ = 25.0f;
f32 TRocket::mColOffsetY    = 20.0f;
f32 TRocket::mTestAng_x;
f32 TRocket::mTestAng_z;

static const char* rocket_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

DEFINE_NERVE(TNerveRocketWait, TLiveActor)
{
	TRocket* self = (TRocket*)spine->getBody();
	if (spine->getTime() == 0) {
		self->mLiveFlag |= LIVE_FLAG_UNK10;
		self->setBckAnm(3);
	}
	return FALSE;
}

DEFINE_NERVE(TNerveRocketFly, TLiveActor)
{
	TRocket* self = (TRocket*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(1);

		TWaterGun* wg = (TWaterGun*)SMS_GetMarioWaterGun();
		MtxPtr mtx    = wg->getEmitMtx(0);

		f32 speed = self->mParams->mSLReleaseSpeed.value;
		JGeometry::TVec3<f32> v;
		v.x             = speed * mtx[0][0];
		v.y             = speed * mtx[1][0];
		v.z             = speed * mtx[2][0];
		self->mVelocity = v;

		self->mLiveFlag |= LIVE_FLAG_AIRBORNE;
		((TRocketManager*)self->mManager)->mActiveFlag = 1;
		self->mUnk1A0                                  = 0;

		f32 angle;
		if (v.z == 0.0f) {
			angle = v.x >= 0.0f ? 90.0f : -90.0f;
		} else if (v.z > 0.0f) {
			angle = matan(v.x, v.z) * (360.0f / 65536.0f);
		} else {
			f32 m = matan(v.x, -v.z) * (360.0f / 65536.0f);
			angle = 180.0f - m;
		}
		f32 wrapped       = callMsWrap(angle, 0.0f, 360.0f);
		self->mRotation.x = 0.0f;
		self->mRotation.y = wrapped;
		self->mRotation.z = 0.0f;

		self->unk64 &= ~1u;
	}

	bool match = (self->mCurrentBckAnm == 1) ? true : false;
	if (!match)
		self->setBckAnm(1);

	JGeometry::TVec3<f32> vel = self->mVelocity;
	self->mRotation.x         = MsGetRotFromZaxis(vel).x;

	gpMarioParticleManager->emitAndBindToPosPtr(0x179, &self->mPosition, 1,
	                                            self);

	if (self->mSpine->getTime() > self->mParams->mSLFlyLimitTime.value)
		self->kill();

	if (!self->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		self->getModel();

	return FALSE;
}

DEFINE_NERVE(TNerveRocketPossessedNozzle, TLiveActor)
{
	TRocket* self = (TRocket*)spine->getBody();
	if (spine->getTime() == 0) {
		SMSRumbleMgr->start(0x15, 0xa, (f32*)nullptr);
		if (gpMSound->gateCheck(0x180c)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x180c, &self->mPosition, 0, nullptr, 0, 4);
		}
		if (gpMSound->gateCheck(0x825)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x825, &self->mPosition, 0, nullptr, 0, 4);
		}
		((TRocketManager*)self->mManager)->mActiveFlag = 0;
		self->mLiveFlag &= ~LIVE_FLAG_UNK10;
		self->mUnk1A0 = 1;
		self->setBckAnm(0);
	}

	SMS_SendMessageToMario((THitActor*)self, 5);

	u8* gamepad = *(u8**)*(u8**)((u8*)gpMarDirector + 0x18);

	u8 marioJumpFrames = (u8)(int)*(f32*)(gamepad + 0xb4);
	if (marioJumpFrames > 0x14) {
		u8 hp = self->mHitPoints;
		if (hp > 1)
			self->mHitPoints -= 1;
	}

	bool bckMatch = (self->mCurrentBckAnm == 2) ? true : false;
	if (!bckMatch) {
		if (gpMSound->gateCheck(0x4807)) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4807, 0, nullptr, 0);
		}
		self->setBckAnm(2);
	}

	bool firePressed;
	u8* gamepad2 = *(u8**)*(u8**)((u8*)gpMarDirector + 0x18);
	if (*(u32*)(gamepad2 + 0xd4) & 0x400) {
		self->unk190 = 0.0f;
		self->expandCollision();
		if (gpMSound->gateCheck(3)) {
			MSoundSESystem::MSoundSE::startSoundActor(3, &self->mPosition, 0,
			                                          nullptr, 0, 4);
		}
		SMSRumbleMgr->start(0x15, 5, (f32*)nullptr);
		firePressed = true;
	} else {
		firePressed = false;
	}

	if (firePressed) {
		spine->pushAfterCurrent(&TNerveRocketFly::theNerve());
		return TRUE;
	}
	return FALSE;
}

const char** TRocket::getBasNameTable() const { return rocket_bastable; }

bool TRocket::isAttack()
{
	return (mSpine->getCurrentNerve() == &TNerveRocketFly::theNerve()) ? true
	                                                                   : false;
}

bool TRocket::isCollidMove(THitActor* other)
{
	if (mSpine->getCurrentNerve() == &TNerveRocketFly::theNerve()) {
		if (other->receiveMessage((THitActor*)this, 0)) {
			kill();
		}
	}
	return false;
}

f32 TRocket::getGravityY() const
{
	f32 g = mGravity;
	if (mSpine->getCurrentNerve() == &TNerveRocketFly::theNerve())
		g = mParams->mSLFlyGravity.get();
	return g;
}

void TRocket::setDeadAnm()
{
	JGeometry::TVec3<f32> p = mPosition;
	TWaterEmitInfo* ei = ((TRocketManager*)mManager)->mWaterEmitInfo;
	ei->mPos.value     = p;
	gpModelWaterManager->emitRequest(*ei);

	if (mUnk1A0) {
		((TRocketManager*)mManager)->mActiveFlag = 1;
		mUnk1A0                                  = 0;
	}

	mLiveFlag |= 0x20000;

	MtxPtr mtx = (MtxPtr)((u8*)mMActor->getModel() + 0x20);
	gpMarioParticleManager->emitAndBindToMtxPtr(0xc1, mtx, 0, nullptr);
	gpMarioParticleManager->emitAndBindToMtxPtr(0xc2, mtx, 0, nullptr);
}

void TRocket::bind()
{
	if (checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	if (mSpine->getCurrentNerve() == &TNerveRocketPossessedNozzle::theNerve()
	    || mSpine->getCurrentNerve() == &TNerveRocketFly::theNerve()) {
		TBGWallCheckRecord rec(mPosition.x, mPosition.y, mPosition.z,
		                       mBodyScale * mWallRadius, 1, 0);
		if (gpMap->isTouchedWallsAndMoveXZ(&rec)) {
			TBGCheckData* wall = rec.mResultWalls[0];
			if (*(THitActor**)((u8*)wall + 0x44))
				(*(THitActor**)((u8*)wall + 0x44))
				    ->receiveMessage((THitActor*)this, 0xe);
			kill();
			return;
		}

		if (mSpine->getCurrentNerve() != &TNerveRocketFly::theNerve())
			return;

		TLiveActor::bind();
		if (checkLiveFlag(LIVE_FLAG_AIRBORNE) ? 1 : 0)
			return;

		if (*(THitActor**)((u8*)mGroundPlane + 0x44))
			(*(THitActor**)((u8*)mGroundPlane + 0x44))
			    ->receiveMessage((THitActor*)this, 0xe);
		kill();
		return;
	}

	TLiveActor::bind();
}

void TRocket::behaveToWater(THitActor* p) { attackToMario(); }

void TRocket::attackToMario()
{
	if (mSpine->getCurrentNerve() == &TNerveRocketWait::theNerve()
	    && ((TRocketManager*)mManager)->mActiveFlag) {
		mSpine->pushNerve(&TNerveRocketPossessedNozzle::theNerve());
	}
}

void TRocket::reset()
{
	mUnk1A0 = 0;
	TSmallEnemy::reset();

	if (mInitialPosSaved) {
		mPosition = mInitialPos;
	}

	mLiveFlag |= LIVE_FLAG_UNK10;
	mLiveFlag &= ~LIVE_FLAG_UNK800;
	mLiveFlag |= LIVE_FLAG_UNK8;

	mSpine->initWith(&TNerveRocketWait::theNerve());
}

void TRocket::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("rocket.bmd", 3);
}

void TRocket::calcRootMatrix()
{
	if (mUnk1A0) {
		J3DModel* model = getModel();
		model->setBaseScale(mScaling);

		Mtx tmp;
		if (mSpine->getCurrentNerve() == &TNerveRocketFly::theNerve()) {
			JGeometry::TRotation3<
			    JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >
			    r;
			r.identity33();
			f32* tm = (f32*)tmp;
			for (int i = 0; i < 12; ++i)
				tm[i] = ((f32*)&r)[i];
			tmp[0][3] = mPosition.x;
			tmp[1][3] = mPosition.y;
			tmp[2][3] = mPosition.z;
		} else {
			TWaterGun* wg = (TWaterGun*)SMS_GetMarioWaterGun();
			MtxPtr emit   = wg->getEmitMtx(0);
			PSMTXCopy(emit, tmp);

			f32 len0 = tmp[0][0] * tmp[0][0] + tmp[1][0] * tmp[1][0]
			           + tmp[2][0] * tmp[2][0];
			if (len0 > 0.0f)
				len0 = JGeometry::TUtil<f32>::sqrt(len0);

			f32 len1 = tmp[0][1] * tmp[0][1] + tmp[1][1] * tmp[1][1]
			           + tmp[2][1] * tmp[2][1];
			if (len1 > 0.0f)
				len1 = JGeometry::TUtil<f32>::sqrt(len1);

			f32 len2 = tmp[0][2] * tmp[0][2] + tmp[1][2] * tmp[1][2]
			           + tmp[2][2] * tmp[2][2];
			if (len2 > 0.0f)
				len2 = JGeometry::TUtil<f32>::sqrt(len2);

			// Target guards the normalized columns in this cross order.
			if (len2 != 0.0f) {
				tmp[0][0] /= len0;
				tmp[1][0] /= len0;
				tmp[2][0] /= len0;
			}

			if (len0 != 0.0f) {
				tmp[0][1] /= len1;
				tmp[1][1] /= len1;
				tmp[2][1] /= len1;
			}

			if (len1 != 0.0f) {
				tmp[0][2] /= len2;
				tmp[1][2] /= len2;
				tmp[2][2] /= len2;
			}

			Mtx rotOff;
			JGeometry::TRotation3<
			    JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >
			    r;
			r.identity33();
			f32* tr = (f32*)rotOff;
			for (int i = 0; i < 12; ++i)
				tr[i] = ((f32*)&r)[i];
			rotOff[0][3] = mNozzleOffsetZ;
			rotOff[1][3] = 0.0f;
			rotOff[2][3] = 0.0f;
			PSMTXConcat(tmp, rotOff, tmp);

			mPosition.x = tmp[0][3];
			mPosition.y = tmp[1][3] - mColOffsetY;
			mPosition.z = tmp[2][3];
		}

		Mtx rot;
		MsMtxSetRotRPH(rot, mTestAng_x, mTestAng_y, mTestAng_z);
		PSMTXConcat(tmp, rot, tmp);
		PSMTXCopy(tmp, (MtxPtr)((u8*)getModel() + 0x20));
	} else {
		TSpineEnemy::calcRootMatrix();
	}

	bool bckMatch = (mCurrentBckAnm == 1) ? true : false;
	if (bckMatch) {
		if (gpMSound->gateCheck(3)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    3, &mPosition, 0, nullptr, 0, 4);
		}
	}
}

void TRocket::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x1000002b;
	unk150     = 0x11;
	mParams    = (TRocketParams*)getSaveParam();
	mSpine->initWith(&TNerveRocketWait::theNerve());
	unk64 |= 0x08000000;
}

void TRocket::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	mInitialPos      = mPosition;
	mInitialPosSaved = 1;
	reset();
}

TRocket::TRocket(const char* name)
    : TSmallEnemy(name)
{
	mUnk1A0          = 0;
	mInitialPosSaved = 0;
}

void TRocketManager::perform(u32 param, JDrama::TGraphics* graphics)
{
	if (param & 1) {
		int i = 0;
		while (true) {
			int limit;
			if (!unk38) {
				limit = mObjNum;
			} else {
				int aen
				    = ((TSpineEnemyParams*)unk38)->mSLActiveEnemyNum.value;
				limit = aen <= mObjNum ? aen : mObjNum;
			}
			if (i >= limit)
				break;
			TRocket* a = (TRocket*)unk18[i];
			if (a->mLiveFlag & LIVE_FLAG_DEAD)
				a->reset();
			++i;
		}
	}
	TEnemyManager::perform(param, graphics);
}

void TRocketManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "rocket.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TRocketManager::initSetEnemies()
{
	for (int i = 0; i < mObjNum; ++i) {
		TGraphWeb* web  = gpConductor->getGraphByName("main");
		TRocket* rocket = (TRocket*)unk18[i];
		if ((rocket->mLiveFlag & LIVE_FLAG_DEAD) && !web->isDummy()) {
			int nodeCount = web->unk8;
			int idx = (int)(rand() * 0.000030517578f * (f32)nodeCount);

			Vec p;
			web->unk0[idx].getPoint(&p);
			rocket->mPosition = *(JGeometry::TVec3<f32>*)&p;
			rocket->mPosition.y += 5.0f;
			rocket->mLiveFlag |= LIVE_FLAG_AIRBORNE;
			rocket->reset();
		}
	}
}

TSmallEnemy* TRocketManager::createEnemyInstance() { return new TRocket("ロケット"); }

void TRocketManager::loadAfter() { JDrama::TNameRef::loadAfter(); }

void TRocketManager::clipEnemies(JDrama::TGraphics* graphics) { }

void TRocketManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);

	TRocketParams* params = new TRocketParams("/enemy/rocket.prm");
	unk38                 = params;

	mWaterEmitInfo = new TWaterEmitInfo("/enemy/rocketexpwater.prm");
}

TRocketManager::TRocketManager(const char* name)
    : TSmallEnemyManager(name)
{
	mActiveFlag    = 1;
	unk64          = 0;
	mWaterEmitInfo = nullptr;
}

TRocketParams::TRocketParams(const char* path)
    : TSmallEnemyParams(path)
    , PARAM_INIT(mSLReleaseSpeed, 10.0f)
    , PARAM_INIT(mSLFlyGravity, 0.0f)
    , PARAM_INIT(mSLFlyLimitTime, 300)
{
	load(mPrmPath);
}
