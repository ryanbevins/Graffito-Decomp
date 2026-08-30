#include <Enemy/AmiNoko.hpp>
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
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <Strategic/Strategy.hpp>
#include <new>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

extern "C" void* __vt__7TAmiHit[];

DEFINE_NERVE(TNerveAmiNokoFreeze, TLiveActor)
{
	TAmiNoko* self = (TAmiNoko*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(2);
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0xCA, (MtxPtr)self->mMActor->unk4->mNodeMatrices, 0, nullptr);
		if (emitter) {
			f32 zero = 0.0f;
			emitter->unk154.x = zero;
			emitter->unk154.y = zero;
			emitter->unk154.z = zero;
			emitter->unk174.x = zero;
			emitter->unk174.y = zero;
			emitter->unk174.z = zero;
		}
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(2)) {
			self->setBckAnm(15);
		} else {
			TSmallEnemyParams* params = (TSmallEnemyParams*)self->getSaveParam();
			if (spine->getTime() > params->getSLFreezeWait()) {
				return true;
			}
		}
	}

	return false;
}

DEFINE_NERVE(TNerveAmiNokoDie, TLiveActor)
{
	TAmiNoko* self = (TAmiNoko*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setBckAnm(1);
		self->unk64 |= 1;
	}

	if (self->checkCurAnmEnd(0) && self->isBckAnm(1)) {
		JGeometry::TVec3<f32> dir = self->mPosition - *gpMarioPos;

		JGeometry::TVec3<f32> normDir = dir;

		f32 zero = 0.0f;
		if (normDir.x == zero && normDir.y == zero && normDir.z == zero) {
			normDir.x = 1.0f;
		}

		MtxPtr baseMtx = self->mMActor->unk4->unk20;
		normDir.x      = baseMtx[0][1];
		normDir.y      = baseMtx[1][1];
		normDir.z      = baseMtx[2][1];
		MsVECNormalize((Vec*)&normDir, (Vec*)&normDir);

		f32 speed   = 20.0f;
		normDir.x  *= speed;
		normDir.y  *= speed;
		normDir.z  *= speed;
		self->mPosition.y += 10.0f;
		self->mVelocity = normDir;
		self->mLiveFlag |= LIVE_FLAG_AIRBORNE;
		self->setBckAnm(0);
	}

	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x174, self->mMActor->unk4->unk20, 1, self);

	if (self->isBckAnm(0) && spine->getTime() > 30) {
		JGeometry::TVec3<f32> dir2 = self->mPosition - *gpMarioPos;

		JGeometry::TVec3<f32> toMario = dir2;

		bool hitWall = false;
		if (gpMap->isTouchedOneWallAndMoveXZ(
		        &self->mPosition.x, self->mPosition.y, &self->mPosition.z,
		        3.0f * self->mBodyRadius)) {
			JGeometry::TVec3<f32> zeroVec(0.0f, 0.0f, 0.0f);
			self->mVelocity = zeroVec;

			s16 rotY = (s16)(182.04445f * self->mRotation.y);
			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitWithRotate(
			        0xE2, &self->mPosition, 0, rotY, 0, 0, nullptr);
			if (emitter) {
				emitter->unk154.x = self->mScaling.x;
				emitter->unk154.y = self->mScaling.y;
				emitter->unk154.z = self->mScaling.z;
				emitter->unk174.x = self->mScaling.x;
				emitter->unk174.y = self->mScaling.y;
				emitter->unk174.z = self->mScaling.z;
			}
			s16 rotY2 = (s16)(182.04445f * self->mRotation.y);
			JPABaseEmitter* emitter2
			    = gpMarioParticleManager->emitWithRotate(
			        0xE3, &self->mPosition, 0, rotY2, 0, 0, nullptr);
			if (emitter2) {
				SMSSetEmitterPolColor(emitter2, 6);
			}
			hitWall = true;
		}
		if (!hitWall) {
			bool isAirborne
			    = self->checkLiveFlag(LIVE_FLAG_AIRBORNE) ? true : false;
			if (isAirborne) {
				f32 len = toMario.length();
				if (len <= 10000.0f) {
					return false;
				}
			}
		}

		self->unk64 |= 1;
		self->mLiveFlag |= LIVE_FLAG_DEAD;
		self->mLiveFlag |= LIVE_FLAG_UNK8;
		self->mLiveFlag &= ~LIVE_FLAG_HIDDEN;
		self->mLiveFlag &= ~LIVE_FLAG_UNK10000;
		self->mHolder = nullptr;
		self->stopAnmSound();
		spine->reset();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		spine->pushAfterCurrent(spine->getDefault());
		self->genRandomItem();
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveAmiNokoTurn, TLiveActor)
{
	TAmiNoko* self = (TAmiNoko*)spine->getBody();

	if (spine->getTime() == 0) {
		if (self->unk210) {
			self->setBckAnm(11);
		} else {
			self->setBckAnm(14);
		}
	}

	if (self->isBckAnm(11) || self->isBckAnm(14)) {
		if (self->checkCurAnmEnd(0)) {
			if (self->unk210) {
				self->setBckAnm(10);
			} else {
				self->setBckAnm(13);
			}
		}
	}

	// Get goal position from pathnode
	JGeometry::TVec3<f32> toGoal = self->getUnkF4().getPoint();
	toGoal.x -= self->mPosition.x;
	toGoal.y -= self->mPosition.y;
	toGoal.z -= self->mPosition.z;

	f32 zero = 0.0f;
	if (zero == toGoal.x && zero == toGoal.y && zero == toGoal.z) {
		toGoal.x = 1.0f;
	}

	PSVECNormalize((Vec*)&toGoal, (Vec*)&toGoal);

	f32 dot = toGoal.x * self->unk1A8.x + toGoal.y * self->unk1A8.y
	          + toGoal.z * self->unk1A8.z;

	if (dot > 0.8f) {
		if (self->checkCurAnmEnd(0)) {
			if (self->isBckAnm(9) || self->isBckAnm(12)) {
				if (self->unk210) {
					self->setBckAnm(15);
				} else {
					self->setBckAnm(15);
				}
			} else if (self->isBckAnm(15)) {
				spine->pushAfterCurrent(&TNerveAmiNokoWalkOnFence::theNerve());
				return true;
			} else {
				if (self->unk210) {
					self->setBckAnm(9);
				} else {
					self->setBckAnm(12);
				}
			}
		}
	}

	self->creepToCurPathNode(0.0f);
	return false;
}

DEFINE_NERVE(TNerveAmiNokoWalkOnFence, TLiveActor)
{
	TAmiNoko* self = (TAmiNoko*)spine->getBody();

	if (spine->getTime() == 0) {
		self->setWalkAnm();
	}

	if (self->isBckAnm(5) || self->isBckAnm(8)) {
		if (self->checkCurAnmEnd(0)) {
			if (self->unk210) {
				self->setBckAnm(4);
			} else {
				self->setBckAnm(7);
			}
		}
	}

	// Get goal position
	JGeometry::TVec3<f32> toGoal = self->unkF4.getPoint();
	toGoal.x -= self->mPosition.x;
	toGoal.y -= self->mPosition.y;
	toGoal.z -= self->mPosition.z;

	// Calculate distance
	f32 dist = JGeometry::TUtil<f32>::sqrt(
	    toGoal.x * toGoal.x + toGoal.y * toGoal.y + toGoal.z * toGoal.z);

	if (dist < 1.5f) {
		if (self->checkCurAnmEnd(0)) {
			if (self->isBckAnm(3) || self->isBckAnm(6)) {
				self->goToRandomNextGraphNode();
				spine->pushAfterCurrent(&TNerveAmiNokoTurn::theNerve());
				return true;
			} else if (self->isBckAnm(4)) {
				self->setBckAnm(3);
			} else if (self->isBckAnm(7)) {
				self->setBckAnm(6);
			}
		}
	}

	self->creepToCurPathNode(2.0f);
	return false;
}

static const char* amiNoko_bastable[] = {
	nullptr,
	"/scene/amiNoko/bas/aminoko_flying1_start.bas",
	"/scene/amiNoko/bas/aminoko_hit1.bas",
	nullptr,
	"/scene/amiNoko/bas/aminoko_run1_loop.bas",
	nullptr,
	nullptr,
	"/scene/amiNoko/bas/aminoko_run2_loop.bas",
	nullptr,
	nullptr,
	"/scene/amiNoko/bas/aminoko_turn1_loop.bas",
	nullptr,
	nullptr,
	"/scene/amiNoko/bas/aminoko_turn2_loop.bas",
	nullptr,
	nullptr,
};

bool TAmiNoko::isCollidMove(THitActor*) { return false; }

const char** TAmiNoko::getBasNameTable() const { return amiNoko_bastable; }

void TAmiNoko::reset() { TWalkerEnemy::reset(); }

void TAmiNoko::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
	stream.read(&mCoinId, 4);
}

void TAmiNoko::setWalkAnm()
{
	if (unk210) {
		setBckAnm(5);
	} else {
		setBckAnm(8);
	}
}

void TAmiNoko::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
	mAmiHit->perform(flags, graphics);
}

void TAmiNoko::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(getManager(), 1);
	mMActor = getActorKeeper()->createMActor("aminoko_model1.bmd", 3);
}

f32 TAmiNoko::getGravityY() const
{
	if (mSpine->getCurrentNerve() == &TNerveAmiNokoDie::theNerve()) {
		return -1.0f;
	}
	return mGravity;
}

TAmiNoko::TAmiNoko(const char* name)
    : TWalkerEnemy(name)
    , unk194(0)
    , unk198(0)
    , unk210(1)
{
	unk19C.set(0.0f, 2.0f, 0.0f);
	unk1A8.set(0.0f, 0.0f, 2.0f);
	unk1B4 = unk19C;
	unk1C0 = unk1A8;
}

void TAmiNoko::behaveToWater(THitActor* water)
{
	if (mSpine->getCurrentNerve() == &TNerveAmiNokoFreeze::theNerve()) {
		return;
	}
	mSpine->pushNerve(&TNerveAmiNokoFreeze::theNerve());
	mSprayedByWaterCooldown = 0;
}

void TAmiNoko::creepToCurPathNode(f32 speed)
{
	if (isBckAnm(4) || isBckAnm(7) || isBckAnm(10) || isBckAnm(13)) {
		// Get current path node position
		JGeometry::TVec3<f32> toGoal = getUnkF4().getPoint();
		toGoal.x -= mPosition.x;
		toGoal.y -= mPosition.y;
		toGoal.z -= mPosition.z;

		f32 zero = 0.0f;
		if (zero == toGoal.x && zero == toGoal.y && zero == toGoal.z) {
			toGoal.x = 1.0f;
		}

		PSVECNormalize((Vec*)&toGoal, (Vec*)&toGoal);

		toGoal.x *= speed;
		toGoal.y *= speed;
		toGoal.z *= speed;

		JGeometry::TVec3<f32> newVel = mLinearVelocity;
		newVel.x += toGoal.x;
		newVel.y += toGoal.y;
		newVel.z += toGoal.z;
		mLinearVelocity = newVel;
	}
}

void TAmiNoko::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x10000021;
	unk150 = 0x11;
	mSpine->initWith(&TNerveAmiNokoWalkOnFence::theNerve());

	mSaveParams = (TAmiNokoSaveLoadParams*)getSaveParam();

	reset();
	setWalkAnm();

	onLiveFlag(LIVE_FLAG_UNK10);
	initialGraphNode();

	if (gpMarDirector->mMap == 8) {
		unk210 = 0;
	}

	TAmiHit* hit = (TAmiHit*)::operator new(sizeof(TAmiHit));
	if (hit) {
		new ((THitActor*)hit) THitActor(
		    "\x83\x41\x83\x7E\x83\x6D\x83\x52\x93\x96\x82\xE8\x94\xBB\x92\xE8");
		*(void**)hit                 = __vt__7TAmiHit;
		*(void**)((u8*)hit + 0x20)   = (u8*)__vt__7TAmiHit + 0x24;
		hit->mOwner = this;

		TIdxGroupObj* enemyGroup
		    = JDrama::TNameRefGen::search<TIdxGroupObj>("\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76");
		enemyGroup->add(hit);

		hit->initHitActor(0x10000021, 1, 0x80000000,
		                  120.0f, 240.0f, 120.0f, 240.0f);
		hit->offHitFlag(HIT_FLAG_NO_COLLISION);
	}

	mAmiHit = hit;
}

void TAmiNoko::bind()
{
	if (isBckAnm(0)) {
		JGeometry::TVec3<f32> nextPos = mPosition;
		nextPos.x += mLinearVelocity.x;
		nextPos.y += mLinearVelocity.y;
		nextPos.z += mLinearVelocity.z;
		nextPos.x += mVelocity.x;
		nextPos.y += mVelocity.y;
		nextPos.z += mVelocity.z;

		f32 gravY = getGravityY();
		mVelocity.y -= gravY;
		if (mVelocity.y < TLiveActor::mVelocityMinY) {
			mVelocity.y = TLiveActor::mVelocityMinY;
		}

		if (checkLiveFlag(LIVE_FLAG_UNK1000)) {
			f32 groundY = gpMap->checkGroundIgnoreWaterSurface(
			    nextPos.x, nextPos.y + mHeadHeight, nextPos.z,
			    &mGroundPlane);
			mGroundHeight = groundY;
		} else {
			f32 groundY = gpMap->checkGround(
			    nextPos.x, nextPos.y + mHeadHeight, nextPos.z,
			    &mGroundPlane);
			mGroundHeight = groundY;
		}

		mGroundHeight += 1.0f;
		if (nextPos.y <= 0.05f + mGroundHeight) {
			u16 flags = *(u16*)((u8*)mGroundPlane + 4);
			u8 isLava = (flags & 0x10) ? 1 : 0;
			if (isLava) {
				kill();
			}
			offLiveFlag(LIVE_FLAG_AIRBORNE);
			mVelocity.set(0.0f, 0.0f, 0.0f);
			nextPos.y = mGroundHeight;
		} else {
			onLiveFlag(LIVE_FLAG_AIRBORNE);
		}

		mLinearVelocity = nextPos - mPosition;
	} else {
		TWalkerEnemy::bind();
	}
}

void TAmiNoko::attackToMario()
{
	const TNerveBase<TLiveActor>* nerve = mSpine->getCurrentNerve();
	if (nerve == &TNerveSmallEnemyChange::theNerve()) {
		return;
	}
	if (unk194 == 0) {
		return;
	}

	s32 doAttack = 1;

	switch (unk198) {
	case 0:
		if (!SMS_GetMarioRfPlane()) {
			break;
		}
		if (gpMarioPos->y < mPosition.y) {
			doAttack = 0;
		}
		break;
	case 1:
		if (!SMS_GetMarioGrPlane()) {
			break;
		}
		if (20.0f + gpMarioPos->y > mPosition.y) {
			doAttack = 0;
		}
		break;
	case 2: {
		const TBGCheckData* marioPlane = SMS_GetMarioWlPlane();
		if (!marioPlane) {
			break;
		}
		f32 dot = marioPlane->getNormal().dot(unk194->getNormal());
		if (dot < 0.0f) {
			doAttack = 0;
		}
		break;
	} }

	if (doAttack == 0) {
		return;
	}

	if (!SMS_SendMessageToMario(this, 9)) {
		return;
	}

	if (mSpine->getCurrentNerve() != &TNerveAmiNokoFreeze::theNerve()) {
		mSpine->pushNerve(&TNerveAmiNokoFreeze::theNerve());
	}
}

bool TAmiNoko::isHitValid(u32 msg)
{
	if (msg == 12 || msg == 1) {
		f32 dx = mPosition.x - gpMarioPos->x;
		f32 dz = mPosition.z - gpMarioPos->z;

		matan(unk19C.z, unk19C.x);

		JGeometry::TVec3<f32> horizontal(dx, 0.0f, dz);
		f32 dot = horizontal.dot(unk19C);
		if (dot > 0.0f || msg == 1) {
			mSpine->pushNerve(&TNerveAmiNokoDie::theNerve());
		}
	}

	return msg == 11 ? true : false;
}

void TAmiNoko::calcRootMatrix()
{
	// Sound check
	MSound* sound = gpMSound;
	if (sound->gateCheck(0x20F1)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x20F1, (const Vec*)&mPosition, 0, nullptr, 0, 4);
	}

	// Emit particles bound to joint matrices
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x180, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x210), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x180, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x1E0), 1,
	    (u8*)this + 0x214);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x180, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x1B0), 1,
	    (u8*)this + 0x428);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x182, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x210), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x181, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x210), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x183, (MtxPtr)((u8*)mMActor->unk4->mNodeMatrices + 0x210), 1, this);

	// Freeze nerve particle effects
	if (mSpine->getCurrentNerve() == &TNerveAmiNokoFreeze::theNerve()
	    && mSpine->getTime() < 46) {
		JPABaseEmitter* emitter =
		    gpMarioParticleManager->emitAndBindToMtxPtr(
		        0x17D, (MtxPtr)mMActor->unk4->mNodeMatrices, 1, this);
		if (emitter) {
			f32 zero = 0.0f;
			emitter->unk154.x = zero;
			emitter->unk154.y = zero;
			emitter->unk154.z = zero;
			emitter->unk174.x = zero;
			emitter->unk174.y = zero;
			emitter->unk174.z = zero;
		}

		// Extract joint translation for position-bound particle
		Mtx* nodeMtx = (Mtx*)((u8*)mMActor->getModel()->mNodeMatrices + 0x120);
		unk1FC.x = (*nodeMtx)[0][3];
		unk1FC.y = (*nodeMtx)[1][3];
		unk1FC.z = (*nodeMtx)[2][3];
		JPABaseEmitter* emitter2 =
		    gpMarioParticleManager->emitAndBindToPosPtr(0x17E, &unk1FC, 1, this);
		if (emitter2) {
			f32 zero2 = 0.0f;
			emitter2->unk154.x = zero2;
			emitter2->unk154.y = zero2;
			emitter2->unk154.z = zero2;
			emitter2->unk174.x = zero2;
			emitter2->unk174.y = zero2;
			emitter2->unk174.z = zero2;
		}
	}

	// Early return if playing BCK anim 0
	if (isBckAnm(0)) {
		unk1CC[0][3] = mPosition.x;
		unk1CC[1][3] = mPosition.y;
		unk1CC[2][3] = mPosition.z;
		PSMTXCopy(unk1CC, mMActor->unk4->unk20);
		return;
	}

	// Main path
	J3DModel* model = getModel();
	Mtx* baseMtx = (Mtx*)((u8*)model + 0x20);

	// Get goal direction
	JGeometry::TVec3<f32> goal = unkF4.getPoint();
	Vec dir;
	dir.x = goal.x - mPosition.x;
	dir.y = goal.y - mPosition.y;
	dir.z = goal.z - mPosition.z;

	if (dir.x == 0.0f && dir.y == 0.0f && dir.z == 0.0f) {
		dir.x = 1.0f;
	}
	PSVECNormalize(&dir, &dir);

	// Wall check
	TBGWallCheckRecord wallRecord;
	wallRecord.mCenter.x = mPosition.x;
	wallRecord.mCenter.y = mPosition.y;
	wallRecord.mCenter.z = mPosition.z;
	wallRecord.mRadius = 10.0f;
	wallRecord.mMaxResults = 4;
	wallRecord.mFlags = 0;
	int wallCount = gpMap->isTouchedWallsAndMoveXZ(&wallRecord);

	f32 bestDist = -1.0f;
	f32 zero = 0.0f;
	int bestWallIdx = -1;
	int wallIdx = 0;

	while (wallCount > 0) {
		const TBGCheckData* wall = wallRecord.mResultWalls[wallIdx];
		f32 dot = wall->mNormal.y * mPosition.y
		          + wall->mNormal.x * mPosition.x
		          + wall->mNormal.z * mPosition.z
		          + wall->mPlaneDistance;
		f32 absDot = __fabsf(dot);
		if (bestWallIdx < 0 || bestDist > absDot
		    || !(bestDist >= zero)) {
			unk198      = 2;
			bestDist    = absDot;
			bestWallIdx = wallIdx;
		}
		wallIdx++;
		wallCount--;
	}

	// Check ground
	const TBGCheckData* bestPlane = wallRecord.mResultWalls[bestWallIdx];
	const TBGCheckData* groundResult;
	f32 groundY = mPosition.y + mBodyScale * mHeadHeight;
	gpMap->checkGround(mPosition.x, groundY, mPosition.z, &groundResult);
	mGroundPlane = groundResult;

	if (mGroundPlane != nullptr) {
		const TBGCheckData* gp = mGroundPlane;
		f32 dot = gp->mNormal.y * mPosition.y + gp->mNormal.x * mPosition.x
		          + gp->mNormal.z * mPosition.z + gp->mPlaneDistance;
		if (dot >= zero) {
			if (bestDist > dot || !(bestDist >= zero)) {
				unk198    = 0;
				bestDist  = dot;
				bestPlane = gp;
			}
		}
	}

	// Check roof
	const TBGCheckData* roofResult;
	gpMap->checkRoof(mPosition, &roofResult);
	if (roofResult != nullptr) {
		f32 dot = roofResult->mNormal.y * mPosition.y
		          + roofResult->mNormal.x * mPosition.x
		          + roofResult->mNormal.z * mPosition.z
		          + roofResult->mPlaneDistance;
		if (dot >= zero) {
			if (bestDist > dot || !(bestDist >= zero)) {
				unk198    = 1;
				bestPlane = roofResult;
			}
		}
	}

	if (bestPlane != nullptr) {
		unk194 = bestPlane;
	}

	// Get rotation speed
	TAmiNokoSaveLoadParams* params = (TAmiNokoSaveLoadParams*)getSaveParam();
	f32 rotSpeed = params->mSLMtxRotSpeed.value;

	// Target normal
	Vec normal;
	if (unk194 != nullptr) {
		normal.x = unk194->mNormal.x;
		normal.y = unk194->mNormal.y;
		normal.z = unk194->mNormal.z;
	} else {
		normal.x = 0.0f;
		normal.y = 1.0f;
		normal.z = 0.0f;
	}

	// Lerp unk19C.x toward normal.x
	if (unk19C.x < normal.x) {
		f32 v = unk19C.x + rotSpeed;
		if (v > normal.x)
			;
		else
			normal.x = v;
	} else {
		f32 v = unk19C.x - rotSpeed;
		if (v <= normal.x)
			;
		else
			normal.x = v;
	}
	unk19C.x = normal.x;

	// Lerp unk19C.y toward normal.y
	if (unk19C.y < normal.y) {
		f32 v = unk19C.y + rotSpeed;
		if (v > normal.y)
			;
		else
			normal.y = v;
	} else {
		f32 v = unk19C.y - rotSpeed;
		if (v <= normal.y)
			;
		else
			normal.y = v;
	}
	unk19C.y = normal.y;

	// Lerp unk19C.z toward normal.z
	if (unk19C.z < normal.z) {
		f32 v = unk19C.z + rotSpeed;
		if (v > normal.z)
			;
		else
			normal.z = v;
	} else {
		f32 v = unk19C.z - rotSpeed;
		if (v <= normal.z)
			;
		else
			normal.z = v;
	}
	unk19C.z = normal.z;

	// Check forward vs direction dot product
	f32 fwdDot = unk1A8.x * dir.x + unk1A8.y * dir.y + unk1A8.z * dir.z;
	if (fwdDot < -0.1f) {
		Mtx rotMtx;
		PSMTXRotAxisRad(rotMtx, (Vec*)&normal, 1.5707964f);
		PSMTXMultVec(rotMtx, &dir, &dir);
	}

	// Lerp unk1A8 toward dir
	if (unk1A8.x < dir.x) {
		f32 v = unk1A8.x + rotSpeed;
		if (v > dir.x)
			;
		else
			dir.x = v;
	} else {
		f32 v = unk1A8.x - rotSpeed;
		if (v <= dir.x)
			;
		else
			dir.x = v;
	}
	unk1A8.x = dir.x;

	if (unk1A8.y < dir.y) {
		f32 v = unk1A8.y + rotSpeed;
		if (v > dir.y)
			;
		else
			dir.y = v;
	} else {
		f32 v = unk1A8.y - rotSpeed;
		if (v <= dir.y)
			;
		else
			dir.y = v;
	}
	unk1A8.y = dir.y;

	if (unk1A8.z < dir.z) {
		f32 v = unk1A8.z + rotSpeed;
		if (v > dir.z)
			;
		else
			dir.z = v;
	} else {
		f32 v = unk1A8.z - rotSpeed;
		if (v <= dir.z)
			;
		else
			dir.z = v;
	}
	unk1A8.z = dir.z;

	PSVECNormalize((Vec*)&unk1A8, (Vec*)&unk1A8);

	// Cross product: up x forward = right
	Vec up;
	up.x = unk19C.x;
	up.y = unk19C.y;
	up.z = unk19C.z;
	Vec fwd;
	fwd.x = unk1A8.x;
	fwd.y = unk1A8.y;
	fwd.z = unk1A8.z;

	Vec right;
	right.x = up.y * fwd.z - up.z * fwd.y;
	right.y = up.z * fwd.x - up.x * fwd.z;
	right.z = up.x * fwd.y - up.y * fwd.x;

	f32 rightLen2 =
	    right.x * right.x + right.y * right.y + right.z * right.z;
	if (rightLen2 <= 0.0000038146973f) {
		// Use previous vectors
		up.x = unk1B4.x;
		up.y = unk1B4.y;
		up.z = unk1B4.z;
		fwd.x = unk1C0.x;
		fwd.y = unk1C0.y;
		fwd.z = unk1C0.z;

		right.x = up.y * fwd.z - up.z * fwd.y;
		right.y = up.z * fwd.x - up.x * fwd.z;
		right.z = up.x * fwd.y - up.y * fwd.x;

		f32 rightLen2b =
		    right.x * right.x + right.y * right.y + right.z * right.z;
		if (rightLen2b <= 0.0000038146973f) {
			right.x = 1.0f;
			right.y = 0.0f;
			right.z = 0.0f;
		}
	} else {
		unk1B4.x = unk19C.x;
		unk1B4.y = unk19C.y;
		unk1B4.z = unk19C.z;
		unk1C0.x = unk1A8.x;
		unk1C0.y = unk1A8.y;
		unk1C0.z = unk1A8.z;
	}

	PSVECNormalize(&right, &right);

	// Cross right x up = forward2
	Vec forward2;
	forward2.x = right.y * up.z - right.z * up.y;
	forward2.y = right.z * up.x - right.x * up.z;
	forward2.z = right.x * up.y - right.y * up.x;

	f32 fwd2Len2 =
	    forward2.x * forward2.x + forward2.y * forward2.y + forward2.z * forward2.z;
	if (fwd2Len2 <= 0.0000038146973f) {
		forward2.x = 0.0f;
		forward2.y = 0.0f;
		forward2.z = 1.0f;
	} else {
		PSVECNormalize(&forward2, &forward2);
	}

	// Cross forward2 x right = up2
	Vec up2;
	up2.x = forward2.y * right.z - forward2.z * right.y;
	up2.y = forward2.z * right.x - forward2.x * right.z;
	up2.z = forward2.x * right.y - forward2.y * right.x;

	f32 up2Len2 = up2.x * up2.x + up2.y * up2.y + up2.z * up2.z;
	if (up2Len2 <= 0.0000038146973f) {
		up2.x = 0.0f;
		up2.y = 1.0f;
		up2.z = 0.0f;
	} else {
		PSVECNormalize(&up2, &up2);
	}

	// Store to base matrix
	(*baseMtx)[0][0] = right.x;
	(*baseMtx)[1][0] = right.y;
	(*baseMtx)[2][0] = right.z;
	(*baseMtx)[0][1] = up2.x;
	(*baseMtx)[1][1] = up2.y;
	(*baseMtx)[2][1] = up2.z;
	(*baseMtx)[0][2] = forward2.x;
	(*baseMtx)[1][2] = forward2.y;
	(*baseMtx)[2][2] = forward2.z;

	Vec rphVec;
	rphVec.x = 0.0f;
	rphVec.y = 1.5707963f;
	rphVec.z = 0.0f;
	MsMtxSetRotRPH(*baseMtx, up2.x, up2.y, up2.z);
	PSMTXMultVec(*baseMtx, &rphVec, &rphVec);

	// Translation: pos - offset * unk19C
	f32 offsetScale = 30.0f;
	(*baseMtx)[0][3] = mPosition.x - offsetScale * unk19C.x;
	(*baseMtx)[1][3] = mPosition.y - offsetScale * unk19C.y;
	(*baseMtx)[2][3] = mPosition.z - offsetScale * unk19C.z;

	PSMTXCopy(*baseMtx, unk1CC);

	// Copy scaling to model
	J3DModel* mdl = getModel();
	mdl->unk14.x = mScaling.x;
	mdl->unk14.y = mScaling.y;
	mdl->unk14.z = mScaling.z;
}

void TAmiHit::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		JGeometry::TVec3<f32> offset = mOwner->unk19C;

		offset.normalize();

		f32 scale = 30.0f;
		offset.x *= scale;
		offset.y *= scale;
		offset.z *= scale;

		mPosition = mOwner->mPosition;

		mPosition.x += offset.x;
		mPosition.y += offset.y;
		mPosition.z += offset.z;

		mPosition.y = mPosition.y - 0.5f * mAttackHeight;

		if (!(mOwner->mLiveFlag & LIVE_FLAG_DEAD)) {
			for (int i = 0; i < mColCount; ++i) {
				THitActor* col = mCollisions[i];
				if (col->mActorType - ACTOR_TYPE_PLAYER == 1) {
					mOwner->attackToMario();
				}
			}
		}
	}

	THitActor::perform(flags, graphics);
}

BOOL TAmiHit::receiveMessage(THitActor* sender, u32 message)
{
	return mOwner->receiveMessage(sender, message);
}

TAmiHit::~TAmiHit() { }

TSpineEnemy* TAmiNokoManager::createEnemyInstance()
{
	return nullptr;
}

void TAmiNokoManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "aminoko_model1.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TAmiNokoManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TAmiNokoSaveLoadParams("/enemy/amiNoko.prm");
	TSmallEnemyManager::load(stream);
}

TAmiNokoManager::TAmiNokoManager(const char* name)
    : TSmallEnemyManager(name)
{
}
