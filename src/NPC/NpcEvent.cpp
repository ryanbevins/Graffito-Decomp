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
#include <Player/MarioMain.hpp>
#include <Player/MarioAccess.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>

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
	BOOL isNormal;
	if (gpMarDirector->unk124 == 0)
		isNormal = TRUE;
	else
		isNormal = FALSE;
	interp->push(TSpcSlice((int)isNormal));
}

static void ev__ForceStartTalk(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	BOOL block;
	if (gpMarDirector->unk124 == 1 || gpMarDirector->unk124 == 2)
		block = TRUE;
	else
		block = FALSE;
	if (!block) {
		if (gpMarDirector->unk124 == 3 || gpMarDirector->unk124 == 4)
			block = TRUE;
	}
	BOOL result = FALSE;
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
	BOOL block;
	if (gpMarDirector->unk124 == 1 || gpMarDirector->unk124 == 2)
		block = TRUE;
	else
		block = FALSE;
	if (!block) {
		if (gpMarDirector->unk124 == 3 || gpMarDirector->unk124 == 4)
			block = TRUE;
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
	TBaseNPC* dummyNpc
	    = JDrama::TNameRefGen::search<TBaseNPC>("\x83\x5F\x83\x7E\x81\x5B\x82\x6D\x82\x6F\x82\x62");
	BOOL result = FALSE;
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
	interp->pop();
	interp->pop();
	interp->pop();
	interp->push();
}

static void evSetNpcTalkForbidCount(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	interp->pop();
	interp->pop();
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
	interp->verifyArgNum(2, &arg_num);
	interp->pop();
	interp->pop();
	interp->push();
}

static void evFireStartDemoCamera(TSpcTypedInterp<TEventWatcher>* interp,
                                  u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	interp->pop();
	interp->pop();
	interp->push();
}

static void evIsDemoMode(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	BOOL isDemo;
	if (gpMarDirector->unk124 == 3)
		isDemo = TRUE;
	else if (gpMarDirector->unk124 == 4)
		isDemo = TRUE;
	else
		isDemo = FALSE;
	interp->push(TSpcSlice((int)isDemo));
}

static void evCheckMonteClear(TSpcTypedInterp<TEventWatcher>* interp,
                              u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int idx = interp->pop().getDataInt();
	char buf[0x20];
	snprintf(buf, 0x20, "\x83\x82\x83\x93\x83\x65%d", idx); // "モンテ%d"
	TBaseNPC* npc = JDrama::TNameRefGen::search<TBaseNPC>(buf);
	BOOL clear    = FALSE;
	if (!(npc->mLiveFlag & 0x400000) && npc->unk178 == 0.0f)
		clear = TRUE;
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

void TNpcEvent::reviveOneSunflower() { (void)ReviveSunflowerCallBack; }

u32 TNpcEvent::mDownSunflowerNum;
