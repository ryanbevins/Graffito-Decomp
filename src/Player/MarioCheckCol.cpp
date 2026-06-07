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
						if ((actorType - 0x08000000) == 1) {
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
					if (mHolder == nullptr
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
	u16 numActors = *(u16*)((u8*)this + 0x48);
	for (u16 i = 0; i < numActors; i++) {
		THitActor** actorList = *(THitActor***)((u8*)this + 0x44);
		THitActor* actor = actorList[i];

		u32 actorFlags = *(u32*)((u8*)actor + 0x4C);

		// Check actor flag bit 26
		if (!(actorFlags & 0x04000000))
			continue;

		// Check mario flags for special collision handling
		if (!checkFlag(0x80000)) {
			if (!checkActionFlag(0x2000)) {
				if (checkActionFlag(0x1000)) {
					if ((mAction - 0x80000000) != 0x8A9) {
						if (mVel.y < 0.0f) {
							if (actor->mPosition.y < mPosition.y) {
								if (trampleExec(actor)) {
									keepDistance(*actor, 0.0f);
								}
							}
						}
					}
				}
			}
		}

		// Hit normal processing
		keepDistance(actor->mPosition, 0.0f, 0.0f);

		// Check hit result flags
		u32 hitFlags = *(u32*)((u8*)actor + 0xF0);
		if (!(hitFlags & 0x100000))
			continue;

		// Process hit reaction
		hitNormal(actor);
		// Store actor and change status
		*(THitActor**)((u8*)this + 0x384) = actor;
		changePlayerStatus(0x383, 0, false);
	}
}
