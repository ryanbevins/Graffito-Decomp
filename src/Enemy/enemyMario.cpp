#include <Enemy/EnemyMario.hpp>
#include <Enemy/EMario.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JMath.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/MapData.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioRecord.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MSoundMainSide.hpp>
#include <System/MarDirector.hpp>
#include <stdlib.h>

static const char* cDirtyFileName = "/scene/map/pollution/H_ma_rak.bti";
static const char* cDirtyTexName  = "H_ma_rak_dummy";

namespace {

inline u16& emFlags(TEnemyMario* mario)
{
	return *(u16*)((u8*)mario + 0x4290);
}

inline u16& emDoing(TEnemyMario* mario)
{
	return *(u16*)((u8*)mario + 0x4292);
}

inline u32& emTimer(TEnemyMario* mario)
{
	return *(u32*)((u8*)mario + 0x42A4);
}

inline u32& emReplayIndex(TEnemyMario* mario)
{
	return *(u32*)((u8*)mario + 0x42A8);
}

inline s16& emTargetYaw(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x4296);
}

inline u16& emRandomYaw(TEnemyMario* mario)
{
	return *(u16*)((u8*)mario + 0x4298);
}

inline f32& emDistToMario(TEnemyMario* mario)
{
	return *(f32*)((u8*)mario + 0x429C);
}

inline s16& emWaterCount(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x4294);
}

inline TEMario*& emOwner(TEnemyMario* mario)
{
	return *(TEMario**)((u8*)mario + 0x42A0);
}

inline s16& emWaterTimer(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42B4);
}

inline s16& emWaterTimerReset(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42B6);
}

inline s16& emTrampleTimer(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42B8);
}

inline s16& emRunAwayNode(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42CC);
}

inline s16& emWaterCooldown(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42BA);
}

inline void*& emEnemyModel(TEnemyMario* mario)
{
	return *(void**)((u8*)mario + 0x42DC);
}

inline JGeometry::TVec3<f32>& emDisappearPos(TEnemyMario* mario)
{
	return *(JGeometry::TVec3<f32>*)((u8*)mario + 0x42E0);
}

inline u8* emSettings(TEnemyMario* mario)
{
	return *(u8**)((u8*)mario + 0x430C);
}

inline f32& emSettingF32(TEnemyMario* mario, u32 offset)
{
	return *(f32*)(emSettings(mario) + offset);
}

inline s16& emSettingS16(TEnemyMario* mario, u32 offset)
{
	return *(s16*)(emSettings(mario) + offset);
}

inline TMarioInputReplay*& emInputReplay(TEnemyMario* mario)
{
	return *(TMarioInputReplay**)((u8*)mario + 0x4300);
}

inline u8* emController(TEnemyMario* mario)
{
	return *(u8**)((u8*)mario + 0x108);
}

inline u32& emControllerFlags(TEnemyMario* mario)
{
	return *(u32*)(emController(mario) + 4);
}

inline u32& emControllerFlags2(TEnemyMario* mario)
{
	return *(u32*)(emController(mario) + 8);
}

inline void setEMStick(TEnemyMario* mario, s16 angle, f32 scale)
{
	f32 zero = 0.0f;
	*(s16*)(emController(mario) + 0)
	    = (s16)((JMASSin(angle) * zero) * scale);
	*(s16*)(emController(mario) + 2)
	    = (s16)((-JMASCos(angle) * zero) * scale);
}

inline void resetOwnerGraph(TEnemyMario* mario)
{
	TEMario* owner        = emOwner(mario);
	owner->unk124->mPrevIdx = -1;
	owner->goToShortestNextGraphNode();
}

inline void pushNearestFlaggedNodeInput(TEnemyMario* mario)
{
	TGraphWeb* graph = emOwner(mario)->unk124->getGraph();
	int node         = graph->findNearestNodeIndex(mario->mPosition, 0xffffffff);
	if (graph->getGraphNode(node).checkFlag(2)) {
		mario->mFaceAngle.y = emTargetYaw(mario);
		emControllerFlags2(mario) |= 0x100;
		emControllerFlags(mario) |= 0x100;
	}
}

inline void getOwnerGraphPoint(TEnemyMario* mario, int node, Vec* out)
{
	emOwner(mario)->unk124->getGraph()->getGraphNode(node).getPoint(out);
}

inline f32 distanceFromMario(const JGeometry::TVec3<f32>& pos)
{
	f32 dx = pos.x - gpMarioPos->x;
	f32 dy = pos.y - gpMarioPos->y;
	f32 dz = pos.z - gpMarioPos->z;
	return JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);
}

inline f32 distanceFromPos(const JGeometry::TVec3<f32>& pos, const Vec& point)
{
	f32 dx = pos.x - point.x;
	f32 dy = pos.y - point.y;
	f32 dz = pos.z - point.z;
	return JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);
}

inline JGeometry::TVec3<f32>& emDownPos(TEnemyMario* mario)
{
	return *(JGeometry::TVec3<f32>*)((u8*)mario + 0x42C0);
}

} // namespace

void TEnemyMario::drawHPMeter(MtxPtr) { }

void TEnemyMario::damageExec(THitActor*, int, int, int, f32, int, f32, s16) { }

void TEnemyMario::playerControl(JDrama::TGraphics* graphics)
{
	unk9C = mFaceAngle.y;
	*(Vec*)((u8*)this + 0x29C) = *(Vec*)&mPosition;
	mSubState &= ~8;
	checkPlayerAction(graphics);
	stateMachine();
	stateMachineUpper();
	thinkSituation();
	thinkWaterSurface();
	thinkSand();
	thinkHeight();
	thinkParams();
	checkRideReCalc();
	checkWet();
}

void TEnemyMario::checkController(JDrama::TGraphics* graphics)
{
	consider();
	playerControl(graphics);
}

void TEnemyMario::checkReturn()
{
	if (!mGroundPlane->checkFlag(0x10))
		return;

	TGraphWeb* graph = emOwner(this)->unk124->getGraph();
	int node         = graph->findNearestNodeIndex(mPosition, 0xffffffff);
	BOOL searching   = TRUE;

	do {
		Vec point;
		graph->getGraphNode(node).getPoint(&point);

		f32 dx = point.x - gpMarioPos->x;
		f32 dy = point.y - gpMarioPos->y;
		f32 dz = point.z - gpMarioPos->z;
		if (JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz)
		    > 1000.0f) {
			mPosition.x = point.x;
			mPosition.y = point.y;
			mPosition.z = point.z;
			searching = FALSE;
		}

		node = (node + 1) % graph->getNodeNum();
	} while (searching);
}

void TEnemyMario::reachGoal()
{
	emFlags(this) |= 1;
	emTimer(this) = 0;
	emDoing(this) = 0x16;
}

u8 TEnemyMario::thinkTrample()
{
	if (emEnemyModel(this) != nullptr)
		return false;

	if (emDoing(this) == 0xF) {
		emTrampleTimer(this)--;
		if (emTrampleTimer(this) > 0) {
			emTimer(this) = 0;
			emDoing(this) = 0xD;
		}
		return true;
	}

	return false;
}

void TEnemyMario::hitWater(THitActor* sender)
{
	if (emEnemyModel(this) != nullptr)
		return;

	if (*(emSettings(this) + 0xA4) != 0)
		return;

	if (emDoing(this) < 0xB || emDoing(this) > 0x19)
		return;

	if (emDoing(this) != 0xB && emDoing(this) != 0xC
	    && emDoing(this) != 0xD)
		return;

	emWaterCooldown(this) = 600;

	if (emWaterCount(this) > 0) {
		emWaterCount(this)--;
		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 30.0f, 0, 0,
		                         4);
		emWaterTimer(this) = emWaterTimerReset(this);

		if (emDoing(this) == 0xC) {
			sleepingEffectKill();

			TGraphWeb* graph = emOwner(this)->unk124->getGraph();
			int node = graph->findNearestNodeIndex(mPosition, 0xffffffff);
			if (graph->getGraphNode(node).checkFlag(2)) {
				mFaceAngle.y = emTargetYaw(this);
				emControllerFlags2(this) |= 0x100;
				emControllerFlags(this) |= 0x100;
			}

			emTimer(this) = 0;
			emDoing(this) = 0xD;
		}

		return;
	}

	if (mAction == ACTION_RUNNING && canSleep()) {
		if (mHeldObject != nullptr) {
			*(u32*)((u8*)mHeldObject + 0xF0) &= ~0x100000;
			dropObject();
		}
		emTimer(this) = 0;
		emDoing(this) = 0xE;
	}
}

void TEnemyMario::consider()
{
	switch (emDoing(this)) {
	case 0:
		emWaiting();
		break;
	case 1:
		emJumping();
		break;
	case 2:
		emWalkAround();
		break;
	case 0xD:
		emDownAnimation();
		break;
	case 0x10:
		emRunAwayToNearestNode();
		break;
	case 0x11:
		emReplayJumpToNearestNode();
		break;
	case 0x12:
		emReplay();
		break;
	case 0x14:
	case 0x15:
		decideDoingAfterCarry();
		break;
	case 0x17:
		emDisappearToGate();
		break;
	case 0x19:
		emReplay();
		break;
	default:
		break;
	}
}

void TEnemyMario::startGateDrawing()
{
	changePlayerStatus(0x133E, 0, true);
	setAnimation(0x129, 1.0f);
	emTimer(this) = 0;
	emDoing(this) = 0x17;
	startSoundActor(0x1980);
}

void TEnemyMario::emWaitingToInviteMario()
{
	Vec invitePos;
	getOwnerGraphPoint(this, 7, &invitePos);
	mPosition.x = invitePos.x;
	mPosition.y = invitePos.y;
	mPosition.z = invitePos.z;

	s16 diff     = emTargetYaw(this) - mFaceAngle.y;
	mFaceAngle.y = emTargetYaw(this) - IConverge(diff, 0, 0x180, 0x180);
	changePlayerStatus(ACTION_IDLE, 0, false);
	changeMontemanWaitingAnim();

	if (distanceFromMario(mPosition) < emSettingF32(this, 0x18)
	    && gpMarioPos->y < mPosition.y + emSettingF32(this, 0x2C)) {
		Vec nextPos;
		getOwnerGraphPoint(this, 8, &nextPos);
		mFaceAngle.y = matan(nextPos.z - invitePos.z, nextPos.x - invitePos.x);
		mModelFaceAngle = mFaceAngle.y;
		changePlayerStatus(ACTION_IDLE, 0, true);
		emReplayIndex(this) = 0;
		emInputReplay(this)->reset();
		*(u16*)((u8*)emInputReplay(this) + 2) = 1;
		emTimer(this) = 0;
		emDoing(this) = 0x15;
	}
}

void TEnemyMario::decideDoingAfterCarry()
{
	if (emFlags(this) & 0x20) {
		emFlags(this) &= ~0x20;
		pushNearestFlaggedNodeInput(this);
		emTimer(this) = 0;
		emDoing(this) = 0xD;
		return;
	}

	if (*(emSettings(this) + 0x54) == 1) {
		emTimer(this) = 0;
		emDoing(this) = 0xC;
		return;
	}

	pushNearestFlaggedNodeInput(this);
	emTimer(this) = 0;
	emDoing(this) = 0xD;
}

void TEnemyMario::emRunAwayToNearestNode() { }

void TEnemyMario::findRunAwayNearestNode()
{
	TGraphWeb* graph = emOwner(this)->unk124->getGraph();
	f32 nearestDist  = 100000.0f;
	f32 secondDist   = 100000.0f;
	int nearestIdx   = 0;
	int secondIdx    = 0;
	Vec nearestPoint;
	Vec secondPoint;

	for (int i = 0; i < graph->getNodeNum(); ++i) {
		Vec point;
		graph->getGraphNode(i).getPoint(&point);
		f32 dist = distanceFromPos(mPosition, point);

		if (dist < nearestDist) {
			secondDist   = nearestDist;
			secondIdx    = nearestIdx;
			secondPoint  = nearestPoint;
			nearestDist  = dist;
			nearestIdx   = i;
			nearestPoint = point;
		} else if (dist < secondDist) {
			secondDist  = dist;
			secondIdx   = i;
			secondPoint = point;
		}
	}

	f32 secondCurrentDist  = distanceFromPos(mPosition, secondPoint);
	f32 nearestCurrentDist = distanceFromPos(mPosition, nearestPoint);
	if (nearestCurrentDist < secondCurrentDist)
		emRunAwayNode(this) = nearestIdx;
	else
		emRunAwayNode(this) = secondIdx;
}

void TEnemyMario::startRunAway()
{
	emTimer(this) = 0;
	emDoing(this) = 0x10;
}

void TEnemyMario::emDownAnimation()
{
	changePlayerStatus(0x133E, 0, true);
	setAnimation(0x13E, 1.0f);

	bool fixedMode = false;
	u8 mode        = gpMarDirector->unk124;
	if (mode == 3 || mode == 4 || mode == 1 || mode == 2)
		fixedMode = true;

	if (!fixedMode)
		emTimer(this)++;

	emDownPos(this)      = mPosition;
	emDisappearPos(this) = emDownPos(this);

	if (!fixedMode && gpMarDirector->getCurrentMap() != 1
	    && emTimer(this) > (u32)emSettingS16(this, 0xCC)) {
		emWaterCount(this) = emSettingS16(this, 0x40);
		emTimer(this)      = 0;
		emDoing(this)      = 0x10;
	}
}

void TEnemyMario::emReplayJumpToNearestNode() { }

void TEnemyMario::emReplay() { }

void TEnemyMario::emDisappearToGate() { }

void TEnemyMario::startDisappear(u16 doing)
{
	emDisappearPos(this) = mPosition;

	bool keepBossFlag = false;
	u8 map            = gpMarDirector->getCurrentMap();
	u8 stage          = gpMarDirector->getCurrentStage();
	if ((map == 1 && stage == 1) || (map == 1 && stage == 9))
		keepBossFlag = true;

	if (!keepBossFlag)
		MSMainProc::setBossLivesFlag(false);

	gpMarioParticleManager->emitAndBindToPosPtr(0xED, &emDisappearPos(this), 0,
	                                            nullptr);
	emTimer(this) = 0;
	emDoing(this) = doing;
}

void TEnemyMario::emWalkAround()
{
	if (emDistToMario(this) < 1500.0f) {
		emTimer(this) = 0;
		emDoing(this) = 0;
		return;
	}

	if (rand() < 10) {
		emTimer(this) = 0;
		emDoing(this) = 3;
	}

	if (rand() < 100) {
		emControllerFlags(this) |= 0x100;
		emTimer(this) = 0;
		emDoing(this) = 2;
		return;
	}

	if (rand() < 100) {
		emRandomYaw(this) = rand();
		emTimer(this)     = 0;
		emDoing(this)     = 4;
		return;
	}

	if (rand() < 50) {
		resetOwnerGraph(this);
		emTimer(this) = 0;
		emDoing(this) = 6;
		return;
	}

	if (rand() < 50) {
		gpPollution->stamp(1, mPosition.x, mPosition.y, mPosition.z, 384.0f);
		emTimer(this) = 0;
		emDoing(this) = 7;
	}

	if (mWallPlane != nullptr) {
		emControllerFlags(this) |= 0x100;
		emTimer(this) = 0;
		emDoing(this) = 2;
		return;
	}

	setEMStick(this, mFaceAngle.y, 0.5f);
}

void TEnemyMario::emJumping()
{
	if (mAction & 0x800) {
		if (mAction == 0x8A7 && mActionTimer < 10)
			return;

		setEMStick(this, mFaceAngle.y, 1.0f);
		emControllerFlags(this) |= 0x100;

		if (-1.0f < mVel.y && mVel.y < 1.0f && rand() < 0xFFF)
			emControllerFlags(this) |= 0x200;
	} else if (mAction == ACTION_HANGING) {
		if (mActionTimer >= 10)
			emControllerFlags(this) |= 0x100;
	} else if (mAction & 0x600) {
		gpPollution->stamp(1, mPosition.x, mPosition.y, mPosition.z, 384.0f);
		emTimer(this) = 0;
		emDoing(this) = 0;
	}
}

void TEnemyMario::emWaiting()
{
	s16 diff = emTargetYaw(this) - mFaceAngle.y;
	if (diff < -0x1555 || diff > 0x1555)
		setEMStick(this, emTargetYaw(this), 0.2f);

	if (emDistToMario(this) < 800.0f) {
		emTimer(this) = 0;
		emDoing(this) = 1;
	}

	if (emDistToMario(this) > 1500.0f || rand() < 0x88) {
		resetOwnerGraph(this);
		emTimer(this) = 0;
		emDoing(this) = 6;
	}
}

BOOL TEnemyMario::tryTake()
{
	if (mHeldObject != nullptr && mAction != 0x383)
		return TRUE;

	TEMario* owner = emOwner(this);
	for (int i = 0; i < owner->mColCount; ++i) {
		THitActor* actor = owner->mCollisions[i];
		u32 type         = actor->mActorType;
		if (type == 0x40000018 || type == 0x2000002A
		    || type == 0x20000022 || type == 0x20000009) {
			if (type == 0x40000018) {
				*(u32*)((u8*)actor + 0xF0) |= 0x100000;
				emFlags(this) |= 0x20;
			}
			unk384 = actor;
			changePlayerStatus(0x383, 0, false);
		}
	}

	return FALSE;
}

void TEnemyMario::changeEMDoing(u16 doing)
{
	emTimer(this) = 0;
	emDoing(this) = doing;
}

void TEnemyMario::startMonteReplay(u32 node_id)
{
	*(u32*)((u8*)this + 0x42A8) = node_id;
	emTimer(this)               = 0;
	emDoing(this)               = 0x19;
}

void TEnemyMario::initEnemyValues()
{
	initValues();
	initModel();
	emTimer(this) = 0;
	emDoing(this) = 0;
}

void TEnemyMario::initModel()
{
	if (mModel != nullptr)
		return;

	TMario::initModel();
	if (mModel != nullptr)
		gpScreenTexture->replace(mModel->getModel()->getModelData(),
		                         cDirtyTexName);
}

void TEnemyMario::initValues()
{
	TMario::initValues();
	emFlags(this)         = 0;
	emDoing(this)         = 0;
	emTimer(this)         = 0;
	emTrampleTimer(this)  = 0;
	emEnemyModel(this)    = nullptr;
	emDisappearPos(this).set(0.0f, 0.0f, 0.0f);
}
