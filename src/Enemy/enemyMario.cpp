#include <Enemy/EnemyMario.hpp>
#include <Enemy/EMario.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Player/MarioAccess.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MSoundMainSide.hpp>
#include <System/MarDirector.hpp>

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

inline s16& emTrampleTimer(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42B8);
}

inline void*& emEnemyModel(TEnemyMario* mario)
{
	return *(void**)((u8*)mario + 0x42DC);
}

inline JGeometry::TVec3<f32>& emDisappearPos(TEnemyMario* mario)
{
	return *(JGeometry::TVec3<f32>*)((u8*)mario + 0x42E0);
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

void TEnemyMario::checkReturn() { }

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

void TEnemyMario::hitWater(THitActor*) { }

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

void TEnemyMario::emWaitingToInviteMario() { }

void TEnemyMario::decideDoingAfterCarry() { }

void TEnemyMario::emRunAwayToNearestNode() { }

void TEnemyMario::findRunAwayNearestNode() { }

void TEnemyMario::startRunAway()
{
	emTimer(this) = 0;
	emDoing(this) = 0x10;
}

void TEnemyMario::emDownAnimation()
{
	changePlayerStatus(0x133E, 0, true);
	setAnimation(0x13E, 1.0f);
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

void TEnemyMario::emWalkAround() { }

void TEnemyMario::emJumping() { }

void TEnemyMario::emWaiting() { }

void TEnemyMario::tryTake() { }

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
