#ifndef NPC_BASE_NPC_HPP
#define NPC_BASE_NPC_HPP

#include <Enemy/Enemy.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <NPC/NpcAnmKind.hpp>
#include <NPC/NpcInitData.hpp>
#include <NPC/NpcSave.hpp>
#include <dolphin/mtx.h>

class TNpcParts;
class TNpcSaveIndividual;
class TNpcSaveNormal;
class SDLModel;
class TMultiMtxEffect;
class TNpcCoin;
class TNpcTrample;
class TNpcThrow;
class TNpcInbetween;

enum EnumNpcStopMotionBlendOnOff {
	NPC_STOP_MOTION_BLEND_OFF = 0,
	NPC_STOP_MOTION_BLEND_ON  = 1,
};
enum EnumHitNpcObjectKind {
	HIT_NPC_OBJECT_KIND_WATER_SPRAY = 0,
	HIT_NPC_OBJECT_KIND_UNK1        = 1,
	HIT_NPC_OBJECT_KIND_UNK2        = 2,
};

// Pending animation request set by request*Anm_; consumed by perform/state code.
struct TNpcKeepAnm {
	TNpcKeepAnm()
	    : mKind(NPC_ANM_KIND_INVALID)
	    , mBlendOn(false)
	{
	}

	void reset() { mKind = NPC_ANM_KIND_INVALID; }
	void keep(EnumNpcAnmKind kind, bool blend)
	{
		mKind    = kind;
		mBlendOn = blend;
	}
	EnumNpcAnmKind getKind() const { return mKind; }
	EnumNpcStopMotionBlendOnOff getBlend() const
	{
		return (EnumNpcStopMotionBlendOnOff)mBlendOn;
	}

private:
	/* 0x0 */ EnumNpcAnmKind mKind;
	/* 0x4 */ bool mBlendOn;
};

class TNpcBalloon;
class TBaseNPC : public TSpineEnemy {
public:
	TBaseNPC(u32, const char* name = "?");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual void moveObject();
	virtual void kill();
	virtual JGeometry::TVec3<f32> getFocalPoint() const;
	virtual const char** getBasNameTable() const;

	bool isCanWalk() const;
	bool isNeedTurnToFirstState() const;
	bool isTurnToMarioWhenTalk() const;
	bool isTurnToMarioWhenApproach() const;
	bool isNerveWalk() const;
	bool isNerveMaybeDontMovement() const;
	bool isNerveMaybeDontCalcAnim0() const;
	bool isNerveMaybeDontCalcAnim1() const;
	bool isNerveCanGoToTalk() const;
	bool isNerveCanGoToWet() const;
	bool isNerveCanGoToSink() const;
	bool isNerveCanGoToTaken() const;
	bool isNerveCanGoToThrow() const;
	bool isNerveCanGoToMad() const;
	bool isNerveCanGoToBlown() const;

	bool isNormalMonteM() const;
	bool isNormalMonteW() const;
	bool isNormalMonte() const { return isNormalMonteM() || isNormalMonteW(); }
	bool isSpecialMonteM() const;
	bool isSpecialMonteW() const;
	bool isSpecialMonte() const
	{
		return isSpecialMonteM() || isSpecialMonteW();
	}
	bool isMonte() const { return isNormalMonte() || isSpecialMonte(); }
	bool isMonteM() const { return isNormalMonteM() || isSpecialMonteM(); }
	bool isMonteW() const { return isNormalMonteW() || isSpecialMonteW(); }

	bool isNormalMareM() const;
	bool isNormalMareW() const;
	bool isNormalMare() const { return isNormalMareM() || isNormalMareW(); }
	bool isSpecialMareM() const;
	bool isSpecialMareW() const;
	bool isSpecialMare() const { return isSpecialMareM() || isSpecialMareW(); }
	bool isMare() const { return isNormalMare() || isSpecialMare(); }
	bool isMareM() const { return isNormalMareM() || isSpecialMareM(); }
	bool isMareW() const { return isNormalMareW() || isSpecialMareW(); }

	bool isJellyFishMare() const;
	bool isSunflower() const;
	bool isChild() const;
	bool isSmallNpc() const;
	bool isPollutionNpc() const;
	bool isBeTrampledNpc() const;
	bool isMadNpc() const;
	BOOL isBehaveToWaterNpc() const;
	bool isBehaveToHitNpc() const;
	bool isPartsAnmNpc() const;
	bool isNeedNeckStraight() const;
	bool isInBodyTurnSearchRange() const;
	bool isInMadSearchRange() const;
	bool isNowCanTaken() const;

	void execWalk(bool);
	bool execUTurn();
	bool execTurnToFirstState();
	void setPosAndInitAfterSinkBottom();

	JGeometry::TVec3<f32> getCursorPos() const;
	void setDummyConnectActor(const JDrama::TActor*);
	void setBalloonMessage(u32, long);
	const GXColor* getPtrInitPollutionColor() const;

	void isNowMotionBlend() const;
	void offStopMotionBlend();
	void onStopMotionBlend();
	void npcWaitIn();
	void npcFallIn();
	bool npcRecoverFromSinking();
	void npcRecoverAfterIn();

	void npcStepIn();
	void npcTalkIn();
	void npcTalking();
	void npcTalkOut();
	void npcTakenIn();
	void npcDanceIn();
	void npcHappyIn(u8);
	void npcWetIn();
	bool npcWetting();
	void npcWetOut();
	void npcSinking();
	void npcThrowIn();
	bool npcThrowing();
	void npcMadIn();
	bool npcMadding();
	void npcBlownIn();
	bool npcBlowning();
	void npcMareStandIn();
	bool npcMareStanding();
	void sunflowerReviveIn();
	bool sunflowerReviving();
	void monteMESetAnmWhenFar();
	void monteMESetAnmWhenNear();

	static TNpcSaveNormal* mPtrSaveNormal;
	static s16 mAngleYDiffWhenTaken;

	void onUnk1D8(u32 flag) { unk1D8 |= flag; }
	void offUnk1D8(u32 flag) { unk1D8 &= ~flag; }
	bool checkUnk1D8(u32 flag) const { return (unk1D8 & flag) != 0; }
	void onUnk1DA(u32 flag) { unk1DA |= flag; }
	void offUnk1DA(u32 flag) { unk1DA &= ~flag; }
	bool checkUnk1DA(u32 flag) const { return (unk1DA & flag) != 0; }
	void onActionFlag(u32 flag) { mActionFlag |= flag; }
	void offActionFlag(u32 flag) { mActionFlag &= ~flag; }
	bool checkActionFlag(u32 flag) const { return (mActionFlag & flag) != 0; }
	bool isSunflowerReviving() const
	{
		bool result = false;
		if (isSunflower() && (unk1D8 & 0x2))
			result = true;
		return result;
	}
	bool isPeachTired() const
	{
		bool result = false;
		if (mActorType == 0x04000018 && (unk1D8 & 0x2))
			result = true;
		return result;
	}
	bool isClean() const { return unk178 == 0.0f; }

private:
	void setIndividualDifference_(JSUMemoryInputStream&);
	void initIndividualAnm_();
	void initBaseActionFlag_();
	void initNpcLight_();
	void setMtxEffect_();
	void initSinkNpc_();
	void changeNerveFromTalk_();
	void changeNerveToWet_();
	void changeNerveToMad_();
	void releaseTaken_();
	void behaveToBeTaken_(THitActor*);
	void behaveToBeTrampled_();
	void behaveToHitObject_(THitActor*, EnumHitNpcObjectKind);
	void behaveToSandBomb_(const TLiveActor*);
	bool isStateGoToMad_() const;
	void changeNerveProc_();
	void execMotionBlend_();
	void movementOnlyTalk_(const JDrama::TGraphics*);

	void sunflowerDownIn_();
	void peachTiredOut_();
	void peachTiredIn_();
	void peachParasolOut_();
	void peachParasolIn_();
	EnumNpcAnmKind getNpcWaitAnmBase_();
	void walkAnmRateChange_();
	void randomizeBckAndBtpFrame_();
	void requestTalkAnm_();
	void setKeepAnm_();
	void requestNpcAnm_(EnumNpcAnmKind, EnumNpcStopMotionBlendOnOff);
	void setNpcAnm_(EnumNpcAnmKind, EnumNpcStopMotionBlendOnOff);
	~TBaseNPC();
	f32  getAnmOffDist_();
	void updateForbidCount_();
	void emitParticle_();
	bool isPolWaitCEffectEmitTime_() const;
	bool isPolWaitLEffectEmitTime_() const;
	bool isPolWaitREffectEmitTime_() const;
	void emitPollutionParticle_(int, MtxPtr);
	void emitDirtyEffect_();
	void emitWashEffect_();
	void emitHappyEffect_();
	void emitSinkEffect_();
	JGeometry::TVec3<f32> getEffectScale_() const;
	void setSmokeEffectMtxPtr_(bool);
	void setPollutionEffectMtxPtr_(const JUTNameTab*);
	void setNoteEffectMtxPtr_(const JUTNameTab*);
	void setHappyEffectMtxPtr_(const JUTNameTab*);
	void setKinoActionFlag_();
	void setMareActionFlag_();
	void setMonteActionFlag_();
	void setVariableDamageRadius_();
	void execNpcObjCollision_();
	void initNpcObjCollision_(const TNpcInitInfo*);

public:
	class TNpcSink {
	public:
		TNpcSink(int counter, int max)
		    : mCounter(counter)
		    , mMax(max)
		{
		}

		bool advance()
		{
			bool result = false;
			mCounter += 1;
			if (mCounter >= mMax) {
				mCounter = mMax;
				result   = true;
			}
			return result;
		}

		/* 0x0 */ int mCounter;
		/* 0x4 */ int mMax;
	};

	/* 0x150 */ SDLModel* mSDLModel;
	/* 0x154 */ MtxPtr mSDLMtx;
	/* 0x158 */ THitActor* mTakenBy;
	/* 0x15C */ TNpcSink* mSinkTimer;
	/* 0x160 */ TMultiMtxEffect* mMultiMtxEffect;
	/* 0x164 */ s32 mNpcKind;
	/* 0x168 */ TNpcParts* mNpcParts;
	/* 0x16C */ s32 _16C;
	enum {
		NPC_ACTION_UNK1    = 0x1,
		NPC_ACTION_UNK2    = 0x2,
		NPC_ACTION_DANCE   = 0x4,
		NPC_ACTION_RUN     = 0x8,
		NPC_ACTION_UNK10   = 0x10,
		NPC_ACTION_UNK20   = 0x20,
		NPC_ACTION_UNK40   = 0x40,
		NPC_ACTION_UNK80   = 0x80,
		NPC_ACTION_UNK100  = 0x100,
		NPC_ACTION_HAPPY   = 0x200,
		NPC_ACTION_UNK400  = 0x400,
		NPC_ACTION_UNK800  = 0x800,
		NPC_ACTION_UNK1000 = 0x1000,
		NPC_ACTION_UNK2000 = 0x2000,
		NPC_ACTION_BURNING = 0x4000,
	};
	/* 0x170 */ u32 mActionFlag;
	/* 0x174 */ GXColor unk174;
	/* 0x178 */ f32 unk178;
	/* 0x17C */ TNpcThrow* unk17C;
	/* 0x180 */ TNpcTrample* mNpcTrample;
	/* 0x184 */ TNpcCoin* mNpcCoin;
	/* 0x188 */ TNpcBalloon* mNpcBalloon;
	/* 0x18C */ TNpcInbetween* mUnk18C;
	/* 0x190 */ TNpcKeepAnm* mKeepAnmCtrl;
	/* 0x194 */ JGeometry::TVec3<f32> mResetPos;
	/* 0x1A0 */ JGeometry::TVec3<f32> mResetRot;
	/* 0x1AC */ JGeometry::TVec3<f32> mEffectScaleBase;
	/* 0x1B8 */ JGeometry::TVec3<f32> mLoadRot;
	/* 0x1C4 */ f32 mSinkBaseY;
	/* 0x1C8 */ f32 unk1C8;
	/* 0x1CC */ s32 unk1CC;
	/* 0x1D0 */ f32 unk1D0;
	/* 0x1D4 */ const JDrama::TActor* mDummyConnectActor;
	enum {
		UNK1D8_FLAG_UNK1 = 0x1,
		UNK1D8_FLAG_UNK2 = 0x2,
		UNK1D8_FLAG_UNK4 = 0x4,
	};
	/* 0x1D8 */ u8 unk1D8;
	/* 0x1D9 */ u8 unk1D9;
	enum {
		UNK1DA_FLAG_UNK1 = 0x1,
		UNK1DA_FLAG_UNK2 = 0x2,
	};
	/* 0x1DA */ u8 unk1DA;
	/* 0x1DC */ s32 unk1DC;
	/* 0x1E0 */ u16 unk1E0;
	/* 0x1E2 */ u16 unk1E2;
	/* 0x1E4 */ u16 unk1E4;
	/* 0x1E8 */ MtxPtr mPtrHappyEffectMtx;
	/* 0x1EC */ MtxPtr mPtrNoteEffectMtx;
	/* 0x1F0 */ JGeometry::TVec3<f32> mNoteEffectPos;
	/* 0x1FC */ MtxPtr mPtrPollutionEffectMtx;
	/* 0x200 */ MtxPtr mPtrPollutionLEffectMtx;
	/* 0x204 */ MtxPtr mPtrPollutionREffectMtx;
	/* 0x208 */ MtxPtr mPtrSmokeEffectMtx;
	/* 0x20C */ JGeometry::TVec3<f32> mSmokeEffectPos;
	/* 0x218 */ f32 mFireScaleMul;
	/* 0x21C */ JGeometry::TVec3<f32> mWaterEffectPos;
	/* 0x228 */ TNpcSaveIndividual* mNpcSaveIndividual;

	class TNpcAnmFrameCounter {
	public:
		TNpcAnmFrameCounter()
		    : mCurFrame(0)
		    , mMaxFrame(1)
		{
		}

		void resetRandom(s16 minFrame, s16 maxFrame)
		{
			mCurFrame = 0;
			s32 frame = minFrame
			            + (s32)((f32)(maxFrame - minFrame)
			                    * ((f32)rand() * (1.0f / 32768.0f)));
			mMaxFrame = frame + 1;
		}

		bool advance()
		{
			bool result = false;
			mCurFrame += 1;
			if (mCurFrame >= mMaxFrame) {
				mCurFrame = mMaxFrame;
				result    = true;
			}
			return result;
		}

		void resetRandomIfZero(int minFrame, int maxFrame)
		{
			if (mCurFrame == 0) {
				mCurFrame = 0;
				mMaxFrame = MsRandI(minFrame, maxFrame);
			}
		}

		/* 0x0 */ s32 mCurFrame;
		/* 0x4 */ s32 mMaxFrame;
	};

	/* 0x22C */ TNpcAnmFrameCounter* mAnmFrameCounter;

	class TNpcNeckAngles {
	public:
		TNpcNeckAngles()
		    : mYaw(0)
		    , mPitch(0)
		{
		}

		void set(s16 yaw, s16 pitch)
		{
			mYaw   = yaw;
			mPitch = pitch;
		}

		/* 0x0 */ s16 mYaw;
		/* 0x2 */ s16 mPitch;
	};

	/* 0x230 */ TNpcNeckAngles* mNeckAngles;
};

extern TBaseNPC* gpCurrentNpc;

#endif
