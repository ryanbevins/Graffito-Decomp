#ifndef NPC_BASE_NPC_HPP
#define NPC_BASE_NPC_HPP

#include <Enemy/Enemy.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <NPC/NpcInitData.hpp>
#include <dolphin/mtx.h>

class TNpcParts;
class TNpcSaveIndividual;
class TNpcSaveNormal;
class SDLModel;
class TMultiMtxEffect;
class TNpcCoin;
class TNpcTrample;

// TODO: should this be here?
enum EnumNpcAnmKind { };
enum EnumNpcStopMotionBlendOnOff {
	NPC_STOP_MOTION_BLEND_OFF = 0,
	NPC_STOP_MOTION_BLEND_ON  = 1,
};
enum EnumHitNpcObjectKind { };

// Pending animation request set by request*Anm_; consumed by perform/state code.
struct TNpcAnmRequest {
	/* 0x0 */ s32 mKind;
	/* 0x4 */ u8 mBlend;
};

// Frame counter used to track how long the current wait/idle animation has
// run; advances each tick until it hits its max.
struct TNpcAnmFrameCounter {
	/* 0x0 */ s32 mCurFrame;
	/* 0x4 */ s32 mMaxFrame;
};

class TNpcBalloon;
class TBaseNPC : public TSpineEnemy {
public:
	void monteMESetAnmWhenNear();
	void monteMESetAnmWhenFar();
	bool sunflowerReviving();
	void sunflowerReviveIn();
	void sunflowerDownIn_();
	void peachTiredOut_();
	void peachTiredIn_();
	void peachParasolOut_();
	void peachParasolIn_();
	bool npcMareStanding();
	void npcMareStandIn();
	bool npcBlowning();
	void npcBlownIn();
	bool npcMadding();
	void npcMadIn();
	bool npcThrowing();
	void npcThrowIn();
	void npcSinking();
	void npcWetOut();
	bool npcWetting();
	void npcWetIn();
	void npcHappyIn(unsigned char);
	void npcDanceIn();
	void npcTakenIn();
	void npcTalkOut();
	void npcTalking();
	void npcTalkIn();
	void npcStepIn();
	void npcRecoverAfterIn();
	bool npcRecoverFromSinking();
	void npcFallIn();
	void npcWaitIn();
	int getNpcWaitAnmBase_();
	void walkAnmRateChange_();
	void randomizeBckAndBtpFrame_();
	void requestTalkAnm_();
	void setKeepAnm_();
	void requestNpcAnm_(EnumNpcAnmKind, EnumNpcStopMotionBlendOnOff);
	void setNpcAnm_(EnumNpcAnmKind, EnumNpcStopMotionBlendOnOff);
	void onStopMotionBlend();
	void offStopMotionBlend();
	void isNowMotionBlend() const;
	void onUnk1D8(u32 flag) { unk1D8 |= flag; }
	void offUnk1D8(u32 flag) { unk1D8 &= ~flag; }
	bool checkUnk1D8(u32 flag) const { return (unk1D8 & flag) != 0; }
	~TBaseNPC();
	const GXColor* getPtrInitPollutionColor() const;
	void setBalloonMessage(u32, long);
	void setDummyConnectActor(const JDrama::TActor*);
	void perform(u32, JDrama::TGraphics*);
	f32  getAnmOffDist_();
	void updateForbidCount_();
	void movementOnlyTalk_(const JDrama::TGraphics*);
	void calcRootMatrix();
	void execMotionBlend_();
	void moveObject();
	virtual BOOL receiveMessage(THitActor* sender, u32 message);
	Vec getFocalPoint() const;
	Vec getCursorPos() const;
	bool isInMadSearchRange() const;
	bool isInBodyTurnSearchRange() const;
	bool isNeedNeckStraight() const;
	BOOL isPartsAnmNpc() const;
	void isBehaveToHitNpc() const;
	BOOL isBehaveToWaterNpc() const;
	bool isMadNpc() const;
	bool isBeTrampledNpc() const;
	bool isPollutionNpc() const;
	bool isSmallNpc() const;
	bool isChild() const;
	bool isSunflower() const;
	bool isJellyFishMare() const;
	bool isSpecialMareW() const;
	bool isSpecialMareM() const;
	bool isNormalMareW() const;
	bool isNormalMareM() const;
	bool isSpecialMonteW() const;
	bool isSpecialMonteM() const;
	bool isNormalMonteW() const;
	bool isNormalMonteM() const;
	void loadAfter();
	void load(JSUMemoryInputStream&);
	TBaseNPC(u32, const char*);
	void setIndividualDifference_(JSUMemoryInputStream&);
	void init(TLiveManager*);
	void initBaseActionFlag_();
	void initIndividualAnm_();
	void initSinkNpc_();
	void setMtxEffect_();
	void initNpcLight_();
	const char** getBasNameTable() const;
	void kill();
	void setPosAndInitAfterSinkBottom();
	void changeNerveProc_();
	bool isNowCanTaken() const;
	bool isStateGoToMad_() const;
	void behaveToSandBomb_(const TLiveActor*);
	void behaveToHitObject_(THitActor*, EnumHitNpcObjectKind);
	void behaveToBeTrampled_();
	void behaveToBeTaken_(THitActor*);
	void releaseTaken_();
	void changeNerveToMad_();
	void changeNerveToWet_();
	void changeNerveFromTalk_();
	BOOL isNerveCanGoToBlown() const;
	bool isNerveCanGoToMad() const;
	BOOL isNerveCanGoToThrow() const;
	BOOL isNerveCanGoToTaken() const;
	BOOL isNerveCanGoToSink() const;
	BOOL isNerveCanGoToWet() const;
	BOOL isNerveCanGoToTalk() const;
	BOOL isNerveMaybeDontCalcAnim1() const;
	BOOL isNerveMaybeDontCalcAnim0() const;
	BOOL isNerveMaybeDontMovement() const;
	bool isNerveWalk() const;
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
	bool isTurnToMarioWhenApproach() const;
	BOOL isTurnToMarioWhenTalk() const;
	bool isNeedTurnToFirstState() const;
	bool execTurnToFirstState();
	bool execUTurn();
	void execWalk(bool);
	void isCanWalk() const;
	void bind();
	void setVariableDamageRadius_();
	void execNpcObjCollision_();
	void initNpcObjCollision_(const TNpcInitInfo*);

	static TNpcSaveNormal* mPtrSaveNormal;
	static s16 mAngleYDiffWhenTaken;

public:
	/* 0x150 */ SDLModel* mSDLModel;
	/* 0x154 */ MtxPtr mSDLMtx;
	/* 0x158 */ THitActor* mTakenBy;
	/* 0x15C */ void* mPollutionStartHelper;
	/* 0x160 */ TMultiMtxEffect* mMultiMtxEffect;
	/* 0x164 */ s32 mNpcKind;
	/* 0x168 */ TNpcParts* mNpcParts;
	/* 0x16C */ s32 _16C;
	/* 0x170 */ u32 mActionFlag;
	/* 0x174 */ GXColor unk174;
	/* 0x178 */ f32 unk178;
	/* 0x17C */ void* unk17C;
	/* 0x180 */ TNpcTrample* mNpcTrample;
	/* 0x184 */ TNpcCoin* mNpcCoin;
	/* 0x188 */ TNpcBalloon* mNpcBalloon;
	/* 0x18C */ void* mUnk18C;
	/* 0x190 */ TNpcAnmRequest* mAnmRequest;
	/* 0x194 */ JGeometry::TVec3<f32> mResetPos;
	/* 0x1A0 */ char unk1A0[0x4];
	/* 0x1A4 */ f32 mResetRotY;
	/* 0x1A8 */ char unk1A8[0x4];
	/* 0x1AC */ JGeometry::TVec3<f32> mEffectScaleBase;
	/* 0x1B8 */ char mLoadRot[0xC];
	/* 0x1C4 */ f32 mSinkBaseY;
	/* 0x1C8 */ f32 unk1C8;
	/* 0x1CC */ s32 unk1CC;
	/* 0x1D0 */ f32 unk1D0;
	/* 0x1D4 */ const JDrama::TActor* mDummyConnectActor;
	/* 0x1D8 */ u8 unk1D8;
	/* 0x1D9 */ u8 unk1D9;
	/* 0x1DA */ u8 unk1DA;
	/* 0x1DB */ u8 unk1DB;
	/* 0x1DC */ s32 unk1DC;
	/* 0x1E0 */ u16 unk1E0;
	/* 0x1E2 */ u16 unk1E2;
	/* 0x1E4 */ u16 unk1E4;
	/* 0x1E6 */ char unk1E6[0x2];
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
	/* 0x22C */ TNpcAnmFrameCounter* mAnmFrameCounter;
	/* 0x230 */ s16* mNeckAngles;
};

#endif
