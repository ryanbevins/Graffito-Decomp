#define MSOUND_EMIT_START_FORCE_JUMP_SOUND
#define JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <Player/MarioMain.hpp>
#undef JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#define PLAYER_YOSHI_DEFINE_ON_YOSHI
#pragma dont_inline on
#include <Player/Yoshi.hpp>
#pragma dont_inline off
#undef PLAYER_YOSHI_DEFINE_ON_YOSHI

#include <Map/Map.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <System/MarDirector.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <Player/Watergun.hpp>
#include <Strategic/LiveActor.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSound.hpp>
#include <Map/MapData.hpp>
#include <Map/MapCollisionData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Camera/Camera.hpp>
#include <System/MarioGamePad.hpp>
#include <System/StageUtil.hpp>
#include <dolphin/mtx.h>
#include <fake_tgmath.h>
#include <Player/ModelWaterManager.hpp>
#include <System/EmitterViewObj.hpp>
#include <MarioUtil/EffectUtil.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <MoveBG/Pool.hpp>
#include <Map/PollutionManager.hpp>

static inline bool isMarioInvincibleInline(const TMario* mario)
{
	if (mario->getUnk14c() > 0)
		return true;

	u8 hasFlag;
	if (mario->getState() & 0x8)
		hasFlag = 1;
	else
		hasFlag = 0;

	if (hasFlag)
		return true;

	if (mario->getAction() == 0x89C)
		return true;

	if (gpMarDirector->isDemoMode3() || gpMarDirector->isDemoMode4()
	    || gpMarDirector->isTalkModeNow()
	    || mario->checkActionFlag(0x1000))
		return true;

	return false;
}

static inline BOOL isMarioForceSlipInline(const TMario* mario, u16 code)
{
	u8 isIce;
	if (code == 0x01 || code == 0x4001 || code == 0x8001 || code == 0xC001)
		isIce = 1;
	else
		isIce = 0;

	if (isIce)
		return true;

	if (mario->unk350 == 2) {
		u8 hasBit;
		if (mario->mState & 0x40)
			hasBit = 1;
		else
			hasBit = 0;

		if (hasBit) {
			if (mario->mGroundPlane->getNormal().y
			    < mario->mDirtyParams.mSlopeAngle.get())
				return true;
		}
	}

	if (mario->mGroundPlane->getNormal().y
	    < mario->mDeParams.mForceSlipAngle.get())
		return true;

	return false;
}

void TMario::addVelocity(f32 vel)
{
	mForwardVel += vel;
	if (mForwardVel > 99.0f)
		mForwardVel = 99.0f;
}

void TMario::windMove(const JGeometry::TVec3<f32>& wind)
{
	mPosition.x += wind.x;
	mPosition.y += wind.y;
	mPosition.z += wind.z;
}

void TMario::flowMove(const JGeometry::TVec3<f32>& flow)
{
	u8 inWater;
	if (mAction & 0x2000)
		inWater = 1;
	else
		inWater = 0;

	if (inWater != 1)
		return;

	mPosition.x += flow.x;
	mPosition.y += flow.y;
	mPosition.z += flow.z;
}

BOOL TMario::moveRequest(const JGeometry::TVec3<f32>& pos)
{
	JGeometry::TVec3<f32> delta;
	JGeometry::TVec3<f32> localPos(pos);
	localPos.sub(mPosition);
	delta = localPos;
	mPosition = *(JGeometry::TVec3<f32>*)&pos;

	f32 dx = delta.x;
	f32 dy = delta.y;
	f32 dz = delta.z;

	// Adjust all position-relative fields by delta
	unk160.x += dx;
	unk160.y += dy;
	unk160.z += dz;
	mLastSafePos.x += dx;
	mLastSafePos.y += dy;
	mLastSafePos.z += dz;
	mWireStartPos.x += dx;
	mWireStartPos.y += dy;
	mWireStartPos.z += dz;
	mWireEndPos.x += dx;
	mWireEndPos.y += dy;
	mWireEndPos.z += dz;
	mLastGroundPos.x += dx;
	mLastGroundPos.y += dy;
	mLastGroundPos.z += dz;

	mLastGroundY += dy;

	mJointMtx0[0][3] += dx;
	mJointMtx0[1][3] += dy;
	mJointMtx0[2][3] += dz;

	mJointMtx1[0][3] += dx;
	mJointMtx1[1][3] += dy;
	mJointMtx1[2][3] += dz;

	mJointMtx2[0][3] += dx;
	mJointMtx2[1][3] += dy;
	mJointMtx2[2][3] += dz;

	mJointMtx3[0][3] += dx;
	mJointMtx3[1][3] += dy;
	mJointMtx3[2][3] += dz;

	checkRideReCalc();

	return true;
}

void TMario::warpRequest(const JGeometry::TVec3<f32>& pos, f32 angle)
{
	JGeometry::TVec3<f32> delta = pos - mPosition;

	moveRequest(pos);

	mFaceAngle.y = DEG2SHORTANGLE(angle);
	mModelFaceAngle = mFaceAngle.y;

	gpCamera->addMoveCameraAndMario(*(const Vec*)&delta);

	if (gpMarDirector->mMap != 7)
		mGamePad->onNeutralMarioKey();

	changePlayerStatus(0x0C400201, 0, true);
}

BOOL TMario::changePlayerTriJump()
{
	int jumpAmount;

	if ((u8)isForceSlip()) {
		jumpAmount = mSlipParamsAll.mMissJump.get();
	} else {
		const TBGCheckData* ground = mGroundPlane;
		u16 bgType = ground->mBGType;

		u8 isSlippery;
		if (bgType == 0x0C || bgType == 0x800C || bgType == 0xA00C)
			isSlippery = 1;
		else
			isSlippery = 0;
		if (isSlippery) {
			jumpAmount = mSlipParamsAllSlider.mMissJump.get();
		} else {
			u8 isSand;
			if (bgType == 0x02 || bgType == 0x8002)
				isSand = 1;
			else
				isSand = 0;
			if (isSand && ground->mNormal.y < 0.866025f) {
				jumpAmount = mSlipParams45.mMissJump.get();
			} else {
				u8 isWet;
				if (bgType == 0x04 || bgType == 0x4004
				    || bgType == 0x8004 || bgType == 0xC004)
					isWet = 1;
				else
					isWet = 0;
				if (isWet) {
					if (ground->mNormal.y > 0.99f) {
						jumpAmount = mSlipParamsWaterGround.mMissJump.get();
					} else {
						jumpAmount = mSlipParamsWaterSlope.mMissJump.get();
					}
				} else {
					jumpAmount = mSlipParamsNormal.mMissJump.get();
				}
			}
		}
	}

	if (jumpAmount != 0) {
		mFaceAngle.x = 0;
		mModelFaceAngle = mFaceAngle.y;

		if (mForwardVel > 0.0f) {
			s16 oppAngle = (s16)(mSlopeAngle + 0x8000);
			u16 diff = (u16)(mFaceAngle.y - oppAngle);

			f32 sinDiff = JMASSin(diff);
			f32 cosDiff = JMASCos(diff);

			f32 fwd = mForwardVel;
			f32 sinComp = fwd * sinDiff;
			f32 cosComp = fwd * cosDiff;
			f32 scaledSin = 0.75f * sinComp;
			f32 sqSum = cosComp * cosComp + scaledSin * scaledSin;

			if (sqSum > 0.0f) {
				double guess = __frsqrte((double)sqSum);
				guess = .5 * guess * (3.0 - guess * guess * sqSum);
				f32 sqrtResult;
				sqrtResult = (f32)(sqSum * guess);
				sqSum = sqrtResult;
			}
			mForwardVel = sqSum;

			mSlideVelX = mForwardVel * JMASSin((u16)mFaceAngle.y);
			mSlideVelZ = mForwardVel * JMASCos((u16)mFaceAngle.y);

			mVel.x = mSlideVelX;
			mVel.z = mSlideVelZ;

			s16 newAngle = matan(scaledSin, cosComp);
			mFaceAngle.y = oppAngle + newAngle;
		}

		dropObject();
		changePlayerStatus(0x02000885, 0, false);
		return TRUE;
	}

	int stickDirection;
	BOOL rotResult = FALSE;
	if (checkStickRotate(&stickDirection)) {
		switch (stickDirection) {
		case 2:
			changePlayerStatus(0x896, 0, false);
			break;
		case 3:
			changePlayerStatus(0x895, 0, false);
			break;
		}
		rotResult = TRUE;
	}

	if (rotResult)
		return TRUE;

	changePlayerStatus(0x02000880, 0, false);
	return TRUE;
}

BOOL TMario::changePlayerJumping(u32 status, u32 arg)
{
	int jumpAmount;

	if ((u8)isForceSlip()) {
		jumpAmount = mSlipParamsAll.mMissJump.get();
	} else {
		const TBGCheckData* ground = mGroundPlane;
		u16 bgType = ground->mBGType;

		u8 isSlippery;
		if (bgType == 0x0C || bgType == 0x800C || bgType == 0xA00C)
			isSlippery = 1;
		else
			isSlippery = 0;
		if (isSlippery) {
			jumpAmount = mSlipParamsAllSlider.mMissJump.get();
		} else {
			u8 isSand;
			if (bgType == 0x02 || bgType == 0x8002)
				isSand = 1;
			else
				isSand = 0;
			if (isSand && ground->mNormal.y < 0.866025f) {
				jumpAmount = mSlipParams45.mMissJump.get();
			} else {
				u8 isWet;
				if (bgType == 0x04 || bgType == 0x4004
				    || bgType == 0x8004 || bgType == 0xC004)
					isWet = 1;
				else
					isWet = 0;
				if (isWet) {
					if (ground->mNormal.y > 0.99f) {
						jumpAmount = mSlipParamsWaterGround.mMissJump.get();
					} else {
						jumpAmount = mSlipParamsWaterSlope.mMissJump.get();
					}
				} else {
					jumpAmount = mSlipParamsNormal.mMissJump.get();
				}
			}
		}
	}

	if (jumpAmount != 0) {
		mFaceAngle.x = 0;
		mModelFaceAngle = mFaceAngle.y;

		if (mForwardVel > 0.0f) {
			s16 oppAngle = (s16)(mSlopeAngle + 0x8000);
			u16 diff = (u16)(mFaceAngle.y - oppAngle);

			f32 sinDiff = JMASSin(diff);
			f32 cosDiff = JMASCos(diff);

			f32 fwd = mForwardVel;
			f32 sinComp = fwd * sinDiff;
			f32 cosComp = fwd * cosDiff;
			f32 scaledSin = 0.75f * sinComp;
			f32 sqSum = cosComp * cosComp + scaledSin * scaledSin;

			if (sqSum > 0.0f) {
				double guess = __frsqrte((double)sqSum);
				guess = .5 * guess * (3.0 - guess * guess * sqSum);
				volatile f32 sqrtResult;
				sqrtResult = (f32)(sqSum * guess);
				sqSum = sqrtResult;
			}
			mForwardVel = sqSum;

			mSlideVelX = mForwardVel * JMASSin((u16)mFaceAngle.y);
			mSlideVelZ = mForwardVel * JMASCos((u16)mFaceAngle.y);

			mVel.x = mSlideVelX;
			mVel.z = mSlideVelZ;

			s16 newAngle = matan(scaledSin, cosComp);
			mFaceAngle.y = oppAngle + newAngle;
		}

		dropObject();
		changePlayerStatus(0x02000885, 0, false);
		return TRUE;
	}

	int stickDirection;
	BOOL rotResult = FALSE;
	if (checkStickRotate(&stickDirection)) {
		switch (stickDirection) {
		case 2:
			changePlayerStatus(0x896, 0, false);
			break;
		case 3:
			changePlayerStatus(0x895, 0, false);
			break;
		}
		rotResult = TRUE;
	}

	if (rotResult)
		return TRUE;

	changePlayerStatus(status, arg, false);
	return TRUE;
}

BOOL TMario::changePlayerDropping(u32 status, u32 arg)
{
	dropObject();
	return changePlayerStatus(status, arg, false);
}

BOOL TMario::changePlayerStatus(u32 status, u32 arg, bool force)
{
	if (!force) {
		if (status == getAction())
			return 0;
		if (checkActionThing())
			return 0;
	}

	if (getAction() == 0x20467)
		return 0;

	if (SMS_isDivingMap()) {
		if ((u32)(status - 0x10020000) != 880 && status != 0x0891
		    && status != 0x1302)
			return 0;
	}

	switch (status & 0x1C0) {
	case 0x40: {
		f32 speed = mIntendedMag <= 8.0f ? mIntendedMag : 8.0f;

		switch (status) {
		case 0x04000440: {
			if (0.0f <= mForwardVel && mForwardVel < speed)
				mForwardVel = speed;
			break;
		}
		case 0x50: {
			u8 facing = 0;
			s16 angleDiff = (s16)(mSlopeAngle - mFaceAngle.y);
			if (angleDiff > -16384 && angleDiff < 16384)
				facing = 1;

			if (facing)
				status = 0x00840452;
			else
				status = 0x00840453;

			startVoice(0x78CF);
			break;
		}
		}
		break;
	}
	case 0x80:
		status = setStatusToJumping(status, arg);
		break;
	}

	mPrevAction = mAction;
	mAction = status;
	mActionArg = arg;
	mActionState = 0;
	mActionTimer = 0;
	return 1;
}

void TMario::throwMario(const JGeometry::TVec3<f32>& throwVec, f32 speed)
{
	f32 dirZ;
	f32 hMag;
	JGeometry::TVec3<f32> dir = throwVec;

	if (dir.squared() <= JGeometry::TUtil<f32>::epsilon())
		dir.y = 1.0f;

	dir.normalize();

	dirZ = dir.z;
	mFaceAngle.y = matan(dirZ, dir.x) + 0x8000;
	mModelFaceAngle = mFaceAngle.y;

	hMag = std::sqrtf(dir.x * dir.x + dirZ * dirZ);

	mForwardVel = speed * -hMag;
	mVel.y = dir.y * speed;
}

void TMario::setPlayerVelocity(f32 speed)
{
	mForwardVel = speed;
	mSlideVelX = mForwardVel * JMASSin(mFaceAngle.y);
	mSlideVelZ = mForwardVel * JMASCos(mFaceAngle.y);
	mVel.x = mSlideVelX;
	mVel.z = mSlideVelZ;
}

void TMario::setNormalAttackArea()
{
	setAttackRadius(mDeParams.mHoldRadius.get());
	setAttackHeight(mDeParams.mAttackHeight.get());
}

BOOL TMario::canBendBody()
{
	u32 act = mAction & 0x1FF;
	if (act >= 0x14B && act <= 0x14F)
		return false;
	if (act >= 0x140 && act <= 0x143)
		return false;
	return true;
}

void TMario::checkThrowObject()
{
	if (mModel->getFrameCtrl(0).checkPass(4.0f)) {
		startVoice(0x788F);
		dropObject();
	}
}

BOOL TMario::onYoshi() const
{
	u8 result = 0;
	if (mYoshi != NULL) {
		if (mYoshi->onYoshi())
			result = 1;
	}
	return result;
}

BOOL TMario::checkGroundPlane(f32 x, f32 y, f32 z, f32* outHeight,
                              const TBGCheckData** outPlane)
{
	*outHeight = gpMap->checkGround(x, y, z, outPlane);

	if ((*outPlane)->isMarioThrough()) {
		*outHeight = gpMap->checkGround(x, *outHeight - 1.0f, z, outPlane);
	}

	u8 isWall;
	if ((*outPlane)->getFlags() & 0x10)
		isWall = 1;
	else
		isWall = 0;

	u8 isSafe;
	if (isWall == 1)
		isSafe = 0;
	else
		isSafe = 1;

	if (isSafe)
		return true;
	return false;
}

f32 TMario::checkRoofPlane(const Vec& pos, f32 height,
                           const TBGCheckData** result)
{
	return gpMap->checkRoof(pos.x, height + 80.0f, pos.z, result);
}

BOOL TMario::isFrontSlip(int param)
{
	int angle = mFaceAngle.y;
	if (param != 0) {
		if (mForwardVel < 0.0f)
			angle = angle + 0x8000;
	}
	s16 diff = (s16)(mSlopeAngle - angle);
	bool result = false;
	if (diff > -0x4000 && diff < 0x4000)
		result = true;
	return result;
}

void TMario::dirtyLimitCheck()
{
	if (getUnk134() < 0.0f)
		unk134 = 0.0f;
	f32 maxDirty = mDirtyParams.mDirtyMax.get();
	if (maxDirty < getUnk134())
		unk134 = maxDirty;
}

void TMario::stateMachine()
{
	BOOL result = true;
	while (result) {
		switch (mAction & 0x1C0) {
		case 0x000:
			result = waitMain();
			break;
		case 0x040:
			result = moveMain();
			break;
		case 0x080:
			result = jumpMain();
			break;
		case 0x0C0:
			result = swimMain();
			break;
		case 0x100:
			result = demoMain();
			break;
		case 0x140:
			result = specMain();
			break;
		case 0x180:
			result = actnMain();
			break;
		}
	}
}

void TMario::calcGroundMtx(const JGeometry::TVec3<f32>& inPos)
{
	JGeometry::TVec3<f32> pos(inPos);
	const TBGCheckData* check;
	pos.y = gpMap->checkGround(pos, &check);

	JGeometry::TVec3<f32> pt1;
	pt1.x = pos.x + JMASCos(mFaceAngle.y);
	pt1.y = 30.0f + pos.y;
	pt1.z = pos.z - JMASSin(mFaceAngle.y);
	pt1.y = gpMap->checkGround(pt1, &check);

	JGeometry::TVec3<f32> pt2;
	pt2.x = pos.x + JMASSin(mFaceAngle.y);
	pt2.y = 30.0f + pos.y;
	pt2.z = pos.z + JMASCos(mFaceAngle.y);
	pt2.y = gpMap->checkGround(pt2, &check);

	mGroundMtx[0][0] = pt1.x - pos.x;
	mGroundMtx[1][0] = pt1.y - pos.y;
	mGroundMtx[2][0] = pt1.z - pos.z;

	mGroundMtx[0][1] = 0.0f;
	mGroundMtx[1][1] = 1.0f;
	mGroundMtx[2][1] = 0.0f;

	mGroundMtx[0][2] = pt2.x - pos.x;
	mGroundMtx[1][2] = pt2.y - pos.y;
	mGroundMtx[2][2] = pt2.z - pos.z;

	mGroundMtx[0][3] = pos.x;
	mGroundMtx[1][3] = 18.0f + pos.y;
	mGroundMtx[2][3] = pos.z;
}

inline void TMario::setPlayerJumpSpeed(f32 speed_mult, f32 force)
{
	mVel.y = mForwardVel * speed_mult + force;
}

u32 TMario::setStatusToJumping(u32 status, u32 arg)
{
	mLastGroundY = mPosition.y;

	s16 health = unk360;
	s32 halfMaxHealth = mDeParams.mFootPrintTimerMax.get() / 2;
	if (health > halfMaxHealth) {
		f32 stampSize = mDirtyParams.mPolSizeJump.get();
		f32 z         = mPosition.z;
		f32 y         = mPosition.y;
		f32 x         = getMpositionX();
		gpPollution->stamp(1, x, y, z, stampSize);
	}

	switch (status) {
	case 0x089C:
	case 0x02000880: {
		// Standard/running jump
		setPlayerJumpSpeed(0.25f, 42.0f);
		mForwardVel *= 0.8f;

		const TBGCheckData* ground = mGroundPlane;
		u8 hasSlipFlag;
		if (ground->mFlags & 0x10)
			hasSlipFlag = 1;
		else
			hasSlipFlag = 0;
		if (hasSlipFlag) {
			break;
		}

		u16 groundType = ground->mBGType;
		u8 isBeachGround;
		if (groundType == 0x108 || groundType == 0x8
		    || groundType == 0x8008)
			isBeachGround = 1;
		else
			isBeachGround = 0;
		if (isBeachGround) {
			f32 jumpPower = ground->getActiveJumpPower();
			mVel.y = 0.01f * jumpPower;
			status = 0x884;
			break;
		}

		u8 isSlipperyGround;
		if (groundType == 0x9 || groundType == 0x8009)
			isSlipperyGround = 1;
		else
			isSlipperyGround = 0;
		if (isSlipperyGround) {
			f32 jumpAdj;
			if (ground != NULL) {
				f32 jumpPower = ground->getActiveJumpPower();
				jumpAdj = 0.01f * jumpPower;
			} else {
				jumpAdj = 0.0f;
			}
			f32 gravity = unkBC;
			mVel.y = getMvelY() + (-gravity + jumpAdj);

			TLiveActor* groundActor =
			    (TLiveActor*)mGroundPlane->mActor;
			if (groundActor != NULL) {
				((THitActor*)groundActor)->receiveMessage((THitActor*)this, 0);
			}

			startVoice(0x78B9);
			status = 0x884;
			break;
		}

		// Normal ground: check speed for voice
		u8 isFast;
		if (unk370 > mDeParams.mFeelDeep.get())
			isFast = 1;
		else
			isFast = 0;
		if (isFast) {
			startVoice(0x78A3);
		} else {
			startVoice(0x78AB);
		}
		break;
	}
	case 0x02000881: {
		// Hip-drop jump
		setPlayerJumpSpeed(mJumpParams.mSecJumpSpeedMult.get(),
		                   mJumpParams.mSecJumpForce.get());
		mForwardVel *= mJumpParams.mSecJumpXZMult.get();
		startVoice(0x78B1);
		break;
	}
	case 0x0882: {
		// Somersault jump
		setPlayerJumpSpeed(mJumpParams.mUltraJumpSpeedMult.get(),
		                   mJumpParams.mUltraJumpForce.get());
		mForwardVel *= mJumpParams.mUltraJumpXZMult.get();
		startVoice(0x78B6);
		break;
	}
	case 0x0883: {
		// Side somersault
		mForwardVel = mJumpParams.mBackJumpForce.get();
		setPlayerJumpSpeed(0.0f, mJumpParams.mBackJumpForceY.get());
		startVoice(0x78B6);
		break;
	}
	case 0x02000886: {
		// Special jump (ground pound variant)
		setPlayerJumpSpeed(0.0f, 62.0f);
		mForwardVel = 24.0f;
		startVoice(0x78B1);
		break;
	}
	case 0x0884: {
		// Ground pound bounce
		if (mGroundPlane != NULL) {
			f32 jumpPower = mGroundPlane->getActiveJumpPower();
			mVel.y = 0.01f * jumpPower;
		} else {
			mVel.y = 0.0f;
		}
		startVoice(0x78B1);
		break;
	}
	case 0x0895:
	case 0x0896: {
		// Backflip
		setPlayerJumpSpeed(0.25f, mJumpParams.mRotateJumpForceY.get());
		mForwardVel *= 0.8f;
		startVoice(0x78B6);
		break;
	}
	case 0x0887: {
		// Spin jump
		setPlayerJumpSpeed(0.0f, mJumpParams.mTurnJumpForce.get());
		mForwardVel = 8.0f;
		mFaceAngle.y
		    = mIntendedYaw;
		startVoice(0x78B6);
		break;
	}
	case 0x0888: {
		// Wall kick
		startVoice(0x78B1);
		mForwardVel = mJumpParams.mBroadJumpForce.get();
		mVel.y = mJumpParams.mBroadJumpForceY.get();
		break;
	}
	case 0x02000889: {
		// Long jump
		startVoice(0x78B1);
		mForwardVel = mJumpParams.mRotBroadJumpForce.get();
		mVel.y = mJumpParams.mRotBroadJumpForceY.get();
		break;
	}
	case 0x0208B4: {
		// Zero velocity
		mVel.y = 31.5f;
		mForwardVel = 8.0f;
		break;
	}
	case 0x0281089A: {
		// Hip-drop-to-slide: check ground type
		startVoice(0x78AB);
		const TBGCheckData* groundResult;
		gpMap->checkGround(getMpositionX(), mPosition.y, mPosition.z,
		                   &groundResult);
		u16 gType = groundResult->mBGType;
		u8 isBeach;
		if (gType == 0x100 || gType == 0x101
		    || (u16)(gType - 0x102) <= 3 || gType == 0x4104)
			isBeach = 1;
		else
			isBeach = 0;
		if (isBeach) {
			setPlayerJumpSpeed(mSurfingParamsWaterRed.mJumpXZRatio.get(),
			                   mSurfingParamsWaterRed.mJumpPow.get());
		} else {
			setPlayerJumpSpeed(mSurfingParamsGroundRed.mJumpXZRatio.get(),
			                   mSurfingParamsGroundRed.mJumpPow.get());
		}
		break;
	}
	case 0x000208B7: {
		// Wall slide jump
		if (mActionArg == 2)
			break;
		mVel.y = mJumpParams.mFireDownForce.get();
		if (mActionArg != 0)
			break;
		mForwardVel = -mJumpParams.mFireBackVelocity.get();
		break;
	}
	case 0x0080088A: {
		// Dive recovery
		startVoice(0x7884);
		f32 clampedVel = 15.0f + mForwardVel;
		if (clampedVel > 48.0f)
			clampedVel = 48.0f;
		mForwardVel = clampedVel;

		u16 angle = mFaceAngle.y;
		mSlideVelX = mForwardVel * jmaSinTable[angle >> jmaSinShift];
		mSlideVelZ = mForwardVel * jmaCosTable[angle >> jmaSinShift];
		mVel.x = mSlideVelX;
		mVel.z = mSlideVelZ;
		break;
	}
	case 0x02000885: {
		// Jumping from certain state
		startVoice(0x78AB);
		setPlayerJumpSpeed(0.25f, 42.0f);
		break;
	}
	case 0x088B: {
		// FLUDD-dependent jump
		TWaterGun* waterGun = mWaterGun;
		if (waterGun == NULL)
			break;

		s32 nozzle = waterGun->mCurrentNozzle;
		if (nozzle == 1) {
			// Rocket
			startVoice(0x78B9);
			rocketEffectStart();
		}
		waterGun = mWaterGun;
		if (waterGun->mCurrentNozzle == 5) {
			// Turbo
			startVoice(0x788F);
		}
		waterGun = mWaterGun;
		if (waterGun->mCurrentNozzle == 4) {
			// Hover
			startVoice(0x78AB);
		}
		setPlayerJumpSpeed(0.0f, 10.0f);
		break;
	}
	case 0x02000890: {
		// Multi-bounce/triple jump
		u16 animId = mAnimationId;
		switch (animId) {
		case 0xD2:
			// Triple jump
			startVoice(0x78B1);
			setPlayerJumpSpeed(0.25f, mDeParams.mTramplePowStep2.get());
			break;
		case 0xD3:
			// Double jump (D3)
			startVoice(0x78B6);
			setPlayerJumpSpeed(0.25f, mDeParams.mTramplePowStep3.get());
			break;
		default:
			// Other
			startVoice(0x78AB);
			setPlayerJumpSpeed(0.25f, mDeParams.mTramplePowStep1.get());
			break;
		}
		mForwardVel *= 0.8f;
		break;
	}
	case 0x0892: {
		// Directional air
		setPlayerJumpSpeed(0.25f, 42.0f);
		mForwardVel = 0.0f;
		u16 angle = mFaceAngle.y;
		mSlideVelX = mForwardVel * jmaSinTable[angle >> jmaSinShift];
		mSlideVelZ = mForwardVel * jmaCosTable[angle >> jmaSinShift];
		mVel.x = mSlideVelX;
		mVel.z = mSlideVelZ;
		startVoice(0x78B6);
		break;
	}
	case 0x0893: {
		// Pole jump
		if (arg == 0) {
			s16 poleAngle = unkF6;
			s32 fixedAngle = -0x2000;
			f32 paramSpeed = mWireParams.mJumpRate.get();
			f32 fAngle = (f32)poleAngle;
			f32 sinVal = jmaSinTable[fixedAngle >> jmaSinShift];
			mVel.y = fAngle * paramSpeed * 1.0f * sinVal;
			f32 cosVal = jmaCosTable[fixedAngle >> jmaSinShift];
			mForwardVel = -(fAngle * paramSpeed * 1.0f) * cosVal;

			u16 faceAngle = mFaceAngle.y;
			mSlideVelX
			    = mForwardVel * jmaSinTable[faceAngle >> jmaSinShift];
			mSlideVelZ
			    = mForwardVel * jmaCosTable[faceAngle >> jmaSinShift];
			mVel.x = mSlideVelX;
			mVel.z = mSlideVelZ;
		} else {
			s16 poleAngle = unkF6;
			f32 fAngle = (f32)poleAngle;
			s32 fixedAngle = 0x6000;
			f32 paramSpeed = mWireParams.mJumpRate.get();
			f32 sinVal = jmaSinTable[fixedAngle >> jmaSinShift];
			mVel.y = fAngle * paramSpeed * 1.0f * sinVal;
			f32 cosVal = jmaCosTable[fixedAngle >> jmaSinShift];
			mForwardVel = -(fAngle * paramSpeed * 1.0f) * cosVal;

			u16 faceAngle = mFaceAngle.y;
			mSlideVelX
			    = mForwardVel * jmaSinTable[faceAngle >> jmaSinShift];
			mSlideVelZ
			    = mForwardVel * jmaCosTable[faceAngle >> jmaSinShift];
			mVel.x = mSlideVelX;
			mVel.z = mSlideVelZ;
		}
		startVoice(0x78B9);
		break;
	}
	case 0x0894: {
		// Slide jump
		setPlayerJumpSpeed(0.0f, 42.0f);
		mForwardVel = 0.0f;
		u16 angle = mFaceAngle.y;
		mSlideVelX = mForwardVel * jmaSinTable[angle >> jmaSinShift];
		mSlideVelZ = mForwardVel * jmaCosTable[angle >> jmaSinShift];
		mVel.x = mSlideVelX;
		mVel.z = mSlideVelZ;
		startVoice(0x78AB);
		break;
	}
	default:
		break;
	}

	// Speed bonus
	f32 speedBonus = unk368;
	int hasSpeedBonus;
	if (speedBonus > 0.0f)
		hasSpeedBonus = 1;
	else
		hasSpeedBonus = 0;
	if (hasSpeedBonus) {
		s16 maxAge = mGraffitoParams.mSinkTime.get();
		f32 fMaxAge = (f32)maxAge;
		f32 minScale = mGraffitoParams.mSinkJumpRateMin.get();
		f32 maxScale = mGraffitoParams.mSinkJumpRateMax.get();
		f32 scaleRange = maxScale - minScale;
		f32 ratio = 1.0f - speedBonus / fMaxAge;
		f32 scale = scaleRange * ratio + minScale;
		mVel.y *= scale;
		mForwardVel *= scale;

		// Decay speed bonus
		f32 decayParam = mGraffitoParams.mSinkRecover.get();
		f32 bonus2 = unk368;
		f32 fAge2 = (f32)maxAge;
		f32 ratio2 = bonus2 / fAge2;
		f32 invRatio = 1.0f - ratio2;
		f32 decay = fAge2 * decayParam;
		f32 newBonus = bonus2 - invRatio * decay;
		unk368 = newBonus;
		if (unk368 < 0.0f)
			unk368 = 0.0f;
	}

	// Yoshi check
	u8 isOnYoshi = 0;
	if (mYoshi != NULL) {
		if (((TYoshi*)mYoshi)->onYoshi())
			isOnYoshi = 1;
	}
	if (isOnYoshi) {
		mVel.y *= mYoshiParams.mJumpYoshiMult.get();
		TYoshi* yoshi = (TYoshi*)mYoshi;
		yoshi->mFlutterState = 0;
		yoshi->mFlutterTimer = yoshi->mMaxFlutterTimer;
	}

	// Store jump-start Y
	unk104 = mPosition.y;

	// Update flag bit 8 based on status bit 25
	if (status & 0x02000000) {
		unk78 |= 0x100;
	} else {
		unk78 &= ~0x100;
	}

	return status;
}

f32 TMario::checkPlayerAround(int angleOffset, f32 distance)
{
	const TBGCheckData* outPlane;

	f32 ox = distance * JMASSin(mFaceAngle.y + angleOffset);
	f32 oy = distance * JMASCos(mFaceAngle.y + angleOffset);

	return gpMap->checkGround(mPosition.x + ox, 100.0f + mPosition.y,
	                          mPosition.z + oy, &outPlane);
}

void TMario::checkRideReCalc()
{
	if (mRidingActor != NULL) {
		Mtx localMtx;
		if (mRidingActor->getRootJointMtx() == NULL) {
			SMS_GetActorMtx(*mRidingActor, localMtx);
		} else {
			PSMTXCopy(*(mRidingActor->getRootJointMtx()), localMtx);
		}
		PSMTXInverse(localMtx, localMtx);

		mRidePrevLocalPos = mRideLocalPos;

		PSMTXMultVec(localMtx, (Vec*)&mPosition,
		             (Vec*)&mRideLocalPos);
	}
}

// Helper macro to avoid caching ctrl in a register (original reloads from unk108 each time)
#define CTRL ((TMarioControllerWork*)unk108)

void TMario::checkController(JDrama::TGraphics* gfx)
{
	// Convert gamepad stick positions to controller stick values
	CTRL->mStickHS16 = (s16)(128.0f * mGamePad->mCompSPos[0]);
	CTRL->mStickVS16 = (s16)(128.0f * mGamePad->mCompSPos[1]);

	// Scale stick values during special movement
	f32 timer368 = unk368;
	int bTimerActive;
	if (timer368 > 0.0f) {
		bTimerActive = 1;
	} else {
		bTimerActive = 0;
	}
	if (bTimerActive) {
		s16 maxTime = mGraffitoParams.mSinkTime.get();
		f32 minScale = mGraffitoParams.mSinkMoveMin.get();
		f32 maxScale = mGraffitoParams.mSinkMoveMax.get();
		f32 ratio = timer368 / (f32)maxTime;
		f32 scale = (maxScale - minScale) * (1.0f - ratio) + minScale;
		CTRL->mStickHS16 = (s16)((f32)CTRL->mStickHS16 * scale);
		CTRL->mStickVS16 = (s16)((f32)CTRL->mStickVS16 * scale);
	}

	// Clear button flags
	CTRL->mInput      = (TMarioControllerWork::Buttons)0;
	CTRL->mFrameInput = (TMarioControllerWork::Buttons)0;

	// Map gamepad meanings to controller buttons
	// A button
	if (mGamePad->mMeaning & TMarioGamePad::MEANING_0x80)
		CTRL->mInput = (TMarioControllerWork::Buttons)(CTRL->mInput | TMarioControllerWork::A);
	if (mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x80)
		CTRL->mFrameInput
		    = (TMarioControllerWork::Buttons)(CTRL->mFrameInput | TMarioControllerWork::A);

	// B button
	if (mGamePad->mMeaning & TMarioGamePad::MEANING_0x100)
		CTRL->mInput = (TMarioControllerWork::Buttons)(CTRL->mInput | TMarioControllerWork::B);
	if (mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x100)
		CTRL->mFrameInput
		    = (TMarioControllerWork::Buttons)(CTRL->mFrameInput | TMarioControllerWork::B);

	// R trigger
	if (mGamePad->mMeaning & TMarioGamePad::MEANING_0x400)
		CTRL->mInput = (TMarioControllerWork::Buttons)(CTRL->mInput | TMarioControllerWork::R);
	if (mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x400)
		CTRL->mFrameInput
		    = (TMarioControllerWork::Buttons)(CTRL->mFrameInput | TMarioControllerWork::R);

	// L trigger (frame only)
	if (mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x1000)
		CTRL->mFrameInput = (TMarioControllerWork::Buttons)(CTRL->mFrameInput | 0x10);

	// Convert analog triggers and get raw values
	u8* pZeroVal  = (u8*)((u8*)this + 0x2358);
	u8* pMidVal   = (u8*)((u8*)this + 0x236C);
	u8* pMaxVal   = (u8*)((u8*)this + 0x2380);
	f32* pMidLevel = (f32*)((u8*)this + 0x2394);

	CTRL->mAnalogRU8 = (u8)(s32)mGamePad->mCompSPos[3];
	CTRL->mAnalogLU8 = (u8)(s32)mGamePad->mCompSPos[2];

	u8 analogR = (u8)(s32)mGamePad->mCompSPos[3];
	u8 analogL = (u8)(s32)mGamePad->mCompSPos[2];

	// Analog R interpolation -> ctrl->mAnalogR
	f32 interpResult;
	if (analogR < *pZeroVal) {
		interpResult = 0.0f;
	} else if (analogR < *pMidVal) {
		interpResult
		    = *pMidLevel * (f32)(s32)(analogR - *pZeroVal) / (f32)(s32)(*pMidVal - *pZeroVal);
	} else if (analogR < *pMaxVal) {
		interpResult
		    = *pMidLevel
		      + (1.0f - *pMidLevel) * (f32)(s32)(analogR - *pMidVal)
		            / (f32)(s32)(*pMaxVal - *pMidVal);
	} else {
		interpResult = 1.0f;
	}
	CTRL->mAnalogR = interpResult;

	// Analog L interpolation -> ctrl->mAnalogL
	if (analogL < *pZeroVal) {
		interpResult = 0.0f;
	} else if (analogL < *pMidVal) {
		interpResult
		    = *pMidLevel * (f32)(s32)(analogL - *pZeroVal) / (f32)(s32)(*pMidVal - *pZeroVal);
	} else if (analogL < *pMaxVal) {
		interpResult
		    = *pMidLevel
		      + (1.0f - *pMidLevel) * (f32)(s32)(analogL - *pMidVal)
		            / (f32)(s32)(*pMaxVal - *pMidVal);
	} else {
		interpResult = 1.0f;
	}
	CTRL->mAnalogL = interpResult;

	// Analog L -> unk10C
	if (analogL < *pZeroVal) {
		interpResult = 0.0f;
	} else if (analogL < *pMidVal) {
		interpResult
		    = *pMidLevel * (f32)(s32)(analogL - *pZeroVal) / (f32)(s32)(*pMidVal - *pZeroVal);
	} else if (analogL < *pMaxVal) {
		interpResult
		    = *pMidLevel
		      + (1.0f - *pMidLevel) * (f32)(s32)(analogL - *pMidVal)
		            / (f32)(s32)(*pMaxVal - *pMidVal);
	} else {
		interpResult = 1.0f;
	}
	unk10C = interpResult;

	// Analog R -> unk110
	if (analogR < *pZeroVal) {
		interpResult = 0.0f;
	} else if (analogR < *pMidVal) {
		interpResult
		    = *pMidLevel * (f32)(s32)(analogR - *pZeroVal) / (f32)(s32)(*pMidVal - *pZeroVal);
	} else if (analogR < *pMaxVal) {
		interpResult
		    = *pMidLevel
		      + (1.0f - *pMidLevel) * (f32)(s32)(analogR - *pMidVal)
		            / (f32)(s32)(*pMaxVal - *pMidVal);
	} else {
		interpResult = 1.0f;
	}
	unk110 = interpResult;

	// Deadzone processing for stick H/V
	CTRL->mStickH = 0.0f;
	CTRL->mStickV = 0.0f;

	if (CTRL->mStickHS16 < -7)
		CTRL->mStickH = (f32)(CTRL->mStickHS16 + 6);
	if (CTRL->mStickHS16 > 7)
		CTRL->mStickH = (f32)(CTRL->mStickHS16 - 6);
	if (CTRL->mStickVS16 < -7)
		CTRL->mStickV = (f32)(CTRL->mStickVS16 + 6);
	if (CTRL->mStickVS16 > 7)
		CTRL->mStickV = (f32)(CTRL->mStickVS16 - 6);

	// Compute stick magnitude
	f32 stickH = CTRL->mStickH;
	f32 stickV = CTRL->mStickV;
	f32 dist2  = stickH * stickH + stickV * stickV;
	f32 stickDist = dist2;

	if (dist2 > 0.0f) {
		double guess = __frsqrte((double)dist2);
		guess = .5 * guess * (3.0 - guess * guess * dist2);
		f32 sqrtResult;
		sqrtResult = (f32)(dist2 * guess);
		stickDist  = sqrtResult;
	}

	// Apply decay from mLengthMultTimes
	s32 times = (s32)mControllerParams.mLengthMultTimes.get();
	for (s32 i = 0; i < times; i++) {
		stickDist *= mControllerParams.mLengthMult.get();
	}

	CTRL->mStickDist = stickDist;

	// Cap stick distance at 64.0f
	if (CTRL->mStickDist > 64.0f) {
		f32 scale = 64.0f / CTRL->mStickDist;
		CTRL->mStickH = CTRL->mStickH * scale;
		CTRL->mStickV = CTRL->mStickV * (64.0f / CTRL->mStickDist);
		CTRL->mStickDist = 64.0f;
	}

	// Compute intended magnitude
	f32 normDist     = (1.0f / 64.0f) * CTRL->mStickDist;
	mIntendedMag     = 32.0f * normDist * normDist;

	// Decrement rotation timer
	if (unkA0 > 0)
		unkA0 = unkA0 - 1;

	// Rotation processing
	s32 rotOffset = 0;
	if (unkA0 > 0) {
		s16 rotTimer = unkA0;
		s16 unk252C  = mGraffitoParams.mDizzyAngleY.get();
		s16 unk2518  = mGraffitoParams.mDizzyWalkCtMax.get();
		f32 unk2540  = mGraffitoParams.mDizzyAngleRate.get();
		f32 unk2554  = mGraffitoParams.mDizzyPowerRate.get();
		f32 unk2568  = mGraffitoParams.mDizzyPower.get();

		u16 sinAngle = (u16)(s32)((f32)rotTimer * unk2540);
		u16 cosAngle = (u16)(s32)((f32)rotTimer * unk2554);

		f32 sinVal = JMASSin(sinAngle);
		f32 cosVal = JMASCos(cosAngle);

		rotOffset = (s32)(sinVal * (f32)unk252C * (f32)unk252C / (f32)unk2518);
		mIntendedMag = mIntendedMag + cosVal * unk2568;
		if (mIntendedMag < 0.0f)
			mIntendedMag = 0.0f;
	}

	// Set intended yaw from stick direction
	if (mIntendedMag > 0.0f) {
		s16 stickAngle = matan(-CTRL->mStickV, CTRL->mStickH);
		s16 camAngle   = *(s16*)((u8*)gpCamera + 0x258);
		mIntendedYaw   = stickAngle + camAngle + rotOffset;
	} else {
		mIntendedYaw = mFaceAngle.y;
	}

	// Watergun turbo nozzle handling
	if (mWaterGun != nullptr) {
		if (mWaterGun->mCurrentNozzle == TWaterGun::Turbo) {
			u8 hasPump;
			if (mPumpState == 0) {
				hasPump = 1;
			} else {
				hasPump = 0;
			}
			if (hasPump && mGamePad->mCompSPos[3] > 0.0f
			    && (f32)(s32)mWaterGun->mCurrentWater > 0.0f) {
				// Turbo nozzle active
				if (0.0f == mIntendedMag)
					mIntendedYaw = mFaceAngle.y;

				f32 rotSpeed = mDeParams.mDashAcc.get();

				mDashSpeed += rotSpeed;
				if (mDashSpeed > 32.0f) {
					mDashSpeed = 32.0f;
					mDashTimer = mDashTimer + 1;
					if ((f32)mDashTimer
					    > (f32)mDeParams.mDashStartTime.get()) {
						mDashTimer = mDeParams.mDashStartTime.get();

						u8 hasFlag;
						if (mState & 0x4000) {
							hasFlag = 1;
						} else {
							hasFlag = 0;
						}
						if (!hasFlag) {
							TNozzleBase* nozzle
							    = mWaterGun->getCurrentNozzle();
							if (((TNozzleTrigger*)nozzle)->unk385 == 1) {
								mState |= 0x4000;
								startSoundActor(0x814);

								u8 isRunning;
								if (mAction & 0x2000) {
									isRunning = 1;
								} else {
									isRunning = 0;
								}
								if (isRunning) {
									changePlayerStatus(
									    0x24D5, 0, false);
								}
							}
						}
					}

					// Check special states
					if ((mAction - 0x04000000u) == 0x440u
					    || mAction == 0x24D5) {
						// keep going
					} else {
						mDashTimer = 0;
						mState &= ~0x4000;
					}
				} else {
					mDashTimer = 0;
					mState &= ~0x4000;
				}

				mIntendedMag = mDashSpeed;
				mWaterGun->rotateProp(mDashSpeed);
			} else {
				if (mDashSpeed > 0.1f) {
					if (0.0f == mIntendedMag)
						mIntendedYaw = mFaceAngle.y;
					mDashSpeed *= mDeParams.mDashBrake.get();
					mIntendedMag = mDashSpeed;
				} else {
					mDashSpeed = 0.0f;
				}
				mDashTimer = 0;
				mState &= ~0x4000;
			}
		} else {
			if (mDashSpeed > 0.1f) {
				if (0.0f == mIntendedMag)
					mIntendedYaw = mFaceAngle.y;
				mDashSpeed *= mDeParams.mDashBrake.get();
				mIntendedMag = mDashSpeed;
			} else {
				mDashSpeed = 0.0f;
			}
			mDashTimer = 0;
			mState &= ~0x4000;
		}

		// Check turbo nozzle prop rotation
		if (mWaterGun->mCurrentNozzle == TWaterGun::Turbo) {
			if ((mAction - 0x0C400000u) == 0x201u
			    || (mAction - 0x04000000u) == 0x440u) {
				f32 propSpeed = mIntendedMag * (1.0f / 32.0f);
				TNozzleBase* nozzle = mWaterGun->getCurrentNozzle();
				*(f32*)((u8*)nozzle + 0x714) = propSpeed;
			}
		}
	}

	// Set stick moved flag
	if (mIntendedMag > 0.0f)
		mInput |= 0x1;

	// B button frame -> input flag 0x2
	if (mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x80)
		mInput |= 0x2;

	// A button held -> input flag 0x80
	if (mGamePad->mMeaning & TMarioGamePad::MEANING_0x80)
		mInput |= 0x80;

	// B button on ctrl -> input flag 0x4000
	if (CTRL->mInput & TMarioControllerWork::B)
		mInput |= 0x4000;

	// Check Yoshi
	u8 isOnYoshi = 0;
	if (mYoshi != nullptr) {
		if (mYoshi->onYoshi())
			isOnYoshi = 1;
	}

	// B frame when not on Yoshi
	if (!isOnYoshi) {
		if (CTRL->mFrameInput & TMarioControllerWork::B) {
			mInput |= 0x8000;
			mInput |= 0x2000;
		}
	}

	// L trigger / Z button check
	if ((mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x2000)
	    || (CTRL->mFrameInput & 0x40)) {
		u8 isSpecialAction;
		if (mAction & 0x800) {
			isSpecialAction = 1;
		} else {
			isSpecialAction = 0;
		}
		if (isSpecialAction == 1)
			mInput |= 0x8000;
	}

	// FLUDD nozzle switch handling
	u8 hasFluddFlag;
	if (mState & 0x8000) {
		hasFluddFlag = 1;
	} else {
		hasFluddFlag = 0;
	}
	if (hasFluddFlag) {
		u8 isTurboActive;
		if (mState & 0x4000) {
			isTurboActive = 1;
		} else {
			isTurboActive = 0;
		}
		if (!isTurboActive) {
			u32 meaning = mGamePad->mMeaning;
			if ((meaning & TMarioGamePad::MEANING_0x400)
			    || (meaning & TMarioGamePad::MEANING_0x2000))
				mInput |= 0x200;
			if (mGamePad->mEnabledFrameMeaning & TMarioGamePad::MEANING_0x400)
				mInput |= 0x100;
		}
	}
}

#undef CTRL

void TMario::checkPlayerAction(JDrama::TGraphics* gfx)
{
	mInput = 0;
	checkController(gfx);
	makeHistory();
	checkCurrentPlane();
	checkRideMovement();
	if (!(mInput & 3))
		mInput |= 0x20;
}

void TMario::makeHistory()
{
	if (mIntendedMag > 0.0f) {
		if (unk534 == 0)
			*(s16*)(&unk536) = mFaceAngle.y;

		unk530[unk534] = mIntendedYaw;
		unk534 = unk534 + 1;

		if ((s32)unk534 >= mControllerParams.mStickRotateTime.get()) {
			for (int i = 0; i < mControllerParams.mStickRotateTime.get(); i++) {
				unk530[i] = unk530[i + 1];
			}
			unk534 = (u8)(mControllerParams.mStickRotateTime.get() - 1);
		}

		s16 diff = (s16)(mIntendedYaw - mFaceAngle.y);
		if (diff >= -0x2000 && diff <= 0x2000) {
			*(s16*)(&unk538) = *(s16*)(&unk538) + 1;
			if (*(s16*)(&unk538) > 0x78) {
				unk53B = 6;
				*(s16*)(&unk538) = 0x78;
			}
		} else {
			*(s16*)(&unk538) = 0;
		}
	} else {
		unk534 = 0;
		*(s16*)(&unk538) = 0;
	}

	if (unk53B != 0) {
		unk53A = 1;
		unk53B = unk53B - 1;
	} else {
		unk53A = 0;
		unk53B = 0;
	}
}

BOOL TMario::checkAllMotions()
{
	u32 flags = mInput;
	if (flags & 0x2) {
		int rotDir;
		BOOL rotResult;
		if (checkStickRotate(&rotDir)) {
			switch (rotDir) {
			case 2:
				changePlayerStatus(0x896, 0, false);
				break;
			case 3:
				changePlayerStatus(0x895, 0, false);
				break;
			}
			rotResult = 1;
		} else {
			rotResult = 0;
		}

		if (rotResult)
			return true;

		return changePlayerStatus(0x02000880, 0, false);
	}

	if (flags & 0x4)
		return changePlayerStatus(0x88C, 0, false);

	if (flags & 0x1)
		return changePlayerStatus(0x04000440, 0, false);

	if (flags & 0x8)
		return changePlayerStatus(0x50, 0, false);

	return false;
}

inline void TMario::makeGraffitoDamage(const TMario::TEParams& params)
{
	mFloorHitActor.mPosition.x = mPosition.x + JMASSin(mFaceAngle.y);
	mFloorHitActor.mPosition.z = mPosition.z + JMASCos(mFaceAngle.y);

	damageExec(&mFloorHitActor, params.mDamage.get(), params.mDownType.get(),
	           params.mWaterEmit.get(), params.mMinSpeed.get(),
	           params.mMotor.get(), params.mDirty.get(),
	           params.mInvincibleTime.get());
}

void TMario::checkGraffitoFire()
{
	if (isMarioInvincibleInline(this))
		return;

	u8 waterFlag;
	if (mState & 0x400)
		waterFlag = 1;
	else
		waterFlag = 0;
	if (waterFlag) return;

	if (mPosition.y - mFloorPosition.y > mGraffitoParams.mFireHeight.get())
		return;

	u32 action = mAction;
	if (action == 0x208B7 || action == 0x8000239) {
		mFaceAngle.y += 0x8000;
	}

	f32 savedForwardVel = mForwardVel;
	f32 savedVelY       = mVel.y;

	makeGraffitoDamage(mDmgParamsGraffitoFire);

	if (unk55C > 0.0f) {
		mVel.y = -savedVelY;
		mForwardVel = savedForwardVel;
	}

	unk14C = mGraffitoParams.mFireInvincibleTime.get();
	dropObject();
	changePlayerStatus(0x208B7, 1, false);
	gpMarioParticleManager->emitAndBindToPosPtr(6, &mPosition, 0, 0);

	if (gpMSound->gateCheck(0x1813)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x1813, (Vec*)&mPosition, 0, (JAISound**)0, 0, 4);
	}
}

void TMario::checkGraffitoSlip()
{
	u8 onSlipSurface;
	if (getMpositionY() <= 4.0f + getFloorPositionY())
		onSlipSurface = 1;
	else
		onSlipSurface = 0;

	if (onSlipSurface) {
		unk360 = mDeParams.mFootPrintTimerMax.get();

		u32 action = getAction();
		if (action == 0x84045D || action == 0x4045E) {
			unk138 = mDirtyParams.mBrakeStartValSlip.get();
			unk13C = mDirtyParams.mDirtyTimeSlip.get();
		}

		action = mAction;
		if (action - 0x40000 == 0x45C || action - 0x40000 == 0x561) {
			unk138 = mDirtyParams.mBrakeStartValRun.get();
			unk13C = mDirtyParams.mDirtyTimeRun.get();
		}

		const TBGCheckData* ground = getGroundPlane();
		if (ground->getNormal().y <= mDirtyParams.mSlopeAngle.get()) {
			unk138 = mDirtyParams.mBrakeStartValSlip.get();
			unk13C = mDirtyParams.mDirtyTimeSlip.get();
			changePlayerStatus(0x4045E, 0, false);
			startVoice(0x78D3);
		} else {
			action = mAction;
			if (action == 0x80088A || action == 0x800456
			    || action == 0x84045D || action == 0x4045E) {
				unk138 = mDirtyParams.mBrakeStartValSlip.get();
				unk13C = mDirtyParams.mDirtyTimeSlip.get();
				changePlayerStatus(0x84045D, 0, false);
				if (mPrevAction != 0x84045D) {
					startVoice(0x78D3);
				}
			} else if (mAction != 0x386) {
				unk138 = mDirtyParams.mBrakeStartValRun.get();
				unk13C = mDirtyParams.mDirtyTimeRun.get();
				if (mAction == 0x560) {
					changePlayerStatus(0x40561, 0, false);
				} else {
					changePlayerStatus(0x4045C, 0, false);
				}
			}
		}

		u8 bit25;
		if (mState & 0x40)
			bit25 = 1;
		else
			bit25 = 0;
		if (!bit25) {
			unk34E = mDirtyParams.mFogTimeYellow.get()
			         + mDirtyParams.mFogTimeRed.get();
		}
		u16 timer = unk34E;
		unk34E = timer - 1;
		timer = unk34E;
		if (timer != 0) {
			if (timer == mDirtyParams.mFogTimeRed.get()) {
				floorDamageExec(1, 3, 0,
				                mMotorParams.mMotorReturn.get());
			}
		} else {
			unk34E = mDirtyParams.mFogTimeYellow.get()
			         + mDirtyParams.mFogTimeRed.get();
		}
	} else {
		u32 action = mAction;
		if (action == 0x84045D || action == 0x4045E) {
			unk138 = mDirtyParams.mBrakeSlipNoPollute.get();
			unk13C = mDirtyParams.mDirtyTimeSlip.get();
		}
	}
}

void TMario::checkGraffitoElec()
{
	u8 bit25;
	if (mState & 0x40)
		bit25 = 1;
	else
		bit25 = 0;
	if (!bit25) {
		unk34E = mDeParams.mGraffitoNoDmgTime.get();
	}

	u16 timer = unk34E;
	if (timer != 0) {
		unk34E = timer - 1;
		return;
	}

	if (isMarioInvincibleInline(this))
		return;

	u32 motionBits = mAction & 0x1C0;
	if (motionBits != 0) {
		if (motionBits != 0x40) return;
	}

	if (unk360 > 0) return;

	changePlayerStatus(0x20338, 0, false);

	if (gpMSound->gateCheck(0x1814)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x1814, (Vec*)&mPosition, 0, (JAISound**)0, 0, 4);
	}
	if (gpMSound->gateCheck(0x3806)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x3806, (Vec*)&mPosition, 0, (JAISound**)0, 0, 4);
	}
}

void TMario::checkGraffito()
{
	// Early exit: ground plane has graffito flag
	u8 hasGrafFlag;
	if (mGroundPlane->mFlags & 0x10)
		hasGrafFlag = 1;
	else
		hasGrafFlag = 0;

	if (hasGrafFlag)
		return;

	// Early exit: on Yoshi
	u8 onYoshiCheck = 0;
	if (mYoshi != NULL) {
		if (((TYoshi*)mYoshi)->onYoshi())
			onYoshiCheck = 1;
	}
	if (onYoshiCheck)
		return;

	// Early exit: unk388 state
	if (unk388 == 1)
		return;
	if (unk388 == 2)
		return;

	// Get pollution type at current position
	s32 isPolluted = 0;
	unk350 = gpPollution->getPollutionType(getMpositionX(), mPosition.y,
	                                       mPosition.z);

	switch (unk350) {
	case 2:
	case 5:
	case 6: {
		// 3x3 grid check
		isPolluted = 1;
		JGeometry::TVec3<f32> pos(mPosition);
		pos.x -= 32.0f;
		pos.z -= 32.0f;

		// Row 0: check 3 columns
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x += 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x += 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;

		// Row 1: z += 32
		pos.z += 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x -= 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x -= 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;

		// Row 2: z += 32
		pos.z += 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x += 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x += 32.0f;
		if (gpPollution->isPolluted(pos.x, pos.y, pos.z))
			break;
		isPolluted = 0;
		break;
	}
	case 0:
	case 1:
	case 3:
	case 7: {
		// Cross pattern check (5 points)
		isPolluted = 1;
		JGeometry::TVec3<f32> pos;
		pos.x = getMpositionX();
		pos.y = mFloorPosition.y;
		pos.z = mPosition.z;

		pos.z -= 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x += 32.0f;
		pos.z += 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x -= 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.x -= 32.0f;
		if (!gpPollution->isPolluted(pos.x, pos.y, pos.z))
			isPolluted = 0;
		pos.z += 32.0f;
		pos.x += 32.0f;
		if (gpPollution->isPolluted(pos.x, pos.y, pos.z))
			break;
		isPolluted = 0;
		(void)&pos;
		break;
	}
	case 4: {
		// Single point check
		JGeometry::TVec3<f32> pos;
		pos.x = getMpositionX();
		pos.y = mFloorPosition.y;
		pos.z = mPosition.z;

		if (gpPollution->isPolluted(pos.x, pos.y, pos.z)) {
			isPolluted = 1;
		} else {
			isPolluted = 0;
		}
		break;
	}
	case 8:
	case 10:
		isPolluted = 0;
		break;
	case 9:
	default:
		break;
	}

	// If on Yoshi and polluted, get off
	u8 onYoshi2 = 0;
	if (mYoshi != NULL) {
		if (((TYoshi*)mYoshi)->onYoshi())
			onYoshi2 = 1;
	}
	if (onYoshi2) {
		if (isPolluted == 1) {
			getOffYoshi(true);
		}
	}

	// Second switch: call appropriate graffito handler
	switch (unk350) {
	case 4:
		if (isPolluted == 1)
			checkGraffitoElec();
		break;
	case 1:
	case 7:
		if (isPolluted == 1)
			checkGraffitoFire();
		break;
	case 2:
		if (isPolluted == 1)
			checkGraffitoSlip();
		break;
	case 3:
		if (isPolluted == 1) {
			mPosition.x = mLastSafePos.x;
			mPosition.z = mLastSafePos.z;
		}
		break;
	case 0:
	case 5:
	case 6:
	case 8:
	default:
		break;
	}

	// Set/clear graffito flag
	if (isPolluted == 1) {
		mState |= 0x40;
	} else {
		mState &= ~0x40;
	}

	// Check floor proximity for effects
	u8 isOnFloor;
	if (mPosition.y <= mFloorPosition.y + 4.0f)
		isOnFloor = 1;
	else
		isOnFloor = 0;

	if (!isOnFloor)
		return;

	// Emit pollution effect if standing in it
	if (isPolluted == 1) {
		JGeometry::TVec3<f32>* planeNormal
		    = (JGeometry::TVec3<f32>*)(((u8*)mGroundPlane) + 0x34);
		SMS_EmitSinkInPollutionEffect(
		    *(JGeometry::TVec3<f32>*)&mPosition, *planeNormal, false);
	}

	// Footprint timer
	s16 footTimer = unk360;
	if (footTimer <= 0)
		return;

	unk360 = footTimer - 1;

	s16 halfDuration = mDeParams.mFootPrintTimerMax.value;
	halfDuration = halfDuration / 2;
	if (unk360 <= halfDuration)
		return;

	// Check ground plane flag (inverted)
	u8 groundFlag;
	if (mGroundPlane->getFlags() & 0x10)
		groundFlag = 1;
	else
		groundFlag = 0;

	u8 notOnGraffito;
	if (groundFlag == 1)
		notOnGraffito = 0;
	else
		notOnGraffito = 1;

	if (!notOnGraffito)
		return;

	// Check trigger flags
	u8 hasTrigger;
	if (mState & 0x30000)
		hasTrigger = 1;
	else
		hasTrigger = 0;

	if (hasTrigger)
		return;

	emitDirtyFootPrint();
}

bool TMario::isInvincible() const
{
	if (unk14C > 0)
		return true;

	if (checkFlag(MARIO_FLAG_NPC_TALKING))
		return true;

	if (mStatus == 0x89C)
		return true;

	if (gpMarDirector->isDemoMode3() || gpMarDirector->isDemoMode4()
	    || gpMarDirector->isTalkModeNow() || checkActionFlag(0x1000))
		return true;

	return false;
}

#pragma dont_inline on
bool TMario::isForceSlip()
{
	if (mGroundPlane->isUnk1())
		return true;

	if (unk350 == 2) {
		if (checkFlag(0x40)) {
			if (mGroundPlane->getNormal().y < mDirtyParams.mSlopeAngle.get())
				return true;
		}
	}

	if (mGroundPlane->getNormal().y < mDeParams.mForceSlipAngle.get())
		return true;

	return false;
}
#pragma dont_inline reset

bool TMario::isUnderWater() const
{
	u8 inWater;
	if (mState & 0x30000)
		inWater = 1;
	else
		inWater = 0;

	if (inWater) {
		f32 floorZ = getMfloorpositionZ();
		f32 param = mSwimParams.mCanBreathDepth.get();
		f32 val = unk16C.y;
		if (val < floorZ - param)
			return true;
	}
	return false;
}

bool TMario::isWallInFront() const
{
	if (mWallPlane != NULL) {
		s16 wallAngle = getWallAngle();
		s16 diff = (s16)(wallAngle - mFaceAngle.y);
		if (diff < -0x71C7 || diff > 0x71C7)
			return true;
	}
	return false;
}

void TMario::thinkSand()
{
	u8 inWater;
	if (mState & 0x30000)
		inWater = 1;
	else
		inWater = 0;

	if (!inWater) {
		u16 code = *(u16*)mGroundPlane;
		u8 isSand;
		if (code == 0x701 || code == 0x4701 ||
		    code == 0x8701 || code == 0xC701)
			isSand = 1;
		else
			isSand = 0;

		if (isSand == 1) {
			mState |= 0x40000;
			emitSandEffect();
			return;
		}
	}
	mState &= ~0x40000;
}

f32 TMario::getJumpAccelControl() const
{
	if (mAction == 0x892)
		return mWireParams.mWireJumpAccelControl.get();
	return mJumpParams.mJumpAccelControl.get();
}

f32 TMario::getJumpSlideControl() const
{
	if (mAction == 0x892)
		return mWireParams.mWireJumpSlideControl.get();

	u8 riding = onYoshi();

	if (riding) {
		u8 fluttering;
		if (mYoshi->mFlutterState == 1)
			fluttering = 1;
		else
			fluttering = 0;

		if (fluttering)
			return mYoshiParams.mHoldOutSldCtrl.get();
	}

	return mJumpParams.mJumpSlideControl.get();
}

BOOL TMario::considerRotateJumpStart()
{
	int rotDir;
	if (checkStickRotate(&rotDir)) {
		switch (rotDir) {
		case 2:
			changePlayerStatus(0x896, 0, false);
			break;
		case 3:
			changePlayerStatus(0x895, 0, false);
			break;
		}
		return true;
	}
	return false;
}

bool TMario::canSquat() const
{
	u8 hasFludd;
	if (getState() & 0x8000)
		hasFludd = 1;
	else
		hasFludd = 0;

	if (hasFludd) {
		if (getWaterGun() != NULL) {
			TNozzleBase* nozzle = getWaterGun()->getCurrentNozzle();
			if (nozzle->mEmitParams.mRocketType.get() != 1) {
				if ((s32)getWaterGun()->getCurrentNozzleType() != 5) {
					if (getInput() & 0x200) {
						return true;
					}
				}
			}
		}
	}
	return false;
}

void TMario::thinkDirty()
{
	u8 isDirty;
	if (mState & 0x40)
		isDirty = 1;
	else
		isDirty = 0;

	if (isDirty) {
		if (mAction == 0x04000440 || mAction == 0x0004045C) {
			unk134 += mDirtyParams.mIncRunning.get();
		}
		if (mAction == 0x00800456 || mAction == 0x0084045D ||
		    mAction == 0x0004045E) {
			unk134 += mDirtyParams.mIncCatching.get();
		}
		if (mAction == 0x50) {
			unk134 += mDirtyParams.mIncSlipping.get();
		}
	}

	u8 inWater;
	if (mState & 0x30000)
		inWater = 1;
	else
		inWater = 0;

	if (inWater) {
		f32 waterLevel = getMfloorpositionZ();
		if (mPosition.y > waterLevel - 200.0f)
			meltInWaterEffect();
		unk360 = 0;
		unk134 -= mDirtyParams.mDecSwimming.get();
	}

	if (mAction == 0x895 || mAction == 0x896) {
		unk134 -= mDirtyParams.mDecRotJump.get();
		unk360 = 0;
	}

	u8 hasShirt;
	if (mState & 0x10)
		hasShirt = 1;
	else
		hasShirt = 0;

	if (hasShirt) {
		unk134 -= mDirtyParams.mDecWaterHit.get();
		unk360 = 0;
	}

	dirtyLimitCheck();
}

void TMario::checkRideMovement()
{
	TLiveActor* rideActor = 0;

	Vec pos;
	pos = *(Vec*)&mPosition;

	f32 sinAmt = 50.0f * JMASSin((u16)mFaceAngle.y);
	pos.x += 0.8f * sinAmt;
	f32 cosAmt = 50.0f * JMASCos((u16)mFaceAngle.y);
	pos.z += 0.8f * cosAmt;

	const TBGCheckData* wallPlane =
	    checkWallPlane(&pos, 50.0f, unk15C);

	const TLiveActor* groundActor;
	if ((groundActor = (TLiveActor*)mGroundPlane->mActor) != 0) {
		u8 actionBit;
		if (mAction & 0x800)
			actionBit = 1;
		else
			actionBit = 0;
		if (!actionBit) {
			u8 nearGround;
			if (mPosition.y <= 4.0f + mFloorPosition.y)
				nearGround = 1;
			else
				nearGround = 0;
			if (nearGround)
				rideActor = (TLiveActor*)groundActor;
		}
	}

	if (groundActor != 0) {
		if (mAction == 0x8008A9) {
			u16 subState = mActionState;
			if (subState == 2 || subState == 3)
				rideActor = (TLiveActor*)groundActor;
		}
	}

	{
		u8 actionBit2;
		if (mAction & 0x20000000)
			actionBit2 = 1;
		else
			actionBit2 = 0;
		if (actionBit2) {
			if (wallPlane != 0) {
				if (wallPlane->mActor != 0)
					rideActor = (TLiveActor*)wallPlane->mActor;
			}
		}
	}

	if (rideActor != 0) {
		if (mRidingActor == 0 || mRidingActor != rideActor) {
			// newRide
			mRidingActor = rideActor;

			TLiveActor* ride = mRidingActor;
			mRidePrevRotY = ride->mRotation.y;

			checkRideReCalc();
		} else {
			// sameRide
			Mtx stackMtx;
			if (mRidingActor->getRootJointMtx() == 0) {
				SMS_GetActorMtx(*mRidingActor, stackMtx);
			} else {
				PSMTXCopy((MtxPtr)mRidingActor->getRootJointMtx(), stackMtx);
			}

			PSMTXMultVec(stackMtx,
			             (Vec*)&mRideLocalPos,
			             (Vec*)&mPosition);

			f32 savedRot = mRidePrevRotY;
			f32 currentRot = mRidingActor->mRotation.y;
			f32 delta = currentRot - savedRot;
			s16 faceAngle = mFaceAngle.y;
			mFaceAngle.y =
			    faceAngle + (int)(32768.0f * delta / 180.0f);

			mRidePrevRotY = mRidingActor->mRotation.y;
		}
	} else {
		mRidingActor = nullptr;
	}
}

void TMario::checkCurrentPlane()
{
	if (checkStatusType(0x1000))
		return;

	if (onYoshi())
		unk15C = 80.0f;
	else
		unk15C = 50.0f;

	TBGWallCheckRecord record(mPosition.x, mPosition.y + 60.0f, mPosition.z,
	                          unk15C, 2, 0);

	gpMap->isTouchedWallsAndMoveXZ(&record);
	if (!isInvincible()) {
		for (int i = 0; i < record.mResultWallsNum; ++i)
			if (record.mResultWalls[i]->isThing5())
				damageExec(&mFloorHitActor, record.mResultWalls[i]->getData());

		if (record.mResultWallsNum == 2
		    && record.mResultWalls[0]->getNormal().dot(
		           record.mResultWalls[1]->getNormal())
		           < -0.9f) {

			JGeometry::TVec3<f32> normal1 = record.mResultWalls[0]->getNormal();
			JGeometry::TVec3<f32> normal2 = record.mResultWalls[1]->getNormal();

			f32 planeDist1 = record.mResultWalls[0]->getPlaneDistance();
			f32 planeDist2 = record.mResultWalls[1]->getPlaneDistance();

			f32 dist1 = normal1.dot(mPosition) + planeDist1;
			f32 dist2 = normal2.dot(mPosition) + planeDist2;

			if ((record.mResultWalls[0]->getActor() != nullptr
			     && record.mResultWalls[0]->getActor()->getActorType()
			            == 0x400002BD)
			    || (record.mResultWalls[1]->getActor() != nullptr
			        && record.mResultWalls[1]->getActor()->getActorType()
			               == 0x400002BD)) {

				if (dist1 < 10.0f || dist2 < 10.0f) {
					int hp = mDeParams.mHpMax.get();
					floorDamageExec(hp, 3, 0, mMotorParams.mMotorReturn.get());
				}
			}
		}
	}

	record.set(mPosition.x, mPosition.y + 30.0f, mPosition.z, unk15C * 0.5f, 1,
	           0);
	gpMap->isTouchedWallsAndMoveXZ(&record);
	if (!isInvincible()) {
		for (int i = 0; i < record.mResultWallsNum; ++i)
			if (record.mResultWalls[i]->isThing5())
				damageExec(&mFloorHitActor, record.mResultWalls[i]->getData());
	}

	checkGroundPlane(mPosition.x, mPosition.y + 25.0f, mPosition.z,
	                 &mFloorPosition.y, &mGroundPlane);

	mFloorPosition.x = gpMap->checkRoof(mPosition.x, mPosition.y + 80.0f,
	                                    mPosition.z, &mRoofPlane);
	if (!isInvincible()) {
		if ((isTouchGround4cm()) && mGroundPlane->isThing5()
		    && !checkStatusType(0x10000)) {

			damageExec(&mFloorHitActor, mGroundPlane->getData());
		}

		if (mPosition.y + 160.0f > mFloorPosition.x && mRoofPlane->isThing5()) {
			damageExec(&mFloorHitActor, mRoofPlane->getData());
		}
	}

	if (mGroundPlane->isLegal()) {
		mSlopeAngle
		    = matan(mGroundPlane->getNormal().z, mGroundPlane->getNormal().x);

		BOOL slipStart;
		if (isForceSlip()) {
			slipStart = TRUE;
		} else {
			const TBGCheckData* plane = mGroundPlane;
			u16 bgType               = plane->mBGType;
			if (plane->isSlider()) {
				slipStart = TRUE;
			} else if ((bgType == 0x2 || bgType == 0x8002)
			           && plane->getNormal().y < 0.8660254f) {
				slipStart = TRUE;
			} else if (plane->isUnk3()) {
				slipStart = FALSE;
			} else if (plane->getNormal().y < mDeParams.mSlipStart.get()) {
				slipStart = TRUE;
			} else {
				slipStart = FALSE;
			}
		}

		if (slipStart || checkFlag(MARIO_FLAG_GROUND_POUND_SIT_UP))
			mInput |= 0x8;

		if (mPosition.y > mFloorPosition.y + 100.0f)
			mInput |= 0x4;

		mState &= ~MARIO_FLAG_GROUND_POUND_SIT_UP;
	}
}
#pragma dont_inline on
TMario::TEParams* TMario::getDmgMapCode(int code) const
{
	switch (code) {
	case 0: return &mDmgMapParams0;
	case 1: return &mDmgMapParams1;
	case 2: return &mDmgMapParams2;
	case 3: return &mDmgMapParams3;
	case 4: return &mDmgMapParams4;
	case 5: return &mDmgMapParams5;
	case 6: return &mDmgMapParams6;
	case 7: return &mDmgMapParams7;
	case 8: return &mDmgMapParams8;
	case 9: return &mDmgMapParams9;
	default: return &mDmgMapParams0;
	}
}
#pragma dont_inline reset

void TMario::thinkParams()
{
	mRotation.y =
	    (f32)mFaceAngle.y * (360.0f / 65536.0f);

	{
		s32 invTimer = getUnk14c();
		if (invTimer > 0) {
			unk14C = invTimer - 1;
		}
	}

	u8 waterFlag;
	if (mState & 0x400)
		waterFlag = 1;
	else
		waterFlag = 0;
	if (waterFlag) return;

	u32 motionBits = mState & 0x30000;
	{
		u8 hasMotion;
		if (motionBits)
			hasMotion = 1;
		else
			hasMotion = 0;
		if (hasMotion) {
			u8 nonZero;
			if (motionBits)
				nonZero = 1;
			else
				nonZero = 0;

			u8 belowThreshold;
			if (nonZero
			    && unk16C.y < getMfloorpositionZ() - mSwimParams.mCanBreathDepth.get()) {
				belowThreshold = 1;
			} else {
				belowThreshold = 0;
			}
			if (!belowThreshold) {
				TBGCheckData* waterFloor = mWaterFloor;
				u16 bgType = waterFloor->mBGType;
				u8 isWaterGround;
				if (bgType == 0x0B || bgType == 0x800B || bgType == 0x103
				    || bgType == 0x101)
					isWaterGround = 1;
				else
					isWaterGround = 0;

				if (isWaterGround) {
					u8 isWaterGround2;
					if (bgType == 0x0B || bgType == 0x800B || bgType == 0x103
					    || bgType == 0x101)
						isWaterGround2 = 1;
					else
						isWaterGround2 = 0;

					if (isWaterGround2) {
						u8 actionBit;
						if (mAction & 0x10000)
							actionBit = 1;
						else
							actionBit = 0;

						if (!actionBit) {
							s16 data = waterFloor->getData();
							TEParams* params;
							switch (data) {
							case 0: params = &mDmgMapParams0; break;
							case 1: params = &mDmgMapParams1; break;
							case 2: params = &mDmgMapParams2; break;
							case 3: params = &mDmgMapParams3; break;
							case 4: params = &mDmgMapParams4; break;
							case 5: params = &mDmgMapParams5; break;
							case 6: params = &mDmgMapParams6; break;
							case 7: params = &mDmgMapParams7; break;
							case 8: params = &mDmgMapParams8; break;
							case 9: params = &mDmgMapParams9; break;
							default: params = &mDmgMapParams0; break;
							}
							floorDamageExec(*params);
						}
					}
				}
			}

			// resetCounters
			unk126 = 0;
			unk128_s16 = mDeParams.mHotTimer.get();
		} else {
			// capBranch
			TMarioCap* cap = mCap;
			if (cap != 0) {
				u8 hatOn;
				if (*(u16*)((u8*)cap + 4) & 1)
					hatOn = 1;
				else
					hatOn = 0;
				if (!hatOn) {
					unk126 = unk126 + 1;
					if ((u16)unk126
					    > (s16)unk128_s16) {
						decHP(1);
						if (gpMSound->gateCheck(0x480C)) {
							MSoundSESystem::MSoundSE::startSoundSystemSE(
							    0x480C, 0, (JAISound**)0, 0);
						}

						unk128_s16 = mDeParams.mHotTimer.get();
						unk126 = 0;
						rumbleStart(20, mMotorParams.mMotorWall.get());
						*(u16*)&unk14C = (int)unk55C;
					}
				}
			}
			unk122 = 0;
		}
	}
	{
		const TBGCheckData* ground = getGroundPlane();
		if (ground != 0) {
			const TLiveActor* actor = ground->getActor();
			if (actor != 0) {
				if (actor->getActorType() == 0x400002C7) {
					emitFootPrintWithEffect(-1, 66);
				}
			}
		}
	}
}

void TMario::thinkWaterSurface()
{
	// Early out if on water (bit 16 of mAction)
	u8 onWater;
	if (mAction & 0x10000)
		onWater = 1;
	else
		onWater = 0;
	if (onWater)
		return;

	// Check if currently in water (bits 14-15 of mState)
	u32 waterBits = mState & 0x30000;
	s32 wasInWater;
	if (waterBits)
		wasInWater = 1;
	else
		wasInWater = 0;

	s32 r31 = wasInWater;
	s32 r30 = 0;

	s32 wasOnSurface;
	if (waterBits != 0)
		wasOnSurface = 1;
	else
		wasOnSurface = 0;

	if ((u8)wasOnSurface == 1) {
		r30 = 1;
	} else {
		mFloorPosition.z = mPosition.y;
	}

	// Clear water surface flags
	mState &= ~0x10000;
	mState &= ~0x20000;

	// Check if ground plane is a pool type
	u32 bgType = mGroundPlane->getBGType();
	u8 isPool;
	if (bgType == BG_TYPE_POOL || bgType == BG_TYPE_INDOOR_POOL
	    || bgType == BG_TYPE_SHADED_POOL)
		isPool = 1;
	else
		isPool = 0;

	if (isPool) {
		mFloorPosition.z = gpPoolManager->getWaterLevel(mGroundPlane);
		if (getMfloorpositionZ() > mPosition.y) {
			r30 = 1;
			mState |= 0x10000;
		}
	}

	// Check height above ground with offset
	{
		f32 heightDiff = mPosition.y - mLastSafePos.y;
		f32 clampedDiff = heightDiff;
		if (heightDiff > 0.0f)
			clampedDiff = 0.0f;

		f32 checkHeight
		    = getMfloorpositionZ() - clampedDiff
		      + mSwimParams.mWaterLevelCheckHeight.get();
		f32 groundHeight = gpMap->checkGround(
		    getMpositionX(), checkHeight, mPosition.z, &mWaterFloor);

		// Check if ground at water level is a water surface type
		u32 groundType = mWaterFloor->mBGType;
		u8 isWaterSurface;
		if (groundType == BG_TYPE_WATER
		    || groundType == BG_TYPE_DAMAGING_WATER
		    || (u16)(groundType - BG_TYPE_SEA_WATER) <= 3
		    || groundType == BG_TYPE_SHADED_POOL)
			isWaterSurface = 1;
		else
			isWaterSurface = 0;

		if (isWaterSurface) {
			mFloorPosition.z = groundHeight;
			if (getMfloorpositionZ() >= mPosition.y) {
				r30 = 1;
				mState |= 0x20000;
			}
		} else {
			// Check ground at current position
			const TBGCheckData* groundCheck2;
			gpMap->checkGround(getMpositionX(), mPosition.y, mPosition.z,
			                   &groundCheck2);
			u8 isSpecialType;
			if (groundCheck2->mBGType == 0x810B)
				isSpecialType = 1;
			else
				isSpecialType = 0;

			if (isSpecialType) {
				r30 = 1;
				mState |= 0x20000;
			}
		}
	}

	if (r30 != 0) {
		// Water surface logic
		f32 posY2 = mPosition.y;
		f32 waterLvl = getMfloorpositionZ();
		if (posY2 < waterLvl) {

		// Check deep water threshold
		f32 deepThreshold = posY2 + mRunParams.mSwimDepth.get();
		if (waterLvl > deepThreshold) {
			// Deep water - check yoshi
			r30 = 0;
			if (mYoshi != NULL) {
				if (mYoshi->onYoshi())
					r30 = 1;
			}

			if ((u8)r30) {
				mYoshi->disappear();
				if (mWaterGun != NULL) {
					mWaterGun->changeNozzle(TWaterGun::Hover, true);
					normalizeNozzle();
				}
			}

			// Check ripple height
			f32 rippleCheck = 160.0f + mPosition.y;
			if (rippleCheck > getMfloorpositionZ())
				rippleEffect();

			swimmingBubbleEffect();

			// Determine if should enter water
			u8 shouldEnter = 1;
			u32 action7C = mAction;
			u8 isBit18;
			if (action7C & 0x2000)
				isBit18 = 1;
			else
				isBit18 = 0;
			u32 statusLow = action7C & 0x1FF;

			if (isBit18)
				shouldEnter = 0;

			// Swimming status range checks
			u8 isSwimming;
			if (statusLow >= 0x168 && statusLow <= 0x16C)
				isSwimming = 1;
			else
				isSwimming = 0;
			if (isSwimming)
				shouldEnter = 0;

			if (statusLow >= 0x145 && statusLow <= 0x14A)
				shouldEnter = 0;

			if (statusLow >= 0x140 && statusLow <= 0x143)
				shouldEnter = 0;

			// Check held object
			s32 holdingObj;
			if (mHolder != 0)
				holdingObj = 1;
			else
				holdingObj = 0;
			if (holdingObj)
				shouldEnter = 0;

			if ((u8)shouldEnter == 1) {
				// Apply water drag
			mForwardVel = mForwardVel * mSwimParams.mStartVMult.get();
			mVel.y = getMvelY() * mSwimParams.mStartVYMult.get();

			// Check if falling from air
			u8 isFalling;
			if (mAction & 0x20000)
				isFalling = 1;
			else
				isFalling = 0;

			if (isFalling) {
				// Entering water from air - dive
				changePlayerStatus(0x24DA, 0, true);
			} else {
				// Check if running on ground
				u8 isRunning;
				if (mState & 0x4000)
					isRunning = 1;
				else
					isRunning = 0;

				if (isRunning) {
					// Wading
					changePlayerStatus(0x24D5, 0, true);
					mVel.y = 0.0f;
					mPosition.y = getMfloorpositionZ();
					startSoundActor(0x828);
				} else {
					// Shallow water entry
					changePlayerStatus(0x22D1, 0, true);
				}
			}
			}
		} else {
			// Shallow water - check frame-based effects
			f32 shallowThreshold = posY2 + mWaterEffectParams.mRunningRippleDepth.get();
			if (waterLvl < shallowThreshold) {
				// Check if in walking state 0x04000440
				u32 actionVal = mAction;
				if ((u32)(actionVal - 0x04000000) == 0x440) {
					J3DFrameCtrl& frameCtrl = getMotionFrameCtrl();
					if (frameCtrl.checkPass(38.0f) || getMotionFrameCtrl().checkPass(8.0f)) {
						runningRippleEffect();
					}
				}
			} else {
				rippleEffect();
			}
		}
		}
	}


	// Check wet ground type
	{
		u32 gndBGType = mGroundPlane->mBGType;
		u8 isWetGround;
		if (gndBGType == 0x4 || gndBGType == 0x4004
		    || gndBGType == 0x8004 || gndBGType == 0xC004)
			isWetGround = 1;
		else
			isWetGround = 0;

		if (isWetGround) {
			u32 actionVal = mAction;
			if ((u32)(actionVal - 0x04000000) == 0x440) {
				J3DFrameCtrl& frameCtrl = getMotionFrameCtrl();
				if (frameCtrl.checkPass(38.0f) || getMotionFrameCtrl().checkPass(8.0f)) {
					runningRippleEffect();
				}
			}
		}
	}

	// Build water surface matrix
	{
		if (mState & 0x30000)
			r30 = 1;
		else
			r30 = 0;

		J3DGetTranslateRotateMtx(0, mModelFaceAngle, 0, mPosition.x,
		                         mFloorPosition.z, mPosition.z, mJointMtx2);

		// Store water position
		mWaterRipplePos.x = getMpositionX();
		mWaterRipplePos.y = getMfloorpositionZ();
		mWaterRipplePos.z = mPosition.z;

		// Copy joint matrix
		u32 modelPtr = (u32)mModel;
		s32 jointIdx = mBoneIDs[10];
		u32 modelData = *(u32*)(modelPtr + 0x8);
		u32 jointMtxArr = *(u32*)(modelData + 0x58);
		MtxPtr anmMtx = (MtxPtr)(jointMtxArr + jointIdx * 0x30);
		PSMTXCopy(anmMtx, mJointMtx0);

		// Check if water state changed
		if (r30 != r31) {
			inOutWaterEffect(getMfloorpositionZ());
			f32 splashHeight = getMfloorpositionZ() - mFloorPosition.y;

			if (r31 == 1 && r30 == 0) {
				// Entering water
				unk362 = 0x78;

				if (splashHeight < 32.0f) {
					// Small splash
					if (gpMSound->gateCheck(0x1939)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1939, &mPosition, 0, (JAISound**)NULL, 0, 4);
					}
				} else if (splashHeight < 80.0f) {
					// Medium splash
					if (gpMSound->gateCheck(0x181D)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x181D, &mPosition, 0, (JAISound**)NULL, 0, 4);
					}
				} else {
					// Large splash
					if (gpMSound->gateCheck(0x181E)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x181E, &mPosition, 0, (JAISound**)NULL, 0, 4);
					}
				}
			} else {
				// Exiting water
				if (splashHeight < 32.0f) {
					if (gpMSound->gateCheck(0x1938)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1938, &mPosition, 0, (JAISound**)NULL, 0, 4);
					}
				} else if (splashHeight < 80.0f) {
					if (gpMSound->gateCheck(0x1805)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1805, &mPosition, 0, (JAISound**)NULL, 0, 4);
					}
				} else {
					if (gpMSound->gateCheck(0x1806)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1806, &mPosition, 0, (JAISound**)NULL, 0, 4);
					}
				}
			}
		}
	}

	// Drowning/air recovery logic
	{
		u8 shouldDrown = 0;

		u8 isInWater2;
		if (mState & 0x30000)
			isInWater2 = 1;
		else
			isInWater2 = 0;

		if (isInWater2) {
			f32 airThreshold = getMfloorpositionZ() - mSwimParams.mCanBreathDepth.get();
			if (unk16C.y < airThreshold) {
				shouldDrown = 1;
			}
		}
		if (!shouldDrown) {
			u8 isDiving;
			if (mState & 0x1000)
				isDiving = 1;
			else
				isDiving = 0;
			if (isDiving)
				shouldDrown = 1;
		}

		if (shouldDrown) {
			f32 prevAir = getUnk12c();
			u8 isHelm;
			isHelm = isWearingHelm();
			if (isHelm) {
				u32 actionVal = mAction;
				if ((u32)(actionVal - 0x10020000) != 0x370) {
					unk12C -= mSwimParams.mAirDecDive.get();
				}
			} else {
				unk12C -= mSwimParams.mAirDec.get();
			}
			f32 currentAir = getUnk12c();

			// Compare truncated values to detect crossing
			s32 prevInt;
			s32 currInt;
			{
				f32 prev = prevAir;
				f32 curr = currentAir;
				// fctiwz + store/load pattern
				s32 prevTrunc = (s32)prev;
				s32 currTrunc = (s32)curr;
				prevInt = prevTrunc;
				currInt = currTrunc;
			}

			if (prevInt != currInt) {
				rumbleStart(0x14, mMotorParams.mMotorWall.get());
				s32 truncHP = (s32)(unk55C);
				unk14C = (s16)truncHP;
			}

			if (getUnk12c() < 1.0f) {
				unk12C = 0.0f;
				loserExec();
				changePlayerStatus(0x000224E0, 0, false);
			}
			return;
		} else {
			unk12C += mSwimParams.mAirInc.get();
			if (getUnk12c() >= getUnk130()) {
				unk12C = getUnk130();
			}
		}
	}
}

void TMario::thinkSituation()
{
	// Save previous situation flags
	mPrevState = mState;

	// Recovery timer
	f32 recoveryVal = unkBC;
	if (recoveryVal < 0.0f) {
		unkBC = recoveryVal + mJumpParams.mTrampolineDec.get();
	} else {
		unkBC = 0.0f;
	}

	// If being held, set position from holder's taking matrix
	if (mHolder != NULL) {
		if (mHolder->getTakingMtx() != NULL) {
			mPosition.x = mHolder->getTakingMtx()[0][3];
			mPosition.y = mHolder->getTakingMtx()[1][3];
			mPosition.z = mHolder->getTakingMtx()[2][3];
		}
	}

	// Clear slippery and shadow bits
	mState &= ~0x2;
	mState &= ~0x20;

	// Check slippery ground
	{
		u8 isSlippery;
		u16 bgType = mGroundPlane->mBGType;
		if (bgType == 0x106 || bgType == 0x108)
			isSlippery = 1;
		else
			isSlippery = 0;
		if (isSlippery) {
			mState |= 0x2;
		}
	}

	// Ground damage check
	{
		u8 didDamage = 0;
		if (isMario()) {
			u8 isOnGround;
			if (mPosition.y <= mFloorPosition.y + 4.0f)
				isOnGround = 1;
			else
				isOnGround = 0;

			if (isOnGround) {
				if (mAction != 0x133F) {
					u8 hasDmgFlag;
					if (mGroundPlane->mFlags & 0x10)
						hasDmgFlag = 1;
					else
						hasDmgFlag = 0;

					u8 isDmgType;
					if (mGroundPlane->mBGType == 0x600)
						isDmgType = 1;
					else
						isDmgType = 0;

					if (hasDmgFlag || isDmgType) {
						unk2BA += mDeParams.mIllegalPlaneCtInc.get();
						if (unk2BA > mDeParams.mIllegalPlaneTime.get()) {
							decHP(mDeParams.mHpMax.get());
						}
						didDamage = 1;
					}
				}
			}
		}

		if (!didDamage) {
			unk2BA -= 1;
			if (unk2BA < 0)
				unk2BA = 0;
		}
	}

	// BGM handling for slippery ground
	if (isMario()) {
		u8 curSlippery;
		if (mState & 0x2)
			curSlippery = 1;
		else
			curSlippery = 0;

		if (curSlippery == 1) {
			u8 prevSlippery;
			if (mPrevState & 0x2)
				prevSlippery = 1;
			else
				prevSlippery = 0;
			if (!prevSlippery) {
				MSBgm::startBGM(0x8001001B);
			}
		}

		u8 curSlippery2;
		if (mState & 0x2)
			curSlippery2 = 1;
		else
			curSlippery2 = 0;

		if (!curSlippery2) {
			u8 prevSlippery2;
			if (mPrevState & 0x2)
				prevSlippery2 = 1;
			else
				prevSlippery2 = 0;
			if (prevSlippery2 == 1) {
				MSBgm::stopBGM(0x8001001B, 10);
			}
		}
	}

	// Death plane check
	{
		const TBGCheckData* checkPlane;
		f32 groundHeight = gpMap->checkGround(
		    getMpositionX(), mPosition.y - getMvelY(), mPosition.z, &checkPlane);

		u8 isDeathPlane;
		if (checkPlane->mBGType == 0x800)
			isDeathPlane = 1;
		else
			isDeathPlane = 0;

		if (isDeathPlane) {
			if (groundHeight > mPosition.y) {
				mState |= 0x400;
				mHealth = 0;
				mAnmSound->stop();
				if (mYoshi != NULL)
					mYoshi->kill();
				changePlayerStatus(0x208B9, 0, true);
				if (mAnimationId != 0x120) {
					startSoundActor(0x786B);
				}
				*(u16*)((u8*)gpCamera + 0x64) |= 0x800;
				*(u16*)((u8*)gpMarDirector + 0x4E) |= 0x8;
				return;
			}
		}
	}

	// Ground collision matrix setup
	J3DGetTranslateRotateMtx(0, mModelFaceAngle, 0, getMpositionX(), mPosition.y,
	                         mPosition.z, mJointMtx1);

	// Light/shadow setup
	mLightID = 0;

	{
		u8 hasShadowBit = 0;
		if (mGroundPlane->mBGType & 0x4000)
			hasShadowBit = 1;
		if (hasShadowBit) {
			if (mFloorPosition.y + 200.0f > mPosition.y) {
				mLightID = mGroundPlane->mData;
				if (mLightID == 1) {
					mState |= 0x20;
				}
			}
		}
	}

	// Cube shadow check
	if (gpCubeShadow != NULL) {
		if ((s32)gpCubeShadow->getInCubeNo(*(Vec*)&mPosition) != -1) {
			mLightID = 1;
		}
	}

	// Water particle ground check
	mState &= ~0x10;
	if (isMario()) {
		u8 isOnGround;
		if (mPosition.y <= mFloorPosition.y + 4.0f)
			isOnGround = 1;
		else
			isOnGround = 0;
		if (isOnGround) {
			if (gpModelWaterManager->askHitWaterParticleOnGround(
			        *(JGeometry::TVec3<f32>*)&mPosition)) {
				mState |= 0x10;
			}
		}
	}

	// Stage warp check (ground type 0x300)
	{
		u8 isWarpGround;
		if (mGroundPlane->mBGType == 0x300)
			isWarpGround = 1;
		else
			isWarpGround = 0;

		if (isWarpGround) {
			u8 isOnYoshi = 0;
			if (mYoshi != NULL) {
				if (mYoshi->onYoshi())
					isOnYoshi = 1;
			}
			if (isOnYoshi) {
				getOffYoshi(false);
			}

			gpMarDirector->setNextStage(mGroundPlane->mData,
			                            (JDrama::TActor*)NULL);
			mSubState &= ~0x2;
			mSubState &= ~0x400;
		}
	}

	// Option map position constraints
	if (SMS_isOptionMap()) {
		mPosition.z = mOptionParams.mZ.get();
		if (getMpositionX() < mOptionParams.mXMin.get())
			mPosition.x = mOptionParams.mXMin.get();
		if (getMpositionX() > mOptionParams.mXMax.get())
			mPosition.x = mOptionParams.mXMax.get();
	}

	// Save ground Y when not airborne
	{
		u8 isAirborne;
		if (mAction & 0x800)
			isAirborne = 1;
		else
			isAirborne = 0;
		if (!isAirborne) {
			mLastGroundY = mPosition.y;
		}
	}

	calcGroundMtx(*(JGeometry::TVec3<f32>*)&mPosition);

	// Area type check for indoor flag
	{
		u8 areaID = *(u8*)((u8*)gpMarDirector + 0x124);
		u8 isIndoor;
		if (areaID == 1 || areaID == 2)
			isIndoor = 1;
		else
			isIndoor = 0;

		u8 hasWaterBit;
		if (mAction & 0x1000)
			hasWaterBit = 1;
		else
			hasWaterBit = 0;

		if (areaID == 3 || areaID == 4 || isIndoor || hasWaterBit) {
			mState |= 0x8;
		} else {
			mState &= ~0x8;
		}
	}
}

void TMario::getOffYoshi(bool knockedOff)
{
	mInput = getInput() & ~0x8000;
	if (knockedOff) {
		changePlayerStatus(0x89C, 0, false);
		getYoshi()->getOff(true);
	} else {
		changePlayerStatus(0x883, 0, false);
		mVel.y = mJumpParams.mGetOffYoshiY.get();
		getYoshi()->getOff(false);
	}
	setAnimation(0x4D, 1.0f);
	unk78 &= ~0x100;
	mPosition.y += 100.0f;
	mForwardVel = -8.0f;
	getWaterGun()->changeNozzle(TWaterGun::Hover, true);
	normalizeNozzle();
	TWaterGun* gun = mWaterGun;
	TNozzleBase* nozzle = gun->getCurrentNozzle();
	gun->mCurrentWater = nozzle->mEmitParams.mAmountMax.get();
}

void TMario::thinkYoshiHeadCollision()
{
	u8 riding = 0;
	if (mYoshi != NULL) {
		if (mYoshi->onYoshi())
			riding = 1;
	}
	if (!riding)
		return;

	JGeometry::TVec3<f32> headPos = mPosition;

	f32 front = mYoshiParams.mHeadFront.get();
	u16 angle = mFaceAngle.y;
	headPos.x += JMASSin(angle) * front;
	headPos.z += JMASCos(angle) * front;

	TBGWallCheckRecord wallCheck(headPos.x, headPos.y + 100.0f, headPos.z,
	                             mYoshiParams.mHeadRadius.get(), 4, 0);
	f32 headZ = headPos.z;

	if (gpMap->isTouchedWallsAndMoveXZ(&wallCheck) == true) {
		f32 dx   = wallCheck.mCenter.x - headPos.x;
		f32 dz   = wallCheck.mCenter.z - headZ;
		f32 dist = sqrtf(dx * dx + dz * dz);

		f32 pushDist = dist;
		if (dist > 0.0f) {
			dx *= 1.0f / dist;
			dz *= 1.0f / dist;

			if (50.0f < dist)
				pushDist = 50.0f;

			dx *= pushDist;
			dz *= pushDist;

			mPosition.x += dx;
			mPosition.z += dz;
		}
	}
}

void TMario::checkWet()
{
	if (!isMario())
		return;

	u8 onYoshiFlag = 0;
	if (mYoshi != NULL) {
		if (mYoshi->onYoshi())
			onYoshiFlag = 1;
	}
	if (onYoshiFlag)
		return;

	s32 wetTimer = unk362;
	if (wetTimer <= 0)
		return;
	unk362 = wetTimer - 1;

	const TBGCheckData* check;
	f32 posX = getMpositionX();
	f32 posZ = getMpositionZ();
	f32 groundY;
	checkGroundPlane(posX, 320.0f + getMpositionY(), posZ, &groundY, &check);

	u16 bgType = check->mBGType;
	u8 isWater;
	if (bgType == 0x100 || bgType == 0x101
	    || (u16)(bgType - 0x102) <= 3 || bgType == 0x4104)
		isWater = 1;
	else
		isWater = 0;

	if (isWater)
		return;

	if (getMfloorpositionZ() > mPosition.y)
		return;

	u8 actionCheck;
	if (getAction() & 0x200)
		actionCheck = 1;
	else
		actionCheck = 0;
	if (actionCheck)
		return;

	if (unk362 & 7)
		return;

	getUnk158()->mPos.value = mPosition;
	getUnk158()->mPos.value.y += 5.0f;
	(Vec&)getUnk158()->mV.value
	    = (Vec) { mVel.x * 0.3f, getMvelY() * 0.3f, mVel.z * 0.3f };

	gpModelWaterManager->emitRequest(*unk158);
}

void TMario::checkEnforceJump()
{
	u8 groundFlag;
	if (getGroundPlane()->getFlags() & 0x10)
		groundFlag = 1;
	else
		groundFlag = 0;

	u8 notWall;
	if (groundFlag == 1)
		notWall = 0;
	else
		notWall = 1;

	if (!notWall)
		return;

	u8 isTrampoline;
	if (mGroundPlane->getBGType() == 0x7 || mGroundPlane->getBGType() == 0x8007)
		isTrampoline = 1;
	else
		isTrampoline = 0;

	if (!isTrampoline)
		return;

	u8 yCheck;
	if (mPosition.y <= 4.0f + mFloorPosition.y)
		yCheck = 1;
	else
		yCheck = 0;

	if (!yCheck)
		return;

	if (!(getPrevAction() & 0x800))
		return;

	gpMSound->startForceJumpSound((Vec*)&mPosition, getSoundFlags(),
	                              0.0f, (u32)mGroundPlane->getData());
	startVoice(0x78B9);
	changePlayerStatus(0x884, 0, false);
	rumbleStart(0x15, mMotorParams.mMotorWall.get());

	if (mGroundPlane->getActor() != NULL) {
		((THitActor*)mGroundPlane->mActor)->receiveMessage((THitActor*)this, 0);
	}
}

void TMario::checkReturn()
{
	if (isMarioInvincibleInline(this))
		return;

	u8 groundFlag;
	if (mGroundPlane->getFlags() & 0x10)
		groundFlag = 1;
	else
		groundFlag = 0;

	u8 isSafe;
	if (groundFlag == 1)
		isSafe = 0;
	else
		isSafe = 1;

	if (!isSafe)
		return;

	*(JGeometry::TVec3<f32>*)&mLastGroundPos = mPosition;
	unk2B4 = *(u32*)((u8*)this + 0x94);
	unk2B8 = (u16)mFaceAngle.z;
}

BOOL TMario::checkStickRotate(int* outDirection)
{
	int increasing = 0;
	int decreasing = 0;

	volatile int q[4];
	for (int i = 0; i < (int)unk534 - 1; i++) {
		f32 val = (f32)unk530[i];

		if (val < -24576.0f || val > 24576.0f)
			q[0] = 1;
		if (-24576.0f <= val && val <= -8192.0f)
			q[1] = 1;
		if (-8192.0f < val && val < 8192.0f)
			q[2] = 1;
		if (8192.0f <= val && val <= 24576.0f)
			q[3] = 1;

		if (val < (f32)unk530[i + 1])
			increasing++;
		else
			decreasing++;
	}

	int quadrants = 0;
	if (q[0] == 1)
		quadrants++;
	if (q[1] == 1)
		quadrants++;
	if (q[2] == 1)
		quadrants++;
	if (q[3] == 1)
		quadrants++;

	if (quadrants >= 4) {
		if (increasing > decreasing)
			*outDirection = 2;
		else
			*outDirection = 3;
		return true;
	}

	*outDirection = 4;
	return false;
}

f32 TMario::getSlideStopCatch()
{
	const TBGCheckData* plane = mGroundPlane;
	u16 bgType = plane->mBGType;

	if ((u8)isMarioForceSlipInline(this, bgType))
		return mSlipParamsAll.mSlideStopCatch.get();

	u8 isTypeC;
	if (bgType == 0xC || bgType == 0x800C || bgType == 0xA00C)
		isTypeC = 1;
	else
		isTypeC = 0;
	if (isTypeC)
		return mSlipParamsAllSlider.mSlideStopCatch.get();

	u8 isType2;
	if (bgType == 0x2 || bgType == 0x8002)
		isType2 = 1;
	else
		isType2 = 0;
	if (isType2) {
		if (plane->getNormal().y < 0.8660254f)
			return mSlipParams45.mSlideStopCatch.get();
	}

	u8 isType4;
	if (bgType == 0x4 || bgType == 0x4004 || bgType == 0x8004 || bgType == 0xC004)
		isType4 = 1;
	else
		isType4 = 0;
	if (isType4) {
		if (plane->getNormal().y > 0.99f)
			return mSlipParamsWaterGround.mSlideStopCatch.get();
		return mSlipParamsWaterSlope.mSlideStopCatch.get();
	}

	return mSlipParamsNormal.mSlideStopCatch.get();
}

f32 TMario::getSlideStopNormal()
{
	const TBGCheckData* plane = mGroundPlane;
	u16 bgType = plane->mBGType;

	if ((u8)isMarioForceSlipInline(this, bgType))
		return mSlipParamsAll.mSlideStopNormal.get();

	u8 isTypeC;
	if (bgType == 0xC || bgType == 0x800C || bgType == 0xA00C)
		isTypeC = 1;
	else
		isTypeC = 0;
	if (isTypeC)
		return mSlipParamsAllSlider.mSlideStopNormal.get();

	u8 isType2;
	if (bgType == 0x2 || bgType == 0x8002)
		isType2 = 1;
	else
		isType2 = 0;
	if (isType2) {
		if (plane->getNormal().y < 0.8660254f)
			return mSlipParams45.mSlideStopNormal.get();
	}

	u8 isType4;
	if (bgType == 0x4 || bgType == 0x4004 || bgType == 0x8004 || bgType == 0xC004)
		isType4 = 1;
	else
		isType4 = 0;
	if (isType4) {
		if (plane->getNormal().y > 0.99f)
			return mSlipParamsWaterGround.mSlideStopNormal.get();
	}

	return mSlipParamsWaterSlope.mSlideStopNormal.get();
}

BOOL TMario::canSlipJump()
{
	const TBGCheckData* plane = mGroundPlane;
	u16 bgType = plane->mBGType;

	if ((u8)isMarioForceSlipInline(this, bgType))
		return *((u8*)this + 0x2BB8);

	// Type 0xC
	u8 isTypeC;
	if (bgType == 0xC || bgType == 0x800C || bgType == 0xA00C)
		isTypeC = 1;
	else
		isTypeC = 0;
	if (isTypeC)
		return *((u8*)this + 0x2C9C);

	// Type 2
	u8 isType2;
	if (bgType == 0x2 || bgType == 0x8002)
		isType2 = 1;
	else
		isType2 = 0;
	if (isType2)
		return *((u8*)this + 0x2D80);

	// Type 4 with slope check
	u8 isType4;
	if (bgType == 0x4 || bgType == 0x4004 || bgType == 0x8004 || bgType == 0xC004)
		isType4 = 1;
	else
		isType4 = 0;
	if (isType4) {
		if (plane->getNormal().y > 0.99f)
			return *((u8*)this + 0x2F48);
		return *((u8*)this + 0x2E64);
	}

	// Type 3
	u8 isType3;
	if (bgType == 0x3 || bgType == 0x8003)
		isType3 = 1;
	else
		isType3 = 0;
	if (isType3)
		return true;

	return true;
}

BOOL TMario::isSlipStart()
{
	const TBGCheckData* plane = mGroundPlane;
	u16 bgType = plane->mBGType;

	if ((u8)isMarioForceSlipInline(this, bgType))
		return true;

	if (mGroundPlane->isSlider())
		return true;

	// Type 2 (wet surface) with slope check
	u8 isType2;
	if (bgType == 0x2 || bgType == 0x8002)
		isType2 = 1;
	else
		isType2 = 0;
	if (isType2) {
		if (plane->getNormal().y < 0.8660254f)
			return true;
	}

	if (mGroundPlane->isUnk3())
		return false;

	if (mGroundPlane->getNormal().y < mDeParams.mSlipStart.get())
		return true;

	return false;
}

const TBGCheckData* TMario::checkWallPlane(Vec* pos, f32 height, f32 radius)
{
	TBGCheckData* result = 0;
	f32 bestDist = radius;
	TBGWallCheckRecord record(pos->x, pos->y + height, pos->z, radius, 4, 0);

	u8 touched = gpMap->isTouchedWallsAndMoveXZ(&record);
	if (touched == 1) {
		for (int i = 0; i < record.mResultWallsNum; i++) {
			TBGCheckData* wall = record.mResultWalls[i];
			if (wall->mActor == mRidingActor) {
				result = wall;
				break;
			}
			const JGeometry::TVec3<f32>& normal = wall->getNormal();
			f32 dist = normal.x * pos->x + normal.y * pos->y
			           + normal.z * pos->z + wall->getPlaneDistance();
			if (dist < 0.0f)
				dist = -dist;
			if (dist < bestDist) {
				result = wall;
				bestDist = dist;
			}
		}
	}

	pos->x = record.mCenter.x;
	pos->z = record.mCenter.z;
	return result;
}

void TMario::thinkHeight()
{
	f32 heightAboveGround;
	u8 isAirborne;
	if (mAction & 0x800)
		isAirborne = 1;
	else
		isAirborne = 0;

	if (isAirborne) {
		heightAboveGround = mPosition.y - mFloorPosition.y;
		if (unk36C < heightAboveGround)
			unk36C = heightAboveGround;
	} else {
		unk36C = 0.0f;
	}

	JGeometry::TVec3<f32> forwardPos;
	forwardPos.x = getMpositionX() + unk15C * JMASSin(mFaceAngle.y);
	forwardPos.y = mPosition.y;
	forwardPos.z = mPosition.z + unk15C * JMASCos(mFaceAngle.y);

	if (checkWallPlane(&forwardPos, 80.0f, unk15C) == NULL) {
		unk370 = mPosition.y - checkPlayerAround(0, 100.0f);
	} else {
		unk370 = 0.0f;
	}
}

void TMario::checkSink()
{
	if (isMarioInvincibleInline(this))
		return;

	u8 groundBit;
	if (mGroundPlane->mFlags & 0x10)
		groundBit = 1;
	else
		groundBit = 0;
	if (groundBit) return;

	if (100.0f + mFloorPosition.y < mPosition.y) {
		unk368 = 0.0f;
		return;
	}

	if (unk350 == 0) {
		u8 bit6;
		if (mState & 0x40)
			bit6 = 1;
		else
			bit6 = 0;
		if (bit6) {
			unk368 += 1.0f;
			unk360 = mDeParams.mFootPrintTimerMax.get();

			if (mHealth > 0) {
				f32 limit = (f32)mGraffitoParams.mSinkTime.get()
				            * mGraffitoParams.mSinkDmgDepth.get();
				if (unk368 > limit)
					unk368 = limit;
			}

			s16 interval = mGraffitoParams.mSinkDmgTime.get();
			if (gpMarDirector->unk58 % interval == 0) {
				floorDamageExec(1, 3, 0,
				                mMotorParams.mMotorReturn.get());
			}

			if (gpMSound->gateCheck(0x100B)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x100B, (Vec*)&mPosition, 0, (JAISound**)0, 0, 4);
			}

			if (unk368 > (f32)mGraffitoParams.mSinkTime.get()) {
				loserExec();
				changePlayerStatus(0x10001123, 0, false);
			}

			SMS_EmitSinkInPollutionEffect(
			    mPosition,
			    mGroundPlane->getNormal(),
			    true);
			startVoice(0x7865);
			return;
		}
	}

	if (unk350 == 5) {
		u8 bit6;
		if (mState & 0x40)
			bit6 = 1;
		else
			bit6 = 0;
		if (bit6) {
			unk374 -= mJumpParams.mGravity.get();
			unk378 += unk374;
			mVel.set(0.0f, 0.0f, 0.0f);
			mForwardVel = 0.0f;
			mSlideVelX = 0.0f;
			mSlideVelZ = 0.0f;
			loserExec();
			changePlayerStatus(0x10001123, 0, false);
			return;
		}
	}

	unk374 = 0.0f;
	unk378 = 0.0f;
	unk368 = 0.0f;
}

void TMario::getRidingMtx(MtxPtr outMtx)
{
	if (mRidingActor->getRootJointMtx() == NULL) {
		SMS_GetActorMtx(*mRidingActor, outMtx);
	} else {
		PSMTXCopy(*(mRidingActor->getRootJointMtx()), outMtx);
	}
}

void TMario::playerControl(JDrama::TGraphics* gfx)
{
	// Save angle and position history
	unk9C = mFaceAngle.y;
	mLastSafePos = mPosition;
	mSubState &= ~0x8;

	// Scene 1: force status change
	u8 areaID = *(u8*)((u8*)gpMarDirector + 0x124);
	if (areaID == 1) {
		if ((mAction - 0x10000000) != 0x1308) {
			changePlayerStatus(0x10001308, 0, false);
		}
	}

	// Camera angle adjustment for original Mario
	if (gpMarioOriginal == this) {
		if ((u8)gpCamera->isLButtonCameraSpecifyMode(
		        *(int*)((u8*)gpCamera + 0x50))) {
			u32 actionLow = mAction & 0x1FF;
			if (!(actionLow >= 0x14B && actionLow <= 0x14F)) {
				if (*(u8*)((u8*)gpMarDirector + 0x124) != 1) {
					s16 camAngle = *(s16*)((u8*)gpCamera + 0x258);
					s16 offsetY = gpCamera->getOffsetAngleY();
					mFaceAngle.y =
					    (s16)((camAngle + 0x8000) - offsetY);
				}
			}
		}
	}

	// Inlined checkPlayerAction
	mInput = 0;
	checkController(gfx);
	makeHistory();
	checkCurrentPlane();
	checkRideMovement();
	if (!(mInput & 3))
		mInput |= 0x20;

	checkCollision();
	considerTake();

	// Yoshi check
	u8 isOnYoshi = 0;
	if (mYoshi != NULL) {
		if (mYoshi->onYoshi())
			isOnYoshi = 1;
	}
	if (isOnYoshi) {
		if (*(u32*)((u8*)mGamePad + 0xD4) & 0x200000) {
			getOffYoshi(false);
		}
	}

	thinkYoshiHeadCollision();

	// Coaster angle interpolation
	s16 stickValue = *(s16*)unk108;
	f32 rate = mDeParams.mToroccoRotSp.get();
	mToroccoAngle = (s16)((f32)stickValue * rate + (f32)mToroccoAngle);

	stateMachine();
	stateMachineUpper();
	thinkSituation();
	thinkWaterSurface();

	// Sand effect handling
	u8 hasSandFlags;
	if (mState & 0x30000)
		hasSandFlags = 1;
	else
		hasSandFlags = 0;

	if (!hasSandFlags) {
		u16 bgType = mGroundPlane->mBGType;
		u8 isSandGround;
		if (bgType == 0x0701 || bgType == 0x4701 || bgType == 0x8701
		    || bgType == 0xC701)
			isSandGround = 1;
		else
			isSandGround = 0;
		if (isSandGround == 1) {
			mState |= 0x40000;
			emitSandEffect();
		} else {
			mState &= ~0x40000;
		}
	} else {
		mState &= ~0x40000;
	}

	// Inlined thinkHeight
	{
		u8 isAirborne;
		if (mAction & 0x800)
			isAirborne = 1;
		else
			isAirborne = 0;

		if (isAirborne) {
			f32 heightAboveGround = mPosition.y - mFloorPosition.y;
			if (unk36C < heightAboveGround)
				unk36C = heightAboveGround;
		} else {
			unk36C = 0.0f;
		}

		JGeometry::TVec3<f32> forwardPos;
		forwardPos.x = getMpositionX() + unk15C * JMASSin(mFaceAngle.y);
		forwardPos.y = mPosition.y;
		forwardPos.z = mPosition.z + unk15C * JMASCos(mFaceAngle.y);

		if (checkWallPlane(&forwardPos, 80.0f, unk15C) == NULL) {
			const TBGCheckData* groundPlane;
			f32 sinV = JMASSin(mFaceAngle.y);
			f32 cosV = JMASCos(mFaceAngle.y);
			f32 dz = 100.0f * cosV;
			f32 dx = 100.0f * sinV;
			f32 groundHeight = gpMap->checkGround(
			    getMpositionX() + dx,
			    100.0f + mPosition.y,
			    mPosition.z + dz,
			    &groundPlane);
			unk370 = mPosition.y - groundHeight;
		} else {
			unk370 = 0.0f;
		}
	}

	thinkParams();

	// Inlined checkRideReCalc
	if (mRidingActor != NULL) {
		Mtx localMtx;
		if (mRidingActor->getRootJointMtx() == NULL) {
			SMS_GetActorMtx(*mRidingActor, localMtx);
		} else {
			PSMTXCopy(*(mRidingActor->getRootJointMtx()), localMtx);
		}
		PSMTXInverse(localMtx, localMtx);

		mRidePrevLocalPos = mRideLocalPos;

		PSMTXMultVec(localMtx, (Vec*)&mPosition,
		             (Vec*)&mRideLocalPos);
	}

	checkWet();
	checkGraffito();
	thinkDirty();
	checkSink();
	gunExec();

	// Stop sound if not in specific action
	if (mAction != 0x208B8) {
		if (mSound != NULL) {
			mSound->stop(1);
		}
	}
}

void TMario::gunExec()
{
	u8 isOnYoshi = 0;
	if (onYoshi())
		isOnYoshi = 1;

	if (!isOnYoshi)
		gpModelWaterManager->unk5D5F = 0;

	if (!checkFlag(MARIO_FLAG_HAS_FLUDD) && !onYoshi())
		return;

	mWaterGun->updateUnk1C88(0);
	mWaterGun->triggerPressureMovement(*(TMarioControllerWork*)unk108);

	mState &= ~0x80;
	if (mState & (MARIO_FLAG_IN_SHALLOW_WATER | MARIO_FLAG_IN_WATER)) {
		if (!onYoshi() && mWaterGun->suck() == true) {
			mState |= 0x80;
			if (checkFlag(MARIO_FLAG_IN_SHALLOW_WATER)
			    && mGroundPlane->isPool())
				gpPoolManager->subWaterLevel(mGroundPlane);
		}

		if (mWaterGun->mCurrentWater == mWaterGun->getMaxWater()
		    && mPumpState == 0) {
			mWaterGun->emit();
			mWaterGun->resetWaterToFull();
		}
	} else if (mPumpState == 0) {
		mWaterGun->emit();
	}

	if (onYoshi() && ((TMarioControllerWork*)unk108)->mAnalogR > 0.0f)
		mWaterGun->emit();

	if (mSubState & 0x80)
		mWaterGun->resetWaterToFull();

	if (mAction != 0x883
	    && mAction != 0x208B8
	    && mGamePad->checkFrameMeaning(0x200000) && !onYoshi()
	    && mAction != 0x800447)
		mWaterGun->changeBackup();

	if ((int)mWaterGun->mCurrentNozzle == TWaterGun::Spray
	    && mWaterGun->mIsEmitWater != 0) {
		JGeometry::TVec3<f32> dir;
		dir.x = JMASSin(mFaceAngle.y);
		dir.y = 0.0f;
		dir.z = JMASCos(mFaceAngle.y);

		for (int i = 0; i < mGraffitoParams.mFootEraseTimes.get(); ++i) {
			f32 radius = mGraffitoParams.mFootEraseSize.get();
			JGeometry::TVec3<f32> pos
			    = mPosition + dir * mGraffitoParams.mFootEraseFront.get();
			gpPollution->clean(pos.x, pos.y, pos.z, radius);
		}
	}
}
