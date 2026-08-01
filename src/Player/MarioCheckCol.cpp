#include <Player/MarioMain.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <JSystem/JMath.hpp>
#include <Player/Watergun.hpp>
#include <Player/Yoshi.hpp>
#include <Player/ModelWaterManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Strategic/LiveActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Enemy/SmallEnemy.hpp>
#include <NPC/NpcBase.hpp>
#include <dolphin/mtx.h>
#include <fake_tgmath.h>

enum {
	MARIO_STATUS_FLAG_JUMPING   = 0x800,
	MARIO_STATUS_FLAG_UNK1000   = 0x1000,
	MARIO_STATUS_FLAG_UNK100000 = 0x100000,
	MARIO_STATUS_FLAG_UNK200000 = 0x200000,
	MARIO_STATUS_TYPE_AND_ID_MASK = 0x1FF,
	MARIO_STATUS_HIP_DROP         = ACTION_HIP_ATTACK,
	MARIO_STATUS_CATCH            = ACTION_CATCHING,
	MARIO_STATUS_OIL_SLIP         = ACTION_OIL_SLIP,
	MARIO_STATUS_JUMP_CATCH       = ACTION_DIVE_RECOVERY,
	MARIO_STATUS_THROWN_DOWN      = ACTION_CATCH_STOP,
	MARIO_STATUS_BACK_JUMP        = ACTION_SIDE_SOMERSAULT,
	MARIO_STATUS_BAR_HANG         = ACTION_BAR_CLIMB_ENTER,
	MARIO_STATUS_PULLING          = 0x560,
	MARIO_STATUS_PULL_JUMP        = ACTION_SLIDE_JUMP,
	MARIO_STATUS_FENCE_PUNCH      = 0x3000036A,
	MARIO_STATUS_KICK_ROOF        = ACTION_FENCE_KICK,
	MARIO_FLAG_UNK200             = 0x200,
	HIT_MESSAGE_SUPER_HIP_DROP    = HIT_MESSAGE_UNK3,
	HIT_MESSAGE_PUSH_UP           = HIT_MESSAGE_UNK2,
	ANIM_HOLD                     = 0xEA,
};

void TMario::hitNormal(THitActor* actor)
{
	u32 action = mAction;

	if (checkActionFlag(0x800)) {
		// Check speed < 0
		if (mVel.y < 0.0f) {
			// Check actor below mario
			if (actor->mPosition.y < mPosition.y) {
				// Check action range
				if (action == ACTION_HIP_ATTACK) {
					if (actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP)) {
						u32 actorType = *(u32*)((u8*)actor + 0x4C);
						bool triJump;
						if ((actorType - 0x08000000) == 1)
							triJump = true;
						else
							triJump = false;

						if (triJump) {
							changePlayerTriJump();
							unk78 = unk78 & ~(1 << 8);
						}
					}
					return;
				}

				if (trampleExec(actor) == TRUE)
					return;
			}
		}
	}

	// Check flag bit 10
	if (checkFlag(0x200)) {
		if (actor->mPosition.y > mPosition.y) {
			actor->receiveMessage(this, HIT_MESSAGE_UNK3);
			return;
		}
	}

	// Check action ranges for kick/trample
	u32 act = mAction;
	if (act == ACTION_CATCHING || act == ACTION_OIL_SLIP
	    || act == ACTION_DIVE_RECOVERY) {
		actor->receiveMessage(this, HIT_MESSAGE_PUNCH);
		actor->receiveMessage(this, HIT_MESSAGE_TRAMPLE);
	}

	// Check watergun nozzle state
	TWaterGun* wg = mWaterGun;
	int nozzleState = *(u8*)((u8*)wg + 0x1C84);
	if (nozzleState != 0)
		return;

	u8 emitState = *(u8*)((u8*)wg + 0x1C86);
	if (emitState == 0)
		return;

	TWaterHitActor* hitActor = &TModelWaterManager::mStaticHitActor;
	hitActor->mPosition      = mPosition;
	hitActor->mPosition.y += 80.0f;
	*(s32*)&hitActor->unk68 = 0;
	actor->receiveMessage(hitActor, HIT_MESSAGE_SPRAYED_BY_WATER);
}

// TODO: wrong size! maybe we return the receiveMessage result?
inline void TMario::hitHipDrop(THitActor* actor)
{
	if (mStatus == MARIO_STATUS_HIP_DROP && mActionState == 2
	    && actor->mPosition.y < mPosition.y) {
		actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
	}
}

inline void TMario::hitPushup(THitActor* actor)
{
	if (checkStatusType(MARIO_STATUS_FLAG_JUMPING) && mVel.y > 0.0f)
		actor->receiveMessage(this, HIT_MESSAGE_PUSH_UP);
	hitNormal(actor);
}

inline void TMario::hitPull(THitActor*) { }

inline void TMario::hitMario(THitActor* actor)
{
	if (mHeldObject != actor && mHolder != actor)
		keepDistance(*actor, 0.0f);
	wantToTakeActor(actor);
	hitNormal(actor);
}

inline void TMario::hitNpc(THitActor* actor)
{
	if (!checkFlag(MARIO_FLAG_HELMET_FLW_CAMERA)
	    && !checkStatusType(MARIO_FLAG_HELMET)
	    && checkStatusType(MARIO_STATUS_FLAG_JUMPING)
	    && mStatus != MARIO_STATUS_HIP_DROP && mVel.y < 0.0f
	    && actor->mPosition.y < mPosition.y
	    && ((TBaseNPC*)actor)->isBeTrampledNpc()) {
		if (trampleExec(actor) == TRUE)
			return;
	}

	keepDistance(*actor, 0.0f);

	if (((TLiveActor*)actor)->checkLiveFlag(LIVE_FLAG_UNK100000))
		wantToTakeActor(actor);
}

inline void TMario::hitPool(THitActor*) { }

inline void TMario::wantToTakeActor(THitActor* actor)
{
	if (canTake(actor)) {
		unk384 = actor;
		changePlayerStatus(MARIO_STATUS_TAKE, 0, false);
	}
}

inline void TMario::hitWantToTake(THitActor* actor)
{
	keepDistance(*actor, 0.0f);
	wantToTakeActor(actor);
}

inline void TMario::hitBarrel(THitActor* actor)
{
	hitWantToTake(actor);
	if (checkStatusType(MARIO_STATUS_FLAG_JUMPING) && mVel.y < 0.0f
	    && actor->mPosition.y < mPosition.y
	    && mStatus == MARIO_STATUS_HIP_DROP) {
		actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
		if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
			TWaterGun* wg     = mWaterGun;
			wg->mCurrentWater = (s32)((const TWaterGun*)wg)
			                        ->getCurrentNozzle()
			                        ->mEmitParams.mAmountMax.get();
		}
	}
}

inline void TMario::hitJumpBase(THitActor* actor)
{
	keepDistance(*actor, 0.0f);
	if (*(s8*)((u8*)actor + 0x138) == 0)
		wantToTakeActor(actor);
}

inline void TMario::hitBrakable(THitActor* actor)
{
	if (checkStatusType(MARIO_STATUS_FLAG_JUMPING) && mVel.y < 0.0f
	    && actor->mPosition.y < mPosition.y
	    && mStatus == MARIO_STATUS_HIP_DROP) {
		actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
	}
}

void TMario::hangPole(THitActor* actor)
{
	// Check action bit 20
	if (checkActionFlag(0x100000))
		return;

	// Check conditions for grabbing pole
	u8 canGrab = 0;
	if (*(u32*)((u8*)this + 0x6C) == 0) {
		if (!onYoshi()) {
			canGrab = 1;
		}
	}

	u8 shouldGrab;
	if (!canGrab) {
		shouldGrab = 0;
	} else {
		u32 action = mAction;
		u32 low9 = action & 0x1FF;
		if (low9 >= 0x80 && low9 <= 0x9F) {
			shouldGrab = 1;
		} else {
			bool actionBit = (action & 0x200000) ? true : false;
			if (actionBit)
				shouldGrab = 1;
			else
				shouldGrab = 0;
		}
	}

	if ((u8)shouldGrab == 1) {
		// Distance check in XZ plane
		f32 actorZ = actor->mPosition.z;
		f32 marioZ = mPosition.z;
		f32 actorX = actor->mPosition.x;
		f32 dz = actorZ - marioZ;
		f32 marioX = mPosition.x;
		f32 dx = actorX - marioX;

		f32 distSqXZ = dx * dx + dz * dz;
		f32 distXZ = distSqXZ;
		if (distSqXZ > 0.0f) {
			distXZ = sqrtf(distSqXZ);
		}

		f32 safeDistXZ = distXZ;
		if (0.0f == distXZ)
			safeDistXZ = 1.0f;

		f32 normZ = dz / safeDistXZ;
		f32 normX = dx / safeDistXZ;

		// Angle-based check
		u32 prevAction = mPrevAction;
		u8 canCatch   = 1;

		f32 cosVal = JMASCos(mFaceAngle.y);
		f32 sinVal = JMASSin(mFaceAngle.y);
		f32 catchRadius = 50.0f + *(f32*)((u8*)actor + 0x58);
		f32 dot = cosVal * normZ + sinVal * normX;
		f32 poleRadius = mBarParams.mCatchRadius.value;
		f32 poleHeight = mBarParams.mCatchAngle.value;

		if (prevAction & 0x100000)
			canCatch = 0;

		if (dot < poleHeight)
			canCatch = 0;

		if (safeDistXZ > catchRadius + poleRadius)
			canCatch = 0;

		// Check Y position
		f32 yCheck = 100.0f;
		if (mPosition.y < yCheck + actor->mPosition.y)
			canCatch = 0;

		if ((u8)canCatch == 1) {
			// Grab pole
			dropObject();
			mHolder = (TTakeActor*)actor;
			mVel.y = 0.0f;
			mForwardVel = 0.0f;
			changePlayerStatus(0x10100341, 0, false);

			actor->receiveMessage(this, HIT_MESSAGE_UNK5);

			mHolderHeightDiff = mPosition.y - actor->mPosition.y;
			return;
		}
	}

	f32 radius = ((TTakeActor*)actor)->getRadiusAtY(mPosition.y);
	keepDistance(actor->mPosition, radius, 0.0f);
}

inline void TMario::hitPickUpEnemy(THitActor* actor)
{
	if (((TSmallEnemy*)actor)->unk164 != 0
	    && !checkStatusType(MARIO_STATUS_FLAG_JUMPING)) {
		hitWantToTake(actor);
		return;
	}
	hitNormal(actor);
	if (((TSmallEnemy*)actor)->doKeepDistance())
		keepDistance(*actor, 0.0f);
}

inline void TMario::hitSurfingBoard(THitActor*) { }

// As in we pull but don't "keep" the object, cuz it's a tentacle/tail?
inline void TMario::hitNoKeepPull(THitActor* actor)
{
	if (mStatus != MARIO_STATUS_PULLING && mStatus != MARIO_STATUS_PULL_JUMP
	    && canTake(actor) && actor->receiveMessage(this, HIT_MESSAGE_TAKE)) {
		changePlayerStatus(MARIO_STATUS_PULLING, 0, false);
		setAnimation(ANIM_HOLD, 1.0f);
		mHeldObject = (TTakeActor*)actor;
	} else {
		hitNormal(actor);
	}
}

bool TSmallEnemy::doKeepDistance() { return false; }

void TMario::checkCollision()
{
	if (checkStatusType(MARIO_STATUS_FLAG_UNK1000))
		return;

	TYoshi* yoshi = mYoshi;
	BOOL yoshiActive;
	if (yoshi->mState == TYoshi::UNMOUNTED || yoshi->mState == 2)
		yoshiActive = 1;
	else
		yoshiActive = 0;

	if (yoshiActive == 1) {
		const JGeometry::TVec3<f32>& yt = yoshi->getTranslation();
		if (yt.y <= mPosition.y && mPosition.y < 100.0f + yt.y) {
			f32 dz   = yt.z - mPosition.z;
			f32 dx   = yt.x - mPosition.x;
			f32 dist = std::sqrtf(dx * dx + dz * dz);

			if (checkStatusType(MARIO_STATUS_FLAG_JUMPING) && !isHolding()
			    && mVel.y < 0.0f && yt.y < mPosition.y && mStatus != 0x89C
			    && mStatus != MARIO_STATUS_THROWN_DOWN
			    && mStatus != MARIO_STATUS_BACK_JUMP && dist < 180.0f) {
				mPosition       = mYoshi->getTranslation();
				mFaceAngle.y    = *(s16*)((u8*)mYoshi + 0x70);
				mModelFaceAngle = mFaceAngle.y;

				if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
					unk3E8              = mWaterGun->mSecondNozzle;
					const TWaterGun* wg = mWaterGun;
					unk3EC              = (f32)(wg->mCurrentWater
                                   / wg->getCurrentNozzle()
                                         ->mEmitParams.mAmountMax.get());
				}
				mYoshi->ride();
				mState |= MARIO_FLAG_HAS_FLUDD;
				if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
					mWaterGun->changeNozzle(TWaterGun::Yoshi, true);
				}
				changePlayerStatus(MARIO_STATUS_WAIT, 0, false);
				return;
			}

			keepDistance(yt, 80.0f, 0.0f);
		}
	}

	for (s32 i = 0; i < (s32)mColCount; i++) {
		if (mCollisions[i]->checkActorType(ACTOR_TYPE_UNK4000000)) {
			hitNpc(mCollisions[i]);
			continue;
		}

		// TODO: switch still a bit wrong!
		switch (mCollisions[i]->getActorType()) {
		// Other mario (enemy mario?)
		case 0x80000001:
			hitMario(mCollisions[i]);
			keepDistance(*mCollisions[i], 0.0f);
			break;

		// Some item?
		case 0x20000008:
		case 0x2000000A:
		case 0x2000000C:
			hitNormal(mCollisions[i]);
			break;

		// Most crap: namekuri, hamukuri, etc
		case 0x8000001:
		case 0x8000003:
		case 0x8000013:
		case 0x8000016:
		case 0x8000017:
		case 0x8000018:
		case 0x8000019:
		case 0x800001A:
		case 0x800001B:
		case 0x800001C:
		case 0x800001D:
		case 0x800001E:
		case 0x800001F:
		case 0x8000020:
		case 0x8000021:
		case 0x8000024:
		case 0x10000001:
		case 0x10000002:
		case 0x10000003:
		case 0x10000004:
		case 0x1000000A:
		case 0x1000000C:
		case 0x1000000D:
		case 0x1000000F:
		case 0x10000010:
		case 0x10000011:
		case 0x10000012:
		case 0x10000013:
		case 0x10000017:
		case 0x10000019:
		case 0x1000001A:
		case 0x1000001B:
		case 0x1000001C:
		case 0x1000001D:
		case 0x1000001F:
		case 0x10000020:
		case 0x10000025:
		case 0x1000002E:
		case 0x10000031:
		case 0x10000037:
		case 0x4000019A:
			hitNormal(mCollisions[i]);
			break;

		// ???
		case 0x8000011:
			hitHipDrop(mCollisions[i]);
			break;

		// Kumokun
		case 0x1000002C:
			hitNormal(mCollisions[i]);
			if (((TSmallEnemy*)mCollisions[i])->doKeepDistance())
				keepDistance(*mCollisions[i], 0.0f);
			// fall through

		// ???
		case 0x10000021:
			hitHipDrop(mCollisions[i]);
			// fall through

		// Amiking
		case 0x10000034:
			if (mStatus == MARIO_STATUS_FENCE_PUNCH
			    && 5.0f <= getMotionFrameCtrl().getFrame()
			    && getMotionFrameCtrl().getFrame() < 9.0f) {
				mCollisions[i]->receiveMessage(this, HIT_MESSAGE_PUNCH);
			}
			if (mStatus == MARIO_STATUS_KICK_ROOF
			    && 9.0f <= getMotionFrameCtrl().getFrame()
			    && getMotionFrameCtrl().getFrame() < 13.0f) {
				mCollisions[i]->receiveMessage(this, HIT_MESSAGE_PUNCH);
			}
			break;

		// Tama noko and something else
		case 0x10000018:
		case 0x1000001E:
			hitPickUpEnemy(mCollisions[i]);
			break;

		// Mame gesso
		case 0x10000008:
			if (((TSmallEnemy*)mCollisions[i])->doKeepDistance())
				keepDistance(*mCollisions[i], 0.0f);
			else
				hitPickUpEnemy(mCollisions[i]);
			break;

		// R1: keepDistance (cases sharing L_80161364 leaf)
		case 0x10000033:
		case 0x400001A6:
			keepDistance(*mCollisions[i], 0.0f);
			break;

		// P: hitNormal + virt[0x19C] check + keepDist
		case 0x10000007:
		case 0x1000000E:
		case 0x10000015:
		case 0x1000002A:
		case 0x1000002D:
			hitNormal(mCollisions[i]);
			if (((TSmallEnemy*)mCollisions[i])->doKeepDistance())
				keepDistance(*mCollisions[i], 0.0f);
			break;

		// A3: hitNormal (placed between P and R2 for body emission order)
		case 0x10000016:
			hitNormal(mCollisions[i]);
			break;

		// ???
		case 0x800000B:
		case 0x800000C:
		case 0x800000F:
		case 0x8000010:
		case 0x8000014:
		case 0x8000015:
		case 0x10000027:
		case 0x10000035:
			keepDistance(*mCollisions[i], 0.0f);
			break;

		// Damaging parts of boss gesso and other stuff
		case 0x8000002:
		case 0x8000005:
		case 0x8000007:
		case 0x10000022:
			keepDistance(*mCollisions[i], 0.0f);
			break;

		// Enemies with pull-able parts --
		// boss gesso tentacles/nose, fire wanwans, etc
		case 0x8000006:
		case 0x8000008:
		case 0x800000D:
		case 0x8000083:
		case 0x10000028:
			hitNoKeepPull(mCollisions[i]);
			break;

		// Nozzle box
		case 0x20000068:
			hitNormal(mCollisions[i]);
			keepDistance(*mCollisions[i], 0.0f);
			break;

		// Football, balloon ball, coconut
		case 0x40000064:
			hitPushup(mCollisions[i]);
			break;

		// ???
		case 0x40000002:
			hitBrakable(mCollisions[i]);
			break;

		// Water & oil barrels
		case 0x4000005A:
		case 0x4000005C:
			hitBarrel(mCollisions[i]);
			break;

		// Misc default-ish stuff -- just don't clip inside
		case 0x20000009:
		case 0x40000010:
		case 0x4000001B:
		case 0x40000026:
		case 0x40000030:
		case 0x40000046:
		case 0x4000005D:
		case 0x4000007E:
		case 0x4000009E:
		case 0x4000009F:
		case 0x400000A0:
		case 0x400000DB:
		case 0x40000136:
		case 0x40000139:
		case 0x40000228:
		case 0x40000233:
		case 0x40000264:
		case 0x40000396:
			keepDistance(*mCollisions[i], 0.0f);
			break;

		// Poles, trees, etc -- "climbable" stuff
		case 0x4000002D:
		case 0x4000002E:
		case 0x4000002F:
		case 0x40000032:
		case 0x40000034:
		case 0x40000035:
		case 0x40000036:
		case 0x40000037:
		case 0x40000039:
		case 0x4000003A:
		case 0x4000003C:
		case 0x40000047:
		case 0x40000049:
		case 0x400000BB:
		case 0x40000244:
		case 0x40000246:
			hangPole(mCollisions[i]);
			break;

		// jump base
		case 0x40000017:
			hitJumpBase(mCollisions[i]);
			break;

		// fruits
		case 0x40000390:
		case 0x40000391:
		case 0x40000392:
		case 0x40000394:
		case 0x40000395:
			hitWantToTake(mCollisions[i]);
			break;

		// durian fruit
		case 0x40000393:
			hitPushup(mCollisions[i]);
			break;

		// various breakable blocks
		case 0x400002BC:
			hitBrakable(mCollisions[i]);
			break;

		// empty cases
		case 0x8000004:
		case 0x8000012:
		case 0x10000005:
		case 0x10000006:
		case 0x10000009:
		case 0x1000000B:
		case 0x10000014:
		case 0x10000023:
		case 0x10000024:
		case 0x10000026:
		case 0x10000032:
		case 0x10000036:
		case 0x40000033:
		case 0x40000038:
		case 0x4000003B:
		case 0x4000005B:
		case 0x4000022B:
			break;
		}
	}
}
