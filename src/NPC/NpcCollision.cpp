#include <Camera/cameralib.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcInitData.hpp>
#include <NPC/NpcSave.hpp>
#include <Player/MarioAccess.hpp>

template <> f32 CLBSquared<f32>(f32);

void TBaseNPC::bind()
{
	JGeometry::TVec3<f32> nextPos = mPosition;
	nextPos.add(mLinearVelocity);
	nextPos.add(mVelocity);

	mVelocity.y -= getGravityY();
	if (mVelocity.y < mVelocityMinY)
		mVelocity.y = mVelocityMinY;

	mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
	    nextPos.x, nextPos.y + mHeadHeight, nextPos.z, &mGroundPlane);
	mGroundHeight += 1.0f;

	if (nextPos.y <= mGroundHeight + 0.05f) {
		if (mGroundPlane != NULL && mGroundPlane->isLegal()) {
			offLiveFlag(LIVE_FLAG_AIRBORNE);
			mVelocity.set(0.0f, 0.0f, 0.0f);
			nextPos.y = mGroundHeight;
		}
	} else {
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	if (mLiveFlag & 0x10000000) {
		gpMap->isTouchedOneWallAndMoveXZ(&nextPos.x,
		                                 nextPos.y + mHeadHeight,
		                                 &nextPos.z, 150.0f);
	}

	mLinearVelocity = nextPos - mPosition;
}

void TBaseNPC::setVariableDamageRadius_()
{
	f32 newRadius
	    = mScaling.x
	      * SMSGetNpcInitData(mActorType - 0x04000001)->mDamageRadius;
	f32 result = newRadius;

	if (isBeTrampledNpc() && !SMS_IsMarioTouchGround4cm()
	    && gpMarioPos->y > mPosition.y) {
		f32 dx = gpMarioPos->x - mPosition.x;
		f32 dz = gpMarioPos->z - mPosition.z;
		f32 sqr = CLBSquared<f32>(newRadius * 3.0f);
		f32 acc = 0.0f;
		acc += dx * dx;
		acc += dz * dz;
		if (acc < sqr) {
			result = mNpcSaveIndividual->mSLDamageRadiusSmall.get();
		}
	}

	mDamageRadius = result;
	calcEntryRadius();
}

void TBaseNPC::execNpcObjCollision_()
{
	for (int i = 0; i < mColCount; i++) {
		bool reverseDir;
		if (isNerveWalk()) {
			reverseDir = false;
		} else {
			if (!mCollisions[i]->checkActorType(0x04000000))
				continue;
			if (!((TBaseNPC*)mCollisions[i])->isNerveWalk())
				continue;
			reverseDir = true;
		}

		JGeometry::TVec3<f32> diff(
		    mPosition.x - mCollisions[i]->mPosition.x, 0.0f,
		    mPosition.z - mCollisions[i]->mPosition.z);

		if (reverseDir) {
			diff.x = -diff.x;
			diff.y = -diff.y;
			diff.z = -diff.z;
		}

		if (diff.x * diff.x + diff.y * diff.y + diff.z * diff.z
		    <= 0.0000038146973f) {
			f32 dy = mPosition.y - mCollisions[i]->mPosition.y;
			f32 ady;
			if (dy >= 0.0f)
				ady = dy;
			else
				ady = -dy;
			if (ady < 0.001f) {
				diff.x = 1.0f;
				diff.y = 10.0f;
				diff.z = 0.0f;
			} else {
				diff.x = 0.0f;
				diff.y = dy;
				diff.z = 0.0f;
			}
		} else {
			f32 mag;
			if (mAttackRadius + mCollisions[i]->mDamageRadius
			        - MsVECMag2(&diff)
			    >= 0.0f) {
				mag = mAttackRadius + mCollisions[i]->mDamageRadius
				      - MsVECMag2(&diff);
			} else {
				mag = -(mAttackRadius
				        + mCollisions[i]->mDamageRadius
				        - MsVECMag2(&diff));
			}
			if (mag < 0.001f)
				mag = 0.001f;

			f32 lsq
			    = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
			if (lsq <= 0.0f) {
				diff.z = 0.0f;
				diff.y = 0.0f;
				diff.x = 0.0f;
			} else {
				f32 invLen = JGeometry::TUtil<f32>::inv_sqrt(lsq);
				invLen *= mag;
				diff.x *= invLen;
				diff.y *= invLen;
				diff.z *= invLen;
			}
		}

		if (reverseDir) {
			JGeometry::TVec3<f32>& pos = mCollisions[i]->mPosition;
			pos.x += diff.x;
			pos.y += diff.y;
			pos.z += diff.z;
		} else {
			mLinearVelocity.x += diff.x;
			mLinearVelocity.y += diff.y;
			mLinearVelocity.z += diff.z;
		}
	}
}

void TBaseNPC::initNpcObjCollision_(const TNpcInitInfo* info)
{
	u16 var1 = 2;
	int var2 = 0x04000000;

	switch (mActorType) {
	case 0x04000006:
	case 0x0400001A:
	case 0x0400001B:
	case 0x0400001D:
		var1 = 0;
		var2 = 0;
		break;
	}

	f32 scaleY       = mScaling.y;
	f32 attackHeight = info->mAttackHeight * scaleY;
	f32 damageHeight = info->mDamageHeight * scaleY;
	f32 scaleX       = mScaling.x;
	f32 attackRadius = info->mAttackRadius * scaleX;
	f32 damageRadius = info->mDamageRadius * scaleX;

	initHitActor(mActorType, var1, var2, attackRadius, attackHeight,
	             damageRadius, damageHeight);

	offHitFlag(HIT_FLAG_NO_COLLISION);
	if (var1 == 0)
		onHitFlag(HIT_FLAG_UNK2);
}
