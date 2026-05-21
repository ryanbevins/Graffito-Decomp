#include <NPC/NpcBase.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcSave.hpp>
#include <System/MarDirector.hpp>
#include <Strategic/Spine.hpp>
#include <Camera/cameralib.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <M3DUtil/LodAnm.hpp>

// NPC TU uses -inline deferred; define functions in REVERSE map order.

void TBaseNPC::execWalk(bool param_1)
{
	if (unk1E2 != 0)
		goto bail;

	{
		bool talking = gpMarDirector->isTalkModeNow();
		bool blocked = talking || gpMarDirector->unk124 == 4;
		if (blocked)
			goto bail;
	}

	if (unk178 != 0.0f)
		goto bail;
	if ((mActionFlag & 0x200) != 0)
		goto bail;

	if (param_1 && (unk1DA & 1)) {
		f32 turnRate = (mActionFlag & 0x8) ? 6.0f : 4.0f;
		SMS_GoRotate(mPosition, unkF4.getPoint(), turnRate, &mRotation.y);

		JGeometry::TVec3<f32> diff = unkF4.getPoint();
		diff.x -= mPosition.x;
		diff.y -= mPosition.y;
		diff.z -= mPosition.z;
		JGeometry::TVec3<f32> tmp1 = diff;
		JGeometry::TVec3<f32> tmp2 = tmp1;
		f32 targetYaw = MsGetRotFromZaxisY(tmp2);

		f32 raw = mRotation.y - targetYaw;
		f32 delta = raw < 0.0f ? -raw : raw;
		while (delta >= 360.0f)
			delta -= 360.0f;
		while (delta < 0.0f)
			delta += 360.0f;
		if (delta < 0.001f) {
			unk1DA = (unk1DA & ~1);
		}
	} else {
		if (param_1) {
			TLodAnm* lod = unkD0;
			f32 target  = mNpcSaveIndividual->mSLMinMarchSpeed.get();
			f32 accel   = mNpcSaveIndividual->mMarchAccel.get();
			int kind    = lod->getCurrentAnmKind();

			if (kind == 8) {
				target = mNpcSaveIndividual->mSLMaxRunSpeed.get();
				accel  = mNpcSaveIndividual->mSLRunAccel.get();
				if (mActionFlag & 0x4000) {
					f32 scale = TBaseNPC::mPtrSaveNormal->mSLSmokeRunMagnif.get();
					target *= scale;
					accel *= scale;
				}
			} else if (kind == 0) {
				target = mNpcSaveIndividual->mMaxMarchSpeed.get();
			}
			CLBChaseGeneralConstantSpecifySpeed<f32>(&mMarchSpeed, target, accel);
		} else {
			CLBChaseGeneralConstantSpecifySpeed<f32>(
			    &mMarchSpeed, 0.0f, mNpcSaveIndividual->mMarchDecrease.get());
		}

		if (mMarchSpeed < 0.001f)
			mTurnSpeed = mNpcSaveIndividual->mWaitTurnSpeed.get();
		else
			mTurnSpeed = mNpcSaveIndividual->mWalkTurnSpeed.get();

		JGeometry::TVec3<f32> point = unkF4.getPoint();
		JGeometry::TVec3<f32> diff;
		diff.set(point.x - mPosition.x, 0.0f, point.z - mPosition.z);
		if (diff.squared() < CLBSquared<f32>(10.0f)) {
			walkToCurPathNode(mMarchSpeed, mTurnSpeed, 0.0f);
		}
	}
	return;

bail:
	mMarchSpeed = 0.0f;
	mTurnSpeed  = 0.0f;
}

BOOL TBaseNPC::execUTurn()
{
	JGeometry::TVec3<f32> diff = unkF4.getPoint();
	diff.x -= mPosition.x;
	diff.y -= mPosition.y;
	diff.z -= mPosition.z;
	f32 targetYaw = MsGetRotFromZaxis(diff).y;

	if (mRotation.y == targetYaw)
		return TRUE;

	if (unk178 != 0.0f || (mActionFlag & 0x200) != 0)
		return FALSE;

	BOOL result    = FALSE;
	f32 wrappedTgt = targetYaw;
	while (wrappedTgt >= 360.0f)
		wrappedTgt -= 360.0f;
	while (wrappedTgt < 0.0f)
		wrappedTgt += 360.0f;

	f32 cur = mRotation.y;
	while (cur >= 360.0f)
		cur -= 360.0f;
	while (cur < 0.0f)
		cur += 360.0f;
	mRotation.y = cur;

	if (mLiveFlag & 0x200000) {
		f32 y = mRotation.y;
		if (y > wrappedTgt)
			mRotation.y = y - 360.0f;
	} else {
		f32 y = mRotation.y;
		if (y < wrappedTgt)
			mRotation.y = y + 360.0f;
	}

	BOOL stillChasing = CLBChaseGeneralConstantSpecifySpeed<f32>(
	    &mRotation.y, wrappedTgt, mNpcSaveIndividual->mUTurnSpeed.get());

	f32 wrap = mRotation.y;
	while (wrap >= 360.0f)
		wrap -= 360.0f;
	while (wrap < 0.0f)
		wrap += 360.0f;
	mRotation.y = wrap;

	if (!stillChasing)
		result = TRUE;
	return result;
}

BOOL TBaseNPC::execTurnToFirstState()
{
	if (mRotation.y == unk1A4)
		return TRUE;

	BOOL result = FALSE;
	s16 cur = CLBRoundf<s16>(mRotation.y * (65536.0f / 360.0f));
	s16 tgt = CLBRoundf<s16>(unk1A4 * (65536.0f / 360.0f));
	s16 spd = CLBRoundf<s16>(mNpcSaveIndividual->mFirstStateTurnSpeed.get()
	                         * (65536.0f / 360.0f));

	if (!CLBChaseGeneralConstantSpecifySpeed<s16>(&cur, tgt, spd)) {
		result      = TRUE;
		mRotation.y = unk1A4;
	} else {
		mRotation.y = (f32)cur * (360.0f / 65536.0f);
	}
	return result;
}

BOOL TBaseNPC::isNeedTurnToFirstState() const
{
	if (unk178 != 0.0f || (mActionFlag & 0x200) != 0)
		return FALSE;

	BOOL result = FALSE;
	s32 t       = mActorType;

	switch (t) {
	case 0x4000008:
	case 0x400001C:
	case 0x400001D:
		goto end;
	}

	{
		const TNerveBase<TLiveActor>* nerve = mSpine->getLatestNerve();
		if (nerve != &TNerveNPCWaitMarioApproach::theNerve()
		    && nerve != &TNerveNPCTurnToMario::theNerve())
			goto end;

		if (mActorType == 0x4000006 || (mActionFlag & 0xC01) == 0)
			result = TRUE;
	}
end:
	return result;
}

BOOL TBaseNPC::isTurnToMarioWhenTalk() const
{
	s32 t        = mActorType;
	BOOL allowed = TRUE;

	switch (t) {
	case 0x4000007:
	case 0x4000008:
	case 0x400001A:
	case 0x400001B:
	case 0x400001D:
		allowed = FALSE;
		break;
	default:
		if ((mActionFlag & 0xC01) != 0) {
			allowed = FALSE;
		}
		break;
	}
	return allowed;
}

BOOL TBaseNPC::isTurnToMarioWhenApproach() const
{
	if (unk178 != 0.0f || (mActionFlag & 0x200) != 0)
		return FALSE;

	s32 t       = mActorType;
	BOOL result = TRUE;
	switch (t) {
	case 0x4000016:
	case 0x4000017:
	case 0x4000018:
		if ((mActionFlag & 0x7E7F) != 0)
			result = FALSE;
		break;
	default:
		result = FALSE;
		break;
	}
	return result;
}
