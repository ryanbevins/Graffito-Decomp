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
		return 0.5 * root * (3.0 - mag * (root * root)) * mag;
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

// Shared peach revive mapping tables (local $2904/$2905), referenced by both
// npcTalking and npcTalkOut.
static const s32 sIndividualPeachBck[] = { 0x15, 0x0, -1, -1 };
static const s32 sIndividualPeachBtp[] = { 0x5, 0x0, -1, -1 };

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

// True when this NPC is one of the "sink/peach interior" actor types (BaseY
// range 0x0400001C..0x0400001E) where animation requests must be ignored.
static inline bool isLockedAnmActor(u32 actorType)
{
	return (s32)actorType >= (s32)0x0400001C
	    && (s32)actorType < (s32)0x0400001E;
}

// ============================================================
// Definition order is *reverse* of binary layout because this
// TU is compiled with -inline deferred.
// ============================================================

void TBaseNPC::setNpcAnm_(EnumNpcAnmKind kind, EnumNpcStopMotionBlendOnOff blend)
{
	if (isLockedAnmActor(mActorType))
		return;

	mAnmRequest->mKind = -1;
	if ((int)kind == unkD0->mCurrentAnmKind)
		return;

	if (!unkD0->setBckAndBtpAnm((int)kind))
		return;

	if ((int)kind == 0x11 && unk1D9 < 3 && unk1D9 >= 0) {
		emitHappyEffect_();
		if (gpMSound->gateCheck(0x8808)) {
			MSoundSESystem::MSoundSE::startSoundNpcActor(
			    0x8808,
			    (const Vec*)((u8*)this + 0x10),
			    0, (JAISound**)NULL, 0, 4);
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
		// Parts animation overrides per actor variant.
		if (mActorType == 0x04000010) {
			// Hat/parasol parts.
			MActor* parts = mNpcParts->getPartsMActor(0, 0);
			bool down     = (unk1D8 & 0x1) ? false : true;
			if (parts) {
				int bck;
				if ((int)kind == 5)
					bck = down ? 6 : 4;
				else
					bck = down ? 5 : 3;
				if (!parts->checkCurBckFromIndex(bck))
					parts->setBckFromIndex(bck);
			}
			MActor* parts3 = mNpcParts->getPartsMActor(3, 0);
			if (parts3) {
				int bck;
				int k = (int)kind;
				if (k >= 5 && k <= 5 + 0x15) {
					switch (k) {
					case 5:
						bck = 7;
						break;
					case 0x11:
						bck = 0xE;
						break;
					case 0x10:
						bck = 0xD;
						break;
					default:
						bck = (unk1D8 & 0x1) ? 0xB : 8;
						if (k == 0x1A) {
							if (unk1D8 & 0x4)
								bck = 9;
							else
								bck = 8;
						}
						break;
					}
				} else {
					bck = (unk1D8 & 0x4) ? 9 : 8;
				}
				if (!parts3->checkCurBckFromIndex(bck))
					parts3->setBckFromIndex(bck);
			}
			MActor* parts4 = mNpcParts->getPartsMActor(4, 0);
			if (parts4) {
				int bck = ((int)kind == 5) ? -1 : 0x11;
				if (!parts4->checkCurBckFromIndex(bck))
					parts4->setBckFromIndex(bck);
			}
		} else if (mActorType == 0x04000015) {
			// Hair / topknot parts.
			int bck;
			int btp = 1;
			int k   = (int)kind;
			if (k == 6) {
				bck = 2;
				btp = 1;
			} else if (k >= 5) {
				bck = 3;
				btp = 1;
			} else {
				bck = 0;
				btp = 0;
			}
			MActor* parts = mNpcParts->getPartsMActor(0xA, 0);
			if (parts) {
				if (!parts->checkCurBckFromIndex(bck))
					parts->setBckFromIndex(bck);
				parts->setBtpFromIndex(btp);
			}
		} else if (mActorType == 0x04000018) {
			// Tail / pouch parts.
			int bck;
			int k = (int)kind;
			if (k == 6)
				bck = 2;
			else if (k > 6)
				bck = 1;
			else if (k >= 5)
				bck = 3;
			else
				bck = 1;
			MActor* parts = mNpcParts->getPartsMActor(9, 0);
			if (parts) {
				if (!parts->checkCurBckFromIndex(bck))
					parts->setBckFromIndex(bck);
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
			req->mBlend         = blend;
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
	requestNpcAnm_(asKind(kind), asBlend(1));
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
			    = CLBPalFrame<long>(mPtrSaveNormal->mStopWalkAnmRateFrame.value);
			CLBChaseDecrease(&unk1D0, 0.0f,
			    mPtrSaveNormal->mStopWalkAnmRateChase.value, 0.001f);
			if (unk1CC < frame)
				unk1CC++;
			if (unk1CC >= frame || unk1D0 == 0.0f) {
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
		if (anmKind == 8 || anmKind == 0) {
			f32 individualRate
			    = mNpcSaveIndividual->mSLMinWalkAnmRate.value
			    * SMSGetAnmFrameRate();
			f32 baseRate;
			f32 maxRate;
			if (anmKind == 8) {
				baseRate = mNpcSaveIndividual->mSLMaxRunAnmRate.value
				         * SMSGetAnmFrameRate();
				maxRate  = mNpcSaveIndividual->mSLMaxRunSpeed.value;
				if (mActionFlag & 0x4000) {
					f32 mul  = mPtrSaveNormal->mSLSmokeRunMagnif.value;
					baseRate = baseRate * mul;
					maxRate  = maxRate * mul;
				}
			} else {
				baseRate = mNpcSaveIndividual->mSLMaxWalkAnmRate.value
				         * SMSGetAnmFrameRate();
				maxRate  = mNpcSaveIndividual->mMaxMarchSpeed.value;
			}
			if (speed > maxRate)
				speed = maxRate;
			f32 ratio
			    = CLBCalcRatio<f32>(mNpcSaveIndividual->mSLMinMarchSpeed.value,
			                        maxRate, speed);
			if (ratio > 1.0f)
				ratio = 1.0f;
			else if (ratio < 0.0f)
				ratio = 0.0f;
			f32 newRate = CLBLinearInbetween<f32>(individualRate, baseRate, ratio);
			if (mColCount != 0)
				newRate = baseRate;
			CLBChaseDecrease(&unk1D0, newRate,
			    mPtrSaveNormal->mMoveWalkAnmRateChase.value, 0.0f);
			mMActor->setFrameRate(unk1D0, 0);
		} else {
			unk1D0 = 0.0f;
			if (mActionFlag & 0x8) {
				requestNpcAnm_(asKind(8), asBlend(1));
			} else {
				requestNpcAnm_(asKind(0), asBlend(1));
			}
		}
	}
}

int TBaseNPC::getNpcWaitAnmBase_()
{
	int result = 1;
	if (unk1E2 == 0) {
		u32 flag = mActionFlag;
		if (flag & 0x2) {
			result = 0xC;
		} else if (flag & 0x10) {
			result = 0x15;
		} else if (flag & 0x20) {
			result = 6;
		} else if (flag & 0x40) {
			result = 0x17;
		} else if (flag & 0x4) {
			result = 0x16;
		}
	}
	return result;
}

void TBaseNPC::npcWaitIn()
{
	int kind = 1;
	u32 flag = mActionFlag;
	if (!(flag & 0x400)) {
		if (unk178 != 0.0f) {
			kind = 0xF;
		} else if (flag & 0x200) {
			kind = 0x11;
		} else if ((flag & 0x1) && !(flag & 0x4)) {
			if (flag & 0x20)
				kind = 0x13;
			else
				kind = 0x12;
		} else if (!unk124->getGraph()->isDummy()) {
			if (mSpine->getLatestNerve() == &TNerveNPCGraphWait::theNerve()) {
				u8 state    = gpMarDirector->unk124;
				bool inMode = (state == 1 || state == 2);
				bool keep   = inMode || state == 4;
				if (!keep)
					kind = getNpcWaitAnmBase_();
			}
		} else {
			kind = getNpcWaitAnmBase_();
		}
	}
	requestNpcAnm_(asKind(kind), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcFallIn()
{
	requestNpcAnm_(asKind(0x2), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

bool TBaseNPC::npcRecoverFromSinking()
{
	bool changed = false;
	if (!(mLiveFlag & 0x08000000)) {
		J3DFrameCtrl* fc = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		if (fc->checkPass(32.0f)) {
			mLiveFlag |= 0x08000000;
			f32 grav = getGravityY();
			f32 v    = 150.0f + (mSinkBaseY - mPosition.y);
			f32 init = 0.0f;
			if (grav > 0.0f) {
				v    = 8.0f * (v * (1.0f / grav)) + 1.0f;
				v    = recoverSqrt(v);
				init = (0.5f * grav) * (1.0f + v);
			}
			mVelocity.y = init;
			if (mVelocity.y < 5.0f)
				mVelocity.y = 5.0f;
		}
	}
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
	requestNpcAnm_(asKind(0x3), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcStepIn()
{
	requestNpcAnm_(asKind(0x4), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcTalkIn()
{
	mLiveFlag |= 0x00080000;
	if (mActorType != 0x0400001C && mActorType != 0x0400001D) {
		if (!(isSunflower() && (unk1D8 & 0x2))) {
			if (mActorType == 0x04000018 && (unk1D8 & 0x2)) {
				requestNpcAnm_(asKind(0x1A), asBlend(1));
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
	bool sunDown = false;
	if (isSunflower() && (unk1D8 & 0x2))
		sunDown = true;
	if (sunDown) {
		if ((unk1D8 & 0x2) && unkD0->mCurrentAnmKind == 0x1A
		    && mMActor->isCurAnmAlreadyEnd(0)) {
			unk1D8 &= ~0x2;
			if (mLiveFlag & 0x00080000)
				requestTalkAnm_();
			else
				npcWaitIn();
		}
		return;
	}
	if (isTurnToMarioWhenTalk()) {
		SMS_GoRotate(mPosition, *gpMarioPos, mTurnSpeed,
		             &((JGeometry::TVec3<f32>*)((u8*)this + 0x30))->y);
		if (!unk124->getGraph()->isDummy())
			unk1DA |= 0x1;
	}
	bool revive = false;
	if (mActorType == 0x04000018 && (unk1D8 & 0x2))
		revive = true;
	if (revive && unkD0->mCurrentAnmKind == 0x1A
	    && mMActor->isCurAnmAlreadyEnd(0) && (unk1D8 & 0x2)) {
		peachTiredOut_();
	}
}

void TBaseNPC::npcTalkOut()
{
	unk1E0 = 0x3C;
	if (!(mLiveFlag & 0x00080000))
		return;
	unk1E2 = 0x78;

	bool peach = false;
	if (mActorType == 0x04000018 && (unk1D8 & 0x2))
		peach = true;
	if (peach && (unk1D8 & 0x2)) {
		peachTiredOut_();
	}

	if (mActionFlag & 0x200) {
		mActionFlag &= ~0x200;
		if (mNpcCoin != 0) {
			Vec cursor = getCursorPos();
			mNpcCoin->requestAppearCoin(cursor, mRotation.y, 0x28);
			unk1E0 = 0x168;
			unk1E2 = 0x168;
		}
	}

	mLiveFlag &= ~0x00080000;
	changeNerveFromTalk_();
	if (unk17C != 0)
		return;
	if (mActorType != 0x04000006)
		return;
	requestNpcAnm_(asKind(0x4), asBlend(1));
}

void TBaseNPC::npcTakenIn()
{
	requestNpcAnm_(asKind(0x9), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcDanceIn()
{
	mActionFlag |= 0x4;
	requestNpcAnm_(asKind(0x16), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcHappyIn(unsigned char arg)
{
	unk1D9        = arg;
	mActionFlag  |= 0x200;
	requestNpcAnm_(asKind(0x11), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

void TBaseNPC::npcWetIn()
{
	bool sunDown = false;
	if (isSunflower() && (unk1D8 & 0x2))
		sunDown = true;
	if (!sunDown) {
		int anm   = 5;
		int blend = 1;
		if (unk178 != 0.0f) {
			anm = 0x19;
		} else if (mActionFlag & 0x1) {
			anm = 0x14;
			if (!(isNormalMonteM() || isNormalMonteW()))
				blend = 0;
		} else {
			bool isMonte = isNormalMonteM() || isNormalMonteW();
			if (isMonte || mActorType == 0x0400000D) {
				if (mLiveFlag & 0x04000000) {
					blend = 0;
				} else if (MsRandF() < 0.5f) {
					anm = 0xB;
				} else {
					blend = 0;
				}
			} else {
				bool isMare = isNormalMareM() || isNormalMareW();
				if (isMare || mActorType == 0x04000011) {
					if (MsRandF() < 0.5f)
						anm = 0xB;
				} else if ((s32)mActorType < (s32)0x04000018
				           && (s32)mActorType >= (s32)0x04000016) {
					if (!(mLiveFlag & 0x04000000) && MsRandF() < 0.5f)
						anm = 0xB;
				}
			}
		}
		requestNpcAnm_(asKind(anm), asBlend(blend));
	}
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
	mAnmFrameCounter->mCurFrame = 0;
}

void TBaseNPC::npcWetOut()
{
	mLiveFlag &= ~0x02000000;
	mLiveFlag &= ~0x04000000;
	if (mSpine->peekTopNerveOrNull() == &TNerveNPCTalk::theNerve()) {
		requestTalkAnm_();
	} else if (mActorType == 0x04000006) {
		requestNpcAnm_(asKind(0x4), asBlend(1));
	}
}

bool TBaseNPC::npcWetting()
{
	bool ret = false;
	if (unk1DA & 0x2) {
		unk1DA &= ~0x2;
		unk1D9       = 0;
		mActionFlag |= 0x200;
		requestNpcAnm_(asKind(0x11), asBlend(1));
		mMarchSpeed = 0.0f;
		mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
		unk1CC      = 0;
		unk1D0      = 0.0f;
		npcWetOut();
		ret = true;
		return ret;
	}
	if (unkD0->mCurrentAnmKind == 0x19) {
		if (unk178 == 0.0f) {
			unk1D9       = 0;
			mActionFlag |= 0x200;
			requestNpcAnm_(asKind(0x11), asBlend(1));
			mMarchSpeed = 0.0f;
			mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
			unk1CC      = 0;
			unk1D0      = 0.0f;
			npcWetOut();
			ret = true;
			return ret;
		}
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			npcWetOut();
			ret = true;
		}
		return ret;
	}

	bool isMare = isNormalMareM() || isNormalMareW();
	if (isMare || mActorType == 0x04000011) {
		int k = unkD0->mCurrentAnmKind;
		switch (k) {
		case 5:
		case 0xB:
		case 0x14:
			// shiver wait
			if (mMActor->isCurAnmAlreadyEnd(0)) {
				TNpcAnmFrameCounter* fc = mAnmFrameCounter;
				if (fc->mCurFrame == 0) {
					fc->mCurFrame = 0;
					f32 r         = MsRandF();
					fc->mMaxFrame = (s32)(240.0f * r) + 0xF1;
				}
				fc->mCurFrame++;
				bool atMax = false;
				if (fc->mCurFrame >= fc->mMaxFrame) {
					fc->mCurFrame = fc->mMaxFrame;
					atMax         = true;
				}
				if (atMax) {
					int anm = 7;
					if (MsRandF() < 0.5f)
						anm = 0x1B;
					requestNpcAnm_(asKind(anm), asBlend(0));
				}
			}
			break;
		case 7:
		case 0x1B:
			if (mMActor->isCurAnmAlreadyEnd(0)) {
				if (isStateGoToMad_()) {
					mLiveFlag |= 0x02000000;
					if (mActorType == 0x04000007 || (mActionFlag & 0x1))
						requestNpcAnm_(asKind(0xA), asBlend(1));
					else {
						requestNpcAnm_(asKind(0x4), asBlend(1));
						mMarchSpeed = 0.0f;
						mTurnSpeed
						    = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
						unk1CC = 0;
						unk1D0 = 0.0f;
					}
				} else {
					npcWetOut();
					ret = true;
				}
			}
			break;
		default:
			break;
		}
		if (isMadNpc() && mActorType != 0x04000006) {
			int k2 = unkD0->mCurrentAnmKind;
			if (k2 == 5 || (k2 >= 0xC && k2 <= 0x14)) {
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
			} else if (k2 >= 4 && k2 < 5) {
				if (mLiveFlag & 0x02000000)
					npcMadding();
			}
		}
		return ret;
	}

	// monte branch
	bool isMonte = isNormalMonteM() || isNormalMonteW() || isSpecialMonteM()
	            || isSpecialMonteW();
	if (isMonte) {
		if (isMadNpc() && mActorType != 0x04000006) {
			int k = unkD0->mCurrentAnmKind;
			if (k == 5 || (k >= 0xC && k <= 0x14)) {
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
			} else if (k >= 4 && k < 5) {
				if (mLiveFlag & 0x02000000)
					npcMadding();
			}
			return ret;
		}
	}

	bool sunWet = false;
	if (isSunflower() && (unk1D8 & 0x2))
		sunWet = true;
	if (sunWet) {
		bool acted = false;
		if ((unk1D8 & 0x2) && unkD0->mCurrentAnmKind == 0x1A
		    && mMActor->isCurAnmAlreadyEnd(0)) {
			unk1D8 &= ~0x2;
			if (mLiveFlag & 0x00080000)
				requestTalkAnm_();
			else
				npcWaitIn();
			acted = true;
		}
		if (acted) {
			npcWetOut();
			ret = true;
		}
		return ret;
	}

	if (mActorType >= 0x04000016 && mActorType < 0x04000018) {
		int k = unkD0->mCurrentAnmKind;
		if (mMActor->isCurAnmAlreadyEnd(0) && (u32)(k - 5) <= 0x13) {
			switch (k) {
			case 5:
				// requestNpcAnm_(7, 0)
				if (mActorType < 0x0400001E && mActorType >= 0x0400001C)
					break;
				if (((TUnk18CStruct*)mUnk18C)->unk24 > 0) {
					if (k == unkD0->mCurrentAnmKind)
						mAnmRequest->mKind = -1;
					else {
						mAnmRequest->mKind  = 7;
						mAnmRequest->mBlend = 0;
					}
				} else {
					setNpcAnm_(asKind(7), asBlend(0));
				}
				break;
			case 0xB:
				if (mActorType < 0x0400001E && mActorType >= 0x0400001C)
					break;
				if (((TUnk18CStruct*)mUnk18C)->unk24 > 0) {
					if (k == 0xA)
						mAnmRequest->mKind = -1;
					else {
						mAnmRequest->mKind  = 0xA;
						mAnmRequest->mBlend = 1;
					}
				} else {
					setNpcAnm_(asKind(0xA), asBlend(1));
				}
				break;
			case 0x14:
				if (mActorType < 0x0400001E && mActorType >= 0x0400001C)
					break;
				if (((TUnk18CStruct*)mUnk18C)->unk24 > 0) {
					if (k == 0x18)
						mAnmRequest->mKind = -1;
					else {
						mAnmRequest->mKind  = 0x18;
						mAnmRequest->mBlend = 1;
					}
				} else {
					setNpcAnm_(asKind(0x18), asBlend(1));
				}
				break;
			case 7:
			case 0xA:
			case 0x18:
				npcWetOut();
				ret = true;
				break;
			default:
				break;
			}
		}
		return ret;
	}

	if (mMActor->isCurAnmAlreadyEnd(0)) {
		int k = unkD0->mCurrentAnmKind;
		if ((u32)(k - 5) <= 0x14) {
			switch (k) {
			case 5:
			case 0xB:
			case 0x14:
			case 0x19:
				npcWetOut();
				ret = true;
				break;
			default:
				break;
			}
		}
	}
	return ret;
}

void TBaseNPC::npcSinking()
{
	f32 sinkHeight = *(f32*)((u8*)mNpcSaveIndividual + 0x2FC);
	f32 sinkSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x2E8);
	f32 targetY    = mSinkBaseY - sinkHeight;
	if (mPosition.y == targetY)
		return;
	if (isPollutionNpc()) {
		f32 ratio = 1.0f / sinkSpeed;
		CLBChaseConstantSpecifyFrame<f32>(
		    &unk178, (mPosition.y - targetY) * ratio, sinkSpeed);
	}
	if (CLBChaseGeneralConstantSpecifySpeed<f32>(&mPosition.y, targetY,
	                                             sinkSpeed))
		return;
	mLiveFlag |= 0x00800000;
	unk64     |= 0x1;
	if (mActorType >= 0x0400001C && mActorType < 0x0400001E)
		return;
	requestNpcAnm_(asKind(0x10), asBlend(1));
}

void TBaseNPC::npcThrowIn()
{
	requestNpcAnm_(asKind(0xD), asBlend(1));
	mMarchSpeed = 0.0f;
	mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
	unk1CC      = 0;
	unk1D0      = 0.0f;
}

bool TBaseNPC::npcThrowing()
{
	bool done    = false;
	s32 spineCtr = *(s32*)((u8*)mSpine + 0x20);
	s16 startF   = mPtrSaveNormal->mSLThrowStartFrame.value;
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
		requestNpcAnm_(asKind(0xA), asBlend(1));
	} else {
		requestNpcAnm_(asKind(0x4), asBlend(1));
		mMarchSpeed = 0.0f;
		mTurnSpeed  = *(f32*)((u8*)mNpcSaveIndividual + 0x144);
		unk1CC      = 0;
		unk1D0      = 0.0f;
	}
}

bool TBaseNPC::npcMadding()
{
	bool ret = false;
	if (!(mLiveFlag & 0x02000000)) {
		ret = true;
		return ret;
	}
	int k = unkD0->mCurrentAnmKind;
	if (k == 4) {
		SMS_GoRotate(mPosition, *gpMarioPos,
		             *(f32*)((u8*)mNpcSaveIndividual + 0x2ac), &mRotation.y);
		JGeometry::TVec3<f32> diff = *gpMarioPos;
		diff.x -= mPosition.x;
		diff.y -= mPosition.y;
		diff.z -= mPosition.z;
		f32 targetYaw = MsGetRotFromZaxisY(diff);
		f32 delta     = fabs(mRotation.y - targetYaw);
		while (delta >= 360.0f)
			delta -= 360.0f;
		while (delta < 0.0f)
			delta += 360.0f;
		if (delta < 0.001f)
			requestNpcAnm_(asKind(0xA), asBlend(0));
		if (!unk124->getGraph()->isDummy())
			unk1DA |= 0x1;
	} else if (k == 0xA || k == 0xB) {
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			ret        = true;
			mLiveFlag &= ~0x02000000;
		}
	}
	return ret;
}

void TBaseNPC::npcBlownIn()
{
	requestNpcAnm_(asKind(0xE), asBlend(1));
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
		TNpcAnmFrameCounter* fc = mAnmFrameCounter;
		if (fc->mCurFrame == 0) {
			fc->mCurFrame = 0;
			f32 r         = (f32)rand() * (1.0f / 32768.0f);
			s32 v         = (s32)(r * 120.0f);
			fc->mMaxFrame = v + 0xF1;
		}
		break;
	}
	default:
		requestNpcAnm_(asKind(7), asBlend(1));
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
			TNpcAnmFrameCounter* fc = mAnmFrameCounter;
			fc->mCurFrame++;
			if (fc->mCurFrame >= fc->mMaxFrame) {
				fc->mCurFrame = fc->mMaxFrame;
				finished      = true;
			}
			if (finished) {
				requestNpcAnm_(asKind(7), asBlend(1));
			}
		}
		break;
	case 7:
		if (mMActor->isCurAnmAlreadyEnd(0))
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
	if (!(unk1D8 & 0x1))
		return;
	unk1D8 &= ~0x1;
	unkD0->unk18 = 0;
	unkD0->unk1C = 0;
}

void TBaseNPC::peachTiredIn_()
{
	if (unk1D8 & 0x2)
		return;
	unk1D8 |= 0x2;
	requestNpcAnm_(asKind(0x10), asBlend(1));
}

inline void TBaseNPC::peachTiredOut_()
{
	if (!(unk1D8 & 0x2))
		return;
	unk1D8 &= ~0x2;
	unk1D8 |= 0x4;
	unkD0->unk18 = (const TAnmBckMapping*)sIndividualPeachBck;
	unkD0->unk1C = (const TAnmBtpMapping*)sIndividualPeachBtp;
	requestNpcAnm_(asKind(6), asBlend(0));
}

void TBaseNPC::sunflowerDownIn_()
{
	static const s32 sIndividualSunflowerBck[]
	    = { 0x2, 0x0, 0x3, 0x4, -1, -1 };
	static const s32 sIndividualSunflowerBtp[]
	    = { 0x3, 0x0, 0x2, 0x0, -1, -1 };
	unk1D8 |= 0x1;
	unk1D8 &= ~0x4;
	unkD0->unk18 = (const TAnmBckMapping*)sIndividualSunflowerBck;
	unkD0->unk1C = (const TAnmBtpMapping*)sIndividualSunflowerBtp;
}

void TBaseNPC::sunflowerReviveIn()
{
	if (!(unk1D8 & 0x1))
		return;
	unk1D8 &= ~0x1;
	unk1D8 |= 0x2;
	unkD0->unk18 = 0;
	unkD0->unk1C = 0;
	requestNpcAnm_(asKind(0x1A), asBlend(1));
}

bool TBaseNPC::sunflowerReviving()
{
	bool acted = false;
	if ((unk1D8 & 0x2) && unkD0->mCurrentAnmKind == 0x1A) {
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			unk1D8 &= ~0x2;
			if (mLiveFlag & 0x80000) {
				EnumNpcAnmKind kind;
				if (mActionFlag & 0x400) {
					kind = asKind(1);
				} else if ((mActionFlag & 0x1) && !(mActionFlag & 0x4)) {
					kind = asKind(0x13);
				} else {
					kind = asKind(6);
				}
				setNpcAnm_(kind, asBlend(1));
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
			EnumNpcAnmKind kind = asKind(1);
			requestNpcAnm_(kind, asBlend(0));
		}
		break;

	case 3:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			EnumNpcAnmKind kind = asKind(2);
			requestNpcAnm_(kind, asBlend(0));
		}
		break;

	default: {
		EnumNpcAnmKind kind = asKind(2);
		requestNpcAnm_(kind, asBlend(1));
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
			EnumNpcAnmKind kind = asKind(4);
			requestNpcAnm_(kind, asBlend(0));
		}
		break;

	case 2:
		if (mMActor->isCurAnmAlreadyEnd(0)) {
			EnumNpcAnmKind kind = asKind(3);
			requestNpcAnm_(kind, asBlend(0));
		}
		break;

	default: {
		EnumNpcAnmKind kind = asKind(3);
		requestNpcAnm_(kind, asBlend(0));
		break;
	}
	}
}
