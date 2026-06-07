#include <Player/MarioMain.hpp>

#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Player/Watergun.hpp>
#include <Strategic/LiveActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Enemy/SmallEnemy.hpp>
#include <NPC/NpcBase.hpp>
#include <dolphin/mtx.h>
#include <fake_tgmath.h>

// MarioCheckCol: -inline deferred, functions in REVERSE address order.

// hitNormal: 0x80161A78, size 0x210
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
	u8 nozzleState = *(u8*)((u8*)wg + 0x1C84);
	if (nozzleState != 0)
		return;

	u8 emitState = *(u8*)((u8*)wg + 0x1C86);
	if (emitState == 0)
		return;

	TModelWaterManager::mStaticHitActor.mPosition = mPosition;
	TModelWaterManager::mStaticHitActor.mPosition.y += 80.0f;
	TModelWaterManager::mStaticHitActor.unk68 = 0;
	actor->receiveMessage(&TModelWaterManager::mStaticHitActor,
	                      HIT_MESSAGE_SPRAYED_BY_WATER);
}

// hangPole: 0x801617E4, size 0x294
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
		} else if (action & 0x200000) {
			shouldGrab = 1;
		} else {
			shouldGrab = 0;
		}
	}

	if ((u8)shouldGrab != 1) {
		f32 radius = ((TTakeActor*)actor)->getRadiusAtY(mPosition.y);
		keepDistance(actor->mPosition, radius, 0.0f);
		return;
	}

	// Distance check in XZ plane
	f32 actorZ = actor->mPosition.z;
	f32 marioZ = mPosition.z;
	f32 actorX = actor->mPosition.x;
	f32 dz = actorZ - marioZ;
	f32 marioX = mPosition.x;
	f32 dx = actorX - marioX;

	f32 distSqXZ = dx * dx + dz * dz;
	f32 distXZ = 0.0f;
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
	u8 canCatch = 1;

	f32 sinVal = JMASSin(mFaceAngle.y);
	f32 cosVal = JMASCos(mFaceAngle.y);
	f32 catchRadius = *(f32*)((u8*)actor + 0x58);
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

	if ((u8)canCatch != 1) {
		f32 radius = ((TTakeActor*)actor)->getRadiusAtY(mPosition.y);
		keepDistance(actor->mPosition, radius, 0.0f);
		return;
	}

	// Grab pole
	dropObject();
	mHolder = (TTakeActor*)actor;
	mVel.y = 0.0f;
	mForwardVel = 0.0f;
	changePlayerStatus(0x10100341, 0, false);

	actor->receiveMessage(this, HIT_MESSAGE_UNK5);

	mHolderHeightDiff = mPosition.y - actor->mPosition.y;
}

bool TSmallEnemy::doKeepDistance() { return false; }

// checkCollision: 0x80160480, size 0x135C
void TMario::checkCollision()
{
	// Check action bit 19
	if (checkActionFlag(0x1000))
		return;

	// Check yoshi state
	TYoshi* yoshi = mYoshi;
	u8 yoshiState = yoshi->mState;
	if (yoshiState == TYoshi::UNMOUNTED || yoshiState == 2) {
		f32 marioY = mPosition.y;
		f32 yoshiFloorY = yoshi->mTranslation.y;
		if (yoshiFloorY <= marioY) {
			f32 extra = 100.0f;
			if (marioY < extra + yoshiFloorY) {
				// XZ distance
				f32 yoshiZ = yoshi->mTranslation.z;
				f32 marioZ = mPosition.z;
				f32 yoshiX = yoshi->mTranslation.x;
				f32 marioX = mPosition.x;
				f32 dz = yoshiZ - marioZ;
				f32 dx = yoshiX - marioX;
				f32 distSq = dx * dx + dz * dz;
				f32 dist = 0.0f;
				if (distSq > 0.0f)
					dist = sqrtf(distSq);

				// Additional checks
				u32 action = mAction;
				if (checkActionFlag(0x800)) {
					if (mHeldObject == nullptr
					    && mVel.y < 0.0f
					    && yoshi->mTranslation.y < mPosition.y
					    && action != ACTION_JUMP_BASIC_089C
					    && (action - 0x00020000) != 0x8B8
					    && action != ACTION_SIDE_SOMERSAULT && dist < 180.0f)
					{
						// Copy yoshi pos
						mPosition = yoshi->mTranslation;

						TYoshi* yoshi2 = mYoshi;
						s16 yoshiAngle = *(s16*)((u8*)yoshi2 + 0x70);
						mFaceAngle.y = yoshiAngle;
						mModelFaceAngle = mFaceAngle.y;

						// HAS_FLUDD check
						if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
							TWaterGun* wg = mWaterGun;
							u8 ntype = wg->mSecondNozzle;
							*(u32*)((u8*)this + 0x3E8) = ntype;

							TWaterGun* wg2 = mWaterGun;
							TNozzleBase* nozzle = wg2->getCurrentNozzle();
							u32 waterA = wg2->mCurrentWater;
							u32 waterB = *(u32*)((u8*)nozzle + 0xCC);
							f32 ratio = (f32)((s32)waterA / (s32)waterB);
							*(f32*)((u8*)this + 0x3EC) = ratio;
						}

						TYoshi* yoshi3 = mYoshi;
						yoshi3->ride();
						mState |= 0x8000;

						if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
							mWaterGun->changeNozzle((TWaterGun::TNozzleType)3, true);
						}

						changePlayerStatus(0x0C400201, 0, false);
						return;
					}
				}
				keepDistance(yoshi->mTranslation, 80.0f, 0.0f);
			}
		}
	}

	// Set attack area
	setNormalAttackArea();

	// Iterate over collision actors
	u16 numActors = mColCount;
	for (u16 i = 0; i < numActors; i++) {
		THitActor* actor = mCollisions[i];
		u32 actorType    = actor->mActorType;

		if (actorType & ACTOR_TYPE_UNK4000000) {
			if (!checkFlag(0x1000) && !checkActionFlag(0x2000)
			    && checkActionFlag(0x800) && mAction != ACTION_HIP_ATTACK
			    && mVel.y < 0.0f && actor->mPosition.y < mPosition.y
			    && ((TBaseNPC*)actor)->isBeTrampledNpc()) {
				if (trampleExec(actor) == TRUE)
					continue;
			}

			keepDistance(*actor, 0.0f);

			if ((*(u32*)((u8*)actor + 0xF0) & 0x100000) && canTake(actor)) {
				unk384 = actor;
				changePlayerStatus(0x383, 0, false);
			}
			continue;
		}

		switch (actorType) {
		case 0x80000001:
			if (mHeldObject != actor && mHolder != actor)
				keepDistance(*actor, 0.0f);

			if (canTake(actor)) {
				unk384 = actor;
				changePlayerStatus(0x383, 0, false);
			}

			hitNormal(actor);
			keepDistance(*actor, 0.0f);
			break;

		case 0x08000001:
		case 0x08000003:
		case 0x08000013:
		case 0x08000024:
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
		case 0x10000016:
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
		case 0x20000008:
		case 0x2000000A:
		case 0x2000000C:
		case 0x4000019A:
			hitNormal(actor);
			break;

		case 0x08000002:
		case 0x08000005:
		case 0x08000007:
		case 0x0800000B:
		case 0x0800000C:
		case 0x0800000F:
		case 0x08000010:
		case 0x08000014:
		case 0x08000015:
		case 0x08000022:
		case 0x08000023:
		case 0x10000022:
		case 0x10000027:
		case 0x10000033:
		case 0x10000035:
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
		case 0x400001A6:
		case 0x40000228:
		case 0x40000233:
		case 0x40000264:
		case 0x40000396:
			keepDistance(*actor, 0.0f);
			break;

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
			hangPole(actor);
			break;

		case 0x08000006:
		case 0x08000008:
		case 0x0800000D:
		case 0x08000083:
		case 0x10000028:
			if (mAction != 0x560 && mAction != ACTION_SLIDE_JUMP
			    && canTake(actor)
			    && actor->receiveMessage(this, HIT_MESSAGE_TAKE)) {
				changePlayerStatus(0x560, 0, false);
				setAnimation(0xEA, 1.0f);
				mHeldObject = (TTakeActor*)actor;
			} else {
				hitNormal(actor);
			}
			break;

		case 0x10000007:
		case 0x1000000E:
		case 0x10000015:
		case 0x1000002A:
		case 0x1000002D:
			hitNormal(actor);
			if (((TSmallEnemy*)actor)->doKeepDistance())
				keepDistance(*actor, 0.0f);
			break;

		case 0x10000008:
			if (((TSmallEnemy*)actor)->doKeepDistance()) {
				keepDistance(*actor, 0.0f);
			} else if (*(u8*)((u8*)actor + 0x164) != 0
			           && !checkActionFlag(0x800)) {
				keepDistance(*actor, 0.0f);
				if (canTake(actor)) {
					unk384 = actor;
					changePlayerStatus(0x383, 0, false);
				}
			} else {
				hitNormal(actor);
				if (((TSmallEnemy*)actor)->doKeepDistance())
					keepDistance(*actor, 0.0f);
			}
			break;

		case 0x10000018:
		case 0x1000001E:
			if (*(u8*)((u8*)actor + 0x164) != 0 && !checkActionFlag(0x800)) {
				keepDistance(*actor, 0.0f);
				if (canTake(actor)) {
					unk384 = actor;
					changePlayerStatus(0x383, 0, false);
				}
			} else {
				hitNormal(actor);
				if (((TSmallEnemy*)actor)->doKeepDistance())
					keepDistance(*actor, 0.0f);
			}
			break;

		case 0x1000002C:
			hitNormal(actor);
			if (((TSmallEnemy*)actor)->doKeepDistance())
				keepDistance(*actor, 0.0f);
			// fall through
		case 0x10000021:
			if (mAction == ACTION_HIP_ATTACK && mActionState == 2
			    && actor->mPosition.y < mPosition.y) {
				actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
			}
			// fall through
		case 0x10000034:
			if (mAction == 0x3000036A) {
				f32 frame = getMotionFrameCtrl().getFrame();
				if (5.0f <= frame && frame < 9.0f)
					actor->receiveMessage(this, HIT_MESSAGE_PUNCH);
			}

			if (mAction == ACTION_FENCE_KICK) {
				f32 frame = getMotionFrameCtrl().getFrame();
				if (9.0f <= frame && frame < 13.0f)
					actor->receiveMessage(this, HIT_MESSAGE_PUNCH);
			}
			break;

		case 0x4000005A:
		case 0x4000005C:
			keepDistance(*actor, 0.0f);
			if (canTake(actor)) {
				unk384 = actor;
				changePlayerStatus(0x383, 0, false);
			}

			if (checkActionFlag(0x800) && mVel.y < 0.0f
			    && actor->mPosition.y < mPosition.y && mAction == ACTION_HIP_ATTACK) {
				actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
				if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
					TWaterGun* wg  = mWaterGun;
					TNozzleBase* n = wg->getCurrentNozzle();
					wg->mCurrentWater = *(u32*)((u8*)n + 0xCC);
				}
			}
			break;

		case 0x40000390:
		case 0x40000391:
		case 0x40000392:
		case 0x40000394:
		case 0x40000395:
			keepDistance(*actor, 0.0f);
			if (canTake(actor)) {
				unk384 = actor;
				changePlayerStatus(0x383, 0, false);
			}
			break;

		case 0x40000017:
			keepDistance(*actor, 0.0f);
			if (*(s8*)((u8*)actor + 0x138) == 0 && canTake(actor)) {
				unk384 = actor;
				changePlayerStatus(0x383, 0, false);
			}
			break;

		case 0x40000064:
		case 0x40000393:
			if (checkActionFlag(0x800) && mVel.y > 0.0f)
				actor->receiveMessage(this, HIT_MESSAGE_UNK2);
			hitNormal(actor);
			break;

		case 0x40000002:
		case 0x400002BC:
			if (checkActionFlag(0x800) && mVel.y < 0.0f
			    && actor->mPosition.y < mPosition.y && mAction == ACTION_HIP_ATTACK) {
				actor->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
			}
			break;

		case 0x20000068:
			hitNormal(actor);
			keepDistance(*actor, 0.0f);
			break;
		}
	}
}
