// NPC/NpcAnm.cpp -- TBaseNPC animation dispatch.

#include <NPC/NpcBase.hpp>
#include <NPC/NpcCoin.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcParts.hpp>
#include <NPC/NpcSave.hpp>
#include <NPC/NpcThrow.hpp>

#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>
#include <M3DUtil/LodAnm.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Enemy/Graph.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/LiveActor.hpp>
#include <Strategic/Spine.hpp>
#include <System/MarDirector.hpp>
#include <stdlib.h>

const char* cNpcPartsNameRootJoint = "__ROOT_JOINT__";

// Local view of the inbetween/blend control block stored at TBaseNPC.mUnk18C
// (matches TUnk18CStruct in NpcInitPrg.cpp).
struct TUnk18CStruct {
	/* 0x00 */ s32 unk0;
	/* 0x04 */ s32 unk4;
	/* 0x08 */ s32 unk8;
	/* 0x0C */ f32 unkC;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
	/* 0x1C */ f32 unk1C;
	/* 0x20 */ f32 unk20;
	/* 0x24 */ s32 unk24;
	/* 0x28 */ f32 unk28;
};

// Double-precision Newton sqrt used by the sink-recovery jump computation;
// matches the inlined frsqrte sequence with double 0.5/3.0 constants.
static inline f32 recoverSqrt(f32 mag)
{
	if (mag > 0.0f) {
		double root = __frsqrte(mag);
		volatile f32 result
		    = 0.5 * root * (3.0 - mag * (root * root)) * mag;
		return result;
	}
	return mag;
}

// Forces MWCC to materialize the "forbid animation" predicate as an explicit
// 0/1 byte (matching the li/clrlwi pattern in requestNpcAnm_) rather than
// folding it directly into a branch.
static inline u8 isForbidAnm(const TUnk18CStruct* ib)
{
	u8 result;
	if (ib->unk24 > 0)
		result = 1;
	else
		result = 0;
	return result;
}

// `CLBChaseConstantSpecifyFrame` is only declared (not defined) in
// cameralib.hpp; provide the body so this TU owns the weak instantiation.
template <class T> void CLBChaseConstantSpecifyFrame(T* slot, T target, T frame)
{
	if (frame < 0.001f) {
		*slot = target;
		return;
	}
	*slot = (target - *slot) * (1.0f / frame) + *slot;
}

// `EnumNpcAnmKind` and `EnumNpcStopMotionBlendOnOff` are forward-declared in
// NpcBase.hpp as empty enums; cast through `int` to ease working with literals.
static inline EnumNpcAnmKind asKind(int v)
{
	return (EnumNpcAnmKind)v;
}
static inline EnumNpcStopMotionBlendOnOff asBlend(int v)
{
	return (EnumNpcStopMotionBlendOnOff)v;
}

// ============================================================
// Definition order is *reverse* of binary layout because this
// TU is compiled with -inline deferred.
// ============================================================

void TBaseNPC::setNpcAnm_(EnumNpcAnmKind kind, EnumNpcStopMotionBlendOnOff blend)
{
	if ((s32)mActorType < (s32)0x0400001E && (s32)mActorType >= (s32)0x0400001C)
		return;

	mAnmRequest->mKind = -1;
	if ((int)kind == unkD0->mCurrentAnmKind)
		return;

	if (!unkD0->setBckAndBtpAnm((int)kind))
		return;

	if ((int)kind == 0x11) {
		switch (unk1D9) {
		case 0:
		case 1:
		case 2:
			emitHappyEffect_();
			if (gpMSound->gateCheck(0x8808)) {
				MSoundSESystem::MSoundSE::startSoundNpcActor(
				    0x8808,
				    (const Vec*)((u8*)this + 0x10),
				    0, (JAISound**)NULL, 0, 4);
			}
			break;
		}
	}

	if (isSunflower()) {
		J3DFrameCtrl* fc = mMActor->getFrameCtrl(MActor::ANM_TYPE_BRK);
		if (unk1D8 & 0x1) {
			if ((int)kind == 0x5) {
				mMActor->setBrkFromIndex(mActorType == 0x0400001A ? 1 : 1);
				*(u8*)((u8*)fc + 4) = 0;
			} else {
				mMActor->setBrkFromIndex(mActorType == 0x0400001A ? 0 : 0);
				*(f32*)((u8*)fc + 0xC) = 0.0f;
			}
		} else if (unk1D8 & 0x2) {
			if ((int)kind == 0x1A) {
				mMActor->setBrkFromIndex(mActorType == 0x0400001A ? 0 : 0);
				*(u8*)((u8*)fc + 4) = 0;
			}
		}
	}

	if (mNpcParts && isPartsAnmNpc()) {
		switch (mActorType) {
		case 0x04000018: {
			bool flag = checkUnk1D8(0x1);
			if (MActor* parts = mNpcParts->getPartsMActor(0, 0)) {
				int bck;
				switch (kind) {
				case NPC_ANM_KIND_UNK5:
					if (flag)
						bck = 4;
					else
						bck = 6;
					break;
				default:
					if (flag)
						bck = 3;
					else
						bck = 5;
					break;
				}
				if (!parts->checkCurBckFromIndex(bck))
					parts->setBckFromIndex(bck);
			}
			if (MActor* parts3 = mNpcParts->getPartsMActor(3, 0)) {
				int bck;
				switch (kind) {
				case NPC_ANM_KIND_TAKEN:
					bck = 7;
					break;
				case NPC_ANM_KIND_UNK10:
					bck = 0xE;
					break;
				case NPC_ANM_KIND_UNK1A:
					bck = 0xD;
					break;
				case NPC_ANM_KIND_UNK5:
				default:
					if (flag)
						bck = 0xB;
					else if (checkUnk1D8(0x4))
						bck = 9;
					else
						bck = 8;
					break;
				}
				if (!parts3->checkCurBckFromIndex(bck))
					parts3->setBckFromIndex(bck);
			}
			if (MActor* parts4 = mNpcParts->getPartsMActor(4, 0)) {
				int bck;
				switch (kind) {
				case NPC_ANM_KIND_UNK5:
					bck = -1;
					break;
				default:
					bck = 0x11;
					break;
				}
				if (!parts4->checkCurBckFromIndex(bck))
					parts4->setBckFromIndex(bck);
			}
			break;
		}
		case 0x04000010: {
			int bck = 1;
			switch (kind) {
			case NPC_ANM_KIND_UNK6:
				bck = 2;
				break;
			case NPC_ANM_KIND_UNK5:
				bck = 3;
				break;
			}
			MActor* parts = mNpcParts->getPartsMActor(9, 0);
			if (parts) {
				if (!parts->checkCurBckFromIndex(bck))
					parts->setBckFromIndex(bck);
			}
			break;
		}
		case 0x04000015: {
			int bck = 0;
			int btp = 0;
			switch (kind) {
			case NPC_ANM_KIND_UNK5:
				bck = 2;
				btp = 1;
				break;
			case NPC_ANM_KIND_UNK6:
				bck = 1;
				break;
			}
			MActor* parts = mNpcParts->getPartsMActor(0xA, 0);
			if (parts) {
				if (!parts->checkCurBckFromIndex(bck))
					parts->setBckFromIndex(bck);
				parts->setBtpFromIndex(btp);
			}
			break;
		}
		}
	}

	if ((int)blend == 1) {
		((TUnk18CStruct*)mUnk18C)->unk24 = ((TUnk18CStruct*)mUnk18C)->unk4;
	} else {
		((TUnk18CStruct*)mUnk18C)->unk24 = 0;
	}
	setCurAnmSound();
}

void TBaseNPC::requestNpcAnm_(EnumNpcAnmKind kind,
                              EnumNpcStopMotionBlendOnOff blend)
{
	if ((s32)mActorType < (s32)0x0400001E && (s32)mActorType >= (s32)0x0400001C)
		return;
	if (isForbidAnm((TUnk18CStruct*)mUnk18C)) {
		if ((int)kind == unkD0->mCurrentAnmKind) {
			mAnmRequest->mKind = -1;
		} else {
			TNpcAnmRequest* req = mAnmRequest;
			req->mKind          = (int)kind;
			req->mBlend         = blend != NPC_STOP_MOTION_BLEND_OFF;
		}
	} else {
		setNpcAnm_(kind, blend);
	}
}

void TBaseNPC::setKeepAnm_()
{
	TNpcAnmRequest* req = mAnmRequest;
	EnumNpcAnmKind prevKind
	    = (EnumNpcAnmKind)req->mKind;
	EnumNpcStopMotionBlendOnOff prevBlend
	    = (EnumNpcStopMotionBlendOnOff)req->mBlend;
	req->mKind = -1;
	if ((int)prevKind != -1) {
		if ((int)prevKind == unkD0->mCurrentAnmKind) {
			mAnmRequest->mKind = -1;
		} else {
			setNpcAnm_(prevKind, prevBlend);
		}
	}
}

void TBaseNPC::requestTalkAnm_()
{
	int kind;
	if (mActionFlag & 0x400) {
		kind = 1;
	} else if ((mActionFlag & 0x1) && !(mActionFlag & 0x4)) {
		kind = 0x13;
	} else {
		kind = 6;
	}
	requestNpcAnm_(asKind(kind), NPC_STOP_MOTION_BLEND_ON);
}

void TBaseNPC::randomizeBckAndBtpFrame_()
{
	s16 endFrame      = 0;
	s16 randStart     = 0;
	s16 bckV          = 0;
	J3DFrameCtrl* bck = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
	if (bck) {
		endFrame  = *(s16*)((u8*)bck + 0x8);
		randStart = endFrame;
		f32 r     = (f32)rand() * (1.0f / 32768.0f);
		bckV      = (s16)((f32)endFrame * r);
		*(f32*)((u8*)bck + 0x10) = (f32)bckV;
	}
	J3DFrameCtrl* btp = mMActor->getFrameCtrl(MActor::ANM_TYPE_BTP);
	if (btp) {
		s16 ef = *(s16*)((u8*)btp + 0x8);
		s16 v;
		if (ef == randStart) {
			v = bckV;
		} else {
			f32 r = (f32)rand() * (1.0f / 32768.0f);
			v     = (s16)((f32)ef * r);
		}
		*(f32*)((u8*)btp + 0x10) = (f32)v;
	}
}

void TBaseNPC::walkAnmRateChange_()
{
	f32 speedSq = mLinearVelocity.x * mLinearVelocity.x
	            + mLinearVelocity.z * mLinearVelocity.z;
	f32 speed   = recoverSqrt(speedSq);
	if (speed < 0.001f) {
		int anmKind = unkD0->mCurrentAnmKind;
		switch (anmKind) {
		case 8:
		case 0: {
			int frame
			    = CLBPalFrame<long>(mPtrSaveNormal->mStopWalkAnmRateFrame.get());
			BOOL chasing = CLBChaseDecrease(
			    &unk1D0, 0.0f, mPtrSaveNormal->mStopWalkAnmRateChase.get(),
			    0.001f);
			if (unk1CC < frame)
				unk1CC++;
			if (unk1CC >= frame || !chasing) {
				unk1CC = 0;
				unk1D0 = 0.0f;
				TUnk18CStruct* ib = (TUnk18CStruct*)mUnk18C;
				bool inBlend      = ib->unk24 > 0;
				bool forcedBlend  = false;
				if (!inBlend && ib->unk28 > 0.0f)
					forcedBlend = true;
				if (!(inBlend || forcedBlend))
					npcWaitIn();
				else if (!(ib->unk24 > 0))
					mMActor->setFrameRate(unk1D0, 0);
			} else {
				mMActor->setFrameRate(unk1D0, 0);
			}
			break;
		}
		default:
			npcWaitIn();
			break;
		}
	} else {
		unk1CC      = 0;
		int anmKind = unkD0->mCurrentAnmKind;
		switch (anmKind) {
		case NPC_ANM_KIND_RUN:
		case NPC_ANM_KIND_WALK: {
			f32 individualRate
			    = mNpcSaveIndividual->mSLMinWalkAnmRate.get()
			    * SMSGetAnmFrameRate();
			f32 baseRate;
			f32 maxRate;
			if (unkD0->mCurrentAnmKind == 8) {
				baseRate = mNpcSaveIndividual->mSLMaxRunAnmRate.get()
				         * SMSGetAnmFrameRate();
				maxRate  = mNpcSaveIndividual->mSLMaxRunSpeed.get();
				if (mActionFlag & 0x4000) {
					f32 mul  = mPtrSaveNormal->mSLSmokeRunMagnif.get();
					baseRate = baseRate * mul;
					maxRate  = maxRate * mul;
				}
			} else {
				baseRate = mNpcSaveIndividual->mSLMaxWalkAnmRate.get()
				         * SMSGetAnmFrameRate();
				maxRate  = mNpcSaveIndividual->mMaxMarchSpeed.get();
			}
			if (speed > maxRate)
				speed = maxRate;
			f32 ratio = MsClamp(
			    CLBCalcRatio<f32>(mNpcSaveIndividual->mSLMinMarchSpeed.get(),
			                      maxRate, speed),
			    0.0f, 1.0f);
			f32 newRate = CLBLinearInbetween<f32>(individualRate, baseRate, ratio);
			if (mColCount != 0)
				newRate = baseRate;
			CLBChaseDecrease(&unk1D0, newRate,
			    mPtrSaveNormal->mMoveWalkAnmRateChase.get(), 0.0f);
			mMActor->setFrameRate(unk1D0, 0);
			break;
		}
		default:
			unk1D0 = 0.0f;
			if (mActionFlag & 0x8) {
				requestNpcAnm_(NPC_ANM_KIND_RUN, NPC_STOP_MOTION_BLEND_ON);
			} else {
				requestNpcAnm_(NPC_ANM_KIND_WALK, NPC_STOP_MOTION_BLEND_ON);
			}
			break;
		}
	}
}

EnumNpcAnmKind TBaseNPC::getNpcWaitAnmBase_()
{
	EnumNpcAnmKind result = asKind(1);
	if (unk1E2 == 0) {
		u32 flag = mActionFlag;
		if (flag & 0x2) {
			result = asKind(0xC);
		} else if (flag & 0x10) {
			result = asKind(0x15);
		} else if (flag & 0x20) {
			result = asKind(6);
		} else if (flag & 0x40) {
			result = asKind(0x17);
		} else if (flag & 0x4) {
			result = asKind(0x16);
		}
	}
	return result;
}

void TBaseNPC::npcWaitIn()
{
	EnumNpcAnmKind kind = asKind(1);
	u32 flag = mActionFlag;
	if (!(flag & 0x400)) {
		if (unk178 != 0.0f) {
			kind = asKind(0xF);
		} else if (flag & 0x200) {
			kind = asKind(0x11);
		} else if ((flag & 0x1) && !(flag & 0x4)) {
			if (flag & 0x20)
				kind = asKind(0x13);
			else
				kind = asKind(0x12);
		} else if (!unk124->getGraph()->isDummy()) {
			if (mSpine->getLatestNerve() == &TNerveNPCGraphWait::theNerve()) {
				if (!gpMarDirector->isThing())
					kind = getNpcWaitAnmBase_();
			}
		} else {
			kind = getNpcWaitAnmBase_();
		}
	}
	requestNpcAnm_(kind, NPC_STOP_MOTION_BLEND_ON);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcFallIn()
{
	requestNpcAnm_((EnumNpcAnmKind)0x2,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

bool TBaseNPC::npcRecoverFromSinking()
{
	if (!(mLiveFlag & 0x08000000)) {
		J3DFrameCtrl* fc = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		if (fc->checkPass(32.0f)) {
			mLiveFlag |= 0x08000000;
			f32 grav = getGravityY();
			f32 init = 0.0f;
			f32 v    = mSinkBaseY - mPosition.y + 150.0f;
			if (grav > 0.0f) {
				init = 0.5f * grav
				       * (recoverSqrt(v * (1.0f / grav) * 8.0f + 1.0f)
				           + 1.0f);
			}
			mVelocity.y = init;
			if (mVelocity.y < 5.0f)
				mVelocity.y = 5.0f;
		}
	}
	bool changed = false;
	if (mMActor->isCurAnmAlreadyEnd(0)) {
		mLiveFlag &= ~0x80;
		changed     = true;
		mVelocity.x = 0.0f;
		mVelocity.y = 0.0f;
		mVelocity.z = 0.0f;
		mPosition.y = mSinkBaseY;
		mLiveFlag &= ~0x08400010;
	} else if (mLiveFlag & 0x08000000) {
		mVelocity.y -= getGravityY();
		if (mVelocity.y < TLiveActor::mVelocityMinY)
			mVelocity.y = TLiveActor::mVelocityMinY;
		mPosition.y += mVelocity.y;
		if (mVelocity.y <= 0.0f && mPosition.y < mSinkBaseY)
			mPosition.y = mSinkBaseY;
	}
	return changed;
}

void TBaseNPC::npcRecoverAfterIn()
{
	requestNpcAnm_((EnumNpcAnmKind)0x3,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcStepIn()
{
	requestNpcAnm_((EnumNpcAnmKind)0x4,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcTalkIn()
{
	mLiveFlag |= 0x00080000;
	if (mActorType != 0x0400001C && mActorType != 0x0400001D) {
		if (!isSunflowerReviving()) {
			if (isPeachTired()) {
				requestNpcAnm_((EnumNpcAnmKind)0x1A,
				               (EnumNpcStopMotionBlendOnOff)1);
			} else {
				requestTalkAnm_();
			}
		}
	}
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcTalking()
{
	if (isSunflowerReviving()) {
		sunflowerReviving();
		return;
	}
	if (isTurnToMarioWhenTalk()) {
		f32 turnSpeed = mTurnSpeed;
		SMS_GoRotate(mPosition, *gpMarioPos, turnSpeed,
		             &((JGeometry::TVec3<f32>*)((u8*)this + 0x30))->y);
		if (!unk124->getGraph()->isDummy())
			unk1DA |= 0x1;
	}
	if (isPeachTired() && unkD0->mCurrentAnmKind == 0x1A
	    && mMActor->isCurAnmAlreadyEnd(0)) {
		peachTiredOut_();
	}
}

void TBaseNPC::npcTalkOut()
{
	unk1E0 = 0x3C;

	if (checkLiveFlag(0x00080000)) {
		unk1E2 = 0x78;

		bool peach = false;
		if (mActorType == 0x04000018 && (unk1D8 & 0x2))
			peach = true;
		if (peach)
			peachTiredOut_();

		if (checkActionFlag(0x200)) {
			offActionFlag(0x200);
			if (mNpcCoin != 0) {
				mNpcCoin->requestAppearCoin(getCursorPos(), mRotation.y, 0x28);
				unk1E0 = 0x168;
				unk1E2 = 0x168;
			}
		}

		mLiveFlag &= ~0x00080000;
		changeNerveFromTalk_();
		if (unk17C == 0 && mActorType == 0x04000006)
			requestNpcAnm_((EnumNpcAnmKind)0x4,
			               (EnumNpcStopMotionBlendOnOff)1);
	}
}

void TBaseNPC::npcTakenIn()
{
	requestNpcAnm_((EnumNpcAnmKind)0x9,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcDanceIn()
{
	mActionFlag |= 0x4;
	requestNpcAnm_((EnumNpcAnmKind)0x16,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcHappyIn(unsigned char arg)
{
	unk1D9        = arg;
	mActionFlag  |= 0x200;
	requestNpcAnm_((EnumNpcAnmKind)0x11,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcWetIn()
{
	if (!isSunflowerReviving()) {
		EnumNpcAnmKind anm = asKind(5);
		EnumNpcStopMotionBlendOnOff blend = NPC_STOP_MOTION_BLEND_ON;
		if (unk178 != 0.0f) {
			anm = asKind(0x19);
		} else if (mActionFlag & 0x1) {
			anm = asKind(0x14);
			bool isMonte = isNormalMonteM() || isNormalMonteW();
			if (isMonte)
				blend = NPC_STOP_MOTION_BLEND_OFF;
		} else {
			bool isMonte = isNormalMonteM() || isNormalMonteW();
			if (isMonte || mActorType == 0x0400000D) {
				if (!(mLiveFlag & 0x04000000) && MsRandF() < 0.5f)
					anm = asKind(0xB);
				else
					blend = NPC_STOP_MOTION_BLEND_OFF;
			} else {
				bool isMare = isNormalMareM() || isNormalMareW();
				if (isMare || mActorType == 0x04000011) {
					if (MsRandF() < 0.5f)
						anm = asKind(0xB);
				} else if (!((s32)mActorType < (s32)0x04000018
				             && (s32)mActorType >= (s32)0x04000016)) {
					(void)mActorType;
				} else if (!(mLiveFlag & 0x04000000)) {
					if (MsRandF() < 0.5f)
						anm = asKind(0xB);
				}
			}
		}
		requestNpcAnm_(anm, blend);
	}
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
	mAnmFrameCounter->mCurFrame = 0;
}

inline void TBaseNPC::npcWetOut()
{
	mLiveFlag &= ~0x02000000;
	mLiveFlag &= ~0x04000000;
	if (mSpine->peekTopNerveOrNull() == &TNerveNPCTalk::theNerve()) {
		requestTalkAnm_();
	} else if (mActorType == 0x04000006) {
		requestNpcAnm_(asKind(0x4), NPC_STOP_MOTION_BLEND_ON);
	}
}

bool TBaseNPC::npcWetting()
{
	bool ret = false;
	if (checkUnk1DA(0x2)) {
		offUnk1DA(0x2);
		npcHappyIn(0);
		npcWetOut();
		ret = true;
	} else if (unkD0->mCurrentAnmKind == 0x19) {
		if (unk178 == 0.0f) {
			npcHappyIn(0);
			npcWetOut();
			ret = true;
		} else if (mMActor->isCurAnmAlreadyEnd(0)) {
			npcWetOut();
			ret = true;
		}
	} else {
		bool isMare = isNormalMareM() || isNormalMareW();
		if (isMare || mActorType == 0x04000011) {
			switch (unkD0->mCurrentAnmKind) {
			case 5:
			case 0xB:
			case 0x14:
				if (mMActor->isCurAnmAlreadyEnd(0)) {
					mAnmFrameCounter->doThing3(240, 480);
					if (mAnmFrameCounter->doThing2()) {
						EnumNpcAnmKind kind = asKind(7);
						if (MsRandF() < 0.5f)
							kind = asKind(0x1B);
						requestNpcAnm_(kind, NPC_STOP_MOTION_BLEND_OFF);
					}
				}
				break;
			case 7:
			case 0x1B:
				if (mMActor->isCurAnmAlreadyEnd(0)) {
					npcWetOut();
					ret = true;
				}
				break;
			}
		} else {
			if (isMonte() && isMadNpc() && mActorType != 0x04000006) {
				switch (unkD0->mCurrentAnmKind) {
				case 5:
				case 0x14:
					if (mMActor->isCurAnmAlreadyEnd(0)) {
						if (isStateGoToMad_()) {
							npcMadIn();
						} else {
							npcWetOut();
							ret = true;
						}
					}
					break;
				case 4:
					if (mLiveFlag & 0x02000000)
						npcMadding();
					break;
				case 0xA:
				case 0xB:
					if (mLiveFlag & 0x02000000) {
						if (npcMadding()) {
							npcWetOut();
							ret = true;
						}
					} else {
						if (mMActor->isCurAnmAlreadyEnd(0)) {
							npcWetOut();
							ret = true;
						}
					}
					break;
				}
			} else {
				if (isSunflowerReviving()) {
					if (sunflowerReviving()) {
						npcWetOut();
						ret = true;
					}
				} else {
					switch (mActorType) {
					case 0x04000016:
					case 0x04000017:
						if (mMActor->isCurAnmAlreadyEnd(0)) {
							switch (unkD0->mCurrentAnmKind) {
							case 5:
								requestNpcAnm_(NPC_ANM_KIND_UNK7,
								               NPC_STOP_MOTION_BLEND_OFF);
								break;
							case 0xB:
								requestNpcAnm_(NPC_ANM_KIND_MAD,
								               NPC_STOP_MOTION_BLEND_ON);
								break;
							case 0x14:
								requestNpcAnm_(NPC_ANM_KIND_UNK18,
								               NPC_STOP_MOTION_BLEND_ON);
								break;
							case 7:
							case 0x10:
							case 0x18:
								npcWetOut();
								ret = true;
								break;
							}
						}
						break;
					default:
						if (mMActor->isCurAnmAlreadyEnd(0)) {
							switch (unkD0->mCurrentAnmKind) {
							case 5:
							case 0xB:
							case 0x14:
							case 0x19:
								npcWetOut();
								ret = true;
								break;
							}
						}
						break;
					}
				}
			}
		}
	}
	return ret;
}

void TBaseNPC::npcSinking()
{
	f32 targetY = mSinkBaseY - mNpcSaveIndividual->mSinkHeight.get();
	if (mPosition.y != targetY) {
		f32 sinkSpeed = mNpcSaveIndividual->mSinkSpeed.get();
		if (isPollutionNpc()) {
			CLBChaseConstantSpecifyFrame(
			    &unk178, 1.0f, (1.0f / sinkSpeed) * (mPosition.y - targetY));
		}
		if (!CLBChaseGeneralConstantSpecifySpeed(&mPosition.y, targetY,
		                                         sinkSpeed)) {
			onLiveFlag(LIVE_FLAG_SINK_BOTTOM);
			onHitFlag(HIT_FLAG_NO_COLLISION);
			requestNpcAnm_(NPC_ANM_KIND_UNK10, NPC_STOP_MOTION_BLEND_ON);
		}
	}
}

void TBaseNPC::npcThrowIn()
{
	requestNpcAnm_((EnumNpcAnmKind)0xD,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

bool TBaseNPC::npcThrowing()
{
	bool done    = false;
	int startF   = mPtrSaveNormal->mSLThrowStartFrame.get();
	int spineCtr = mSpine->getTime();
	if ((spineCtr == 0 && startF < 0x14) || spineCtr == startF - 0x14) {
		unk64 |= 0x1;
		unk1DC = CLBPalFrame<long>(0x1E);
	} else if (spineCtr == startF) {
		((TNpcThrow*)unk17C)->throwMario((THitActor*)this);
	} else if (mMActor->isCurAnmAlreadyEnd(0)) {
		done = true;
	}
	return done;
}

void TBaseNPC::npcMadIn()
{
	mLiveFlag |= 0x02000000;
	if (mActorType == 0x04000007 || (mActionFlag & 0x1)) {
		requestNpcAnm_((EnumNpcAnmKind)0xA,
		               (EnumNpcStopMotionBlendOnOff)1);
	} else {
		npcStepIn();
	}
}

bool TBaseNPC::npcMadding()
{
	bool ret = false;
	if (!(mLiveFlag & 0x02000000)) {
		ret = true;
	} else {
		switch (unkD0->mCurrentAnmKind) {
		case 4:
			SMS_GoRotate(mPosition, *gpMarioPos,
			             *(f32*)((u8*)mNpcSaveIndividual + 0x2ac), &mRotation.y);
			JGeometry::TVec3<f32> axis = *gpMarioPos;
			axis -= mPosition;
			JGeometry::TVec3<f32> copy  = axis;
			JGeometry::TVec3<f32> copy2 = copy;
			JGeometry::TVec3<f32> copy3;
			copy3.set(copy2);
			f32 delta = MsWrap(abs(mRotation.y - MsGetRotFromZaxisY(copy3)), 0.0f,
			                   360.0f);
			if (delta < 0.001f)
				requestNpcAnm_(NPC_ANM_KIND_MAD, NPC_STOP_MOTION_BLEND_OFF);
			if (!unk124->getGraph()->isDummy())
				unk1DA |= 0x1;
			break;

		case 0xA:
		case 0xB:
			if (mMActor->isCurAnmAlreadyEnd(0)) {
				ret        = true;
				mLiveFlag &= ~0x02000000;
			}
			break;
		}
	}
	return ret;
}

void TBaseNPC::npcBlownIn()
{
	requestNpcAnm_((EnumNpcAnmKind)0xE,
	               (EnumNpcStopMotionBlendOnOff)1);
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
	*(s32*)mAnmFrameCounter = 0;
}

bool TBaseNPC::npcBlowning()
{
	bool ret = false;
	BOOL flag;
	if (mLiveFlag & LIVE_FLAG_AIRBORNE)
		flag = TRUE;
	else
		flag = FALSE;
	if (!flag)
		ret = true;
	return ret;
}

void TBaseNPC::npcMareStandIn()
{
	int k = unkD0->mCurrentAnmKind;
	switch (k) {
	case 5:
	case 0xE: {
		mAnmFrameCounter->doThing3(240, 360);
		break;
	}
	default:
		requestNpcAnm_((EnumNpcAnmKind)0x7,
		               (EnumNpcStopMotionBlendOnOff)1);
		break;
	}
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

bool TBaseNPC::npcMareStanding()
{
	bool finished = false;
	int k         = unkD0->mCurrentAnmKind;
	switch (k) {
	case 5:
	case 0xE:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			if (mAnmFrameCounter->doThing2()) {
				requestNpcAnm_((EnumNpcAnmKind)0x7,
				               (EnumNpcStopMotionBlendOnOff)0);
			}
		}
		break;
	case 7:
		if (mMActor->isCurAnmAlreadyEnd(0))
			finished = true;
		break;
	default:
		finished = true;
		break;
	}
	return finished;
}

void TBaseNPC::peachParasolIn_()
{
	static const s32 sIndividualPeachBck[]
	    = { 0x15, 0x10, 0x16, 0x12, -1, -1 };
	static const s32 sIndividualPeachBtp[] = { 0x4, 0x2, -1, -1 };
	unk1D8 |= 0x1;
	unkD0->unk18 = (const TAnmBckMapping*)sIndividualPeachBck;
	unkD0->unk1C = (const TAnmBtpMapping*)sIndividualPeachBtp;
}

void TBaseNPC::peachParasolOut_()
{
	if (!checkUnk1D8(0x1))
		return;
	offUnk1D8(0x1);
	unkD0->unk18 = 0;
	unkD0->unk1C = 0;
}

void TBaseNPC::peachTiredIn_()
{
	if (checkUnk1D8(0x2))
		return;
	onUnk1D8(0x2);
	requestNpcAnm_((EnumNpcAnmKind)0x10,
	               (EnumNpcStopMotionBlendOnOff)1);
}

inline void TBaseNPC::peachTiredOut_()
{
	if (!checkUnk1D8(0x2))
		return;
	offUnk1D8(0x2);
	onUnk1D8(0x4);
	static const s32 sIndividualPeachBck[] = { 0x15, 0x0, -1, -1 };
	static const s32 sIndividualPeachBtp[] = { 0x5, 0x0, -1, -1 };
	unkD0->unk18 = (const TAnmBckMapping*)sIndividualPeachBck;
	unkD0->unk1C = (const TAnmBtpMapping*)sIndividualPeachBtp;
	requestNpcAnm_(asKind(6), NPC_STOP_MOTION_BLEND_OFF);
}

void TBaseNPC::sunflowerDownIn_()
{
	static const s32 sIndividualSunflowerBck[]
	    = { 0x2, 0x0, 0x3, 0x4, -1, -1 };
	static const s32 sIndividualSunflowerBtp[]
	    = { 0x3, 0x0, 0x2, 0x0, -1, -1 };
	unk1D8 |= 0x1;
	unk1D8 &= ~0x2;
	unkD0->unk18 = (const TAnmBckMapping*)sIndividualSunflowerBck;
	unkD0->unk1C = (const TAnmBtpMapping*)sIndividualSunflowerBtp;
}

void TBaseNPC::sunflowerReviveIn()
{
	if (checkUnk1D8(0x1)) {
		offUnk1D8(0x1);
		onUnk1D8(0x2);
		unkD0->unk18 = 0;
		unkD0->unk1C = 0;
		requestNpcAnm_((EnumNpcAnmKind)0x1A,
		               (EnumNpcStopMotionBlendOnOff)1);
	}
}

bool TBaseNPC::sunflowerReviving()
{
	bool acted = false;
	if ((unk1D8 & 0x2) && unkD0->mCurrentAnmKind == 0x1A) {
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			unk1D8 &= ~0x2;
			if (mLiveFlag & 0x80000) {
				requestTalkAnm_();
			} else {
				npcWaitIn();
			}
			acted = true;
		}
	}
	return acted;
}

void TBaseNPC::monteMESetAnmWhenFar()
{
	switch (unkD0->mCurrentAnmKind) {
	case 1:
		break;

	case 2:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			requestNpcAnm_(NPC_ANM_KIND_UNK1, NPC_STOP_MOTION_BLEND_OFF);
		}
		break;

	case 3:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			requestNpcAnm_(NPC_ANM_KIND_UNK2, NPC_STOP_MOTION_BLEND_OFF);
		}
		break;

	default: {
		requestNpcAnm_(NPC_ANM_KIND_UNK2, NPC_STOP_MOTION_BLEND_ON);
		break;
	}
	}
}

void TBaseNPC::monteMESetAnmWhenNear()
{
	switch (unkD0->mCurrentAnmKind) {
	case 4:
		break;

	case 3:
	case 0xA:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			requestNpcAnm_(NPC_ANM_KIND_UNK4, NPC_STOP_MOTION_BLEND_OFF);
		}
		break;

	case 2:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			requestNpcAnm_(NPC_ANM_KIND_UNK3, NPC_STOP_MOTION_BLEND_OFF);
		}
		break;

	default: {
		requestNpcAnm_(NPC_ANM_KIND_UNK3, NPC_STOP_MOTION_BLEND_OFF);
		break;
	}
	}
}
