#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <Enemy/Graph.hpp>
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
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcParts.hpp>
#include <NPC/NpcSave.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <dolphin/gx/GXStruct.h>
#include <string.h>

extern "C" int rand();

BOOL NPCNeckCallBack(J3DNode*, int);

const char* cManiyaParentViewObjName    = "マニヤ親゙゙ニ゙ヶ";
const char* cManiyaChildViewObjName     = "マニヤ子゙゙ニ゙ヶ";
const char* cNotUseFastCubeViewObjName0 = "ピンゲ26";
const char* cNotUseFastCubeViewObjName1 = "ピンゲ27";
const char* cEyeMaterialName            = "_eye_mat";
const char* cNeckJointName              = "kubi";

void SMS_InitChangeNpcColor(const MActor*, const TColorChangeInfo*, s16,
                            const GXColor*);

struct TPollutionStartHelper {
	/* 0x0 */ int mCounter;
	/* 0x4 */ int mMax;
};

struct TUnk22CStruct {
	/* 0x0 */ int unk0;
	/* 0x4 */ int unk4;
};

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
		mMActor->setLightType(0);
		if (checkLiveFlag(LIVE_FLAG_UNK10)) {
			mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
			    mPosition.x, mPosition.y + 1.0f, mPosition.z, &mGroundPlane);
		}
		return;
	}

	onLiveFlag(LIVE_FLAG_UNK100);
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	onLiveFlag(LIVE_FLAG_UNK1000);

	if (gpMarDirector->mMap == 8
	    && strcmp(mName, cNotUseFastCubeViewObjName0) != 0
	    && strcmp(mName, cNotUseFastCubeViewObjName1) != 0) {
		onLiveFlag(LIVE_FLAG_UNK2000);
	}

	if (mActorType == 0x4000006)
		onLiveFlag(0x18);

	if (isJellyFishMare() || isSunflower() || mActorType == 0x4000007)
		onLiveFlag(LIVE_FLAG_UNK10);

	if (mMActor->unkC != nullptr)
		mMActor->unkC->initNormalMotionBlend();

	if (isPollutionNpc()) {
		static int sCheckPollutedStartCounter = 0;
		int max  = CLBPalFrame<int>(30);
		TPollutionStartHelper* h = new TPollutionStartHelper;
		if (h != nullptr) {
			h->mCounter = sCheckPollutedStartCounter;
			h->mMax     = max;
		}
		mPollutionStartHelper = h;
		sCheckPollutedStartCounter++;
		if (sCheckPollutedStartCounter >= max)
			sCheckPollutedStartCounter = 0;
	}

	TUnk22CStruct* st22C = new TUnk22CStruct;
	if (st22C != nullptr) {
		st22C->unk0 = 0;
		st22C->unk4 = 1;
	}
	*(TUnk22CStruct**)((u8*)this + 0x22C) = st22C;

	if (unk124->getGraph()->isDummy()) {
		mSpine->initWith(&TNerveNPCWaitMarioApproach::theNerve());
	} else {
		mSpine->initWith(&TNerveNPCGraphWander::theNerve());
		unk124->mCurrIdx = -1;
		goToShortestNextGraphNode();
	}

	initNpcObjCollision_(initData);

	{
		static const char* sWaistJointName = "koshi_null";
		static const TMtxEffectInitDataEntry sMtxEffectInitData[]
		    = { { 0x04000001, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000002, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000003, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000004, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000005, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000007, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000008, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x04000009, &sWaistJointName, "Npc/MonteM", 0, 1, { 0, 0 } },
			    { 0x0400000A, &sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
			    { 0x0400000B, &sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
			    { 0x0400000C, &sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
			    { 0x0400000D, &sWaistJointName, "Npc/MonteW", 0, 1, { 0, 0 } },
			    { 0x04000018, &sWaistJointName, "Npc/Peach", 0, 1, { 0, 0 } },
			    { 0, nullptr, nullptr, 0, 0, { 0, 0 } } };

		const TMtxEffectInitDataEntry* entry = sMtxEffectInitData;
		while (entry->mActorType != 0) {
			if (entry->mActorType == mActorType) {
				mMultiMtxEffect             = new TMultiMtxEffect;
				mMultiMtxEffect->mNumBones  = entry->mNumBones;
				u16* boneIds                = new u16[entry->mNumBones];
				u8* types                   = new u8[entry->mNumBones];
				JUTNameTab* nameTab = getModel()->getModelData()->unkB0;
				for (int i = 0; i < entry->mNumBones; i++) {
					boneIds[i] = nameTab->getIndex(entry->mBoneNames[i]);
					types[i]   = entry->mMtxType;
				}
				mMultiMtxEffect->mBoneIDs        = boneIds;
				mMultiMtxEffect->mMtxEffectType  = types;
				mMultiMtxEffect->setup(getModel(), entry->mParamName);
				mMultiMtxEffect->flagOn();
				break;
			}
			entry++;
		}
	}

	JUTNameTab* nameTab = getModel()->getModelData()->unkB0;
	mNpcKind            = nameTab->getIndex(cNeckJointName);
	if (mNpcKind != -1) {
		s16* angles = new s16[2];
		if (angles != nullptr) {
			angles[0] = 0;
			angles[1] = 0;
		}
		mNeckAngles = angles;
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
		mSDLMtx = (Mtx*)((u8*)mSDLModel->mNodeMatrices + matIdx * 0x30);
	}

	initAnmSound();

	TUnk18CStruct* unk18c = new TUnk18CStruct;
	if (unk18c != nullptr) {
		unk18c->unk4
		    = CLBPalFrame<long>(mPtrSaveNormal->mMotionBlendFrame.value);
		unk18c->unk0
		    = CLBPalFrame<long>(mPtrSaveNormal->mPosInbetweenFrame.value);
		unk18c->unk8  = 0;
		unk18c->unkC  = 0.0f;
		unk18c->unk10 = 0.0f;
		unk18c->unk14 = 0.0f;
		unk18c->unk18 = 0.0f;
		unk18c->unk1C = 0.0f;
		unk18c->unk20 = 0.0f;
		unk18c->unk24 = 0;
		unk18c->unk28 = 0.0f;
	}
	mUnk18C = unk18c;

	const TNpcInitAnmInfo* anmInfo = SMSGetNpcInitAnmData(idx);
	initLodAnm(anmInfo->unk0, 0, mNpcSaveIndividual->mLodChangeDist.value);

	mMActor->setLightType(1);

	if (checkLiveFlag(LIVE_FLAG_UNK10)) {
		mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
		    mPosition.x, mPosition.y + 1.0f, mPosition.z, &mGroundPlane);
	}
}

void TBaseNPC::setIndividualDifference_(JSUMemoryInputStream& stream)
{
	u32 idx                            = mActorType - 0x4000001;
	const TNpcInitInfo* initData       = SMSGetNpcInitData(idx);
	const TNpcInitAnmInfo* initAnmData = SMSGetNpcInitAnmData(idx);

	s16 colorData[8];
	for (int i = 0; i < 2; i++) {
		s32 r, g, b;
		stream.read(&r, 4);
		colorData[i * 4 + 0] = (s16)r;
		stream.read(&g, 4);
		colorData[i * 4 + 1] = (s16)g;
		stream.read(&b, 4);
		colorData[i * 4 + 2] = (s16)b;
		colorData[i * 4 + 3] = (s16)0xff;
	}

	if (isPollutionNpc()) {
		unk178               = colorData[0] * (1.0f / 255.0f);
		u8 pollMax           = mNpcSaveIndividual->mPollutionMax.value;
		*((u8*)this + 0x177) = (u8)(s32)(unk178 * (f32)pollMax);
	}

	s32 streamS32a;
	stream.read(&streamS32a, 4);
	s32 streamS32b;
	stream.read(&streamS32b, 4);
	_16C = streamS32b;
	s32 streamS32c;
	stream.read(&streamS32c, 4);
	s32 streamS32d;
	stream.read(&streamS32d, 4);
	f32 fStream2 = (f32)streamS32d;
	s32 streamS32e;
	stream.read(&streamS32e, 4);
	f32 fStream3 = (f32)streamS32e;
	s32 sFlag;
	stream.read(&sFlag, 4);

	if (streamS32a < 0)
		streamS32a = 0;
	if (streamS32c < 0)
		streamS32c = 0;

	u32 nColorEntries       = mManager->unk28;
	const GXColor* polColor = getPtrInitPollutionColor();

	for (s32 entryIdx = 0; entryIdx < (s32)nColorEntries; entryIdx++) {
		for (int slot = 0; slot < 2; slot++) {
			const TColorChangeInfo* info
			    = initData->unk34[slot][entryIdx];
			s16 idx = ((s16*)colorData)[slot];
			if (info != nullptr) {
				SMS_InitChangeNpcColor(
				    mMActorKeeper->mActors[entryIdx], info, idx, polColor);
			}
		}
	}

	if (getPtrInitPollutionColor() != nullptr) {
		J3DModel* model         = getModel();
		J3DModelData* modelData = model->mModelData;
		JUTNameTab* matNameTab  = *(JUTNameTab**)((u8*)modelData + 0xB4);
		u16 numMaterials        = modelData->mMaterialNum;
		for (u16 i = 0; i < numMaterials; i++) {
			const char* matName = matNameTab->getName(i);
			if (strcmp(matName, cEyeMaterialName) == 0) {
				J3DMaterial* mat   = modelData->mMaterials[i];
				void* sub          = *(void**)((u8*)mat + 0x4);
				u16 matIdx         = *(u16*)((u8*)sub + 0x4);
				void* matPacketArr = *(void**)((u8*)model + 0x84);
				void* matPacket
				    = *(void**)((u8*)matPacketArr + matIdx * 0x34 + 0xC);
				if (matPacket == NULL) {
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
		}
	} else {
		streamS32a &= ~0x60;
	}

	if (TFlagManager::smInstance->getBool(0x50003) && isSunflower())
		sunflowerDownIn_();

	if (streamS32a > 0)
		mNpcParts = new TNpcParts((u32)streamS32a,
		                          (const J3DGXColorS10*)&colorData[4], this);

	if (initAnmData->unk4 != nullptr)
		*(const TAnmBtpMapping**)((u8*)unkD0 + 0x1C) = initAnmData->unk4;

	{
		if (isNormalMonteM() || isNormalMonteW() || isSpecialMonteM()
		    || isSpecialMonteW()) {
			setMonteActionFlag_();
			if ((mActionFlag & 0x400) != 0) {
				static const s32 sIndividualHoldArrowBck[]
				    = { 0xE, 0x10, -1, -1 };
				*(const s32**)((u8*)unkD0 + 0x18) = sIndividualHoldArrowBck;
			}
		} else if (isNormalMareM() || isNormalMareW() || isSpecialMareM()
		           || isSpecialMareW()) {
			setMareActionFlag_();
		} else if (mActorType == 0x4000016 || mActorType == 0x4000017) {
			setKinoActionFlag_();
			if ((mActionFlag & 0x100) != 0) {
				switch (mActorType) {
				case 0x4000017: {
					static const s32 sIndividualKinojiiBck[]
					    = { 0xB, 0xC, 2, 3, 0xA, 0x11,
						    5,   7,   0xD, 0xE, -1,  -1 };
					static const s32 sIndividualKinojiiBtp[]
					    = { 1, 2, 0, 5, -1, -1 };
					*(const s32**)((u8*)unkD0 + 0x18) = sIndividualKinojiiBck;
					*(const s32**)((u8*)unkD0 + 0x1C) = sIndividualKinojiiBtp;
					break;
				}
				case 0x4000016: {
					static const s32 sIndividualKinopioBck[]
					    = { 0xF, 0x10, 6, 7,    4,    5, 0xE, 0x18,
						    9,   0xB,  0x13, 0x14, -1,   -1 };
					static const s32 sIndividualKinopioBtp[]
					    = { 1, 2, 0, 5, -1, -1 };
					*(const s32**)((u8*)unkD0 + 0x18) = sIndividualKinopioBck;
					*(const s32**)((u8*)unkD0 + 0x1C) = sIndividualKinopioBtp;
					break;
				}
				}
			}
		} else {
			_16C        = 0;
			mActionFlag = 0;
		}
	}

	switch (mActorType) {
	case 0x4000014: {
		f32 r   = (f32)rand() * (1.0f / 32768.0f);
		s32 sel = (s32)(2.0f * r);
		if (sel == 0) {
			static const s32 sIndividualMareWA0Bck[] = { 0, 3 };
			*(const s32**)((u8*)unkD0 + 0x18) = sIndividualMareWA0Bck;
		}
		break;
	}
	case 0x400000F: {
		f32 r   = (f32)rand() * (1.0f / 32768.0f);
		s32 sel = (s32)(3.0f * r);
		if (sel == 1) {
			static const s32 sIndividualMareMA1Bck[] = { 0, 4, -1, -1 };
			static const s32 sIndividualMareMA1Btp[] = { 2, 0, -1, -1 };
			*(const s32**)((u8*)unkD0 + 0x18) = sIndividualMareMA1Bck;
			*(const s32**)((u8*)unkD0 + 0x1C) = sIndividualMareMA1Btp;
		} else if (sel == 0) {
			static const s32 sIndividualMareMA0Bck[] = { 0, 3, -1, -1 };
			static const s32 sIndividualMareMA0Btp[] = { 2, 3, -1, -1 };
			*(const s32**)((u8*)unkD0 + 0x18) = sIndividualMareMA0Bck;
			*(const s32**)((u8*)unkD0 + 0x1C) = sIndividualMareMA0Btp;
		}
		break;
	}
	case 0x4000019:
		if (strcmp(mName, cManiyaParentViewObjName) == 0) {
			static const s32 sIndividualParentRaccoonDogAnmBck[]
			    = { 0, 1, -1, -1 };
			mActionFlag |= 0x800;
			*(const s32**)((u8*)unkD0 + 0x18)
			    = sIndividualParentRaccoonDogAnmBck;
		} else if (strcmp(mName, cManiyaChildViewObjName) == 0) {
			static const s32 sIndividualChildRaccoonDogAnmBck[]
			    = { 0, 2, -1, -1 };
			mActionFlag |= 0x800;
			onLiveFlag(LIVE_FLAG_UNK10000);
			*(const s32**)((u8*)unkD0 + 0x18)
			    = sIndividualChildRaccoonDogAnmBck;
		}
		break;
	}

	if ((streamS32c & 1) != 0) {
		struct TwoFloats {
			f32 a;
			f32 b;
		};
		TwoFloats* p = new TwoFloats;
		if (p != nullptr) {
			p->a = fStream2;
			p->b = fStream3;
		}
		*((TwoFloats**)((u8*)this + 0x17C)) = p;
	}

	bool wantSmoke = false;
	if (isNormalMonteM() && (mActionFlag & 0x4000) != 0)
		wantSmoke = true;

	bool smokeAllowed = true;
	if (sFlag == 0x7d0 || sFlag == 0xc8
	    || (sFlag >= 0 && sFlag < 0x32)) {
		bool isInRange      = (sFlag >= 0 && sFlag < 0x32);
		bool blueCoinTaken  = false;
		if (isInRange) {
			if (TFlagManager::smInstance->getBlueCoinFlag(
			        gpApplication.mCurrArea.getStage(), (u8)sFlag)) {
				blueCoinTaken = true;
			}
		}
		bool wantCoin = !blueCoinTaken;
		if (wantSmoke && wantCoin) {
			mActionFlag &= ~0x4088;
			smokeAllowed = false;
		} else {
			if (wantCoin)
				sFlag = 0x7d0;
			mNpcCoin = new TNpcCoin((s32)sFlag);
		}
	}

	if (wantSmoke && smokeAllowed) {
		bool sNeg = ((streamS32a >> 11) & 1) != 0;
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
