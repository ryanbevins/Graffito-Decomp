#include "Enemy/PathNode.hpp"
#include "Strategic/SolidStack.hpp"
#include <Enemy/Graph.hpp>
#include <M3DUtil/LodAnm.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Strategic/Spine.hpp>
#include <NPC/NpcNerve.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcSave.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarDirector.hpp>
#include <stdlib.h>

template <class T> T CLBSquared(T);

static inline bool isDownNpcBlocked(TBaseNPC* npc)
{
	bool blocked = false;
	if (npc->mActorType - 0x04000000 == 0x18 && (npc->unk1D8 & 0x2))
		blocked = true;
	return blocked;
}

static inline bool isSunflowerBlocked(TBaseNPC* npc)
{
	bool blocked = false;
	if (npc->isSunflower() && (npc->unk1D8 & 0x2))
		blocked = true;
	return blocked;
}

static inline bool isSinkOnlyNpc(TBaseNPC* npc)
{
	return npc->mActorType == 0x0400001C || npc->mActorType == 0x0400001D;
}

DEFINE_NERVE(TNerveNPCGraphWander, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->mAnmFrameCounter->resetRandom(
		    TBaseNPC::mPtrSaveNormal->mSLGraphWanderMinFrame.get(),
		    TBaseNPC::mPtrSaveNormal->mSLGraphWanderMaxFrame.get());
	}

	npc->execWalk(true);

	JGeometry::TVec3<f32> goal = npc->unkF4.getPoint();
	JGeometry::TVec3<f32> diff(goal.x - npc->mPosition.x, 0.0f,
	                           goal.z - npc->mPosition.z);
	f32 distSq = diff.squared();

	if (npc->unk114.size() != 0) {
		if (distSq < CLBSquared<f32>(100.0f)) {
			npc->unkF4 = npc->unk114.pop();
		}
		return FALSE;
	}

	bool oneWay = false;
	bool markTurnDir = false;
	const TGraphTracer* tracer = npc->getTracer();
	int curIndex              = tracer->getCurGraphIndex();
	const TGraphWeb* graph    = tracer->getGraph();
	const TGraphNode& node    = graph->unk0[curIndex];
	if (node.getRailNode()->mConnectionNum == 1) {
		oneWay = true;
		if (npc->getTracer()
		        ->getGraph()
		        ->unk0[npc->getTracer()->mCurrIdx]
		        .getRailNode()
		        ->mPitch
		    == 0) {
			markTurnDir = true;
		}
	}

	bool counterDone = npc->mAnmFrameCounter->advance();

	if (oneWay) {
		if (!(distSq < CLBSquared<f32>(50.0f)))
			return FALSE;
	} else {
		if (!(distSq < CLBSquared<f32>(100.0f)))
			return FALSE;
	}

	if (!(npc->mActionFlag & 0x80) && counterDone) {
		spine->pushAfterCurrent(&TNerveNPCGraphWait::theNerve());
		return TRUE;
	}

	npc->goToShortestNextGraphNode();
	if (oneWay) {
		spine->pushAfterCurrent(&TNerveNPCUTurn::theNerve());
		if (markTurnDir)
			npc->mLiveFlag |= 0x200000;
		return TRUE;
	}

	return FALSE;
}

#pragma dont_inline on
TGraphWeb* TGraphTracer::getGraph() const { return unk0; }

int TGraphTracer::getCurGraphIndex() const { return mCurrIdx; }
#pragma dont_inline off

DEFINE_NERVE(TNerveNPCUTurn, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (npc->execUTurn()) {
		npc->mMarchSpeed = 0.0f;
		npc->mLiveFlag &= ~0x200000;
		spine->pushAfterCurrent(&TNerveNPCGraphWander::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveNPCGraphWait, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->mAnmFrameCounter->resetRandom(
		    TBaseNPC::mPtrSaveNormal->mSLGraphWaitMinFrame.get(),
		    TBaseNPC::mPtrSaveNormal->mSLGraphWaitMaxFrame.get());
	}

	if (npc->mMarchSpeed < 0.001f) {
		if (npc->mAnmFrameCounter->advance()) {
			spine->pushAfterCurrent(&TNerveNPCGraphWander::theNerve());
			return TRUE;
		}
	} else {
		npc->execWalk(false);
	}
	return FALSE;
}

DEFINE_NERVE(TNerveNPCWaitContinue, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		if (npc->mHolder != nullptr)
			npc->npcTakenIn();
		else
			npc->npcWaitIn();
	}
	return FALSE;
}

static inline void execCommonWaitApproach(TBaseNPC* npc,
                                          TSpineBase<TLiveActor>* spine,
                                          bool nearMario)
{
	u32 actorType = npc->mActorType;
	if (actorType == 0x0400001C || actorType == 0x0400001D)
		return;
	if (actorType - 0x04000000 == 0x18 && (npc->unk1D8 & 0x2))
		return;
	if (isSunflowerBlocked(npc)) {
		npc->sunflowerReviving();
		return;
	}
	if (actorType == 0x04000006) {
		if (nearMario)
			npc->monteMESetAnmWhenNear();
		else
			npc->monteMESetAnmWhenFar();
		npc->execTurnToFirstState();
		return;
	}

	if (nearMario && npc->isTurnToMarioWhenApproach()) {
		SMS_GoRotate(npc->mPosition, *gpMarioPos, npc->mTurnSpeed,
		             &npc->mRotation.y);

		JGeometry::TVec3<f32> diff = *gpMarioPos;
		diff.x -= npc->mPosition.x;
		diff.y -= npc->mPosition.y;
		diff.z -= npc->mPosition.z;
		JGeometry::TVec3<f32> dir = diff;
		f32 targetYaw;
		if (dir.z == 0.0f) {
			if (dir.x >= 0.0f)
				targetYaw = 90.0f;
			else
				targetYaw = -90.0f;
		} else if (dir.z >= 0.0f) {
			targetYaw = (360.0f / 65536.0f) * matan(dir.z, dir.x);
		} else {
			f32 theta = matan(-dir.z, dir.x) * (360.0f / 65536.0f);
			targetYaw = 180.0f - theta;
		}

		f32 delta = npc->mRotation.y - targetYaw;
		if (delta < 0.0f)
			delta = -delta;
		while (delta >= 360.0f)
			delta -= 360.0f;
		while (delta < 0.0f)
			delta += 360.0f;

		if (delta < 0.001f)
			npc->npcWaitIn();
		else
			npc->npcStepIn();
		return;
	}

	if (npc->isNeedTurnToFirstState()) {
		if (npc->execTurnToFirstState())
			npc->npcWaitIn();
		else
			npc->npcStepIn();
		return;
	}

	if (spine->getTime() == 0)
		npc->npcWaitIn();
}

DEFINE_NERVE(TNerveNPCWaitMarioApproach, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (npc->isInBodyTurnSearchRange() && npc->unk178 == 0.0f) {
		spine->pushAfterCurrent(&TNerveNPCTurnToMario::theNerve());
		return TRUE;
	}

	execCommonWaitApproach(npc, spine, false);
	return FALSE;
}

DEFINE_NERVE(TNerveNPCTurnToMario, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (!npc->isInBodyTurnSearchRange() || npc->unk178 != 0.0f) {
		spine->pushAfterCurrent(&TNerveNPCWaitMarioApproach::theNerve());
		return TRUE;
	}

	execCommonWaitApproach(npc, spine, true);
	return FALSE;
}

DEFINE_NERVE(TNerveNPCWet, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcWetIn();
	}
	if (npc->npcWetting())
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCSink, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	npc->npcSinking();
	return FALSE;
}

DEFINE_NERVE(TNerveNPCRecoverFromSink, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (npc->npcRecoverFromSinking()) {
		spine->pushAfterCurrent(&TNerveNPCRecoverAfter::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveNPCRecoverAfter, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0)
		npc->npcRecoverAfterIn();

	if (npc->unkD0->mCurrentAnmKind == 3
	    && npc->mMActor->isCurAnmAlreadyEnd(0))
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCSetPosAfterSinkBottom, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	npc->setPosAndInitAfterSinkBottom();
	return TRUE;
}

DEFINE_NERVE(TNerveNPCTalk, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();

	bool canTalk = gpMarDirector->isThing();

	if (canTalk) {
		if (spine->getTime() == 0)
			npc->npcTalkIn();
		npc->npcTalking();
	} else {
		if (npc->mActorType == 0x0400001C)
			return TRUE;
		npc->npcTalkOut();
	}
	return FALSE;
}

DEFINE_NERVE(TNerveNPCThrow, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcThrowIn();
	}
	if (npc->npcThrowing())
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCMad, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcMadIn();
	}
	if (npc->npcMadding())
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveNPCBlown, TLiveActor)
{
	bool isMare;
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0)
		npc->npcBlownIn();

	if (npc->npcBlowning()) {
		isMare = true;
		if (!npc->isNormalMareM() && !npc->isNormalMareW())
			isMare = false;
		if (isMare)
			spine->pushAfterCurrent(&TNerveNPCMareStand::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveNPCMareStand, TLiveActor)
{
	TBaseNPC* npc = (TBaseNPC*)spine->getBody();
	if (spine->getTime() == 0) {
		npc->npcMareStandIn();
	}
	if (npc->npcMareStanding())
		return TRUE;
	return FALSE;
}
