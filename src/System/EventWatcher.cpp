// This TU owns the weak out-of-line copy of JGeometry::TVec3<f32>::set(const
// Vec&). Several other TUs (CameraWarp, lensflare, sunmodel, bosstelesa)
// reference it but none emit it, so it must be defined here.
#define JGEOMETRY_EVENTWATCHER_TVEC3_SET_VEC_OUT_OF_LINE
#define JGEOMETRY_TVEC3_ADD_OUT_OF_LINE
#define J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#define J3DMTXCALC_MAYA_INIT_OUT_OF_LINE

// TUtil<f32>::sqrt is called out-of-line in this TU (owner lives elsewhere).
#define JG_TUTIL_SQRT_OUT_OF_LINE

#include <System/EventWatcher.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/LiveActor.hpp>
#include <M3DUtil/MActor.hpp>

class TEMario : public TLiveActor {
public:
	BOOL isGoal();
	bool isDownWaitingToTalk() const;
	bool isReachedToGate() const;
	void startMonteReplay(u32);
	void forceDisappear();
	void startGateDrawing();
	void startRunAway();
};
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSoundBGM.hpp>
#include <GC2D/Talk2D2.hpp>
#include <GC2D/GCConsole2.hpp>
#include <GC2D/ConsoleStr.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcEvent.hpp>
#include <Map/MapEventSink.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/PollutionLayer.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/ModelGate.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/MapObjItem2.hpp>
#include <MoveBG/MapObjBall.hpp>
#include <Enemy/Conductor.hpp>
#include <Player/MarioMain.hpp>
#include <Player/Watergun.hpp>
#include <Player/MarioAccess.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <GC2D/SunGlass.hpp>
#include <JSystem/JMath.hpp>
#include <System/EmitterViewObj.hpp>
#include <MoveBG/MapObjTown.hpp>

// rogue includes needed for matching sinit & bss
#include <M3DUtil/InfectiousStrings.hpp>

#undef J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#undef J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#undef JGEOMETRY_EVENTWATCHER_TVEC3_SET_VEC_OUT_OF_LINE
#undef JGEOMETRY_TVEC3_ADD_OUT_OF_LINE

#pragma dont_inline on
namespace JGeometry {
void TVec3<f32>::set(const Vec& v)
{
	x = v.x;
	y = v.y;
	z = v.z;
}
}
#pragma dont_inline off

// TODO: from M3UJoint or J3DJoint?
static void dummy()
{
	(Vec) { 0.0f, 0.0f, 0.0f };
	(Vec) { 1.0f, 1.0f, 1.0f };
}

// Owned by this TU (weak): a trace/debug helper that is stripped to an empty
// body in the release build. Several other TUs reference it but only this TU
// emits the definition, so it must live here for the project to link.
void SpcTrace(const char*, ...) { }

template <class T> inline T* get_name_ref(TSpcSlice slice)
{
	T* result = nullptr;

	switch (slice.typeof()) {
	case TSpcSlice::TYPE_STRING:
		result = JDrama::TNameRefGen::search<T>(slice.getDataString());
		break;

	case TSpcSlice::TYPE_INT:
		result = (T*)slice.getDataInt();
		break;
	}

	return result;
}

static void evGetSystemFlag(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TSpcSlice idSlice = interp->pop();
	int id            = idSlice.getDataInt();

	interp->push((int)TFlagManager::getInstance()->getFlag(id));
}

static void evSetSystemFlag(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	TSpcSlice valueSlice = interp->pop();
	TSpcSlice flagSlice  = interp->pop();
	int flag             = flagSlice.getDataInt();
	int value            = valueSlice.getDataInt();

	TFlagManager::getInstance()->setFlag(flag, value);

	interp->push();
}

static void evGetNameRefHandle(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);

	JDrama::TNameRef* ref = JDrama::TNameRefGen::search<JDrama::TNameRef>(
	    interp->pop().getDataString());

	interp->push((int)ref);
}

static void evGetNameRefName(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);

	int ref = interp->pop().getDataInt();

	const char* name = ref ? ((JDrama::TNameRef*)ref)->getName() : "";

	TSpcSlice slice;
	slice.setDataString(name);

	interp->push(slice);
}

static void getNameRefPtr(TSpcSlice) { }

static void evGetNPCType(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int type        = -1;
	THitActor* hit  = get_name_ref<THitActor>(interp->pop());
	if (hit)
		type = hit->mActorType - 0x04000001;
	interp->push(type);
}

static void evSetFlagNPCDontTalk(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	bool value      = interp->pop().getDataInt() != 0;
	TLiveActor* npc = get_name_ref<TLiveActor>(interp->pop());
	if (npc) {
		if (value)
			npc->onLiveFlag(LIVE_FLAG_UNK10000);
		else
			npc->offLiveFlag(LIVE_FLAG_UNK10000);
	}
	interp->push();
}

static void evSetFlagNPCDontThrow(TSpcTypedInterp<TEventWatcher>* interp,
                                  u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	bool value      = interp->pop().getDataInt() != 0;
	TLiveActor* npc = get_name_ref<TLiveActor>(interp->pop());
	if (npc) {
		if (value)
			npc->mLiveFlag |= 0x20000000;
		else
			npc->mLiveFlag &= ~0x20000000;
	}
	interp->push();
}

static void evSetFlagNPCDead(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	bool value      = interp->pop().getDataInt() != 0;
	TLiveActor* npc = get_name_ref<TLiveActor>(interp->pop());
	if (npc) {
		if (value)
			npc->onLiveFlag(LIVE_FLAG_DEAD);
		else
			npc->offLiveFlag(LIVE_FLAG_DEAD);
	}
	interp->push();
}

static void evIsNearSameActors(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(3, &arg_num);

	TLiveActor* base = get_name_ref<TLiveActor>(interp->pop());
	if (!base) {
		interp->push(0);
		return;
	}

	u32 actorType      = base->mActorType;
	f32 radius         = interp->pop().getDataFloat();
	TLiveActor* center = get_name_ref<TLiveActor>(interp->pop());
	if (!center) {
		interp->push(0);
		return;
	}

	int count = 0;
	int num   = gpMapObjManager->getObjNum();
	for (int i = 0; i < num; ++i) {
		THitActor* o = gpMapObjManager->getObj(i);
		if (o->mActorType == actorType) {
			JGeometry::TVec3<f32> diff = center->mPosition;
			diff.sub(o->mPosition);
			if (JGeometry::TUtil<f32>::sqrt(diff.squared()) <= radius)
				++count;
		}
	}

	interp->push(count);
}

static void evIsNearActors(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	int count = 0;
	if (arg_num >= 3) {
		TLiveActor* base = get_name_ref<TLiveActor>(
		    interp->mProcessStack
		        .mData[interp->mProcessStack.mSize - (arg_num - 0)]);
		if (base) {
			f32 radius = interp->mProcessStack
			                 .mData[(interp->mProcessStack.mSize - 1)
			                        - (arg_num - 2)]
			                 .getDataFloat();
			count = 1;
			for (u32 i = 2; i < arg_num; ++i) {
				TLiveActor* other = get_name_ref<TLiveActor>(
				    interp->mProcessStack
				        .mData[interp->mProcessStack.mSize
				               - (arg_num - i)]);
				if (other) {
					JGeometry::TVec3<f32> diff = base->mPosition;
					diff.sub(other->mPosition);
					if (JGeometry::TUtil<f32>::sqrt(diff.squared())
					    <= radius)
						++count;
				}
			}
		}
	}

	for (int i = 0; i < (int)arg_num; ++i)
		interp->pop();
	interp->push(count);
}

static void evGetTalkNPC(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);

	TBaseNPC* npc = gpMarDirector->getTalkingNPC();

	interp->push(!npc ? 0 : (int)npc);
}

static void evGetTalkNPCName(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);

	TBaseNPC* npc = gpMarDirector->getTalkingNPC();

	if (!npc) {
		const char* name = "";
		TSpcSlice slice;
		slice.setDataString(name);
		interp->push(slice);
	} else {
		const char* name = npc->getName();
		TSpcSlice slice;
		slice.setDataString(name);
		interp->push(slice);
	}
}

static void evSetTalkMsgID(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int p1 = TSpcSlice(interp->pop()).getDataInt();
	int p2 = TSpcSlice(interp->pop()).getDataInt();
	gpTalk2D->setMessageID(p2, p1);
	interp->push();
}

static void evGetTalkMode(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push((int)gpTalk2D->getTalkMode());
}

static void evGetTalkSelectedValue(TSpcTypedInterp<TEventWatcher>* interp,
                                   u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push((int)gpTalk2D->getSelectedValue());
}

static void evSetValue2TalkVariable(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	interp->pop();
	interp->pop();
	interp->push();
}

static void evIsTalkModeNow(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	int value = gpMarDirector->isTalkModeNow() ? 1 : 0;
	interp->push(value);
}

static void evSetFlagNPCCanTaken(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int value        = TSpcSlice(interp->pop()).getDataInt();
	const char* name = interp->pop().getDataString();
	TLiveActor* npc  = JDrama::TNameRefGen::search<TLiveActor>(name);
	if (npc) {
		if (value)
			npc->onLiveFlag(LIVE_FLAG_UNK100000);
		else
			npc->offLiveFlag(LIVE_FLAG_UNK100000);
	}
	interp->push();
}

// TODO: removeme
extern const TNerveBase<TLiveActor>* NerveGetByIndex(int param_1);

static void evPushNerve4LiveActor(TSpcTypedInterp<TEventWatcher>* interp,
                                  u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	TSpcSlice nerveSlice                = interp->pop();
	int nerveId                         = nerveSlice.getDataInt();
	const TNerveBase<TLiveActor>* nerve = NerveGetByIndex(nerveId);
	const char* actorName               = interp->pop().getDataString();

	TLiveActor* liveActor = JDrama::TNameRefGen::search<TLiveActor>(actorName);
	if (liveActor && nerve)
		liveActor->mSpine->pushNerve(nerve);

	interp->push();
}

static void evIsOnLiveActorFlag(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int flag = TSpcSlice(interp->pop()).getDataInt();

	TLiveActor* liveActor = get_name_ref<TLiveActor>(interp->pop());

	int result = 0;
	if (liveActor)
		result = liveActor->mLiveFlag & flag;
	interp->push(result);
}

static void evSetHide4LiveActor(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int value             = TSpcSlice(interp->pop()).getDataInt();
	const char* actorName = interp->pop().getDataString();

	TLiveActor* liveActor = JDrama::TNameRefGen::search<TLiveActor>(actorName);
	if (liveActor) {
		if (value) {
			liveActor->onLiveFlag(LIVE_FLAG_HIDDEN);
			liveActor->onHitFlag(HIT_FLAG_NO_COLLISION);
		} else {
			liveActor->offLiveFlag(LIVE_FLAG_HIDDEN);
			liveActor->offHitFlag(HIT_FLAG_NO_COLLISION);
		}
	}

	interp->push();
}

static void evSetDead4LiveActor(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int value             = TSpcSlice(interp->pop()).getDataInt();
	const char* actorName = interp->pop().getDataString();

	TLiveActor* liveActor = JDrama::TNameRefGen::search<TLiveActor>(actorName);
	if (liveActor) {
		if (value) {
			liveActor->onLiveFlag(LIVE_FLAG_DEAD);
			liveActor->onHitFlag(HIT_FLAG_NO_COLLISION);
		} else {
			liveActor->offLiveFlag(LIVE_FLAG_DEAD);
			liveActor->offHitFlag(HIT_FLAG_NO_COLLISION);
		}
	}

	interp->push();
}

static void evSetTimeLimit(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int time = TSpcSlice(interp->pop()).getDataInt();
	OSResetStopwatch(&gpMarDirector->unkE8);
	gpMarDirector->unk120 = time;
	interp->push();
}

static void evSetAttentionTime(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int tmp = interp->pop().getDataInt();

	// not implemented?

	interp->push();
}

static void evSetPollutionIncreaseCount(TSpcTypedInterp<TEventWatcher>* interp,
                                        u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int tmp = interp->pop().getDataInt();

	// not implemented?

	interp->push();
}

static void evGetRestTime(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push(gpMarDirector->getRestTime());
}

static void evGetPollutionLevel(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push((int)gpPollution->getPollutionDegree());
}

static void evSetEventStart(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
}

static void evSetEventEnd(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
}

static void evSetNextStage(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int scenario = TSpcSlice(interp->pop()).getDataInt();
	int stage    = TSpcSlice(interp->pop()).getDataInt();

	gpMarDirector->setNextStage((scenario & 0xff) + ((stage + 1) << 8),
	                            nullptr);

	interp->push();
}

static void evRegisterMovie(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int movieId = TSpcSlice(interp->pop()).getDataInt();
	gpMarDirector->fireStreamingMovie(movieId);
	interp->push();
}

static void evGameOver(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarDirector->onUnk4CFlag(0x1);
	interp->push();
}

static void evIsGraffitoCoverage0(TSpcTypedInterp<TEventWatcher>* interp,
                                  u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push(gpPollution->cleanedAll() ? 1 : 0);
}

static void evSetGraffitoMultiplied(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int value = TSpcSlice(interp->pop()).getDataInt();
	TPollutionManager* pollution = gpPollution;
	int i                        = 0;
	if (value != 0) {
		for (; i < pollution->getJointModelNum(); ++i)
			((TPollutionLayer*)pollution->getJointModel(i))->unk32 |= 1;
	} else {
		for (; i < pollution->getJointModelNum(); ++i)
			((TPollutionLayer*)pollution->getJointModel(i))->unk32 &= ~1;
	}
	interp->push();
}

static void evIsBossDefeated(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push(gpConductor->isBossDefeated() ? 1 : 0);
}

static void evLaunchEventClearDemo(TSpcTypedInterp<TEventWatcher>* interp,
                                   u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	TGCConsole2* console = gpMarDirector->getConsole();
	console->unk94->startAppearShineGet();
	console->unk34[0x13] = 1;
	interp->push();
}

static void evIsEMarioReachedToGoal(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEMario* m = get_name_ref<TEMario>(interp->pop());
	interp->push((int)m->isReachedToGate());
}

static void evIsEMarioDownWaitingToTalk(TSpcTypedInterp<TEventWatcher>* interp,
                                        u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEMario* m = get_name_ref<TEMario>(interp->pop());
	interp->push((int)m->isDownWaitingToTalk());
}

static void evStartEMarioRunAway(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEMario* m = get_name_ref<TEMario>(interp->pop());
	m->startRunAway();
	interp->push();
}

static void evStartEMarioGateDrawing(TSpcTypedInterp<TEventWatcher>* interp,
                                     u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEMario* m = get_name_ref<TEMario>(interp->pop());
	m->startGateDrawing();
	interp->push();
}

static void evStartEMarioDisappear(TSpcTypedInterp<TEventWatcher>* interp,
                                   u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEMario* m = get_name_ref<TEMario>(interp->pop());
	m->forceDisappear();
	interp->push();
}

static void evStartOpenModelGate(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TModelGate* gate = get_name_ref<TModelGate>(interp->pop());
	gate->startOpen();
	interp->push();
}

static void evIsMapEventFinishedAll(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TMapEvent* event = get_name_ref<TMapEvent>(interp->pop());
	interp->push(event->isFinishedAll());
}

static void evRaiseBuilding(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);

	int id = TSpcSlice(interp->pop()).getDataInt();

	TMapEventSinkShadowMario* event
	    = JDrama::TNameRefGen::search<TMapEventSinkShadowMario>(
	        "イベント（カゲマリオゲート）");

	if (event)
		event->raiseBuilding(id);

	interp->push();
}

static void evForceCloseTalk(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);

	gpTalk2D->forceCloseTalk();

	interp->push();
}

static void evInsertTimer(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);

	int p1 = interp->pop().getDataInt();
	int p2 = interp->pop().getDataInt();

	if (p2 == 1)
		gpMarDirector->getConsole()->startAppearTimer(0, p1);
	else if (p2 == 2)
		gpMarDirector->getConsole()->startAppearTimer(1, p1);
	else
		gpMarDirector->getConsole()->startDisappearTimer();

	interp->push();
}

static void evStartTimer(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);

	int time = interp->pop().getDataInt();

	gpMarDirector->startTimer();
	gpMarDirector->getConsole()->startMoveTimer(time);

	interp->push();
}

static void evStartMonteman(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEMario* mario = JDrama::TNameRefGen::search<TEMario>("モンテマン");
	u32 value      = TSpcSlice(interp->pop()).getDataInt();
	if (mario)
		mario->startMonteReplay(value);
	interp->push();
}

static void evStopTimer(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarDirector->getConsole()->stopMoveTimer();
	interp->push();
}

static void evMonteManReachFlag(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	int result = 0;
	interp->verifyArgNum(0, &arg_num);
	TEMario* mario = JDrama::TNameRefGen::search<TEMario>("モンテマン");
	if ((u8)mario->isGoal())
		result = 1;
	interp->push(result);
}

static void evGetTime(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push(gpMarDirector->getConsole()->getFinishedTime());
}

static void evKillShine(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TShine* shine = get_name_ref<TShine>(interp->pop());
	shine->kill();
	interp->push();
}

static void evKillMushroom1up(TSpcTypedInterp<TEventWatcher>* interp,
                              u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	get_name_ref<TMushroom1up>(interp->pop())->kill();
	interp->push();
}

static void evAppearMushroom1up(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TMushroom1up* mushroom = get_name_ref<TMushroom1up>(interp->pop());
	mushroom->appear();
	SMSGetMSound()->startSoundSystemSE(0x4854, 0, nullptr, 0);
	interp->push();
}

static void evAppearShineFromNPC(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(3, &arg_num);
	const char* src       = interp->pop().getDataString();
	TSpcSlice npcSlice    = interp->pop();
	const char* shineName = interp->pop().getDataString();
	TLiveActor* npc       = get_name_ref<TLiveActor>(npcSlice);

	if (strcmp(src, "") != 0) {
		gpItemManager->makeShineAppearWithDemo(shineName, src,
		                                       npc->mPosition.x,
		                                       npc->mPosition.y,
		                                       npc->mPosition.z);
	} else {
		TShine* shine = JDrama::TNameRefGen::search<TShine>(shineName);
		shine->mInitialPosition = npc->mPosition;
		shine->mPosition        = npc->mPosition;
		shine->appearWithTime(1200, -1, -1, -1);
	}

	interp->push();
}

static void evAppearShine(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);

	const char* p1 = interp->pop().getDataString();
	const char* p2 = interp->pop().getDataString();

	if (strcmp(p1, "") != 0) {
		gpItemManager->makeShineAppearWithDemoOffset(p2, p1, 0.0f, 0.0f, 0.0f);
	} else {
		TShine* shine = JDrama::TNameRefGen::search<TShine>(p2);
		shine->appearWithTime(1200, -1, -1, -1);
	}
	interp->push();
}

static void evAppearShineFromNPCWithoutDemo(TSpcTypedInterp<TEventWatcher>* interp,
                                            u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	TSpcSlice fromSlice = interp->pop();
	const char* dstName = interp->pop().getDataString();
	TLiveActor* from = get_name_ref<TLiveActor>(fromSlice);
	TShine* shine    = JDrama::TNameRefGen::search<TShine>(dstName);
	shine->mPosition.set(from->mPosition);
	shine->makeObjAppeared();
	interp->push();
}

static void evAppearShineFromKageMario(TSpcTypedInterp<TEventWatcher>* interp,
                                       u32 arg_num)
{
	interp->verifyArgNum(3, &arg_num);
	int shineNum     = interp->pop().getDataInt();
	const char* src  = interp->pop().getDataString();
	const char* dst  = interp->pop().getDataString();
	TLiveActor* from = JDrama::TNameRefGen::search<TLiveActor>(src);
	TShine* shine    = JDrama::TNameRefGen::search<TShine>(dst);
	shine->mPosition = from->mPosition;
	shine->appearSimple(shineNum);
	interp->push();
}

static void evAppearShineForWoodBox(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);

	int index = interp->pop().getDataInt();

	if (index) // huh?
		index = 1;

	static const char* sShineViewObjName[] = {
		"木箱ゲーム用シャイン１",
		"木箱ゲーム用シャイン２",
	};

	gpItemManager->makeShineAppearWithDemo(sShineViewObjName[index],
	                                       "木箱ゲーム用シャインカメラ",
	                                       -4010.0f, 9850.0f, -4040.0f);

	interp->push();
}

static void evChangeNozzle(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TWaterGun::TNozzleType id
	    = (TWaterGun::TNozzleType)interp->pop().getDataInt();
	if (id == 7)
		gpMarioOriginal->setDivHelm();
	else
		gpMarioOriginal->mWaterGun->changeNozzle(id, true);
	interp->push();
}

static void evStartMarioTalking(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarioOriginal->startTalking();
	interp->push();
}

static void evCheckWoodBox(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int p1 = interp->pop().getDataInt();
	int p2 = interp->pop().getDataInt();

	int count = p2 - p1 + 1;

	char buffer[] = "ゲーム木箱00";
	for (int i = p2; i <= p1; ++i) {
		if (i < 10) {
			buffer[10] = '0' + i;
			buffer[11] = 0;
		} else {
			buffer[10] = '0' + i / 10;
			buffer[11] = '0' + i % 10;
		}
		TMapObjBase* obj = JDrama::TNameRefGen::search<TMapObjBase>(buffer);
		if (obj && obj->checkLiveFlag(LIVE_FLAG_DEAD))
			--count;
	}

	interp->push(count);
}

static void evRefreshWoodBox(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int p1 = interp->pop().getDataInt();
	int p2 = interp->pop().getDataInt();

	char buffer[] = "ゲーム木箱00";
	for (int i = p2; i <= p1; ++i) {
		if (i < 10) {
			buffer[10] = '0' + i;
			buffer[11] = 0;
		} else {
			buffer[10] = '0' + i / 10;
			buffer[11] = '0' + i % 10;
		}
		TMapObjBase* obj = JDrama::TNameRefGen::search<TMapObjBase>(buffer);
		if (obj)
			obj->appear();
	}

	interp->push();
}

static void evKillWoodBox(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int p1 = interp->pop().getDataInt();
	int p2 = interp->pop().getDataInt();

	char buffer[] = "ゲーム木箱00";
	for (int i = p2; i <= p1; ++i) {
		if (i < 10) {
			buffer[10] = '0' + i;
			buffer[11] = 0;
		} else {
			buffer[10] = '0' + i / 10;
			buffer[11] = '0' + i % 10;
		}
		TMapObjBase* obj = JDrama::TNameRefGen::search<TMapObjBase>(buffer);
		if (obj)
			obj->makeObjDead();
	}

	interp->push();
}

static void evIsInsideCube(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int cubeId = interp->pop().getDataInt();

	// TODO: getPos10cmAbove or something like that?
	JGeometry::TVec3<f32> pos = gpMarioOriginal->mPosition;
	pos.y += 10.0f;

	interp->push(gpCubeArea->isInCube(pos, cubeId) ? 1 : 0);
}

static void evSetMarioWaiting(TSpcTypedInterp<TEventWatcher>* interp,
                              u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarioOriginal->changePlayerStatus(0xC400201, 0, true);
	interp->push();
}

static void evStartMareBottleDemo(TSpcTypedInterp<TEventWatcher>* interp,
                                  u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	TLiveActor* obj = JDrama::TNameRefGen::search<TLiveActor>("ＥＸビン");
	obj->getMActor()->setBck("exbottle_bottle_in");
	TMario* mario = gpMarioOriginal;
	mario->mPosition = obj->mPosition;
	mario->changePlayerStatus(0x1310, 0, true);
	interp->push();
}

static void evIsFinishMareBottleDemo(TSpcTypedInterp<TEventWatcher>* interp,
                                     u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	interp->push(JDrama::TNameRefGen::search<TLiveActor>("ＥＸビン")
	                     ->getMActor()
	                     ->curAnmEndsNext(0, nullptr)
	                 ? 1
	                 : 0);
}

static void evIsInsideFastCube(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int p1 = interp->pop().getDataInt();
	int p2 = interp->pop().getDataInt();

	// TODO: getPos10cmAbove or something like that?
	JGeometry::TVec3<f32> pos = gpMarioOriginal->mPosition;
	pos.y += 10.0f;

	int value;
	switch (p2) {
	case 0:
		value = gpCubeFastA->isInCube(pos, p1) ? 1 : 0;
		break;
	case 1:
		value = gpCubeFastB->isInCube(pos, p1) ? 1 : 0;
		break;
	case 2:
		value = gpCubeFastC->isInCube(pos, p1) ? 1 : 0;
		break;
	default:
		value = 0;
		break;
	}

	interp->push(value);
}

static void evSetTransScale(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(7, &arg_num);
	f32 tz = interp->pop().getDataFloat();
	f32 ty = interp->pop().getDataFloat();
	f32 tx = interp->pop().getDataFloat();
	f32 sz = interp->pop().getDataFloat();
	f32 sy = interp->pop().getDataFloat();
	f32 sx = interp->pop().getDataFloat();

	TMapObjBase* obj = get_name_ref<TMapObjBase>(interp->pop());

	obj->makeObjAppeared();
	JGeometry::TVec3<f32> scale(sx, sy, sz);
	JGeometry::TVec3<f32> rotation(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> translation(tx, ty, tz);
	obj->changeObjSRT(scale, rotation, translation);

	interp->push();
}

static void evSetEventID(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	u16 p1          = interp->pop().getDataInt();
	// TODO: type unconfirmed
	TMapObjBase* event = get_name_ref<TMapObjBase>(interp->pop());
	event->unk134      = p1;
	interp->push();
}

static void evManiCoinDown(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarDirector->getConsole()->startAppearStar();
	interp->push();
}

static void evStartBGM(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	MSBgm::startBGM(interp->pop().getDataInt());
	interp->push(TSpcSlice());
}

static void evEggYoshiStartFruit(TSpcTypedInterp<TEventWatcher>* interp,
                                 u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TEggYoshi* egg = get_name_ref<TEggYoshi>(interp->pop());
	if (!egg->checkLiveFlag(LIVE_FLAG_DEAD))
		egg->startFruit();
	interp->push();
}

static void evPutNozzle(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TItemNozzle* nozzle = get_name_ref<TItemNozzle>(interp->pop());
	nozzle->put();
	interp->push();
}

static void evStopBGM(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	MSBgm::stopBGM(interp->pop().getDataInt(), 10);
	interp->push(TSpcSlice());
}

static void evStartSE(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	SMSGetMSound()->startSoundSystemSE((u32)interp->pop().getDataInt(), 0,
	                                   nullptr, 0);
	interp->push(TSpcSlice());
}

static void evStartEventSE(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int se;
	switch (interp->pop().getDataInt()) {
	case 0:
		se = 0x4842;
		break;
	case 1:
		se = 0x484f;
		break;
	}
	SMSGetMSound()->startSoundSystemSE(se, 0, nullptr, 0);
	interp->push(TSpcSlice());
}

static void evStartMiss(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarioOriginal->loserExec();
	interp->push();
}

static void evChangeSunglass(TSpcTypedInterp<TEventWatcher>* interp,
                             u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int value = interp->pop().getDataInt();
	TSunGlass* glass
	    = JDrama::TNameRefGen::search<TSunGlass>("サングラスフェーダ");
	if (value == 0) {
		glass->startFade(2, true);
		gpMarioOriginal->wearGlass();
		if (TFlagManager::smInstance->getShineFlag(0x77))
			gpMarioOriginal->mState |= 0x100000;
	} else {
		glass->startFade(3, true);
		gpMarioOriginal->takeOffGlass();
		if (TFlagManager::smInstance->getShineFlag(0x77))
			gpMarioOriginal->mState &= ~0x100000;
	}
	interp->push();
}

static void evSetCollision(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);
	int value = interp->pop().getDataInt();
	THitActor* hitActor = get_name_ref<THitActor>(interp->pop());

	if (!value)
		hitActor->onHitFlag(HIT_FLAG_NO_COLLISION);
	else
		hitActor->offHitFlag(HIT_FLAG_NO_COLLISION);

	interp->push();
}

static void evWarpMario(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(4, &arg_num);
	int angle = interp->pop().getDataInt();
	int z     = interp->pop().getDataInt();
	int y     = interp->pop().getDataInt();
	int x     = interp->pop().getDataInt();
	SMS_MarioWarpRequest(JGeometry::TVec3<f32>((f32)x, (f32)y, (f32)z),
	                     (f32)angle);
	interp->push();
}

static void evStartAppearJetBalloon(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(2, &arg_num);

	int p1 = interp->pop().getDataInt();
	int p2 = interp->pop().getDataInt();

	switch (p2) {
	case 0:
		if (p1 == 1)
			gpMarDirector->getConsole()->startAppearJetBalloon(0, 8);
		break;

	case 1:
		if (p1 == 1)
			gpMarDirector->getConsole()->startAppearJetBalloon(1, 10);
		break;

	case 2:
		if (p1 == 1)
			gpMarDirector->getConsole()->startAppearRedCoin();
		break;
	}

	interp->push();
}

static void evSetEventForWaterMelon(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	TBigWatermelon* melon = get_name_ref<TBigWatermelon>(interp->pop());
	melon->startEvent();
	interp->push();
}

static void evAppearReadyGo(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarDirector->getConsole()->unk94->startAppearReady();
	interp->push();
}

static void evAppear8RedCoinsAndTimer(TSpcTypedInterp<TEventWatcher>* interp,
                                      u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);

	const char* switchName = "赤コイン用スイッチ";
	TRedCoinSwitch* sw
	    = JDrama::TNameRefGen::search<TRedCoinSwitch>(switchName);
	TCoinRed* coin;
	int time = sw->unk138;

	for (int i = 0; i < 8; ++i) {
		coin = (TCoinRed*)gpItemManager->makeObjAppeared(0x2000000f);
		coin->killByTimer(time - coin->unk150);

		f32 y = 70.0f + coin->mPosition.y;
		f32 z = coin->mPosition.z;
		f32 x = coin->mPosition.x;
		coin->unk158 = x;
		coin->unk15C = y;
		coin->unk160 = z;

		MtxPtr nodeMtx = coin->getModel()->mNodeMatrices[0];
		gpMarioParticleManager->emitAndBindToMtxPtr(0x58, nodeMtx, 0, coin);
		const JGeometry::TVec3<f32>* emitPos
		    = (const JGeometry::TVec3<f32>*)&coin->unk158;
		gpMarioParticleManager->emit(0xE5, emitPos, 0, nullptr);
		gpMarioParticleManager->emit(0xE6, emitPos, 0, nullptr);
	}

	gpMarDirector->getConsole()->startAppearTimer(
	    1, (s32)((f32)time * (1.0f / 120.0f)));
	gpMarDirector->startTimer();
	gpMarDirector->getConsole()->startMoveTimer(10);

	interp->push();
}

static void evWarpFrontToMario(TSpcTypedInterp<TEventWatcher>* interp,
                               u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	THitActor* actor = (THitActor*)interp->pop().getDataInt();

	s16 angle = *gpMarioAngleY;

	Vec front;
	front.x = 0.0f;
	front.y = 0.0f;
	front.z = 400.0f;

	u16 trigAngle = *gpMarioAngleY;
	f32 oldX = front.x;
	front.x = front.z * JMASSin(trigAngle) + oldX * JMASCos(trigAngle);
	f32 zCos = front.z * JMASCos(trigAngle);
	front.z = zCos + -oldX * JMASSin(trigAngle);

	JGeometry::TVec3<f32> offset;
	offset.set(front);

	JGeometry::TVec3<f32> pos = *gpMarioPos;
	pos.add(offset);

	actor->mPosition = pos;
	actor->mRotation.y = SHORTANGLE2DEG((f32)(s16)(angle - 0x8000));

	interp->push();
}

static void evOnNeutralMarioKey(TSpcTypedInterp<TEventWatcher>* interp,
                                u32 arg_num)
{
	interp->verifyArgNum(0, &arg_num);
	gpMarDirector->unk18[0]->onNeutralMarioKey();
	interp->push();
}

static void evInvalidatePad(TSpcTypedInterp<TEventWatcher>* interp, u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	int frames = interp->pop().getDataInt();

	gpMarDirector->unk18[0]->mDisabledFrames = frames;

	interp->push();
}

static void evIsWaterMelonIsReached(TSpcTypedInterp<TEventWatcher>* interp,
                                    u32 arg_num)
{
	interp->verifyArgNum(1, &arg_num);
	THitActor* actor = (THitActor*)interp->pop().getDataInt();
	int result       = 0;
	f32 dx           = -4660.0f - actor->mPosition.x;
	f32 dz           = 12000.0f - actor->mPosition.z;
	f32 distSq        = 0.0f;
	distSq += dx * dx;
	distSq += dz * dz;
	if (distSq <= 90000.0f)
		result = 1;
	interp->push(result);
}

template <> void TSpcTypedBinary<TEventWatcher>::initUserBuiltin()
{
	// clang-format off
  bindSystemDataToSymbol("getSystemFlag", (u32)&evGetSystemFlag);
  bindSystemDataToSymbol("setSystemFlag", (u32)&evSetSystemFlag);
  bindSystemDataToSymbol("getNameRefHandle", (u32)&evGetNameRefHandle);
  bindSystemDataToSymbol("getNameRefName", (u32)&evGetNameRefName);
  bindSystemDataToSymbol("getNPCType", (u32)&evGetNPCType);
  bindSystemDataToSymbol("setFlagNPCDontTalk", (u32)&evSetFlagNPCDontTalk);
  bindSystemDataToSymbol("setFlagNPCDontThrow", (u32)&evSetFlagNPCDontThrow);
  bindSystemDataToSymbol("setFlagNPCDead", (u32)&evSetFlagNPCDead);
  bindSystemDataToSymbol("isNearSameActors", (u32)&evIsNearSameActors);
  bindSystemDataToSymbol("isNearActors", (u32)&evIsNearActors);
  bindSystemDataToSymbol("getTalkNPC", (u32)&evGetTalkNPC);
  bindSystemDataToSymbol("getTalkNPCName", (u32)&evGetTalkNPCName);
  bindSystemDataToSymbol("setTalkMsgID", (u32)&evSetTalkMsgID);
  bindSystemDataToSymbol("getTalkMode", (u32)&evGetTalkMode);
  bindSystemDataToSymbol("getTalkSelectedValue", (u32)&evGetTalkSelectedValue);
  bindSystemDataToSymbol("setValue2TalkVariable", (u32)&evSetValue2TalkVariable);
  bindSystemDataToSymbol("isTalkModeNow", (u32)&evIsTalkModeNow);
  bindSystemDataToSymbol("setFlagNPCCanTaken", (u32)&evSetFlagNPCCanTaken);
  bindSystemDataToSymbol("pushNerve4LiveActor", (u32)&evPushNerve4LiveActor);
  bindSystemDataToSymbol("isOnLiveActorFlag", (u32)&evIsOnLiveActorFlag);
  bindSystemDataToSymbol("setHide4LiveActor", (u32)&evSetHide4LiveActor);
  bindSystemDataToSymbol("setDead4LiveActor", (u32)&evSetDead4LiveActor);
  bindSystemDataToSymbol("setTimeLimit", (u32)&evSetTimeLimit);
  bindSystemDataToSymbol("setAttentionTime", (u32)&evSetAttentionTime);
  bindSystemDataToSymbol("setPollutionIncreaseCount", (u32)&evSetPollutionIncreaseCount);
  bindSystemDataToSymbol("getRestTime", (u32)&evGetRestTime);
  bindSystemDataToSymbol("getPollutionLevel", (u32)&evGetPollutionLevel);
  bindSystemDataToSymbol("setNextStage", (u32)&evSetNextStage);
  bindSystemDataToSymbol("registerMovie", (u32)&evRegisterMovie);
  bindSystemDataToSymbol("gameOver", (u32)&evGameOver);
  bindSystemDataToSymbol("isGraffitoCoverage0", (u32)&evIsGraffitoCoverage0);
  bindSystemDataToSymbol("setGraffitoMultiplied", (u32)&evSetGraffitoMultiplied);
  bindSystemDataToSymbol("isBossDefeated", (u32)&evIsBossDefeated);
  bindSystemDataToSymbol("launchEventClearDemo", (u32)&evLaunchEventClearDemo);
  bindSystemDataToSymbol("isEMarioReachedToGoal", (u32)&evIsEMarioReachedToGoal);
  bindSystemDataToSymbol("isEMarioDownWaitingToTalk", (u32)&evIsEMarioDownWaitingToTalk);
  bindSystemDataToSymbol("startEMarioRunAway", (u32)&evStartEMarioRunAway);
  bindSystemDataToSymbol("startEMarioGateDrawing", (u32)&evStartEMarioGateDrawing);
  bindSystemDataToSymbol("startEMarioDisappear", (u32)&evStartEMarioDisappear);
  bindSystemDataToSymbol("startOpenModelGate", (u32)&evStartOpenModelGate);
  bindSystemDataToSymbol("isMapEventFinishedAll", (u32)&evIsMapEventFinishedAll);
  bindSystemDataToSymbol("raiseBuilding", (u32)&evRaiseBuilding);
  bindSystemDataToSymbol("forceCloseTalk", (u32)&evForceCloseTalk);
  bindSystemDataToSymbol("insertTimer", (u32)&evInsertTimer);
  bindSystemDataToSymbol("startTimer", (u32)&evStartTimer);
  bindSystemDataToSymbol("startMonteman", (u32)&evStartMonteman);
  bindSystemDataToSymbol("stopTimer", (u32)&evStopTimer);
  bindSystemDataToSymbol("monteManReachFlag", (u32)&evMonteManReachFlag);
  bindSystemDataToSymbol("getTime", (u32)&evGetTime);
  bindSystemDataToSymbol("killShine", (u32)&evKillShine);
  bindSystemDataToSymbol("killMushroom1up", (u32)&evKillMushroom1up);
  bindSystemDataToSymbol("appearMushroom1up", (u32)&evAppearMushroom1up);
  bindSystemDataToSymbol("appearShineFromNPC", (u32)&evAppearShineFromNPC);
  bindSystemDataToSymbol("appearShineFromNPCWithoutDemo", (u32)&evAppearShineFromNPCWithoutDemo);
  bindSystemDataToSymbol("appearShineFromKageMario", (u32)&evAppearShineFromKageMario);
  bindSystemDataToSymbol("appearShine", (u32)&evAppearShine);
  bindSystemDataToSymbol("appearShineForWoodBox", (u32)&evAppearShineForWoodBox);
  bindSystemDataToSymbol("changeNozzle", (u32)&evChangeNozzle);
  bindSystemDataToSymbol("startMarioTalking", (u32)&evStartMarioTalking);
  bindSystemDataToSymbol("isInsideCube", (u32)&evIsInsideCube);
  bindSystemDataToSymbol("setMarioWaiting", (u32)&evSetMarioWaiting);
  bindSystemDataToSymbol("setTransScale", (u32)&evSetTransScale);
  bindSystemDataToSymbol("setEventID", (u32)&evSetEventID);
  bindSystemDataToSymbol("startBGM", (u32)&evStartBGM);
  bindSystemDataToSymbol("stopBGM", (u32)&evStopBGM);
  bindSystemDataToSymbol("startMiss", (u32)&evStartMiss);
  bindSystemDataToSymbol("startSE", (u32)&evStartSE);
  bindSystemDataToSymbol("startEventSE", (u32)&evStartEventSE);
  bindSystemDataToSymbol("changeSunglass", (u32)&evChangeSunglass);
  bindSystemDataToSymbol("setCollision", (u32)&evSetCollision);
  bindSystemDataToSymbol("warpMario", (u32)&evWarpMario);
  bindSystemDataToSymbol("startAppearJetBalloon", (u32)&evStartAppearJetBalloon);
  bindSystemDataToSymbol("appear8RedCoinsAndTimer", (u32)&evAppear8RedCoinsAndTimer);
  bindSystemDataToSymbol("warpFrontToMario", (u32)&evWarpFrontToMario);
  bindSystemDataToSymbol("appearReadyGo", (u32)&evAppearReadyGo);
  bindSystemDataToSymbol("onNeutralMarioKey", (u32)&evOnNeutralMarioKey);
  bindSystemDataToSymbol("invalidatePad", (u32)&evInvalidatePad);
  bindSystemDataToSymbol("checkWoodBox", (u32)&evCheckWoodBox);
  bindSystemDataToSymbol("refreshWoodBox", (u32)&evRefreshWoodBox);
  bindSystemDataToSymbol("killWoodBox", (u32)&evKillWoodBox);
  bindSystemDataToSymbol("maniCoinFallDown", (u32)&evManiCoinDown);
  bindSystemDataToSymbol("eggYoshiStartFruit", (u32)&evEggYoshiStartFruit);
  bindSystemDataToSymbol("putNozzle", (u32)&evPutNozzle);
  bindSystemDataToSymbol("startMareBottleDemo", (u32)&evStartMareBottleDemo);
  bindSystemDataToSymbol("isFinishMareBottleDemo", (u32)&evIsFinishMareBottleDemo);
  bindSystemDataToSymbol("isInsideFastCube", (u32)&evIsInsideFastCube);
  bindSystemDataToSymbol("setEventForWaterMelon", (u32)&evSetEventForWaterMelon);
  bindSystemDataToSymbol("isWaterMelonIsReached", (u32)&evIsWaterMelonIsReached);
	// clang-format on
	TNpcEvent::initNpcBuiltin(this);
}

TEventWatcher::TEventWatcher(const char* name)
    : JDrama::TViewObj(name)
    , mBinary(nullptr)
    , mInterp(nullptr)
{
}

TEventWatcher::TEventWatcher(const char* name, const char* script)
    : JDrama::TViewObj(name)
    , mBinary(nullptr)
    , mInterp(nullptr)
{
	launchScript(script);
}

void TEventWatcher::launchScript(const char* script)
{
	if (void* res = JKRGetResource(script)) {
		mBinary = new TSpcTypedBinary<TEventWatcher>(res);
		mBinary->init();
		mInterp = new TSpcTypedInterp<TEventWatcher>(mBinary, this, 0x20, 0x20,
		                                             0x20, 0x20);
	}
}

void TEventWatcher::perform(u32 param_1, JDrama::TGraphics*)
{
	if ((param_1 & 1) && mInterp)
		mInterp->update();
}
