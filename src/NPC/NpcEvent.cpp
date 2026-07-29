#define J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#define J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#define TSPINEBASE_GETLATESTNERVE_OUT_OF_LINE
#include <NPC/NpcEvent.hpp>
#include <stdio.h>
#include <MSound/MSoundBGM.hpp>
#include <NPC/NpcBase.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <Strategic/SpcInterp.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Nerve.hpp>
#include <Strategic/LiveActor.hpp>
#include <Strategic/HitActor.hpp>
#include <MoveBG/MapObjHide.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#undef J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#undef J3DMTXCALC_BASIC_INIT_OUT_OF_LINE

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeNameBasic[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeNameSoftimage[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeNameMotionBlend[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeNameUser[]
    = "MActorMtxCalcType_User ユーザー定義";

// TODO: removeme
extern const TNerveBase<TLiveActor>* NerveGetByIndex(int param_1);

static void evGetAddressFromViewObjName(TSpcTypedInterp<TEventWatcher>* interp,
                                        u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	const char* name = interp->pop().getDataString();
	JDrama::TNameRef* ref
	    = JDrama::TNameRefGen::search<JDrama::TNameRef>(name);
	interp->push(TSpcSlice((int)ref));
}

static void evCheckCurNerve4Npc(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int nerveIdx                          = interp->pop().getDataInt();
	TBaseNPC* npc                         = (TBaseNPC*)interp->pop().getDataInt();
	const TNerveBase<TLiveActor>* nerve   = NerveGetByIndex(nerveIdx);
	BOOL match                            = FALSE;
	if (npc->mSpine->getCurrentNerve() == nerve)
		match = TRUE;
	interp->push(TSpcSlice((int)match));
}

#pragma dont_inline on
template <>
TNerveBase<TLiveActor>* TSpineBase<TLiveActor>::getLatestNerve() const
{
	if (mCurrent)
		return mCurrent;
	return mPrevious;
}
#pragma dont_inline off

static void evCheckLatestNerve4Npc(TSpcTypedInterp<TEventWatcher>* interp,
                                   u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int nerveIdx                        = interp->pop().getDataInt();
	TBaseNPC* npc                       = (TBaseNPC*)interp->pop().getDataInt();
	const TNerveBase<TLiveActor>* nerve = NerveGetByIndex(nerveIdx);
	BOOL match                          = FALSE;
	if (npc->mSpine->getLatestNerve() == nerve)
		match = TRUE;
	interp->push(TSpcSlice((int)match));
}

static void evIsNpcSinkBottom(TSpcTypedInterp<TEventWatcher>* interp,
                              u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TBaseNPC* npc = (TBaseNPC*)interp->pop().getDataInt();
	BOOL isSink   = FALSE;
	if (npc->mLiveFlag & 0x800000)
		isSink = TRUE;
	interp->push(TSpcSlice((int)isSink));
}

static void evIsGameModeNormal(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	BOOL isNormal = FALSE;
	if (gpMarDirector->unk124 == 0)
		isNormal = TRUE;
	interp->push(TSpcSlice((int)isNormal));
}

static void ev__ForceStartTalk(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	bool block   = true;
	bool talking = block;
	BOOL result  = FALSE;
	if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
		talking = false;
	if (!talking) {
		bool inDemo = true;
		if (gpMarDirector->unk124 != 3 && gpMarDirector->unk124 != 4)
			inDemo = false;
		if (!inDemo)
			block = false;
	}
	if (!block && SMS_IsMarioTouchGround4cm()
	    && !(gpMarioOriginal->mAction & 0x800)) {
		TBaseNPC* npc = (TBaseNPC*)interp->pop().getDataInt();
		gpMarDirector->unkA0  = npc;
		gpMarDirector->unk126 = 1;
		result                = TRUE;
	} else {
		interp->pop();
	}
	interp->push(TSpcSlice((int)result));
}

static void ev__ForceStartTalkExceptNpc(TSpcTypedInterp<TEventWatcher>* interp,
                                        u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	BOOL result = FALSE;
	(void)interp->pop().mData.asString;
	bool block   = true;
	bool talking = block;
	if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
		talking = false;
	if (!talking) {
		bool inDemo = true;
		if (gpMarDirector->unk124 != 3 && gpMarDirector->unk124 != 4)
			inDemo = false;
		if (!inDemo)
			block = false;
	}
	if (!block && SMS_IsMarioTouchGround4cm()
	    && !(gpMarioOriginal->mAction & 0x800)) {
		TBaseNPC* dummy = JDrama::TNameRefGen::search<TBaseNPC>(
		    "\x83\x5F\x83\x7E\x81\x5B\x82\x6D\x82\x6F\x82\x62");
		if (dummy) {
			gpMarDirector->unkA0  = dummy;
			gpMarDirector->unk126 = 1;
			result                = TRUE;
		}
	}
	interp->push(TSpcSlice((int)result));
}

static void evConnectDummyNpc(TSpcTypedInterp<TEventWatcher>* interp,
                              u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	BOOL result = FALSE;
	TBaseNPC* dummyNpc
	    = JDrama::TNameRefGen::search<TBaseNPC>("\x83\x5F\x83\x7E\x81\x5B\x82\x6D\x82\x6F\x82\x62");
	if (dummyNpc) {
		JDrama::TActor* actor = (JDrama::TActor*)interp->pop().getDataInt();
		dummyNpc->setDummyConnectActor(actor);
		result = TRUE;
	} else {
		interp->pop();
	}
	interp->push(TSpcSlice((int)result));
}

static void evOnTalkToDummyNpc(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	TBaseNPC* npc
	    = JDrama::TNameRefGen::search<TBaseNPC>("\x83\x5F\x83\x7E\x81\x5B\x82\x6D\x82\x6F\x82\x62");
	if (npc) {
		npc->mLiveFlag &= ~1u;
		npc->mLiveFlag &= ~0x40000u;
		*(u16*)((u8*)npc + 0x1E0) = 0x78;
	}
	interp->push();
}

static void evSetNpcBalloonMessage(TSpcTypedInterp<TEventWatcher>* interp,
                                   u32 arg_num)
{
	interp->verifyArgNum(3, &arg_num);
	long arg2     = interp->pop().getDataInt();
	u32 arg1      = interp->pop().getDataInt();
	TBaseNPC* npc = (TBaseNPC*)interp->pop().getDataInt();
	npc->setBalloonMessage(arg1, arg2);
	interp->push();
}

static void evSetNpcTalkForbidCount(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	u16 count                 = (u16)interp->pop().getDataInt();
	TBaseNPC* npc             = (TBaseNPC*)interp->pop().getDataInt();
	*(u16*)((u8*)npc + 0x1E0) = count;
	interp->push();
}

static void evNpcDanceOn(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TBaseNPC* npc = (TBaseNPC*)interp->pop().getDataInt();
	npc->npcDanceIn();
	interp->push();
}

static void evNpcDanceOffHappyOn(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TBaseNPC* npc = (TBaseNPC*)interp->pop().getDataInt();
	npc->mActionFlag &= ~0x4u;
	npc->npcHappyIn(2);
	interp->push();
}

static void evResetFruitNum(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TFruitBasketEvent* basket
	    = (TFruitBasketEvent*)interp->pop().getDataInt();
	basket->reset();
	interp->push();
}

static void evGetFruitNum(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int fruitType = interp->pop().getDataInt();
	TFruitBasketEvent* basket
	    = (TFruitBasketEvent*)interp->pop().getDataInt();
	int idx;
	switch (fruitType) {
	case 0:
		idx = 0;
		break;
	case 1:
		idx = 4;
		break;
	case 2:
		idx = 3;
		break;
	case 3:
		idx = 1;
		break;
	default:
		idx = 0;
		break;
	}
	interp->push(TSpcSlice(basket->getFruitNum(idx)));
}

static void evSetFruitType(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(3, &arg_num);
	int doSet     = interp->pop().getDataInt();
	int fruitType = interp->pop().getDataInt();
	int target    = interp->pop().getDataInt();
	if (doSet != 0) {
		u32 actorType;
		switch (fruitType) {
		case 0:
			actorType = 0x40000394;
			break;
		case 1:
			actorType = 0x40000390;
			break;
		case 2:
			actorType = 0x40000392;
			break;
		case 3:
			actorType = 0x40000393;
			break;
		}
		*(u32*)((u8*)target + 0x150) = actorType;
	} else {
		*(u32*)((u8*)target + 0x150) = 0;
	}
	interp->push();
}

static void evFireStartDemoCamera(TSpcTypedInterp<TEventWatcher>* interp,
                                  u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	const char* name = interp->pop().getDataString();
	gpMarDirector->fireStartDemoCamera(
	    name, (const JGeometry::TVec3<f32>*)NULL, -1, 0.0f, true,
	    (s32(*)(u32, u32))NULL, 0, (JDrama::TActor*)NULL,
	    JDrama::TFlagT<u16>(0));
	interp->push();
}

static void evIsDemoMode(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	bool isDemo  = true;
	BOOL pushVal = FALSE;
	if (gpMarDirector->unk124 != 3 && gpMarDirector->unk124 != 4)
		isDemo = false;
	if (isDemo)
		pushVal = TRUE;
	interp->push(TSpcSlice((int)pushVal));
}

static void evCheckMonteClear(TSpcTypedInterp<TEventWatcher>* interp,
                              u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int idx = interp->pop().getDataInt();
	char buf[0x20];
	snprintf(buf, 0x20, "\x83\x82\x83\x93\x83\x65%d", idx); // "モンテ%d"
	TBaseNPC* npc = JDrama::TNameRefGen::search<TBaseNPC>(buf);
	BOOL clear;
	if (!(npc->mLiveFlag & 0x400000) && npc->unk178 == 0.0f)
		clear = TRUE;
	else
		clear = FALSE;
	interp->push(TSpcSlice((int)clear));
}

void TNpcEvent::initNpcBuiltin(TSpcTypedBinary<TEventWatcher>* binary)
{
	// clang-format off
	binary->bindSystemDataToSymbol("getAddressFromViewObjName", (u32)&evGetAddressFromViewObjName);
	binary->bindSystemDataToSymbol("checkCurNerve4Npc",         (u32)&evCheckCurNerve4Npc);
	binary->bindSystemDataToSymbol("checkLatestNerve4Npc",      (u32)&evCheckLatestNerve4Npc);
	binary->bindSystemDataToSymbol("isNpcSinkBottom",           (u32)&evIsNpcSinkBottom);
	binary->bindSystemDataToSymbol("isGameModeNormal",          (u32)&evIsGameModeNormal);
	binary->bindSystemDataToSymbol("__forceStartTalk",          (u32)&ev__ForceStartTalk);
	binary->bindSystemDataToSymbol("__forceStartTalkExceptNpc", (u32)&ev__ForceStartTalkExceptNpc);
	binary->bindSystemDataToSymbol("connectDummyNpc",           (u32)&evConnectDummyNpc);
	binary->bindSystemDataToSymbol("onTalkToDummyNpc",          (u32)&evOnTalkToDummyNpc);
	binary->bindSystemDataToSymbol("setNpcBalloonMessage",      (u32)&evSetNpcBalloonMessage);
	binary->bindSystemDataToSymbol("setNpcTalkForbidCount",     (u32)&evSetNpcTalkForbidCount);
	binary->bindSystemDataToSymbol("npcDanceOn",                (u32)&evNpcDanceOn);
	binary->bindSystemDataToSymbol("npcDanceOffHappyOn",        (u32)&evNpcDanceOffHappyOn);
	binary->bindSystemDataToSymbol("resetFruitNum",             (u32)&evResetFruitNum);
	binary->bindSystemDataToSymbol("getFruitNum",               (u32)&evGetFruitNum);
	binary->bindSystemDataToSymbol("setFruitType",              (u32)&evSetFruitType);
	binary->bindSystemDataToSymbol("fireStartDemoCamera",       (u32)&evFireStartDemoCamera);
	binary->bindSystemDataToSymbol("isDemoMode",                (u32)&evIsDemoMode);
	binary->bindSystemDataToSymbol("checkMonteClear",           (u32)&evCheckMonteClear);
	// clang-format on
}

void TNpcEvent::initDownSunflowerNum()
{
	if (TFlagManager::smInstance->getBool(0x50003)) {
		mDownSunflowerNum = 5;
	} else {
		mDownSunflowerNum = 0;
	}
}

static s32 ReviveSunflowerCallBack(u32 npc_u, u32 phase)
{
	if (phase == 0) {
		TBaseNPC* npc = (TBaseNPC*)npc_u;
		npc->sunflowerReviveIn();
		u32 soundID = 0x8808;
		if (TNpcEvent::mDownSunflowerNum == 0)
			soundID = 0x4847;
		if (gpMSound->gateCheck(soundID)) {
			MSoundSESystem::MSoundSE::startSoundNpcActor(
			    soundID, (const Vec*)((u8*)npc + 0x10), 0, (JAISound**)NULL, 0,
			    4);
		}
	}
	return 1;
}

static const char* sCameraNames[5] = {
	"\x82\xD0\x82\xDC\x82\xED\x82\xE8\x83\x4A\x83\x81\x83\x89\x30", // ひまわりカメラ0
	"\x82\xD0\x82\xDC\x82\xED\x82\xE8\x83\x4A\x83\x81\x83\x89\x31", // ひまわりカメラ1
	"\x82\xD0\x82\xDC\x82\xED\x82\xE8\x83\x4A\x83\x81\x83\x89\x32", // ひまわりカメラ2
	"\x82\xD0\x82\xDC\x82\xED\x82\xE8\x83\x4A\x83\x81\x83\x89\x33", // ひまわりカメラ3
	"\x82\xD0\x82\xDC\x82\xED\x82\xE8\x83\x4A\x83\x81\x83\x89\x34"  // ひまわりカメラ4
};

void TNpcEvent::reviveOneSunflower()
{
	s32 down = mDownSunflowerNum;
	if (down <= 0)
		return;

	static const char* sViewObjName
	    = "\x82\xD0\x82\xDC\x82\xED\x82\xE8"; // ひまわり

	int idx = 5 - down;
	char buf[0x40];
	snprintf(buf, 0x40, "%s%d", sViewObjName, idx);
	JDrama::TActor* actor = JDrama::TNameRefGen::search<JDrama::TActor>(buf);
	mDownSunflowerNum -= 1;

	JGeometry::TVec3<f32>* pos
	    = (JGeometry::TVec3<f32>*)((u8*)actor + 0x1B8);
	gpMarDirector->fireStartDemoCamera(sCameraNames[idx], pos, -1, 0.0f,
	                                   true, &ReviveSunflowerCallBack, 0,
	                                   actor, JDrama::TFlagT<u16>(0));

	if (mDownSunflowerNum == 0) {
		gpItemManager->makeShineAppearWithDemo(
		    "\x82\xD0\x82\xDC\x82\xED\x82\xE8\x97\x70\x83\x56\x83\x83\x83\x43"
		    "\x83\x93",
		    "\x82\xD0\x82\xDC\x82\xED\x82\xE8\x83\x56\x83\x83\x83\x43\x83\x93"
		    "\x83\x4A\x83\x81\x83\x89",
		    pos->x, 500.0f + pos->y, pos->z);
		TFlagManager::smInstance->setBool(false, 0x50003);
	}
}

s32 TNpcEvent::mDownSunflowerNum;
