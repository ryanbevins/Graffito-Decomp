#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphBase/Components/J3DGXColorS10.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <Map/Map.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcCoin.hpp>
#include <NPC/NpcInitData.hpp>
#include <NPC/NpcInitAnmData.hpp>
#include <NPC/NpcInbetween.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcParts.hpp>
#include <NPC/NpcSave.hpp>
#include <NPC/NpcThrow.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/gx/GXStruct.h>
#include <string.h>

extern "C" int rand();

BOOL NPCNeckCallBack(J3DNode*, int);

const char* cManiyaParentViewObjName    = "マニ屋親タヌキ";
const char* cManiyaChildViewObjName     = "マニ屋子タヌキ";
const char* cNotUseFastCubeViewObjName0 = "モンテ26";
const char* cNotUseFastCubeViewObjName1 = "モンテ27";
const char* cEyeMaterialName            = "_eye_mat";
const char* cNeckJointName              = "kubi";

void SMS_InitChangeNpcColor(const MActor*, const TColorChangeInfo*, s16,
                            const GXColor*);

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

struct TMtxEffectInitDataEntry {
	/* 0x0 */ u32 mActorType;
	/* 0x4 */ const char* const* mBoneNames;
	/* 0x8 */ const char* mParamName;
	/* 0xC */ u8 mMtxType;
	/* 0xD */ u8 mNumBones;
	/* 0xE */ u8 pad[2];
};

inline void TBaseNPC::initNpcLight_()
{
	mMActor->setLightType(1);
	if (checkLiveFlag(LIVE_FLAG_UNK10)) {
		mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
		    mPosition.x, mPosition.y + 10.0f, mPosition.z, &mGroundPlane);
	}
}

inline void TBaseNPC::initSinkNpc_()
{
	static int sCheckPollutedStartCounter = 0;
	int max = CLBPalFrame<int>(30);
	mSinkTimer = new TNpcSink(sCheckPollutedStartCounter, max);
	sCheckPollutedStartCounter++;
	if (sCheckPollutedStartCounter >= max)
		sCheckPollutedStartCounter = 0;
}

inline void TBaseNPC::setMtxEffect_()
{
	static const char* sWaistJointName[] = { "koshi_null" };
	static const TMtxEffectInitDataEntry sMtxEffectInitData[]
	    = { { 0x04000001, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000002, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000003, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000004, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000005, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000007, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000008, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x04000009, sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
		    { 0x0400000A, sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
		    { 0x0400000B, sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
		    { 0x0400000C, sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
		    { 0x0400000D, sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
		    { 0x04000018, sWaistJointName, "Npc/Peach", 0, 1, { 0, 0 } },
		    { 0, nullptr, nullptr, 0, 0, { 0, 0 } } };

	const TMtxEffectInitDataEntry* entry = sMtxEffectInitData;
	for (;;) {
		if (entry->mActorType == 0)
			return;
		if (entry->mActorType == mActorType)
			break;
		entry += 1;
	}

	mMultiMtxEffect            = new TMultiMtxEffect;
	mMultiMtxEffect->mNumBones = entry->mNumBones;

	u16* boneIds = new u16[entry->mNumBones];
	u8* types    = new u8[entry->mNumBones];

	JUTNameTab* nameTab = getModel()->getModelData()->unkB0;
	for (int i = 0; i < entry->mNumBones; ++i) {
		boneIds[i] = nameTab->getIndex(entry->mBoneNames[i]);
		types[i]   = entry->mMtxType;
	}

	mMultiMtxEffect->mBoneIDs       = boneIds;
	mMultiMtxEffect->mMtxEffectType = types;
	mMultiMtxEffect->setup(getModel(), entry->mParamName);
	mMultiMtxEffect->flagOn();
}

void TBaseNPC::init(TLiveManager* manager)
{
	u32 idx            = mActorType - 0x4000001;
	mNpcSaveIndividual = gpConductor->unkF4->unk8[idx];

	if (manager == nullptr) {
		onLiveFlag(LIVE_FLAG_DEAD);
		initHitActor(mActorType, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
		unk64 |= 1;
		mSpine->initWith(&TNerveNPCWaitMarioApproach::theNerve());
		mTurnSpeed = mNpcSaveIndividual->mWaitTurnSpeed.value;
		gpConductor->registerAloneActor(this);
		return;
	}

	const TNpcInitInfo* initData = SMSGetNpcInitData(idx);
	mManager                     = manager;
	manager->manageActor(this);
	mMActorKeeper = new TMActorKeeper(manager);

	u32 keeperFlags = 0;
	if (mActorType == 0x400001D)
		keeperFlags = 3;
	mMActorKeeper->createMActorFromNthData(0, keeperFlags);
	if (manager->unk28 == 2)
		mMActorKeeper->createMActorFromNthData(1, 3);

	mMActor       = mMActorKeeper->mActors[0];
	mBodyScale    = 1.0f;
	mBodyRadius   = 10.0f;
	mMarchSpeed   = 0.0f;
	mWallRadius   = mScaling.x * initData->mAttackRadius;
	mHeadHeight   = mPtrSaveNormal->mSLHeadHeightNormal.value;
	mGravity      = mPtrSaveNormal->mGravityY.value;
	mScaledBodyRadius
	    = mScaling.x * mNpcSaveIndividual->mCircleShadowSize.value;

	if (mActorType == 0x400001D) {
		onLiveFlag(0x2018);
		initNpcObjCollision_(initData);
		mSpine->initWith(&TNerveNPCWaitMarioApproach::theNerve());
		mTurnSpeed = 0.0f;
		initNpcLight_();
		return;
	}

	onLiveFlag(LIVE_FLAG_UNK1000000);
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	onLiveFlag(LIVE_FLAG_UNK1000);

	if (gpMarDirector->mMap != 8
	    || (strcmp(mName, cNotUseFastCubeViewObjName0) != 0
	        && strcmp(mName, cNotUseFastCubeViewObjName1) != 0)) {
		onLiveFlag(LIVE_FLAG_UNK2000);
	}

	if (mActorType == 0x4000006)
		onLiveFlag(0x18);

	if (isJellyFishMare() || isSunflower() || mActorType == 0x4000007)
		onLiveFlag(LIVE_FLAG_UNK10);

	if (mMActor->unkC != nullptr)
		mMActor->unkC->initNormalMotionBlend();

	if (isPollutionNpc())
		initSinkNpc_();

	mAnmFrameCounter = new TNpcAnmFrameCounter;

	if (unk124->getGraph()->isDummy()) {
		mSpine->initWith(&TNerveNPCWaitMarioApproach::theNerve());
	} else {
		mSpine->initWith(&TNerveNPCGraphWander::theNerve());
		unk124->mPrevIdx = -1;
		goToShortestNextGraphNode();
	}

	initNpcObjCollision_(initData);
	setMtxEffect_();

	JUTNameTab* nameTab = getModel()->getModelData()->unkB0;
	mNpcKind            = nameTab->getIndex(cNeckJointName);
	if (mNpcKind != -1) {
		mNeckAngles = new TNpcNeckAngles;
		mMActor->setJointCallback(mNpcKind, NPCNeckCallBack);
	}

	setHappyEffectMtxPtr_(nameTab);

	if (mActorType == 0x4000009 || mActorType == 0x4000012)
		setNoteEffectMtxPtr_(nameTab);

	if (isPollutionNpc())
		setPollutionEffectMtxPtr_(nameTab);

	if (initData->unk0 != nullptr) {
		mSDLModel = SMS_CreateMinimumSDLModel(initData->unk0->unk0);
		JUTNameTab* matNameTab = mSDLModel->mModelData->unkB0;
		u16 matIdx             = matNameTab->getIndex(initData->unk0->unk4);
		mSDLMtx = (MtxPtr)((u8*)mSDLModel->mNodeMatrices + matIdx * 0x30);
	}

	initAnmSound();

	mUnk18C = new TNpcInbetween(
	    CLBPalFrame<long>(mPtrSaveNormal->mPosInbetweenFrame.value),
	    CLBPalFrame<long>(mPtrSaveNormal->mMotionBlendFrame.value));

	const TNpcInitAnmInfo* anmInfo = SMSGetNpcInitAnmData(idx);
	initLodAnm(anmInfo->unk0, 0, mNpcSaveIndividual->mLodChangeDist.value);

	initNpcLight_();
}

inline void TBaseNPC::initBaseActionFlag_()
{
	static const TAnmBckMapping sIndividualHoldArrowBck[] = {
		{ 0xE, 0x10 },
		{ -1, -1 },
	};
	static const TAnmBckMapping sIndividualKinopioBck[] = {
		{ 0xF, 0x10 }, { 6, 7 },        { 4, 5 },   { 0xE, 0x18 },
		{ 9, 0xB },    { 0x13, 0x14 },  { -1, -1 },
	};
	static const TAnmBtpMapping sIndividualKinopioBtp[] = {
		{ 1, 2 },
		{ 0, 5 },
		{ -1, -1 },
	};
	static const TAnmBckMapping sIndividualKinojiiBck[] = {
		{ 0xB, 0xC }, { 2, 3 },     { 0xA, 0x11 },
		{ 5, 7 },     { 0xD, 0xE }, { -1, -1 },
	};
	static const TAnmBtpMapping sIndividualKinojiiBtp[] = {
		{ 1, 2 },
		{ 0, 5 },
		{ -1, -1 },
	};

	if (isMonte()) {
		setMonteActionFlag_();
		if (checkActionFlag(0x400))
			unkD0->unk18 = sIndividualHoldArrowBck;
	} else if (isMare()) {
		setMareActionFlag_();
	} else if (mActorType == 0x4000016 || mActorType == 0x4000017) {
		setKinoActionFlag_();
		if (checkActionFlag(0x100)) {
			switch (mActorType) {
			case 0x4000016:
				unkD0->unk18 = sIndividualKinopioBck;
				unkD0->unk1C = sIndividualKinopioBtp;
				break;
			case 0x4000017:
				unkD0->unk18 = sIndividualKinojiiBck;
				unkD0->unk1C = sIndividualKinojiiBtp;
				break;
			}
		}
	} else {
		_16C        = 0;
		mActionFlag = 0;
	}
}

inline void TBaseNPC::initIndividualAnm_()
{
	static const TAnmBckMapping sIndividualParentRaccoonDogAnmBck[] = {
		{ 0, 1 },
		{ -1, -1 },
	};
	static const TAnmBckMapping sIndividualChildRaccoonDogAnmBck[] = {
		{ 0, 2 },
		{ -1, -1 },
	};
	static const TAnmBckMapping sIndividualMareMA0Bck[] = {
		{ 0, 3 },
		{ -1, -1 },
	};
	static const TAnmBtpMapping sIndividualMareMA0Btp[] = {
		{ 2, 3 },
		{ -1, -1 },
	};
	static const TAnmBckMapping sIndividualMareMA1Bck[] = {
		{ 0, 4 },
		{ -1, -1 },
	};
	static const TAnmBtpMapping sIndividualMareMA1Btp[] = {
		{ 2, 0 },
		{ -1, -1 },
	};
	static const TAnmBckMapping sIndividualMareWA0Bck[] = {
		{ 0, 3 },
	};

	switch (mActorType) {
	case 0x4000019:
		if (strcmp(mName, cManiyaParentViewObjName) == 0) {
			mActionFlag |= 0x800;
			unkD0->unk18 = sIndividualParentRaccoonDogAnmBck;
		} else if (strcmp(mName, cManiyaChildViewObjName) == 0) {
			mActionFlag |= 0x800;
			onLiveFlag(LIVE_FLAG_UNK10000);
			unkD0->unk18 = sIndividualChildRaccoonDogAnmBck;
		}
		break;
	case 0x400000F: {
		switch ((int)(MsRandF() * 3.0f)) {
		case 0:
			unkD0->unk18 = sIndividualMareMA0Bck;
			unkD0->unk1C = sIndividualMareMA0Btp;
			break;
		case 1:
			unkD0->unk18 = sIndividualMareMA1Bck;
			unkD0->unk1C = sIndividualMareMA1Btp;
			break;
		}
		break;
	}
	case 0x4000014: {
		switch ((int)(MsRandF() * 2.0f)) {
		case 0:
			unkD0->unk18 = sIndividualMareWA0Bck;
			break;
		}
		break;
	}
	}
}

void TBaseNPC::setIndividualDifference_(JSUMemoryInputStream& stream)
{
	u32 idx                            = mActorType - 0x4000001;
	const TNpcInitInfo* initData       = SMSGetNpcInitData(idx);
	const TNpcInitAnmInfo* initAnmData = SMSGetNpcInitAnmData(idx);

	J3DGXColorS10 colorData[2];
	for (int i = 0; i < 2; i++) {
		colorData[i].color.r = stream.readS32();
		colorData[i].color.g = stream.readS32();
		colorData[i].color.b = stream.readS32();
		colorData[i].color.a = 0xff;
	}

	if (isPollutionNpc()) {
		unk178               = colorData[0].color.b * (1.0f / 255.0f);
		u8 pollMax           = mNpcSaveIndividual->mPollutionMax.value;
		*((u8*)this + 0x177) = (u8)(s32)(unk178 * (f32)pollMax);
	}

	s32 streamS32a = stream.readS32();
	_16C           = stream.readS32();
	s32 streamS32c = stream.readS32();
	f32 fStream2   = (f32)stream.readS32();
	f32 fStream3   = (f32)stream.readS32();
	s32 sFlag      = stream.readU32();

	if (streamS32a < 0)
		streamS32a = 0;
	if (streamS32c < 0)
		streamS32c = 0;

	{
		int nColorEntries      = mManager->unk28;
		s16* indices           = &colorData[0].color.r;
		const GXColor* polColor = getPtrInitPollutionColor();

		for (int entryIdx = 0; entryIdx < nColorEntries; entryIdx++) {
			for (int slot = 0; slot < 2; slot++) {
				s16 idx = indices[slot];
				if (initData->unk34[slot][entryIdx] != nullptr) {
					SMS_InitChangeNpcColor(
					    mMActorKeeper->getMActor(entryIdx),
					    initData->unk34[slot][entryIdx], idx, polColor);
				}
			}
		}
	}

	if (getPtrInitPollutionColor() != nullptr) {
		J3DModel* model         = getModel();
		J3DModelData* modelData = model->getModelData();
		JUTNameTab* matNameTab  = modelData->getMaterialName();
		for (u16 i = 0, e = modelData->getMaterialNum(); i < e; i++) {
			if (strcmp(matNameTab->getName(i), cEyeMaterialName) != 0) {
				J3DMaterial* mat = modelData->getMaterialNodePointer(i);
				J3DShapePacket* shape
				    = model->getShapePacket(mat->getShape()->getIndex());
				if (shape->unkC == nullptr) {
					SMS_InitPacket_OneTevKColor(
					    model, i, GX_KCOLOR0, &unk174);
				}
			}
		}
	}

	if (mActorType == 0x4000018) {
		streamS32a |= 6;
		if ((streamS32a & 0x10) != 0) {
			streamS32a |= 0x60;
			peachParasolIn_();
		} else {
			streamS32a &= ~0x60;
		}
	}

	if (TFlagManager::smInstance->getBool(0x50003) && isSunflower())
		sunflowerDownIn_();

	if (streamS32a > 0)
		mNpcParts = new TNpcParts((u32)streamS32a, &colorData[1], this);

	if (initAnmData->unk4 != nullptr)
		unkD0->unk1C = initAnmData->unk4;

	initBaseActionFlag_();
	initIndividualAnm_();

	if ((streamS32c & 1) != 0)
		unk17C = new TNpcThrow(fStream2, fStream3);

	bool wantSmoke = isNormalMonteM() && (mActionFlag & 0x4000) != 0
	                   ? true
	                   : false;

	bool smokeAllowed = true;
	if (sFlag == 0x7d0 || sFlag == 0xc8
	    || (sFlag >= 0 && sFlag < 0x32)) {
		bool blueCoinTaken
		    = sFlag >= 0 && sFlag < 0x32
		          && TFlagManager::smInstance->getBlueCoinFlag(
		              gpApplication.mCurrArea.getStage(), (u8)sFlag)
		      ? true
		      : false;

		if (wantSmoke && blueCoinTaken) {
			mActionFlag &= ~0x4088;
			smokeAllowed = false;
		} else {
			if (blueCoinTaken)
				sFlag = 0x7d0;
			mNpcCoin = new TNpcCoin((s32)sFlag);
		}
	}

	if (wantSmoke && smokeAllowed) {
		bool sNeg = (streamS32a & 0x800) != 0;
		setSmokeEffectMtxPtr_(sNeg);
	}

	npcWaitIn();
	randomizeBckAndBtpFrame_();

	f32 frame = mMActor->getFrameCtrl(0)->getFrame();
	if (mNpcParts != nullptr) {
		if (isJellyFishMare())
			mNpcParts->addJellyFishParts(frame);
		mNpcParts->setPartsAnmFrame(frame);
	}
}
