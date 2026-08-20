#define JG_TUTIL_SQRT_OUT_OF_LINE
#include <Enemy/EnemyMario.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EMario.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JMath.hpp>
#include <M3DUtil/M3UJoint.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <Map/MapData.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <MSound/MAnmSound.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioAnimeData.hpp>
#include <Player/MarioEffect.hpp>
#include <Player/MarioRecord.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/question.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarioGamePad.hpp>
#include <System/MSoundMainSide.hpp>
#include <System/MarDirector.hpp>
#include <System/ParamInst.hpp>
#include <System/Particles.hpp>
#include <System/Params.hpp>
#include <dolphin/gx.h>
#include <math.h>
#include <stdlib.h>

static const char cDirtyFileName[] = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]  = "H_ma_rak_dummy";

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

#include <M3DUtil/InfectiousStrings.hpp>

static unkTMarioAnimeFilesStruct marioAnimeFiles[199] = {
	{ 0x00000001, "hgup" },
	{ 0x00000001, "bdwn" },
	{ 0x00000001, "bkdwn" },
	{ 0x00000001, "tree_climb" },
	{ 0x00000001, "tree_catch" },
	{ 0x00000000, "tree_stand" },
	{ 0x00000000, "tree_wait" },
	{ 0x00000000, "brake" },
	{ 0x00000001, "brend" },
	{ 0x00000001, "hgdwn" },
	{ 0x00000001, "fjpend" },
	{ 0x00000000, "firejmp" },
	{ 0x00000001, "sdwnf" },
	{ 0x00000001, "jfdwn" },
	{ 0x00000000, "hang" },
	{ 0x00000001, "hgjmp" },
	{ 0x00000001, "ladder_hang_catch" },
	{ 0x00000000, "hiped" },
	{ 0x00000001, "hipsr" },
	{ 0x00000000, "hipat" },
	{ 0x00000001, "run1" },
	{ 0x00000001, "2jmed" },
	{ 0x00000000, "2jmp2" },
	{ 0x00000001, "jump" },
	{ 0x00000001, "jmped" },
	{ 0x00000001, "2jmp1" },
	{ 0x00000000, "land" },
	{ 0x00000001, "laend" },
	{ 0x00000001, "lost" },
	{ 0x00000001, "door_openr" },
	{ 0x00000001, "door_openl" },
	{ 0x00000001, "throw" },
	{ 0x00000001, "ladder_hang_move_l" },
	{ 0x00000001, "ladder_hang_move_r" },
	{ 0x00000001, "raise" },
	{ 0x00000001, "push" },
	{ 0x00000001, "ride_shell" },
	{ 0x00000001, "put" },
	{ 0x00000001, "roll" },
	{ 0x00000001, "run2" },
	{ 0x00000000, "shock" },
	{ 0x00000001, "sfbdn" },
	{ 0x00000001, "sffdn" },
	{ 0x00000001, "sdown" },
	{ 0x00000001, "shfdn" },
	{ 0x00000000, "swait" },
	{ 0x00000001, "swlkl" },
	{ 0x00000001, "swlkr" },
	{ 0x00000000, "sldct" },
	{ 0x00000000, "slpbk" },
	{ 0x00000001, "sldwn" },
	{ 0x00000001, "slped" },
	{ 0x00000000, "slpla" },
	{ 0x00000000, "slip" },
	{ 0x00000001, "sstep" },
	{ 0x00000000, "sqend" },
	{ 0x00000000, "sqsta" },
	{ 0x00000000, "sqwat" },
	{ 0x00000000, "turn" },
	{ 0x00000000, "trned" },
	{ 0x00000001, "tjmp2" },
	{ 0x00000001, "tjmp1" },
	{ 0x00000000, "wait" },
	{ 0x00000000, "ladder_hang_wait_l" },
	{ 0x00000000, "ladder_hang_wait_r" },
	{ 0x00000000, "walk" },
	{ 0x00000001, "wjmp" },
	{ 0x00000000, "wsld" },
	{ 0x00000000, "pump" },
	{ 0x00000000, "hgpmp" },
	{ 0x00000000, "step1" },
	{ 0x00000001, "step2" },
	{ 0x00000001, "step3" },
	{ 0x00000000, "jkick" },
	{ 0x00000000, "dgrun" },
	{ 0x00000000, "carry_p" },
	{ 0x00000001, "hmov_l" },
	{ 0x00000001, "hmov_r" },
	{ 0x00000000, "t_wait" },
	{ 0x00000001, "hot_wait" },
	{ 0x00000000, "rope_walk" },
	{ 0x00000001, "rope_run" },
	{ 0x00000000, "rope_wait" },
	{ 0x00000000, "rope_wtosw" },
	{ 0x00000000, "rope_wtosw_r" },
	{ 0x00000000, "rope_swait" },
	{ 0x00000001, "rope_whg" },
	{ 0x00000001, "rope_swhg" },
	{ 0x00000000, "rope_hgwat" },
	{ 0x00000001, "rope_return" },
	{ 0x00000001, "rope_hmovr" },
	{ 0x00000001, "rope_hmovl" },
	{ 0x00000000, "sinking" },
	{ 0x00000000, "sink_down" },
	{ 0x00000001, "door_kick" },
	{ 0x00000001, "hold" },
	{ 0x00000000, "hold_wait" },
	{ 0x00000001, "hold_back" },
	{ 0x00000001, "hold_move_r" },
	{ 0x00000001, "hold_move_l" },
	{ 0x00000001, "hold_drag" },
	{ 0x00000000, "hold_to_hang" },
	{ 0x00000000, "hold_hang" },
	{ 0x00000001, "hang_to_hold" },
	{ 0x00000001, "hold_return" },
	{ 0x00000000, "spin_p" },
	{ 0x00000001, "turbo_dash" },
	{ 0x00000001, "broad_jump" },
	{ 0x00000001, "jump_rolling" },
	{ 0x00000001, "giant_rolling" },
	{ 0x00000001, "fence_catch" },
	{ 0x00000001, "fence_jcatch" },
	{ 0x00000000, "fence_wait" },
	{ 0x00000001, "fence_move_l" },
	{ 0x00000001, "fence_move_r" },
	{ 0x00000001, "fence_move_up" },
	{ 0x00000001, "fence_move_down" },
	{ 0x00000001, "fence_punch" },
	{ 0x00000001, "ladder_hang_kick" },
	{ 0x00000001, "ladder_roll_up" },
	{ 0x00000001, "ladder_roll_down" },
	{ 0x00000001, "ride_shell" },
	{ 0x00000000, "ride_shell_wait" },
	{ 0x00000001, "swim_start" },
	{ 0x00000000, "swim_wait" },
	{ 0x00000001, "wait_to_swim" },
	{ 0x00000001, "swim" },
	{ 0x00000001, "swim_to_wait" },
	{ 0x00000001, "swim_damage" },
	{ 0x00000001, "swim_down" },
	{ 0x00000001, "demo_shine_get" },
	{ 0x00000000, "demo_gate_in" },
	{ 0x00000001, "demo_gate_out" },
	{ 0x00000001, "roll_jump" },
	{ 0x00000001, "get_fail" },
	{ 0x00000001, "tree_move_l" },
	{ 0x00000001, "tree_move_r" },
	{ 0x00000001, "die" },
	{ 0x00000000, "monteman_wait" },
	{ 0x00000001, "swim_start" },
	{ 0x00000000, "swim_wait" },
	{ 0x00000001, "wait_to_paddle" },
	{ 0x00000001, "paddle_start" },
	{ 0x00000001, "swim" },
	{ 0x00000001, "paddle_end" },
	{ 0x00000001, "paddle_to_wait" },
	{ 0x00000001, "float" },
	{ 0x00000000, "damage_wait" },
	{ 0x00000000, "fepmp" },
	{ 0x00000000, "swpmp" },
	{ 0x00000000, "thrown" },
	{ 0x00000001, "thrown_end" },
	{ 0x00000001, "bottle_in" },
	{ 0x00000001, "sand_fill_head" },
	{ 0x00000001, "sand_fill_head_end" },
	{ 0x00000001, "sandfill_leg" },
	{ 0x00000001, "sandfill_leg_end" },
	{ 0x00000000, "damage_wait_start" },
	{ 0x00000001, "swim_dive" },
	{ 0x00000001, "draw" },
	{ 0x00000000, "swim_p_damage" },
	{ 0x00000000, "swim_p_down" },
	{ 0x00000001, "pivot" },
	{ 0x00000000, "demo_gate_out_get2" },
	{ 0x00000001, "demo_gate_out_appear" },
	{ 0x00000000, "belt_up" },
	{ 0x00000000, "yawn" },
	{ 0x00000001, "sit" },
	{ 0x00000001, "sit_wait" },
	{ 0x00000001, "sit_end" },
	{ 0x00000001, "sleep" },
	{ 0x00000001, "sleep_wait" },
	{ 0x00000001, "sleep_end" },
	{ 0x00000000, "dive_wait" },
	{ 0x00000001, "dive_land" },
	{ 0x00000001, "door_gacha_l" },
	{ 0x00000001, "door_gacha_r" },
	{ 0x00000001, "shock_down" },
	{ 0x00000001, "demo_gate_out_appear_get" },
	{ 0x00000000, "demo_gate_out_rolling_get" },
	{ 0x00000000, "demo_gate_out_rolling" },
	{ 0x00000000, "fall_down_wait" },
	{ 0x00000000, "yo_wait" },
	{ 0x00000000, "yo_walk" },
	{ 0x00000000, "yo_run" },
	{ 0x00000000, "yo_eat" },
	{ 0x00000000, "yo_eat_end" },
	{ 0x00000000, "yo_jump" },
	{ 0x00000000, "yo_jump_fall" },
	{ 0x00000000, "yo_jump_end" },
	{ 0x00000000, "yo_hold_jump" },
	{ 0x00000000, "yo_ride" },
	{ 0x00000000, "yo_hip_start" },
	{ 0x00000000, "yo_hip_pose" },
	{ 0x00000000, "yo_hip_end" },
	{ 0x00000000, "yo_damage" },
	{ 0x00000001, "demo_shine_get_yo" },
	{ 0x00000001, "yo_slide_pose" },
	{ 0x00000001, "yo_slide_end" },
};

namespace {

static const char* sEnemyMarioModelNames[] = {
	"マリオモドキ_0", "マリオモドキ_1", "マリオモドキ_2",
	"マリオモドキ_3", "モンテマン",
};

static const char* sEnemyMarioBmdFileNames[] = {
	"/kagemario/kagemario_model.bmd",
	"/kagemario/kagemario_model.bmd",
	"/kagemario/kagemario_model.bmd",
	"/kagemario/kagemario_model.bmd",
	"/scene/map/map/pad/monteman_model.bmd",
	nullptr,
	nullptr,
};

static const char* sRecordFileNamesDolpic1[] = {
	nullptr, "BH", "CH", "DH", "EH", "FH", "GH", nullptr,
};

static const char* sRecordFileNamesMonteMan[] = { "AB0", "AB1", "AB2" };

class TEnemyMarioParams : public TParams {
public:
	TEnemyMarioParams(const char* path, bool can_carry)
	    : TParams(path)
	    , PARAM_INIT(mSearchDist, 1000.0f)
	    , PARAM_INIT(mSearchHeight, 300.0f)
	    , PARAM_INIT(mWaterCtMax, (s16)64)
	    , PARAM_INIT(mStopFlag, (u8)1)
	    , PARAM_INIT(mStampFlag, (u8)1)
	    , PARAM_INIT(mRandomFlag, (u8)1)
	    , PARAM_INIT(mCarryFlag, (u8)(can_carry ? 1 : 0))
	    , PARAM_INIT(mInvincibleFlag, (u8)0)
	    , PARAM_INIT(mRandomPow, 1.0f)
	    , PARAM_INIT(mDownTime, (s16)1200)
	    , PARAM_INIT(mPolluteFlag, (u8)0)
	    , PARAM_INIT(mPolluteSize, 160.0f)
	{
		TParams::load(mPrmPath);
	}

	TParamRT<f32> mSearchDist;
	TParamRT<f32> mSearchHeight;
	TParamRT<s16> mWaterCtMax;
	TParamRT<u8> mStopFlag;
	TParamRT<u8> mStampFlag;
	TParamRT<u8> mRandomFlag;
	TParamRT<u8> mCarryFlag;
	TParamRT<u8> mInvincibleFlag;
	TParamRT<f32> mRandomPow;
	TParamRT<s16> mDownTime;
	TParamRT<u8> mPolluteFlag;
	TParamRT<f32> mPolluteSize;
};

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

inline f32& emRunAwaySpeed(TEnemyMario* mario)
{
	return *(f32*)((u8*)mario + 0x42D0);
}

inline s16& emWaterCooldown(TEnemyMario* mario)
{
	return *(s16*)((u8*)mario + 0x42BA);
}

inline f32& emJumpSpeedCap(TEnemyMario* mario)
{
	return *(f32*)((u8*)mario + 0x42BC);
}

inline f32& emTremblePower(TEnemyMario* mario)
{
	return *(f32*)((u8*)mario + 0x42AC);
}

inline f32& emWaterRange(TEnemyMario* mario)
{
	return *(f32*)((u8*)mario + 0x42B0);
}

inline u8& emScenarioType(TEnemyMario* mario)
{
	return *(u8*)((u8*)mario + 0x42D4);
}

inline u8*& emReplayLinkTable(TEnemyMario* mario)
{
	return *(u8**)((u8*)mario + 0x4304);
}

inline J3DModel*& emEnemyModel(TEnemyMario* mario)
{
	return *(J3DModel**)((u8*)mario + 0x42DC);
}

inline MActor*& emEnemyMActor(TEnemyMario* mario)
{
	return *(MActor**)((u8*)mario + 0x42F0);
}

inline J3DModel*& emEnemyShadowModel(TEnemyMario* mario)
{
	return *(J3DModel**)((u8*)mario + 0x42EC);
}

inline f32& emEnemyModelScale(TEnemyMario* mario)
{
	return *(f32*)((u8*)mario + 0x42F4);
}

inline JGeometry::TVec3<f32>& emDisappearPos(TEnemyMario* mario)
{
	return *(JGeometry::TVec3<f32>*)((u8*)mario + 0x42E0);
}

inline JGeometry::TVec3<f32>& emGateBasePos(TEnemyMario* mario)
{
	return *(JGeometry::TVec3<f32>*)((u8*)mario + 0x178);
}

inline u8*& emSettings(TEnemyMario* mario)
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

inline TMarioInputReplay**& emInputReplayArray(TEnemyMario* mario)
{
	return *(TMarioInputReplay***)((u8*)mario + 0x42F8);
}

inline TMarioInputReplay**& emInputReplayArrayBackup(TEnemyMario* mario)
{
	return *(TMarioInputReplay***)((u8*)mario + 0x42FC);
}

inline u16& emInputReplayCanPlay(TMarioInputReplay* replay)
{
	return *(u16*)((u8*)replay + 2);
}

inline u8* emController(TEnemyMario* mario)
{
	return *(u8**)((u8*)mario + 0x108);
}

inline TMarioControllerWork* emControllerWork(TEnemyMario* mario)
{
	return (TMarioControllerWork*)emController(mario);
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

inline void normalizeDir(Vec* dir)
{
	f32 len = dir->x * dir->x + dir->y * dir->y + dir->z * dir->z;
	if (len <= 0.0000038146973f) {
		dir->x = 0.0f;
		dir->y = 0.0f;
		dir->z = 0.0f;
	} else {
		f32 invLen = JGeometry::TUtil<f32>::inv_sqrt(len);
		dir->x *= invLen;
		dir->y *= invLen;
		dir->z *= invLen;
	}
}

inline void playInputReplay(TEnemyMario* mario, TMarioInputReplay* replay)
{
	u8* controller = emController(mario);
	replay->play(&mario->mIntendedMag, &mario->mIntendedYaw,
	             (u32*)(controller + 4), (u32*)(controller + 8),
	             controller + 0xD, controller + 0xC);
}

inline JGeometry::TVec3<f32>& emDownPos(TEnemyMario* mario)
{
	return *(JGeometry::TVec3<f32>*)((u8*)mario + 0x42C0);
}

inline s16& marioUnk14C(TMario* mario)
{
	return *(s16*)((u8*)mario + 0x14C);
}

inline s16& marioUnk14E(TMario* mario)
{
	return *(s16*)((u8*)mario + 0x14E);
}

inline BOOL isEnemyModelDrawState(TEnemyMario* mario)
{
	return emDoing(mario) == 0x13 && emSettings(mario)[0x68] == 1;
}

} // namespace

void TEnemyMario::perform(u32 flags, JDrama::TGraphics* graphics)
{
	MActor* ownerActor   = nullptr;
	J3DModel* ownerModel = nullptr;

	if (emEnemyModel(this) == nullptr) {
		ownerActor  = emOwner(this)->mMActor;
		ownerModel = ownerActor->getModel();
	}

	if (marioUnk14E(this) > 0)
		marioUnk14E(this)--;

	if (emWaterCooldown(this) > 0)
		emWaterCooldown(this)--;

	u32 doMovement = flags & 1;
	if (doMovement != 0) {
		if (emFlags(this) & 0x10) {
			emWaterCount(this) = 0;
			hitWater(this);
		}

		if (mAction != ACTION_RUNNING || marioUnk14E(this) == 0) {
			checkController(graphics);
			setPositions();
		}
	}

	if (doMovement != 0) {
		if (mAction != ACTION_RUNNING || marioUnk14E(this) == 0)
			calcAnim(2, graphics);

		if (emEnemyModel(this) != nullptr) {
			J3DModel* model = mModel->getModel();
			for (u16 i = 0; i < model->getModelData()->getJointNum(); ++i)
				PSMTXCopy(emEnemyModel(this)->mNodeMatrices[i],
				          model->mNodeMatrices[i]);
			emEnemyModel(this)->calcWeightEnvelopeMtx();
		} else {
			ownerActor->calcAnm();
			J3DModel* model = mModel->getModel();
			for (u16 i = 0; i < model->getModelData()->getJointNum(); ++i)
				PSMTXCopy(ownerModel->mNodeMatrices[i],
				          model->mNodeMatrices[i]);
			ownerModel->calcWeightEnvelopeMtx();
			PSMTXCopy(ownerModel->mNodeMatrices[mBoneIDs[5]],
			          emEnemyShadowModel(this)->unk20);
			emEnemyShadowModel(this)->calc();

			if (isEnemyModelDrawState(this)) {
				Mtx scaleMtx;
				Mtx rotMtx;
				f32 scale = emEnemyModelScale(this);
				PSMTXScale(scaleMtx, scale, scale, scale);
				MsMtxSetRotRPH(rotMtx, 0.0f, 180.0f, 0.0f);
				PSMTXConcat(mModel->getModel()->unk20, scaleMtx,
				            scaleMtx);
				PSMTXConcat(scaleMtx, rotMtx, scaleMtx);
				PSMTXCopy(scaleMtx, emEnemyMActor(this)->getModel()->unk20);
				emEnemyMActor(this)->calcAnm();
			}
		}

		mAttackRadius = 800.0f;
		mAttackHeight = 150.0f;
		mDamageRadius = 60.0f;
		mDamageHeight = 40.0f;
		calcEntryRadius();

		if (emWaterTimer(this) > 0)
			emWaterTimer(this)--;
		else
			emWaterTimer(this) = 0;
	}

	if (flags & 4) {
		calcView(graphics);

		if (emFlags(this) & 2) {
			((TMBindShadowBody*)unk390)->entryDrawShadow();
			gpQuestionManager->request(mPosition, 50.0f);
		}

		if (emEnemyModel(this) != nullptr) {
			emEnemyModel(this)->viewCalc();
		} else {
			ownerActor->viewCalc();
			emEnemyShadowModel(this)->viewCalc();
			if (isEnemyModelDrawState(this))
				emEnemyMActor(this)->viewCalc();
		}
	}

	if (flags & 0x200) {
		if (emEnemyModel(this) == nullptr) {
			if (emWaterTimer(this) > 0) {
				SMS_AddDamageFogEffect(
				    emOwner(this)->mMActor->getModel()->getModelData(),
				    mPosition, graphics);
			} else {
				SMS_ResetDamageFogEffect(
				    emOwner(this)->mMActor->getModel()->getModelData());
			}

			if (isEnemyModelDrawState(this))
				gpPollution->stampModel(emEnemyMActor(this)->getModel());
		}

		if (emFlags(this) & 2) {
			if (emEnemyModel(this) == nullptr) {
				ownerActor->entry();
				emEnemyShadowModel(this)->entry();
			} else {
				emEnemyModel(this)->entry();
			}
		}
	}

	if (flags & 8) {
		if (mTrembleModelEffect != nullptr)
			mTrembleModelEffect->movement();

		BOOL drawBuffers = (mSubState & 2) != 0;
		if (marioUnk14C(this) > 0 && !(marioUnk14C(this) & 4))
			drawBuffers = false;
		if (mState & 4)
			drawBuffers = false;
		if (emDoing(this) == 7)
			drawBuffers = false;
		if (marioUnk14E(this) > 0 && !(marioUnk14E(this) & 4))
			drawBuffers = false;

		if (drawBuffers && mTrembleModelEffect != nullptr) {
			j3dSys.unk4C = 7;
			unk394->draw();
			unk398->draw();
		}

		if ((emFlags(this) & 0x80) && emWaterCooldown(this) > 0)
			drawHPMeter(graphics->getUnkB4());
	}
}

void TEnemyMario::drawHPMeter(MtxPtr mtx)
{
	Vec worldPos = mPosition;
	worldPos.y += 64.0f;

	Vec screenPos;
	PSMTXMultVec(mtx, &worldPos, &screenPos);

	Mtx identity;
	PSMTXIdentity(identity);
	GXSetCurrentMtx(GX_PNMTX0);
	GXLoadPosMtxImm(identity, GX_PNMTX0);
	GXLoadNrmMtxImm(identity, GX_PNMTX0);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
	              GX_COLOR0A0);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
	GXSetCullMode(GX_CULL_NONE);

	f32 left   = screenPos.x - 48.0f;
	f32 top    = screenPos.y - 10.0f;
	f32 bottom = screenPos.y + 10.0f;
	f32 z      = screenPos.z;

	GXColor backColor = { 0, 0, 0, 0xC0 };
	GXSetChanMatColor(GX_COLOR0A0, backColor);
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(left - 5.0f, top - 5.0f, z);
	GXPosition3f32(left + 101.0f, top - 5.0f, z);
	GXPosition3f32(left + 101.0f, bottom + 5.0f, z);
	GXPosition3f32(left - 5.0f, bottom + 5.0f, z);
	GXEnd();

	f32 gaugeRight = left + (f32)emWaterCount(this) * 1.5f;
	GXColor gaugeColor = { 0x40, 0x40, 0xFF, 0xFF };
	GXSetChanMatColor(GX_COLOR0A0, gaugeColor);
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition3f32(left, top, z);
	GXPosition3f32(gaugeRight, top, z);
	GXPosition3f32(gaugeRight, bottom, z);
	GXPosition3f32(left, bottom, z);
	GXEnd();
}

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
	f32 dx        = gpMarioPos->x - mPosition.x;
	f32 dz        = gpMarioPos->z - mPosition.z;
	emTargetYaw(this) = matan(dz, dx);
	emDistToMario(this) = JGeometry::TUtil<f32>::sqrt(dx * dx + dz * dz);

	TMarioControllerWork* controller = emControllerWork(this);
	u32 prevInput                   = controller->mInput;
	controller->mStickHS16          = 0;
	controller->mStickVS16          = 0;
	controller->mInput              = (TMarioControllerWork::Buttons)0;
	controller->mFrameInput         = (TMarioControllerWork::Buttons)0;
	controller->mAnalogRU8          = 0;
	controller->mAnalogLU8          = 0;

	consider();

	controller->mStickH = 0.0f;
	controller->mStickV = 0.0f;

	if (controller->mStickHS16 <= -7)
		controller->mStickH = controller->mStickHS16 + 6;
	if (controller->mStickHS16 >= 7)
		controller->mStickH = controller->mStickHS16 - 6;
	if (controller->mStickVS16 <= -7)
		controller->mStickV = controller->mStickVS16 + 6;
	if (controller->mStickVS16 >= 7)
		controller->mStickV = controller->mStickVS16 - 6;

	f32 stickLen = JGeometry::TUtil<f32>::sqrt(
	    controller->mStickH * controller->mStickH
	    + controller->mStickV * controller->mStickV);
	controller->mStickDist = stickLen;
	if (controller->mStickDist > 1500.0f) {
		f32 scale = 1500.0f / controller->mStickDist;
		controller->mStickH *= scale;
		controller->mStickV *= scale;
		controller->mStickDist = 1500.0f;
	}

	controller->mFrameInput
	    = (TMarioControllerWork::Buttons)(controller->mInput
	                                      & (controller->mInput ^ prevInput));

	mIntendedMag = 1500.0f
	    * ((controller->mStickDist * 100.0f)
	       * (controller->mStickDist * 100.0f))
	    * 3000.0f;
	if (mIntendedMag > 0.0f)
		mIntendedYaw = matan(-controller->mStickV, controller->mStickH);
	else
		mIntendedYaw = emTargetYaw(this);

	if (emDoing(this) == 0xB)
		emReplay();

	if (emDoing(this) == 0x11) {
		TMarioInputReplay* replay
		    = emInputReplayArray(this)[emReplayIndex(this)];
		playInputReplay(this, replay);
		replay = emInputReplayArray(this)[emReplayIndex(this)];
		if (emInputReplayCanPlay(replay) != 1) {
			changePlayerStatus(0x133E, 0, true);
			setAnimation(0x114, 1.0f);
			emTimer(this) = 0;
			emDoing(this) = 0x14;
		}
	}

	if (emDoing(this) == 0x15) {
		playInputReplay(this, emInputReplay(this));
		if (emInputReplayCanPlay(emInputReplay(this)) != 1) {
			changePlayerStatus(0x133E, 0, true);
			setAnimation(0x114, 1.0f);
			emTimer(this) = 0;
			emDoing(this) = 0x18;
		}
	}

	if (emDoing(this) == 0x19) {
		TMarioInputReplay* replay
		    = emInputReplayArray(this)[emReplayIndex(this)];
		playInputReplay(this, replay);
		replay = emInputReplayArray(this)[emReplayIndex(this)];
		if (emInputReplayCanPlay(replay) != 1) {
			emFlags(this) |= 1;
			changeEMDoing(0x16);
		}
	}

	if (mIntendedMag > 0.0f)
		mInput |= 1;

	if (controller->mFrameInput & 0x100)
		mInput |= 2;
	if (controller->mInput & 0x100)
		mInput |= 0x80;
	if (controller->mInput & 0x200)
		mInput |= 0x4000;
	if (controller->mFrameInput & 0x200)
		mInput |= 0x8000;
}

void TEnemyMario::checkReturn()
{
	if (!mGroundPlane->checkFlag(0x10))
		return;

	int node = emOwner(this)->unk124->getGraph()->findNearestNodeIndex(
	    mPosition, 0xffffffff);
	BOOL searching   = TRUE;

	do {
		Vec point;
		getOwnerGraphPoint(this, node, &point);

		f32 dx = point.x - gpMarioPos->x;
		f32 dy = point.y - gpMarioPos->y;
		f32 dz = point.z - gpMarioPos->z;
		if (JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz)
		    > 1000.0f) {
			*(Vec*)&mPosition = point;
			searching = FALSE;
		}

		node = (node + 1) % emOwner(this)->unk124->getGraph()->getNodeNum();
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

	switch ((s32)(u16)doing()) {
	case 0xF:
		emTrampleTimer(this)--;
		if (emTrampleTimer(this) > 0) {
			emTimer(this) = 0;
			doingRef() = 0xD;
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

	switch (doing()) {
	case 0xB:
	case 0xC:
	case 0xD:
		break;
	default:
		return;
	}

	emWaterCooldown(this) = 600;

	if (emWaterCount(this) > 0) {
		emWaterCount(this)--;
		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 30.0f, 0, 0,
		                         4);
		emWaterTimer(this) = emWaterTimerReset(this);

		if (doing() == 0xC) {
			sleepingEffectKill();

			TGraphWeb* graph = emOwner(this)->unk124->getGraph();
			int node = graph->findNearestNodeIndex(mPosition, 0xffffffff);
			if (graph->getGraphNode(node).checkFlag(2)) {
				mFaceAngle.y = emTargetYaw(this);
				emControllerFlags2(this) |= 0x100;
				emControllerFlags(this) |= 0x100;
			}

			emTimer(this) = 0;
			doingRef() = 0xD;
		}

		return;
	}

	if (mAction == ACTION_RUNNING && canSleep()) {
		if (mHeldObject != nullptr) {
			*(u32*)((u8*)mHeldObject + 0xF0) &= ~0x100000;
			dropObject();
		}
		emTimer(this) = 0;
		doingRef() = 0xE;
	}
}

void TEnemyMario::consider()
{
	switch (emDoing(this)) {
	case 0:
		emWaiting();
		break;
	case 1:
		if (emDistToMario(this) < 400.0f) {
			emControllerFlags(this) |= 0x100;
			emTimer(this) = 0;
			emDoing(this) = 2;
		}

		if (emDistToMario(this) < 1300.0f) {
			setEMStick(this, emTargetYaw(this), 1.0f);
			*(s16*)emController(this) = -*(s16*)emController(this);
			*(s16*)(emController(this) + 2)
			    = -*(s16*)(emController(this) + 2);
		} else {
			emDoing(this) = 0;
		}
		break;
	case 2:
		emJumping();
		break;
	case 3:
		if (emDistToMario(this) > 1500.0f) {
			setEMStick(this, emTargetYaw(this), 1.0f);
		} else {
			emTimer(this) = 0;
			emDoing(this) = 0;
		}
		break;
	case 4: {
		s16 diff = emRandomYaw(this) - mFaceAngle.y;
		if (rand() < 100) {
			emControllerFlags(this) |= 0x100;
			emTimer(this) = 0;
			emDoing(this) = 2;
			break;
		}

		if (diff < -0x1555 || diff > 0x1555) {
			setEMStick(this, emRandomYaw(this), 0.2f);
		} else {
			emTimer(this) = 0;
			emDoing(this) = 0;
		}
		break;
	}
	case 5:
		emWalkAround();
		break;
	case 6: {
		TEMario* owner = emOwner(this);
		JGeometry::TVec3<f32> ownerTarget = owner->unk104.getPoint();
		Vec ownerPos;
		ownerPos.x = owner->mPosition.x;
		ownerPos.y = owner->mPosition.y;
		ownerPos.z = owner->mPosition.z;

		f32 dx = ownerTarget.x - ownerPos.x;
		f32 dy = ownerTarget.y - ownerPos.y;
		f32 dz = ownerTarget.z - ownerPos.z;
		if (JGeometry::TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz)
		    < 100.0f) {
			if (emDistToMario(this) > 3000.0f)
				owner->goToRandomNextGraphNode();
			else
				owner->goToRandomEscapeGraphNode();
		}

		const JGeometry::TVec3<f32>& goal = owner->unkF4.getPoint();
		s16 yaw = matan(goal.z - mPosition.z, goal.x - mPosition.x);
		setEMStick(this, yaw, 1.0f);
		emTimer(this)++;
		if (emTimer(this) % 100 == 0) {
			gpConductor->makeEnemyAppear(mPosition, "ハムクリマネージャー",
			                             1, 0);
		}
		break;
	}
	case 7:
		unk64 |= 1;
		emOwner(this)->unk64 |= 1;
		emTimer(this)++;
		if (gpPollution == nullptr
		    || !gpPollution->isPolluted(mPosition.x, mPosition.y,
		                                mPosition.z)
		    || emTimer(this) > 7200) {
			emTimer(this) = 0;
			marioUnk14C(this) = 120;
			emTimer(this) = 0;
			emDoing(this) = 8;
		}
		break;
	case 8:
		if (marioUnk14C(this) == 0) {
			emTimer(this) = 0;
			emDoing(this) = 0;
			unk64 &= ~1;
			emOwner(this)->unk64 &= ~1;
		}
		break;
	case 9:
		unk64 |= 1;
		emOwner(this)->unk64 |= 1;
		emFlags(this) &= ~2;
		*(u16*)((u8*)this + 0x114) &= ~2;
		changePlayerStatus(0x133E, 0, false);
		break;
	case 0xA:
		emDisappearToGate();
		break;
	case 0xC:
		if (distanceFromMario(mPosition) < emSettingF32(this, 0x18)
		    && gpMarioPos->y < mPosition.y + emSettingF32(this, 0x2C)) {
			pushNearestFlaggedNodeInput(this);
			emTimer(this) = 0;
			emDoing(this) = 0xD;
		}
		break;
	case 0xD:
		emReplayJumpToNearestNode();
		break;
	case 0xE:
		changePlayerStatus(0x133E, 0, true);
		setAnimation(0x2C, 1.0f);
		if (getMotionFrameCtrl().getFrame() > 25.0f) {
			emTimer(this) = 0;
			emDoing(this) = 0xF;
		}
		break;
	case 0xF:
		emDownAnimation();
		break;
	case 0x10:
		emRunAwayToNearestNode();
		break;
	case 0x12:
		if (tryTake()) {
			if (emEnemyMActor(this) != nullptr
			    && emSettings(this)[0x68] == 1) {
				emEnemyMActor(this)->setBck("stamp_koopa_sign_draw1");
				emTimer(this) = 0;
				emDoing(this) = 0x13;
			} else {
				decideDoingAfterCarry();
			}
		}
		break;
	case 0x13:
		if (emEnemyMActor(this)->curAnmEndsNext(0, nullptr))
			decideDoingAfterCarry();
		break;
	case 0x14:
		emWaitingToInviteMario();
		break;
	case 0x16: {
		s16 diff = emTargetYaw(this) - mFaceAngle.y;
		mFaceAngle.y
		    = emTargetYaw(this) - IConverge(diff, 0, 0x180, 0x180);
		changePlayerStatus(ACTION_IDLE, 0, false);
		changeMontemanWaitingAnim();
		break;
	}
	case 0x17:
		getOwnerGraphPoint(this, 8, &mPosition);
		mFaceAngle.y      = -0x8000;
		mModelFaceAngle   = -0x8000;
		if (isLast1AnimeFrame())
			startDisappear(0xA);
		break;
	case 0x18:
	case 0x1A: {
		s16 diff = emTargetYaw(this) - mFaceAngle.y;
		mFaceAngle.y
		    = emTargetYaw(this) - IConverge(diff, 0, 0x180, 0x180);
		break;
	}
	case 0x1B: {
		JDrama::TGraphics graphics;
		TMario::checkController(&graphics);
		break;
	}
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

#pragma dont_inline on
void TEnemyMario::decideDoingAfterCarry()
{
	bool hadCarryFlag = (emFlags(this) & 0x20) ? true : false;
	if (hadCarryFlag) {
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
#pragma dont_inline off

void TEnemyMario::emRunAwayToNearestNode()
{
	Vec target;
	getOwnerGraphPoint(this, emRunAwayNode(this), &target);

	gpMarioParticleManager->emitAndBindToPosPtr(0x1AA, &emDisappearPos(this),
	                                            1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x1AB, &emDisappearPos(this),
	                                            1, this);

	if (emTimer(this) >= 8 && emTimer(this) < 300)
		emFlags(this) &= ~2;
	else
		emFlags(this) |= 2;

	switch (emTimer(this)) {
	case 0:
		findRunAwayNearestNode();
		emDisappearPos(this) = mPosition;
		emDisappearPos(this).y += 80.0f;
		gpMarioParticleManager->emit(0xED, &emDisappearPos(this), 0,
		                             nullptr);
		if (gpMSound->gateCheck(0x1976))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x1976, &mPosition, 0, nullptr, 0, 4);
		break;
	case 100: {
		Vec dir;
		dir.x = target.x - emDownPos(this).x;
		dir.y = target.y - emDownPos(this).y;
		dir.z = target.z - emDownPos(this).z;

		f32 len = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
		if (len <= 0.0000038146973f) {
			dir.x = 0.0f;
			dir.y = 0.0f;
			dir.z = 0.0f;
		} else {
			f32 invLen = JGeometry::TUtil<f32>::inv_sqrt(len);
			dir.x *= invLen;
			dir.y *= invLen;
			dir.z *= invLen;
		}

		dir.x *= emRunAwaySpeed(this);
		dir.y *= emRunAwaySpeed(this);
		dir.z *= emRunAwaySpeed(this);

		emDisappearPos(this).x += dir.x;
		emDisappearPos(this).y += dir.y;
		emDisappearPos(this).z += dir.z;

		f32 dx = target.x - emDisappearPos(this).x;
		f32 dz = target.z - emDisappearPos(this).z;
		f32 speed = emRunAwaySpeed(this);
		if (dx * dx + dz * dz < speed * speed)
			emTimer(this) = 200;
		emTimer(this)--;
		break;
	}
	case 200:
		mPosition = target;
		mPosition.y += 5.0f;
		break;
	case 220:
		gpMarioParticleManager->emit(0xED, &emDisappearPos(this), 0,
		                             nullptr);
		if (gpMSound->gateCheck(0x197C))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x197C, &mPosition, 0, nullptr, 0, 4);
		break;
	case 300:
		mPosition = target;
		mPosition.y += 5.0f;
		changePlayerStatus(0x0C400201, 0, true);
		if (gpMarDirector->mMap == 1) {
			Vec next;
			getOwnerGraphPoint(this, 7, &next);
			mFaceAngle.y
			    = matan(next.z - target.z, next.x - target.x);
			mModelFaceAngle = mFaceAngle.y;
			emReplayIndex(this) = emRunAwayNode(this);
			emInputReplayArray(this) = emInputReplayArrayBackup(this);
			emInputReplayArray(this)[emReplayIndex(this)]->reset();
			emInputReplayCanPlay(
			    emInputReplayArray(this)[emReplayIndex(this)])
			    = 1;
			emTimer(this) = 0;
			emDoing(this) = 0x11;
		} else {
			pushNearestFlaggedNodeInput(this);
			emTimer(this) = 0;
			emDoing(this) = 0xD;
		}
		break;
	default:
		break;
	}

	emTimer(this)++;
}

void TEnemyMario::findRunAwayNearestNode()
{
	f32 nearestDist  = 100000.0f;
	f32 secondDist   = 100000.0f;
	int nearestIdx   = 0;
	int secondIdx    = 0;
	Vec nearestPoint;
	Vec secondPoint;

	for (int i = 0; i < emOwner(this)->unk124->getGraph()->getNodeNum(); ++i) {
		Vec point;
		emOwner(this)->unk124->getGraph()->getGraphNode(i).getPoint(&point);
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

	bool fixedMode = true;
	u8 mode        = gpMarDirector->unk124;
	if (mode != 3 && mode != 4) {
		fixedMode = true;
		if (mode != 1 && mode != 2)
			fixedMode = false;
	}

	if (fixedMode) {
		emDownPos(this)      = mPosition;
		emDisappearPos(this) = emDownPos(this);
	} else {
		emTimer(this)++;

		emDownPos(this)      = mPosition;
		emDisappearPos(this) = emDownPos(this);

		if (gpMarDirector->getCurrentMap() != 1
		    && emTimer(this) > (u32)emSettingS16(this, 0xCC)) {
			emWaterCount(this) = emSettingS16(this, 0x40);
			emTimer(this)      = 0;
			emDoing(this)      = 0x10;
		}
	}
}

void TEnemyMario::emReplayJumpToNearestNode()
{
	TGraphWeb* graph = emOwner(this)->unk124->getGraph();
	int node         = graph->findNearestNodeIndex(mPosition, 0xffffffff);
	if (graph->getGraphNode(node).checkFlag(2)) {
		emControllerFlags2(this) |= 0x100;
		emControllerFlags(this) |= 0x100;
		if (mVel.y > emJumpSpeedCap(this))
			mVel.y = emJumpSpeedCap(this);
	}

	emTimer(this)++;

	graph = emOwner(this)->unk124->getGraph();
	node  = graph->findNearestNodeIndex(mPosition, 0xffffffff);
	TGraphNode* currentNode = &graph->getGraphNode(node);

	Vec currentPoint;
	currentNode->getPoint(&currentPoint);

	mPosition.x += (currentPoint.x - mPosition.x) * 0.05f;
	mPosition.z += (currentPoint.z - mPosition.z) * 0.05f;
	mPosition.y += (currentPoint.y - mPosition.y) * 0.05f;

	if (mAction != 0x0C400201) {
		int nearest = graph->findNearestNodeIndex(mPosition, 0xffffffff);
		if (graph->getGraphNode(nearest).checkFlag(2))
			return;

		mPosition = currentPoint;
		mVel.x = mVel.y = mVel.z = mForwardVel = 0.0f;
		resetHistory();
		changePlayerStatus(0x0C400201, 0, true);
	}

	u8* links = emReplayLinkTable(this) + node * 6;

	Vec marioDir;
	marioDir.x = gpMarioPos->x - currentPoint.x;
	marioDir.y = gpMarioPos->y - currentPoint.y;
	marioDir.z = gpMarioPos->z - currentPoint.z;
	normalizeDir(&marioDir);

	TGraphNode* targetNode = nullptr;
	u8* settings          = emSettings(this);

	if (settings[0x7C] == 0) {
		f32 best = 1.0f;
		for (int i = 0; i < 3; ++i) {
			u8 nextNode = links[i * 2];
			if (nextNode == 0xFF)
				continue;

			TGraphNode* candidateNode = &graph->getGraphNode(nextNode);
			Vec candidatePoint;
			candidateNode->getPoint(&candidatePoint);

			Vec candidateDir;
			candidateDir.x = candidatePoint.x - currentPoint.x;
			candidateDir.y = candidatePoint.y - currentPoint.y;
			candidateDir.z = candidatePoint.z - currentPoint.z;
			normalizeDir(&candidateDir);

			f32 dot = marioDir.x * candidateDir.x
			          + marioDir.y * candidateDir.y
			          + marioDir.z * candidateDir.z;
			if (dot < best) {
				best                 = dot;
				targetNode           = candidateNode;
				emReplayIndex(this) = links[i * 2 + 1];
			}
		}
	} else {
		int candidateSlots[3];
		f32 weights[3];
		int count = 0;

		for (int i = 0; i < 3; ++i) {
			weights[i] = 0.0f;
			u8 nextNode = links[i * 2];
			if (nextNode == 0xFF)
				continue;

			Vec candidatePoint;
			graph->getGraphNode(nextNode).getPoint(&candidatePoint);

			Vec candidateDir;
			candidateDir.x = candidatePoint.x - currentPoint.x;
			candidateDir.y = candidatePoint.y - currentPoint.y;
			candidateDir.z = candidatePoint.z - currentPoint.z;
			normalizeDir(&candidateDir);

			weights[count] = marioDir.x * candidateDir.x
			                 + marioDir.y * candidateDir.y
			                 + marioDir.z * candidateDir.z;
			candidateSlots[count] = i;
			count++;
		}

		f32 total = 0.0f;
		for (int i = 0; i < count; ++i) {
			weights[i] = powf(1.0f - weights[i], emSettingF32(this, 0xB8));
			total += weights[i];
		}

		if (total != 0.0f) {
			for (int i = 0; i < count; ++i)
				weights[i] /= total;
		}

		int choice = 0;
		f32 roll  = rand() * 0.000030517578f;
		for (int i = 0; i < count; ++i) {
			roll -= weights[i];
			if (roll <= 0.0f) {
				choice = i;
				break;
			}
		}

		if (count > 0) {
			int slot             = candidateSlots[choice];
			emReplayIndex(this) = links[slot * 2 + 1];
			targetNode          = &graph->getGraphNode(links[slot * 2]);
		}
	}

	Vec targetPoint = currentPoint;
	if (targetNode != nullptr)
		targetNode->getPoint(&targetPoint);

	mPosition = currentPoint;
	mFaceAngle.y
	    = matan(targetPoint.z - currentPoint.z, targetPoint.x - currentPoint.x);
	mVel.x = mVel.y = mVel.z = mForwardVel = 0.0f;
	resetHistory();
	changePlayerStatus(0x0C400201, 0, true);

	TMarioInputReplay* replay = emInputReplayArray(this)[emReplayIndex(this)];
	replay->reset();
	emInputReplayCanPlay(replay) = 1;

	emTimer(this) = 0;
	emDoing(this) = 0xB;
}

void TEnemyMario::emReplay()
{
	u8* controller          = emController(this);
	TMarioInputReplay* replay = emInputReplayArray(this)[emReplayIndex(this)];
	replay->play(&mIntendedMag, &mIntendedYaw, (u32*)(controller + 4),
	             (u32*)(controller + 8), controller + 0xD, controller + 0xC);

	u8* settings = emSettings(this);
	if (settings[0xE0] != 0 && gpPollution != nullptr) {
		gpPollution->stamp(1, mPosition.x, mPosition.y, mPosition.z,
		                   emSettingF32(this, 0xF4));
	}

	replay = emInputReplayArray(this)[emReplayIndex(this)];
	if (emInputReplayCanPlay(replay) == 1)
		return;

	if (settings[0x90] == 1 && mHeldObject == nullptr) {
		emTimer(this) = 0;
		emDoing(this) = 0x12;
		return;
	}

	if (emEnemyMActor(this) != nullptr && settings[0x68] == 1) {
		emEnemyMActor(this)->setBck("stamp_koopa_sign_draw1");
		emEnemyMActor(this)->setFrameRate(SMSGetAnmFrameRate(), 0);
		emTimer(this) = 0;
		emDoing(this) = 0x13;
		startSoundActor(0x1980);
		startSoundActor(0x1981);
		return;
	}

	TGraphWeb* graph = emOwner(this)->unk124->getGraph();
	int node         = graph->findNearestNodeIndex(mPosition, 0xffffffff);
	if (graph->getGraphNode(node).checkFlag(0x40)) {
		emTimer(this) = 0;
		emDoing(this) = 0x16;
		return;
	}

	if (settings[0x54] == 1) {
		emTimer(this) = 0;
		emDoing(this) = 0xC;
		return;
	}

	pushNearestFlaggedNodeInput(this);
	emTimer(this) = 0;
	emDoing(this) = 0xD;
}

void TEnemyMario::emDisappearToGate()
{
	if (emTimer(this) >= 8)
		emFlags(this) &= ~2;
	else
		emFlags(this) |= 2;

	unk64 |= 1;
	emOwner(this)->unk64 |= 1;

	gpMarioParticleManager->emitAndBindToPosPtr(0x1AA, &emDisappearPos(this),
	                                            1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x1AB, &emDisappearPos(this),
	                                            1, this);

	if (emTimer(this) == 0) {
		emDisappearPos(this) = emGateBasePos(this);
		gpMarioParticleManager->emit(0xED, &emDisappearPos(this), 0,
		                             nullptr);

		if (gpMSound->gateCheck(0x1976))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x1976, &mPosition, 0, nullptr, 0, 4);
	}

	if (emTimer(this) > 100) {
		emDisappearPos(this).y += 0.025f * (f32)emTimer(this);
		emDisappearPos(this).z -= 0.03f * (f32)emTimer(this);
	}

	emTimer(this)++;
}

void TEnemyMario::startDisappear(u16 doing)
{
	emDisappearPos(this) = mPosition;

	bool keepBossFlag = false;
	u8 map            = gpMarDirector->getCurrentMap();
	u8 stage          = gpMarDirector->getCurrentStage();
	if (map == 1 && stage == 1)
		keepBossFlag = true;
	else if (map == 1 && stage == 9)
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
		f32 z = mPosition.z;
		f32 y = mPosition.y;
		f32 x = mPosition.x;
		gpPollution->stamp(1, x, y, z, 384.0f);
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
		*(u32*)((u8*)unk108 + 4) |= 0x100;

		if (-1.0f < mVel.y && mVel.y < 1.0f && rand() < 0xFFF)
			*(u32*)((u8*)unk108 + 4) |= 0x200;
	} else if (mAction == ACTION_HANGING) {
		if (mActionTimer >= 10)
			*(u32*)((u8*)unk108 + 4) |= 0x100;
	} else if (mAction & 0x600) {
		f32 z = mPosition.z;
		f32 y = mPosition.y;
		f32 x = mPosition.x;
		gpPollution->stamp(1, x, y, z, 384.0f);
		emTimer(this) = 0;
		emDoing(this) = 0;
	}
}

void TEnemyMario::emWaiting()
{
	s16 diff = emTargetYaw(this) - mFaceAngle.y;
	if (diff < -0x1555 || diff > 0x1555)
		setEMStick(this, emTargetYaw(this), 0.2f);

	if (distToMario() < 800.0f) {
		emTimer(this) = 0;
		emDoing(this) = 1;
	}

	if (distToMario() > 1500.0f || rand() < 0x88) {
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
		if (type == 0x04000018 || type == 0x2000002A
		    || type == 0x20000022 || type == 0x20000009) {
			if (type == 0x04000018) {
				*(u32*)((u8*)actor + 0xF0) |= 0x100000;
				emFlags(this) |= 0x20;
			}
			unk384 = actor;
			changePlayerStatus(0x383, 0, false);
		}
	}

	return FALSE;
}

#pragma dont_inline on
void TEnemyMario::changeEMDoing(u16 doing)
{
	emTimer(this) = 0;
	emDoing(this) = doing;
}
#pragma dont_inline off

void TEnemyMario::startMonteReplay(u32 node_id)
{
	int node = emOwner(this)->unk124->getGraph()->findNearestNodeIndex(
	    mPosition, 0xffffffff);

	Vec current;
	emOwner(this)->unk124->getGraph()->getGraphNode(node).getPoint(&current);
	mPosition = current;

	Vec next;
	emOwner(this)->unk124->getGraph()->getGraphNode(node + 1).getPoint(&next);
	mFaceAngle.y = matan(next.z - current.z, next.x - current.x);

	f32 zero    = 0.0f;
	mVel.x      = zero;
	mVel.y      = zero;
	mVel.z      = zero;
	mForwardVel = zero;

	resetHistory();
	changePlayerStatus(0x0C400201, 0, true);

	emReplayIndex(this) = node_id;
	emInputReplayArray(this)[emReplayIndex(this)]->reset();
	*(u16*)((u8*)emInputReplayArray(this)[emReplayIndex(this)] + 2) = 1;

	emTimer(this) = 0;
	emDoing(this) = 0x19;
}

void TEnemyMario::initEnemyValues()
{
	f32 zero = 0.0f;

	emFlags(this)        = 2;
	emDoing(this)        = 0x1A;
	emWaterCount(this)   = 0;
	emTargetYaw(this)    = 0;
	emRandomYaw(this)    = 0;
	emDistToMario(this)  = zero;
	emTimer(this)        = 0;
	emReplayIndex(this)  = 0;
	emTremblePower(this) = 2.5f;
	emScenarioType(this) = 0;
	emWaterRange(this)   = 120.0f;
	emWaterTimer(this)   = 0;
	emWaterTimerReset(this) = 120;
	emTrampleTimer(this) = 3;
	emWaterCooldown(this) = 0;
	emJumpSpeedCap(this) = 15.0f;
	emRunAwayNode(this)  = 0;
	emRunAwaySpeed(this) = 10.0f;

	onHitFlag(1);
	emOwner(this)->onHitFlag(1);

	unk388                  = 1;
	emEnemyModel(this)      = nullptr;

	int modelType = 6;
	for (int i = 0; i < 5; ++i) {
		if (strcmp(sEnemyMarioModelNames[i], emOwner(this)->getName()) == 0) {
			modelType = i;
			break;
		}
	}

	J3DModelData* enemyModelData = nullptr;
	if (modelType >= 0 && modelType < 4) {
		unk388 = 1;
	} else if (modelType == 4) {
		void* enemyResource
		    = JKRFileLoader::getGlbResource(sEnemyMarioBmdFileNames[modelType]);
		enemyModelData
		    = J3DModelLoaderDataBase::load(enemyResource, 0x10040000);
		unk388 = 2;
	}

	emEnemyShadowModel(this) = nullptr;
	emEnemyMActor(this)     = nullptr;
	emEnemyModelScale(this) = 3.0f;

	if (enemyModelData != nullptr) {
		unk388             = 2;
		emEnemyModel(this) = new J3DModel(enemyModelData, 0, 1);
	} else {
		unk388 = 1;
		void* brushResource = JKRFileLoader::getGlbResource(
		    "/scene/kagemario/kagemario_brush.bmd");
		J3DModelData* brushData
		    = J3DModelLoaderDataBase::load(brushResource, 0x11040000);
		emEnemyShadowModel(this) = new J3DModel(brushData, 0, 1);

		ResTIMG* dirtyTexture
		    = (ResTIMG*)JKRFileLoader::getGlbResource(cDirtyFileName);
		if (dirtyTexture != nullptr)
			SMS_ChangeTextureAll(brushData, cDirtyTexName, *dirtyTexture);

		emEnemyMActor(this) = SMS_MakeMActor(
		    "/scene/kagemario/stamp_koopa_sign",
		    "/scene/kagemario/stamp_koopa_sign/stamp_koopa_sign_model1.bmd", 3,
		    0x10210000);

		SMS_LoadParticle("/scene/kagemario/jpa/ms_kgm_change.jpa", 0xED);
		SMS_LoadParticle("/scene/kagemario/jpa/ms_kgm_move_a.jpa", 0x1AA);
		SMS_LoadParticle("/scene/kagemario/jpa/ms_kgm_move_b.jpa", 0x1AB);
	}

	emDisappearPos(this).set(zero, zero, zero);

	int flagState = TFlagManager::smInstance->getFlag(0x60003);
	if (flagState == 0) {
		onHitFlag(1);
		emOwner(this)->onHitFlag(1);
	} else {
		offHitFlag(1);
		emOwner(this)->offHitFlag(1);
	}

	const char* prmPath = "/../map/pad/Setting.prm";
	bool canCarry      = true;
	if (flagState == 2) {
		emScenarioType(this) = 1;
		prmPath              = "/../map/pad2/Setting.prm";
		canCarry             = false;
	} else if (flagState == 3) {
		emScenarioType(this) = 2;
		prmPath              = "/../map/pad3/Setting.prm";
	}

	emSettings(this)   = (u8*)new TEnemyMarioParams(prmPath, canCarry);
	emWaterCount(this) = emSettingS16(this, 0x40);

	emReplayLinkTable(this)       = nullptr;
	emInputReplayArray(this)      = nullptr;
	emInputReplayArrayBackup(this) = nullptr;
	emInputReplay(this)           = nullptr;

	int replayCount       = 0;
	const char** replayNames = nullptr;
	{
		char linkPath[256];
		if (emScenarioType(this) == 0) {
			snprintf(linkPath, sizeof(linkPath),
			         "/scene/map/map/pad/linkdata.bin");
		} else {
			snprintf(linkPath, sizeof(linkPath),
			         "/scene/map/map/pad%d/linkdata.bin",
			         emScenarioType(this));
		}

		u8* linkData = (u8*)JKRFileLoader::getGlbResource(linkPath);
		if (linkData != nullptr) {
			JKRFileLoader* sceneVolume = JKRFileLoader::getVolume("scene");
			JSUMemoryInputStream stream(
			    linkData, sceneVolume->getResSize(linkData));

			stream.skip(6);
			stream.readString();
			stream.skip(2);
			stream.readString();

			u32 linkCount;
			stream.read(&linkCount, sizeof(linkCount));
			emReplayLinkTable(this) = new u8[linkCount * 6];

			char** names = new char*[linkCount * 3];
			for (u32 i = 0; i < linkCount * 3; ++i)
				names[i] = new char[3];

			replayNames = (const char**)names;
			for (u32 row = 0; row < linkCount; ++row) {
				stream.skip(6);
				stream.readString();
				stream.skip(2);
				stream.readString();

				char rowName = (char)('A' + row);
				for (int col = 0; col < 3; ++col) {
					stream.skip(2);

					char link;
					stream.read(&link, 1);

					u32 tableOffset = row * 6 + col * 2;
					if (link == '*') {
						emReplayLinkTable(this)[tableOffset]     = 0xFF;
						emReplayLinkTable(this)[tableOffset + 1] = 0xFF;
					} else {
						snprintf(names[replayCount], 3, "%c%c", rowName,
						         link);
						emReplayLinkTable(this)[tableOffset]
						    = link - 'A';
						emReplayLinkTable(this)[tableOffset + 1]
						    = replayCount;
						replayCount++;
					}
				}
			}
		}
	}

	if (emScenarioType(this) != 0 && emOwner(this)->unk124 != nullptr) {
		char graphName[32];
		snprintf(graphName, sizeof(graphName), "mariomodoki%d",
		         emScenarioType(this));
		emOwner(this)->unk124->setGraph(gpConductor->getGraphByName(graphName));
	}

	emTremblePower(this) = 2.5f;
	if (unk388 == 2) {
		replayNames = sRecordFileNamesMonteMan;
		replayCount = 3;
	}

	if (replayCount > 0) {
		emInputReplayArray(this) = new TMarioInputReplay*[replayCount];
		for (int i = 0; i < replayCount; ++i) {
			char replayPath[256];
			if (emScenarioType(this) == 0) {
				snprintf(replayPath, sizeof(replayPath),
				         "/scene/map/map/pad/tutorial%s.pad",
				         replayNames[i]);
			} else {
				snprintf(replayPath, sizeof(replayPath),
				         "/scene/map/map/pad%d/tutorial%s.pad",
				         emScenarioType(this), replayNames[i]);
			}

			u8* replayData = (u8*)JKRFileLoader::getGlbResource(replayPath);
			emInputReplayArray(this)[i] = new TMarioInputReplay;
			emInputReplayArray(this)[i]->init(replayData);
		}
	}

	setGamePad(gpMarDirector->unk18[1]);
	emFlags(this) = 2;

	switch (emOwner(this)->unk154) {
	case 0:
		emDoing(this) = 0xC;
		break;
	case 1:
		emDoing(this) = 0xB;
		break;
	case 2:
		emDoing(this) = 0x12;
		break;
	case 3:
		emDoing(this) = 0x16;
		break;
	default:
		emDoing(this) = 9;
		break;
	}

	if (flagState == 0)
		emDoing(this) = 9;

	if (flagState == 2)
		emReplayIndex(this) = emOwner(this)->unk15C;
	else if (flagState == 3)
		emReplayIndex(this) = emOwner(this)->unk160;
	else
		emReplayIndex(this) = emOwner(this)->unk158;

	if (emOwner(this)->unk124 != nullptr
	    && emOwner(this)->unk124->getGraph() != nullptr) {
		TGraphWeb* graph = emOwner(this)->unk124->getGraph();
		graph->getGraphNode(emReplayIndex(this)).getPoint(&mPosition);
		emOwner(this)->mPosition = mPosition;
	}

	if (replayCount > 0) {
		emInputReplayArray(this)[emReplayIndex(this)]->reset();
		*(u16*)((u8*)emInputReplayArray(this)[emReplayIndex(this)] + 2) = 1;
	} else {
		emInputReplayArray(this) = nullptr;
	}

	if (gpMarDirector->mMap == 1 && gpMarDirector->unk7D == 1) {
		emInputReplayArrayBackup(this) = new TMarioInputReplay*[8];
		for (int i = 0; i < 8; ++i) {
			if (sRecordFileNamesDolpic1[i] != nullptr) {
				char replayPath[256];
				snprintf(replayPath, sizeof(replayPath),
				         "/scene/map/map/pad/tutorial%s.pad",
				         sRecordFileNamesDolpic1[i]);
				u8* replayData
				    = (u8*)JKRFileLoader::getGlbResource(replayPath);
				emInputReplayArrayBackup(this)[i] = new TMarioInputReplay;
				emInputReplayArrayBackup(this)[i]->init(replayData);
			} else {
				emInputReplayArrayBackup(this)[i] = nullptr;
			}
		}

		u8* inviteReplay = (u8*)JKRFileLoader::getGlbResource(
		    "/scene/map/map/pad/tutorialHI.pad");
		emInputReplay(this) = new TMarioInputReplay;
		emInputReplay(this)->init(inviteReplay);
	}

	if (gpMarDirector->mMap == 0xC) {
		if (strcmp(emOwner(this)->getName(), "マリオ@1") == 0) {
			unk388 = 3;
			setGamePad(gpMarDirector->unk18[1]);
		}
		if (strcmp(emOwner(this)->getName(), "マリオ@2") == 0) {
			unk388 = 4;
			setGamePad(gpMarDirector->unk18[2]);
		}
		if (strcmp(emOwner(this)->getName(), "マリオ@3") == 0) {
			unk388 = 5;
			setGamePad(gpMarDirector->unk18[3]);
		}
		emDoing(this) = 0x1B;
	}

	if (unk388 >= 3 && unk388 <= 5 && mModel != nullptr) {
		mTrembleModelEffect = new TTrembleModelEffect;
		mTrembleModelEffect->init(mModel->getModel());
		mTrembleModelEffect->clash(emTremblePower(this));
	}

	mSubState |= 2;
	mAction     = 0x0C400201;
	mPrevAction = mAction;
	mState &= ~0x8000;

	if (emScenarioType(this) == 2 && gpMapObjWave != nullptr)
		gpMapObjWave->noWave();

	mHandModels[0][0] = nullptr;
	mHandModels[0][1] = nullptr;
	mHandModels[1][0] = nullptr;
	mHandModels[1][1] = nullptr;
	mWaterGun         = nullptr;
	mCap              = nullptr;
	mYoshi            = nullptr;
	mMultiMtxEffect   = nullptr;
}

void TEnemyMario::initModel()
{
	unk394 = nullptr;
	unk398 = nullptr;
	unk39C = 0;
	unk3A0 = 0;

	M3UModelMario* originalModel = gpMarioOriginal->mModel;
	mBodyModelData              = originalModel->getModel()->getModelData();
	unk3C4       = mBodyModelData->getJointName()->getIndex("center");
	mBoneIDs[1]  = mBodyModelData->getJointName()->getIndex("chn_chest");
	mBoneIDs[0]  = mBodyModelData->getJointName()->getIndex("jnt_chest");
	mBoneIDs[2]  = mBodyModelData->getJointName()->getIndex("jnt_arm_R1");
	mBoneIDs[3]  = mBodyModelData->getJointName()->getIndex("jnt_arm_L1");
	mBoneIDs[4]  = mBodyModelData->getJointName()->getIndex("jnt_hand_R");
	mBoneIDs[5]  = mBodyModelData->getJointName()->getIndex("jnt_hand_L");
	mBoneIDs[6]  = mBodyModelData->getJointName()->getIndex("chn_foot_R");
	mBoneIDs[7]  = mBodyModelData->getJointName()->getIndex("jnt_foot_R");
	mBoneIDs[8]  = mBodyModelData->getJointName()->getIndex("chn_foot_L");
	mBoneIDs[9]  = mBodyModelData->getJointName()->getIndex("jnt_foot_L");
	mBoneIDs[10] = mBodyModelData->getJointName()->getIndex("jnt_head");
	mBoneIDs[11] = mBodyModelData->getJointName()->getIndex("M_head");

	J3DModel* bodyModel = new J3DModel(mBodyModelData, 0, 1);
	mHandModels[0][0]  = nullptr;
	mHandModels[0][1]  = nullptr;
	mHandModels[1][0]  = nullptr;
	mHandModels[1][1]  = nullptr;

	mAnmSoundTbl = new JAIAnimeSound*[199];
	char buffer[0x100];
	for (int i = 0; i < 199; ++i) {
		snprintf(buffer, 0xff, "/mario/bas/ma_%s.bas",
		         marioAnimeFiles[i].unk4);
		loadBas((void**)&mAnmSoundTbl[i], buffer);
	}

	J3DAnmTexPattern** anmTexPattern = new J3DAnmTexPattern*[0x18];
	J3DTexNoAnm** anmTexNoAnm        = new J3DTexNoAnm*[0x18];
	for (int i = 0; i < 0x18; ++i) {
		loadAnmTexPattern(&anmTexPattern[i], marioAnimeTexPatternFilenames[i],
		                  mBodyModelData);
		u16 matCount   = anmTexPattern[i]->getUpdateMaterialNum();
		anmTexNoAnm[i] = new J3DTexNoAnm[matCount];
		for (int j = 0; j < matCount; ++j) {
			anmTexNoAnm[i][j].setAnmIndex(j);
			anmTexNoAnm[i][j].setAnmTexPattern(anmTexPattern[i]);
		}
	}

	M3UMtxCalcSIAnmBlendQuat* anmBlendQuat
	    = new M3UMtxCalcSIAnmBlendQuat[2];
	anmBlendQuat[0].unk50 = 0.0f;
	J3DFrameCtrl* frameCtrl = new J3DFrameCtrl[3];

	M3UModelCommonMario* marioCommon = new M3UModelCommonMario;
	marioCommon->unk4                = originalModel->unk4->unk4;
	marioCommon->unk18               = anmBlendQuat;
	marioCommon->unk8                = anmTexPattern;
	marioCommon->unk8                = originalModel->unk4->unk8;
	marioCommon->unkC                = anmTexNoAnm;

	M3UModelMario* modelMario = new M3UModelMario;
	modelMario->unk8          = bodyModel;
	modelMario->unk4          = marioCommon;
	modelMario->unk20         = marioCommon;
	modelMario->unkC          = frameCtrl;

	frameCtrl[2].setRate(SMSGetAnmFrameRate());

	M3UMarioMtxCalcSetInfo* setInfo = new M3UMarioMtxCalcSetInfo[2];
	setInfo[0].mJointIdx           = 0;
	setInfo[0].unk2                = 2;
	setInfo[0].mMtxCalcIdx         = 0;
	setInfo[0].mAnmTransformIdx[0] = 0x14;
	setInfo[0].mAnmTransformIdx[1] = 0x41;
	setInfo[0].mFrameCtrlIdx       = 0;
	setInfo[1].mJointIdx           = mBoneIDs[0];
	setInfo[1].unk2                = 2;
	setInfo[1].mMtxCalcIdx         = 1;
	setInfo[1].mAnmTransformIdx[0] = 0;
	setInfo[1].mAnmTransformIdx[1] = 0;
	setInfo[1].mFrameCtrlIdx       = 1;
	modelMario->unk10              = 2;
	modelMario->unk24              = setInfo;

	u8* blendFlags      = new u8[2];
	blendFlags[0]       = 0;
	blendFlags[1]       = 2;
	modelMario->unk1C   = blendFlags;
	modelMario->changeMtxCalcSIAnmBQAnmTransform(0, 0, 0x3E);
	modelMario->changeMtxCalcSIAnmBQAnmTransform(1, 0, 0x41);
	modelMario->getFrameCtrl(1).setRate(0.0f);
	marioCommon->unk18[1].unk50 = 0.0f;
	mModel                      = modelMario;

	setAnimation(0xC3, 1.0f);

	J3DTransformInfo transformInfo;
	transformInfo.mScale.x     = 1.0f;
	transformInfo.mScale.y     = 1.0f;
	transformInfo.mScale.z     = 1.0f;
	transformInfo.mRotation.x  = mFaceAngle.x;
	transformInfo.mRotation.y  = mFaceAngle.y;
	transformInfo.mRotation.z  = mFaceAngle.z;
	transformInfo.mTranslate.x = mPosition.x;
	transformInfo.mTranslate.y = mPosition.y;
	transformInfo.mTranslate.z = mPosition.z;

	Mtx transform;
	J3DGetTranslateRotateMtx(transformInfo, transform);
	mModel->getModel()->setBaseTRMtx(transform);
	mModel->updateInMotion();
	mModel->getModel()->calc();

	mSurfGesso = nullptr;
	mTorocco   = nullptr;
	mPinaRail  = nullptr;
	mKoopaRail = nullptr;

	mMultiMtxEffect              = new TMultiMtxEffect;
	mMultiMtxEffect->mNumBones   = 3;
	mMultiMtxEffect->mBoneIDs    = new u16[3];
	mMultiMtxEffect->mBoneIDs[0] = mBoneIDs[0];
	mMultiMtxEffect->mBoneIDs[1] = mBoneIDs[2];
	mMultiMtxEffect->mBoneIDs[2] = mBoneIDs[3];
	mMultiMtxEffect->mMtxEffectType    = new u8[3];
	mMultiMtxEffect->mMtxEffectType[0] = 0;
	mMultiMtxEffect->mMtxEffectType[1] = 0;
	mMultiMtxEffect->mMtxEffectType[2] = 0;
	mMultiMtxEffect->setup(mModel->getModel(), "Mario");
}

void TEnemyMario::initValues()
{
	f32 zero = 0.0f;

	mHealth = mDeParams.mHpMax.get();
	unk134  = zero;
	*(f32*)&unk138 = 1.0f;
	unk13C  = 0;
	*(f32*)&unk140 = zero;

	unk108 = (u32)operator new(0x24);
	u8* controller = (u8*)unk108;
	*(s16*)(controller + 0x00) = 0;
	*(s16*)(controller + 0x02) = 0;
	*(f32*)(controller + 0x10) = zero;
	*(f32*)(controller + 0x14) = zero;
	*(f32*)(controller + 0x18) = zero;
	*(u32*)(controller + 0x04) = 0;
	*(u32*)(controller + 0x08) = 0;
	*(u8*)(controller + 0x0C)  = 0;
	*(u8*)(controller + 0x0D)  = 0;
	unk10C = zero;
	unk110 = zero;

	unk154 = new TWaterEmitInfo("/Mario/DamageWaterEmit.prm");
	unk158 = new TWaterEmitInfo("/Mario/WetWaterEmit.prm");

	unk388 = 1;
	unk530 = new s16[60];
	for (int i = 0; i < 60; ++i)
		unk530[i] = 0;
	unk534 = 0;
	unk536 = 0;
	unk538 = 0;
	unk53A = 0;
	unk53B = 0;

	initModel();

	unk3D8    = zero;
	unk3DC    = zero;
	mCap      = nullptr;
	mWaterGun = nullptr;
	mYoshi    = nullptr;

	mMarioEffect = new TMarioEffect;
	((TMarioEffect*)mMarioEffect)->init(this);

	unk414.x = zero;
	unk414.y = zero;
	unk414.z = 1.0f;
	mMarioScreenPos.x = zero;
	mMarioScreenPos.y = zero;
	mMarioScreenPos.z = zero;
	mWarpInDir.x = zero;
	mWarpInDir.y = zero;
	mWarpInDir.z = zero;
	unk468       = zero;
	unk46C       = zero;

	mAnmSound = new MAnmSound(gpMSound);
	mAnmSound->initAnmSound(nullptr, 1, zero);
	unk4EC          = 0;
	mBlendLogicOp   = 10;
	mWaterWakeAlpha = 0;

	unk390 = (u32)new TMBindShadowBody(this, mModel->getModel(), 1.0f);
}
