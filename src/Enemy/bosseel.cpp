#include <Camera/CameraShake.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/BossEel.hpp>
#include <Enemy/Conductor.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MSound/MSound.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarioGamePad.hpp>
#include <Player/MarioMain.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <math.h>
#include <stdlib.h>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };
static f32 testHeight;

static s32 hoseiDiveCameraCallback(u32, u32);

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[]
    = "MActorMtxCalcType_User ユーザー定義";
static const char* MtxCalcTypeName[] = {
	MtxCalcTypeName0,
	MtxCalcTypeName1,
	MtxCalcTypeName2,
	MtxCalcTypeName3,
};
static const f32 dummy2933[3] = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2935[3] = { 1.0f, 1.0f, 1.0f };

static inline void setEffectMtxOnTex1(J3DMaterial* mat, MtxPtr mtx)
{
	mat->getTexGenBlock()->getTexMtx(1)->setEffectMtx(mtx);
}

f32 TBossEel::mOpenRollSpeed = 0.3f;
u8 TBossEel::mUseObjCollision = TRUE;
f32 TBossEel::mForcePow       = 10.0f;
u8 TBossEel::mUseMapCollision;

static const char* bosseel_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/bosseel/bas/meoto_in_loop.bas",
	nullptr,
	"/scene/bosseel/bas/meoto_mogu.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/bosseel/bas/meoto_out_loop.bas",
	"/scene/bosseel/bas/meoto_paku.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char* bossEelTears_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char cDummyTextureName[] = "M_dummy";
static const char cTearsRecoverCollisionName[] = "回復コリジョン";
static const char cBossEelTearsManagerName[] = "めおとウナギ涙マネージャー";
static const char cBossEelEndCameraName[] = "meoto_end_camera";
static const char cBossEelShineName[]     = "シャイン（ボス用）";
static const char cBossEelShineCameraName[]
    = "めおとウナギシャインカメラ";
static const char cBossEelEnemyGroupJointName[] = "敵グルーブ";
static const char cBossEelEnemyGroupName[]      = "敵グループ";
static const char cBossEelHeadName[] = "めおとウナギの頭部";
static const char cBossEelBodyCollisionName[]    = "体コリジョン";
static const char cBossEelBarrierCollisionName[] = "障害コリジョン";
static const char cBossEelEyeModelPath[]         = "/scene/bosseel/eye.bmd";
static const char cBossEelEyeName[]              = "めおとウナギ目";
static const char cBossEelToothModelPath[]       = "/scene/bosseel/tooth.bmd";
static const char cBossEelBadToothModelPath[]
    = "/scene/bosseel/bad_tooth.bmd";
static const char cBossEelGoldToothModelPath[]
    = "/scene/bosseel/gold_tooth.bmd";
static const char cBossEelToothName[] = "めおとウナギの歯";
static const char cBossEelHeartCoinModelPath[]
    = "/scene/bosseel/meoto_heartcoin.bmd";
static const char cBossEelHeartCoinName[] = "めおとウナギハートコイン";
static const char cBossEelAwaCollisionName[] = "泡コリジョン";
static const char cBossEelVortexName[]       = "めおとウナギ渦";
static const char cBossEelCollisionCubeName[] = "コリジョンキューブ";

static const char* sEyePartsJointTable[] = {
	"eye1",
	"eye2",
	"eye3",
	"eye4",
};

static const char* sToothPartsJointTable[] = {
	"ha1", "ha2", "ha3", "ha4", "ha5", "ha6", "ha7", "ha8",
};

static const char* sCollisionJointTable[] = {
	"headcol1",
	"headcol2",
	"sebone5",
	"sebone5",
};

static const char* sCollisionFileTable[] = {
	"/bosseel/meoto_head",
	"/bosseel/meoto_head2",
	"/bosseel/meoto_camera",
	"/bosseel/meoto_out_loop",
};

static inline void setBossEelParticleScale(JPABaseEmitter* emitter,
                                           const TBossEel* eel)
{
	if (!emitter)
		return;

	emitter->unk154.x = eel->mScaling.x;
	emitter->unk154.y = eel->mScaling.y;
	emitter->unk154.z = eel->mScaling.z;
	emitter->unk174.x = eel->mScaling.x;
	emitter->unk174.y = eel->mScaling.y;
	emitter->unk174.z = eel->mScaling.z;
}

static inline void emitToothParticle(TBossEelTooth* tooth, s32 particle_id)
{
	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
	    particle_id, &tooth->mPosition, 1, tooth);
	setBossEelParticleScale(emitter, tooth->unk6C);
}

static inline void playBossEelSound(u32 sound_id,
                                    const JGeometry::TVec3<f32>* position)
{
	if (gpMSound->gateCheck(sound_id)) {
		MSoundSESystem::MSoundSE::startSoundActor(sound_id, position, 0,
		                                          nullptr, 0, 4);
	}
}

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

DEFINE_NERVE(TNerveBEelTearsMoveUp, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		tears->mMActor = tears->mMActorKeeper->getMActor("tears.bmd");
		tears->mMActor->setBckFromIndex(1);
	}

	tears->mPosition.y += tears->unk15C->mSLTearsUpSpeed.get();
	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsGenerate, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		tears->mMActor = tears->mMActorKeeper->getMActor("tears.bmd");
		tears->mMActor->setBckFromIndex(2);
	}

	if (tears->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBEelTearsMoveUp::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsSplit, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		if (gpMSound->gateCheck(0x8926)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8926, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->mMActor
		    = tears->mMActorKeeper->getMActor("tears_waterhit.bmd");
		tears->mMActor->setBckFromIndex(3);

		MActor* actor = tears->mMActor;
		actor->setFrameRate(
		    tears->unk15C->mSLHitAnmFrameRate.get() * SMSGetAnmFrameRate(),
		    0);
	}

	if (tears->checkCurAnmEnd(0)) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0xd5, &tears->mPosition, 0, nullptr);
		if (emitter) {
			emitter->unk154.x = tears->mScaling.x;
			emitter->unk154.y = tears->mScaling.y;
			emitter->unk154.z = tears->mScaling.z;
			emitter->unk174.x = tears->mScaling.x;
			emitter->unk174.y = tears->mScaling.y;
			emitter->unk174.z = tears->mScaling.z;
		}

		((u8*)tears->unk16C)[0x81] = FALSE;
		tears->unk16C->offHitFlag(HIT_FLAG_NO_COLLISION);
		((u8*)tears->unk16C)[0x80] = TRUE;
		tears->unk16C->mPosition = tears->mPosition;

		((TBEelTearsManager*)tears->mManager)->splitTears(tears->mPosition);

		if (gpMSound->gateCheck(0x8927)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8927, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->onLiveFlag(LIVE_FLAG_HIDDEN);
		spine->pushAfterCurrent(&TNerveBEelTearsMarioRecover::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsWaterHit, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		if (gpMSound->gateCheck(0x8926)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8926, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->mMActor
		    = tears->mMActorKeeper->getMActor("tears_waterhit.bmd");
		tears->mMActor->setBckFromIndex(3);
	}

	--tears->unk164;
	f32 frameRate = tears->unk15C->mSLHitAnmFrameRate.get();

	if (tears->unk164 < 0) {
		MActor* actor = tears->mMActor;
		actor->setFrameRate(-frameRate * SMSGetAnmFrameRate(), 0);
		if (tears->getCurAnmFrameNo(0) < 1.0f)
			return TRUE;
	} else {
		MActor* actor = tears->mMActor;
		actor->setFrameRate(frameRate * SMSGetAnmFrameRate(), 0);
	}

	if (tears->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBEelTearsMarioRecover::theNerve());

		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0xd5, &tears->mPosition, 0, nullptr);
		if (emitter) {
			emitter->unk154.x = tears->mScaling.x;
			emitter->unk154.y = tears->mScaling.y;
			emitter->unk154.z = tears->mScaling.z;
			emitter->unk174.x = tears->mScaling.x;
			emitter->unk174.y = tears->mScaling.y;
			emitter->unk174.z = tears->mScaling.z;
		}

		((u8*)tears->unk16C)[0x81] = FALSE;
		tears->unk16C->offHitFlag(HIT_FLAG_NO_COLLISION);
		((u8*)tears->unk16C)[0x80] = TRUE;
		tears->unk16C->mPosition = tears->mPosition;

		((TBEelTearsManager*)tears->mManager)->splitTears(tears->mPosition);

		if (gpMSound->gateCheck(0x8927)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x8927, &tears->mPosition, 0, nullptr, 0, 4);
		}

		tears->onLiveFlag(LIVE_FLAG_HIDDEN);
		return TRUE;
	}

	if (tears->unk160)
		tears->mPosition.y += tears->unk15C->mSLTearsDamageUpSpeed.get();

	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsMarioRecover, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (((u8*)tears->unk16C)[0x80] == FALSE) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0xd6, gpMarioPos, 0, nullptr);
		if (emitter) {
			emitter->unk154.x = tears->mScaling.x;
			emitter->unk154.y = tears->mScaling.y;
			emitter->unk154.z = tears->mScaling.z;
			emitter->unk174.x = tears->mScaling.x;
			emitter->unk174.y = tears->mScaling.y;
			emitter->unk174.z = tears->mScaling.z;
		}

		tears->kill();
		return TRUE;
	}

	JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
	    0x19d, &tears->mPosition, 1, tears);
	if (emitter) {
		emitter->unk154.x = tears->mScaling.x;
		emitter->unk154.y = tears->mScaling.y;
		emitter->unk154.z = tears->mScaling.z;
		emitter->unk174.x = tears->mScaling.x;
		emitter->unk174.y = tears->mScaling.y;
		emitter->unk174.z = tears->mScaling.z;
	}

	if (spine->getTime() > 1000) {
		tears->kill();
		return TRUE;
	}

	if (((u8*)tears->unk16C)[0x81] != FALSE) {
		tears->mPosition.y += tears->unk15C->mSLTearsUpSpeed.get();
		tears->unk16C->mPosition.y = tears->mPosition.y;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveOilBallStay, TLiveActor)
{
	TLiveActor* body = spine->getBody();
	MActor* actor    = body->mMActor;
	TOilBall* oilBall = (TOilBall*)body;

	if (actor->checkCurBckFromIndex(3)
	    || (oilBall->checkCurAnmEnd(0)
	        && !oilBall->mMActor->checkCurBckFromIndex(1))) {
		oilBall->mMActor = oilBall->mMActorKeeper->getMActor("tears.bmd");
		oilBall->mMActor->setBckFromIndex(1);
	}

	return FALSE;
}

TBossEel::TBossEel(const char* name)
    : TSpineEnemy(name)
    , unk1A8(nullptr)
    , unk1AC(nullptr)
    , unk1B0(nullptr)
    , unk1BC(1.0f)
    , unk1C0(0)
    , unk1C4(0)
    , unk1CC(500.0f)
    , unk1D0(TRUE)
    , unk1D4(2350.0f)
    , unk1D8(0.75f)
    , unk1E8(nullptr)
    , unk1F0(FALSE)
    , unk1F4(0.0f)
    , unk1FC(FALSE)
    , unk1FD(FALSE)
    , unk1FE(FALSE)
    , unk200(0)
    , unk210(nullptr)
    , unk214(nullptr)
    , unk218(nullptr)
    , unk21C(FALSE)
    , unk21D(TRUE)
{
	TBossEelUnk1EC* unk = new TBossEelUnk1EC;
	if (unk != nullptr) {
		unk->unk0 = 0;
		unk->unk4 = 1;
	}
	unk1EC = unk;
}

BOOL TBossEel::receiveMessage(THitActor*, u32) { return FALSE; }

BOOL TBossEel::hasMapCollision() const { return TRUE; }

const char** TBossEel::getBasNameTable() const { return bosseel_bastable; }

static s32 hoseiDiveCameraCallback(u32 actor, u32 state)
{
	if (state == 1) {
		JGeometry::TVec3<f32> cameraPos = ((JDrama::TActor*)actor)->mPosition;
		cameraPos.y += 12300.0f;
		gpCamera->warpPosAndAt(cameraPos, *gpMarioPos);
	}

	return 0;
}

bool TBossEel::isInBossEelMoguDemo()
{
	if (mMActor->checkCurBckFromIndex(12)
	    || mMActor->checkCurBckFromIndex(3))
		return true;

	return false;
}

static inline TBossEelEye* getBossEelEye(TBossEel* eel, int index)
{
	return *(TBossEelEye**)((u8*)eel + 0x15C + index * 4);
}

void TBossEel::collideToMario()
{
	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	JGeometry::TVec3<f32> correction;
	correction.zero();

	for (int i = 0; i < 2; ++i) {
		u16 jointIndex = *(u16*)((u8*)this + 0x1A0 + i * 2);
		MtxPtr mtx     = mMActor->getModel()->mNodeMatrices[jointIndex];

		JGeometry::TVec3<f32> normal;
		normal.x = mtx[0][1];
		normal.y = mtx[1][1];
		normal.z = mtx[2][1];
		if (i == 0)
			normal.scale(-1.0f);
		normal.normalize();

		JGeometry::TVec3<f32> jointPos;
		jointPos.x = mtx[0][3];
		jointPos.y = mtx[1][3];
		jointPos.z = mtx[2][3];

		JGeometry::TVec3<f32> delta;
		delta.x = marioPos.x - jointPos.x;
		delta.y = marioPos.y - jointPos.y;
		delta.z = marioPos.z - jointPos.z;

		f32 dist = delta.length();
		if (dist == 0.0f || dist >= unk1D4)
			continue;

		f32 dot = delta.dot(normal);
		if (dot >= 0.0f)
			continue;

		JGeometry::TVec3<f32> localCorrection;
		f32 penetration = unk1D4 - dist;
		if (-dot < penetration) {
			localCorrection = normal;
			localCorrection.scale(-dot);
		} else {
			localCorrection.normalize(delta);
			localCorrection.scale(penetration);
		}

		correction.add(localCorrection);
	}

	marioPos.add(correction);
	SMS_MarioMoveRequest(marioPos);
}

#define START_BOSS_EEL_EYE_CLOSE(eye)                                         \
	do {                                                                       \
		TBossEelEye* closeEye          = (eye);                                \
		closeEye->unk60               = closeEye->unk18->getCurAnmIdx(0);     \
		closeEye->unk5C               = 1;                                     \
		closeEye->unk64               = 1.0f;                                  \
		J3DAnmTransform* closeOldAnm  = nullptr;                               \
		if (closeEye->unk18->unkC)                                             \
			closeOldAnm = closeEye->unk18->unkC->unk24;                        \
		if (closeEye->unk18->unkC)                                             \
			closeEye->unk18->unkC->setOldMotionBlendAnmPtr(closeOldAnm);       \
		closeEye->unk18->setBckFromIndex(1);                                  \
		if (closeEye->unk18->unkC)                                             \
			closeEye->unk18->unkC->setMotionBlendRatio(closeEye->unk64);       \
	} while (0)

#define START_BOSS_EEL_BCK(eel, bck_index)                                    \
	do {                                                                       \
		TBossEel* bckEel            = (eel);                                   \
		bckEel->unk1B8             = bckEel->mMActor->getCurAnmIdx(0);        \
		bckEel->unk1B4             = (bck_index);                             \
		bckEel->unk1BC             = 1.0f;                                    \
		J3DAnmTransform* bckOldAnm = nullptr;                                 \
		if (bckEel->mMActor->unkC)                                            \
			bckOldAnm = bckEel->mMActor->unkC->unk24;                         \
		if (bckEel->mMActor->unkC)                                            \
			bckEel->mMActor->unkC->setOldMotionBlendAnmPtr(bckOldAnm);        \
		bckEel->mMActor->setBckFromIndex(bck_index);                          \
		if (bckEel->mMActor->unkC)                                            \
			bckEel->mMActor->unkC->setMotionBlendRatio(bckEel->unk1BC);       \
		f32 bckRate = 0.25f * SMSGetAnmFrameRate();                           \
		bckEel->mMActor->getFrameCtrl(0)->setRate(bckRate);                   \
		const char* bckBas       = nullptr;                                   \
		const char** bckBasTable = bckEel->getBasNameTable();                 \
		if (bckBasTable)                                                      \
			bckBas = bckBasTable[bck_index];                                  \
		bckEel->setAnmSound(bckBas);                                          \
	} while (0)

void TBossEel::forceShedTears(bool use_back_eye)
{
	unk1D0 = !unk1D0;

	int eyeIndex;
	if (use_back_eye)
		eyeIndex = unk1D0 ? 3 : 2;
	else
		eyeIndex = unk1D0 ? 1 : 0;

	TBossEelEye** eyeSlot = &unk15C[eyeIndex];
	MtxPtr mtx             = (*eyeSlot)->getConnectedMtx();
	TBossEelEye* eye       = *eyeSlot;
	START_BOSS_EEL_EYE_CLOSE(eye);
	(*eyeSlot)->unk6C = 0;
	shedTears(mtx);
}

void TBossEel::shedTears(MtxPtr mtx)
{
	JGeometry::TVec3<f32> position;
	position.x = mtx[0][3];
	position.y = mtx[1][3];
	position.z = mtx[2][3];

	TBEelTears* tears = (TBEelTears*)gpConductor->makeOneEnemyAppear(
	    position, cBossEelTearsManagerName, 0);
	if (!tears)
		return;

	tears->unk168 = mtx;
	tears->reset();

	JGeometry::TVec3<f32> velocity;
	velocity.x = 0.0f;
	velocity.y = 0.0f;
	velocity.z = 100.0f;

	Mtx rot;
	MsMtxSetRotRPH(rot, 0.0f, rand() * 0.000030517578f * 360.0f, 0.0f);
	PSMTXMultVec(rot, &velocity, &velocity);

	tears->mPosition.x += velocity.x;
	tears->mPosition.z += velocity.z;

	MsVECNormalize(&velocity, &velocity);
	velocity.x *= 10.0f;
	velocity.z *= 10.0f;
	tears->mVelocity = velocity;
}

void TBossEel::updateTearsCnt()
{
	++unk1C4;

	int interval = unk1E8->mSLGenTearsTime.value;
	f32 diff     = fabsf(mPosition.y - gpMarioPos->y);
	if (diff > 30000.0f)
		interval *= 4;
	else if (diff > 15000.0f)
		interval *= 3;
	else if (diff > 6000.0f)
		interval *= 2;

	if (unk1C4 == interval - 100) {
		int eyeIndex     = dummy1210[unk1C0];
		TBossEelEye* eye = getBossEelEye(this, eyeIndex);
		START_BOSS_EEL_EYE_CLOSE(eye);
		START_BOSS_EEL_EYE_CLOSE(eye->unk68);
	}

	if (unk1C4 > interval) {
		unk1C4           = 0;
		int eyeIndex     = dummy1210[unk1C0];
		TBossEelEye* eye = getBossEelEye(this, eyeIndex);
		MtxPtr mtx       = eye->getConnectedMtx();
		START_BOSS_EEL_EYE_CLOSE(eye);
		eye->unk6C = 0;
		shedTears(mtx);

		++unk1C0;
		if (unk1C0 >= 4)
			unk1C0 = 0;
	}
}

MtxPtr TBossEel::getTakingMtx()
{
	return mMActor->getModel()->mNodeMatrices[7];
}

void TBossEel::perform(u32 flags, JDrama::TGraphics* graphics)
{
	u32 calcFlag = flags & 2;

	if (calcFlag && mSpine->getCurrentNerve() == &TNerveBossEelDie::theNerve()) {
		unk1DC.set(0.0f, -14000.0f, 0.0f);
		gpMarioParticleManager->emit(0x198, &unk1DC, 1, this);
	}

	if (mMActor->checkCurBckFromIndex(4))
		((TBossEelAwaCollision*)unk214)->perform(flags, graphics);

	if (mLiveFlag & LIVE_FLAG_UNK200)
		return;

	if (flags & 1) {
		f32 scale = unk1E8->mSLBodyScale.value;
		mScaling.set(scale, scale, scale);
		mPosition.y = unk150.y + unk1E8->mSLInitTransYOffset.value + unk1F4;

		TCubeGeneralInfo* largeCube = &(*unk1AC->unk14)[0];
		largeCube->unkC.x          = mPosition.x;
		largeCube->unkC.y          = mPosition.y + 9600.0f * mScaling.y;
		largeCube->unkC.z          = mPosition.z;
		largeCube->unk24.x         = 7000.0f * mScaling.x;
		largeCube->unk24.y         = 10000.0f * mScaling.y;
		largeCube->unk24.z         = 7000.0f * mScaling.z;

		moveObject();
		mMActor->calcAnm();
		calcRootMatrix();
		collideToMario();

		TMtx34f jointMtx;
		jointMtx.set(mMActor->getModel()->mNodeMatrices[unk1A0[2]]);
		unk190[2]->moveMtx(jointMtx);

		if (mUseMapCollision) {
			TMtx34f mapJointMtx;
			for (int i = 0; i < 2; ++i) {
				mapJointMtx.set(
				    mMActor->getModel()->mNodeMatrices[unk1A0[i]]);
				unk190[i]->moveMtx(mapJointMtx);
			}
		}

		if (mUseObjCollision
		    && mSpine->getCurrentNerve() != &TNerveBossEelDie::theNerve())
			((THitActor*)unk1B0)->offHitFlag(HIT_FLAG_NO_COLLISION);
		else
			((THitActor*)unk1B0)->onHitFlag(HIT_FLAG_NO_COLLISION);

		THitActor* head = (THitActor*)unk1A8;
		head->mPosition = mPosition;
		head->mPosition.y += unk1E8->mSLBodyToHeadDistance.value * mScaling.y;
		head->mAttackRadius = unk1E8->mSLHeadAttackRadius.value * mScaling.x;
		head->mAttackHeight = unk1E8->mSLHeadAttackHeight.value * mScaling.x;
		head->mDamageRadius = unk1E8->mSLHeadDamageRadius.value * mScaling.x;
		head->mDamageHeight = unk1E8->mSLHeadDamageHeight.value * mScaling.x;
		head->calcEntryRadius();

		mAttackRadius = unk1E8->mSLBodyAttackRadius.value * mScaling.x;
		mAttackHeight = unk1E8->mSLBodyAttackHeight.value * mScaling.x;
		mDamageRadius = unk1E8->mSLBodyDamageRadius.value * mScaling.x;
		mDamageHeight = unk1E8->mSLBodyDamageHeight.value * mScaling.x;
		calcEntryRadius();

		TCubeGeneralInfo* smallCube = &(*unk1AC->unk14)[1];
		smallCube->unk18.x          = mRotation.x;
		smallCube->unk18.y          = mRotation.y;
		smallCube->unk18.z          = mRotation.z;
		smallCube->unkC.x           = mPosition.x;
		smallCube->unkC.y           = mPosition.y + 1900.0f;
		smallCube->unkC.z           = mPosition.z;
		smallCube->unk24.x          = 1100.0f;
		smallCube->unk24.y          = 1000.0f;
		smallCube->unk24.z          = 1100.0f;

		largeCube->unkC.x  = mPosition.x;
		largeCube->unkC.y  = mPosition.y + 9600.0f * mScaling.y;
		largeCube->unkC.z  = mPosition.z;
		largeCube->unk24.x = 7000.0f * mScaling.x;
		largeCube->unk24.y = 10000.0f * mScaling.y;
		largeCube->unk24.z = 7000.0f * mScaling.z;

		if (mHitPoints != 0) {
			unk1C8 = FALSE;
			for (int i = 0; i < head->getColNum(); ++i) {
				THitActor* hitActor = head->getCollision(i);
				if (hitActor->isActorTypeOf(ACTOR_TYPE_PLAYER))
					unk1C8 = TRUE;
			}

			if (mSpine->getCurrentNerve()
			        != &TNerveBossEelFirstSpin::theNerve()
			    && mSpine->getCurrentNerve()
			        != &TNerveBossEelSecondSpin::theNerve()
			    && mSpine->getCurrentNerve()
			        != &TNerveBossEelMouthOpenWait::theNerve()) {
				if (mMActor->checkCurBckFromIndex(10)
				    || mMActor->checkCurBckFromIndex(16)
				    || mMActor->checkCurBckFromIndex(18)
				    || mMActor->checkCurBckFromIndex(19))
					updateTearsCnt();
			}

			unk1BC -= 0.01f;
			if (unk1BC > 1.0f)
				unk1BC = 1.0f;
			else if (unk1BC < 0.0f)
				unk1BC = 0.0f;

			if (mMActor->unkC)
				mMActor->unkC->setMotionBlendRatio(unk1BC);
		}

		bool allTeethBroken = true;
		for (int i = 0; i < 8; ++i) {
			if (unk16C[i]->unk70 > 1) {
				allTeethBroken = false;
				break;
			}
		}

		if (allTeethBroken
		    && mSpine->getCurrentNerve() != &TNerveBossEelDie::theNerve()) {
			mSpine->setNext(&TNerveBossEelDie::theNerve());
			head->onHitFlag(HIT_FLAG_NO_COLLISION);
		}
	}

	if (calcFlag) {
		offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		updateAnmSound();

		if (mSpine->getCurrentNerve()
		        == &TNerveBossEelSecondSpin::theNerve()
		    || mSpine->getCurrentNerve()
		        == &TNerveBossEelFirstSpin::theNerve()) {
			MtxPtr mtx = mMActor->getModel()->mNodeMatrices[5];
			unk204.x   = mtx[0][3];
			unk204.y   = mtx[1][3];
			unk204.z   = mtx[2][3];

			JPABaseEmitter* emitter;
			if (mLiveFlag & LIVE_FLAG_UNK10000)
				emitter = gpMarioParticleManager->emit(0x193, &unk204, 1, this);
			else
				emitter = gpMarioParticleManager->emit(0x194, &unk204, 1, this);
			setBossEelParticleScale(emitter, this);

			if (mLiveFlag & LIVE_FLAG_UNK10000)
				emitter = gpMarioParticleManager->emit(0x195, &unk204, 1, this);
			else
				emitter = gpMarioParticleManager->emit(0x196, &unk204, 1, this);
			setBossEelParticleScale(emitter, this);
		}

		if (mMActor->checkCurBckFromIndex(15)) {
			J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
			MtxPtr mtx              = mMActor->getModel()->mNodeMatrices[5];
			unk204.x                = mtx[0][3];
			unk204.y                = mtx[1][3];
			unk204.z                = mtx[2][3];

			if (frameCtrl->checkPass(102.0f)) {
				JPABaseEmitter* emitter
				    = gpMarioParticleManager->emitAndBindToPosPtr(
				        0xD4, &unk204, 0, nullptr);
				setBossEelParticleScale(emitter, this);
			}

			if (frameCtrl->getFrame() < 40.0f) {
				JPABaseEmitter* emitter
				    = gpMarioParticleManager->emit(0x197, &unk204, 1, this);
				setBossEelParticleScale(emitter, this);
			}
		}

		if (mMActor->checkCurBckFromIndex(14)) {
			mMActor->getFrameCtrl(0);
			MtxPtr mtx = mMActor->getModel()->mNodeMatrices[5];
			unk204.x   = mtx[0][3];
			unk204.y   = mtx[1][3];
			unk204.z   = mtx[2][3];
			playBossEelSound(0x8120, &mPosition);

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emitAndBindToPosPtr(
			        0x199, &unk204, 1, this);
			if (emitter) {
				emitter->unk154.x = 3.0f;
				emitter->unk154.y = 3.0f;
				emitter->unk154.z = 3.0f;
				emitter->unk174.x = 3.0f;
				emitter->unk174.y = 3.0f;
				emitter->unk174.z = 3.0f;
			}
		}
	}

	if (flags & 4)
		mMActor->viewCalc();

	if (flags & 0x200)
		mMActor->entry();

	for (int i = 0; i < 4; ++i)
		unk15C[i]->testPerform(flags, graphics);

	for (int i = 0; i < 8; ++i)
		unk16C[i]->testPerform(flags, graphics);

	((TBossEelHeartCoin*)unk218)->perform(flags, graphics);
	unk18C->perform(flags, graphics);
	((TBossEelBodyCollision*)unk1B0)->perform(flags, graphics);
	((TBossEelBarrierCollision*)unk210)->perform(flags, graphics);
}

void TBossEel::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager);
	mMActor       = mMActorKeeper->createMActorFromAllBmd(0);
	unk1E8       = &((TBossEelManager*)manager)->mSaveParams;
	unk150       = mPosition;

	onLiveFlag(LIVE_FLAG_UNK8 | LIVE_FLAG_UNK10);

	mGroundHeight
	    = gpMap->checkGround(mPosition.x, mPosition.y + mBodyScale * mHeadHeight,
	                         mPosition.z, nullptr);

	if (mMActor->unkC)
		mMActor->unkC->initNormalMotionBlend();

	mSpine->initWith(&TNerveBossEelWaitAppear::theNerve());

	initHitActor(0x08000003, 1, 0x80000000,
	             unk1E8->mSLBodyAttackRadius.value,
	             unk1E8->mSLBodyAttackHeight.value,
	             unk1E8->mSLBodyDamageRadius.value,
	             unk1E8->mSLBodyDamageHeight.value);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	J3DModel* model = mMActor->getModel();
	if (!model->getSkinDeform())
		model->setSkinDeform(new J3DSkinDeform, J3D_DEFORM_ATTACH_FLAG_UNK_1);
	mMActor->resetDL();

	TIdxGroupObj* enemyGroup
	    = JDrama::TNameRefGen::search<TIdxGroupObj>(cBossEelEnemyGroupName);
	JGadget::TList_pointer_void* enemyList
	    = (JGadget::TList_pointer_void*)((u8*)enemyGroup + 0x10);
	JUTNameTab* jointName = model->getModelData()->getJointName();

	unk1A8 = new THitActor(cBossEelHeadName);
	((THitActor*)unk1A8)
	    ->initHitActor(0x08000003, 2, 0x80000000,
	                   unk1E8->mSLHeadAttackRadius.value,
	                   unk1E8->mSLHeadAttackHeight.value,
	                   unk1E8->mSLHeadDamageRadius.value,
	                   unk1E8->mSLHeadDamageHeight.value);
	enemyList->insert(enemyList->end(), unk1A8);
	((THitActor*)unk1A8)->offHitFlag(HIT_FLAG_NO_COLLISION);

	unk1B0
	    = new TBossEelBodyCollision(model->getBaseTRMtx(),
	                                cBossEelBodyCollisionName);
	((TBossEelBodyCollision*)unk1B0)->initCollision();
	((TBossEelBodyCollision*)unk1B0)->unk7C = this;
	enemyList->insert(enemyList->end(), unk1B0);
	((THitActor*)unk1B0)->offHitFlag(HIT_FLAG_NO_COLLISION);

	unk210 = new TBossEelBarrierCollision(model->mNodeMatrices[7],
	                                      cBossEelBarrierCollisionName);
	((TBossEelBarrierCollision*)unk210)->initCollision();
	enemyList->insert(enemyList->end(), unk210);
	((THitActor*)unk210)->offHitFlag(HIT_FLAG_NO_COLLISION);

	SDLModelData* eyeModelData = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(cBossEelEyeModelPath), 0x10240000));
	for (int i = 0; i < 4; ++i) {
		unk15C[i] = new TBossEelEye(this,
		                            jointName->getIndex(sEyePartsJointTable[i]),
		                            eyeModelData, 3, cBossEelEyeName);
	}
	unk15C[0]->unk68 = unk15C[1];
	unk15C[1]->unk68 = unk15C[0];
	unk15C[2]->unk68 = unk15C[3];
	unk15C[3]->unk68 = unk15C[2];
	unk15C[2]->getMActor()->getFrameCtrl(0)->setFrame(100.0f);
	unk15C[3]->getMActor()->getFrameCtrl(0)->setFrame(100.0f);

	SDLModelData* toothModelData[3];
	toothModelData[0] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(cBossEelToothModelPath), 0x10100000));
	toothModelData[1] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(cBossEelBadToothModelPath),
	    0x10100000));
	toothModelData[2] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(cBossEelGoldToothModelPath),
	    0x10100000));

	for (int i = 0; i < 8; ++i) {
		u8 toothType = 0;
		if (i == 1 || i == 4 || i == 7)
			toothType = 1;
		if (i == 6)
			toothType = 2;

		unk16C[i] = new TBossEelTooth(toothType, this,
		                              sToothPartsJointTable[i],
		                              toothModelData[toothType],
		                              cBossEelToothName);
		unk16C[i]->unkBC = (i <= 2 || i == 7) ? TRUE : FALSE;
	}

	SDLModelData* heartModelData = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource(cBossEelHeartCoinModelPath),
	    0x10240000));
	unk218 = new TBossEelHeartCoin(this, 0, heartModelData, 3,
	                               cBossEelHeartCoinName);

	for (int i = 0; i < 4; ++i) {
		unk1A0[i]  = jointName->getIndex(sCollisionJointTable[i]);
		unk190[i] = new TMapCollisionMove();
		unk190[i]->init(sCollisionFileTable[i], 2, this);
		unk190[i]->moveTrans(mPosition);
	}

	unk214 = new TBossEelAwaCollision(model->mNodeMatrices[unk1A0[2]],
	                                  cBossEelAwaCollisionName);
	((TBossEelAwaCollision*)unk214)->initCollision();
	enemyList->insert(enemyList->end(), unk214);
	((THitActor*)unk214)->onHitFlag(HIT_FLAG_NO_COLLISION);

	unk18C = new TBossEelVortex(this, cBossEelVortexName);
	unk18C->initHitActor(0x08000003, 3, 0x80000000,
	                      unk1E8->mSLVortexAttackRadius.value,
	                      unk1E8->mSLVortexAttackHeight.value,
	                      unk1E8->mSLVortexDamageRadius.value,
	                      unk1E8->mSLVortexDamageHeight.value);
	enemyList->insert(enemyList->end(), unk18C);
	unk18C->offHitFlag(HIT_FLAG_NO_COLLISION);

	unk1AC = new TCubeManagerBase(cBossEelCollisionCubeName, (u8)2);
	TCubeGeneralInfo* cubeInfo = &(*unk1AC->unk14)[0];
	cubeInfo->unkC.x  = mPosition.x;
	cubeInfo->unkC.y  = mPosition.y + 9600.0f;
	cubeInfo->unkC.z  = mPosition.z;
	cubeInfo->unk24.x = 7000.0f;
	cubeInfo->unk24.y = 10000.0f;
	cubeInfo->unk24.z = 7000.0f;

	initAnmSound();
	model->calc();
}

TBossEelHeartCoin::TBossEelHeartCoin(TBossEel* boss, int index,
                                     SDLModelData* model_data, u32 flags,
                                     const char* name)
    : TSharedParts(boss, index, model_data, flags, name)
    , unk1C(FALSE)
    , unk7C(nullptr)
{
	unk7C = boss;

	for (int i = 0; i < 20; ++i) {
		mCoins[i] = gpItemManager->newAndRegisterCoinReal();
		mCoins[i]->kill();
	}
}

void TBossEelHeartCoin::generate(JGeometry::TVec3<f32>& position)
{
	unk70.set(position.x, position.y, position.z);
	unk1C = TRUE;

	for (int i = 0; i < 20; ++i) {
		mCoins[i]->appear();
		mCoins[i]->onMapObjFlag(0x10000000);

		MtxPtr mtx = unk18->getModel()->mNodeMatrices[0];
		TCoin* coin = mCoins[i];
		coin->mPosition.set(mtx[0][3], mtx[1][3], mtx[2][3]);
	}
}

void TBossEelHeartCoin::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (!unk1C)
		return;

	u32 calcFlag = flags & 2;

	if (calcFlag) {
		MActor* bossActor = unk7C->mMActor;
		int jointIndex = bossActor->getModel()
		                     ->getModelData()
		                     ->getJointName()
		                     ->getIndex("ha7");
		MtxPtr mtx = bossActor->getModel()->mNodeMatrices[jointIndex];

		if (bossActor->checkCurBckFromIndex(3)
		    && bossActor->getFrameCtrl(0)->getFrame() < 700.0f) {
			unk70.set(mtx[0][3], mtx[1][3], mtx[2][3]);
		}

		unk70.y = mtx[1][3];

		TPosition3f partMtx;
		partMtx.translation(unk70.x, unk70.y, unk70.z);
		PSMTXCopy(partMtx, unk18->getModel()->unk20);
	}

	unk18->perform(flags, graphics);

	if (calcFlag) {
		for (int i = 0; i < 20; ++i) {
			MtxPtr mtx = unk18->getModel()->mNodeMatrices[i + 2];
			TCoin* coin = mCoins[i];
			coin->mPosition.set(mtx[0][3], mtx[1][3], mtx[2][3]);
		}
	}
}

static inline void resetBossEelEyeMotionBlend(TBossEelEye* eye)
{
	eye->unk60 = eye->unk18->getCurAnmIdx(0);
	eye->unk5C = 0;
	eye->unk64 = 1.0f;

	J3DAnmTransform* oldAnm = nullptr;
	if (eye->unk18->unkC)
		oldAnm = eye->unk18->unkC->unk24;

	if (eye->unk18->unkC)
		eye->unk18->unkC->setOldMotionBlendAnmPtr(oldAnm);

	eye->unk18->setBckFromIndex(0);

	if (eye->unk18->unkC)
		eye->unk18->unkC->setMotionBlendRatio(eye->unk64);
}

void TBossEelEye::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk10->checkLiveFlag(
	        LIVE_FLAG_DEAD | LIVE_FLAG_HIDDEN | LIVE_FLAG_CLIPPED_OUT))
		return;

	if (flags & 2) {
		f32 z = getConnectedMtx()[2][3];
		f32 y = getConnectedMtx()[1][3];
		f32 x = getConnectedMtx()[0][3];
		unk70.x = x;
		unk70.y = y;
		unk70.z = z;

		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
		    0x192, &unk70, 1, this);
		if (emitter) {
			const JGeometry::TVec3<f32>& scale = unk10->mScaling;
			emitter->unk154.x                  = scale.x;
			emitter->unk154.y                  = scale.y;
			emitter->unk154.z                  = scale.z;
			emitter->unk174.x                  = scale.x;
			emitter->unk174.y                  = scale.y;
			emitter->unk174.z                  = scale.z;
		}

		Mtx mtx;
		PSMTXCopy(getConnectedMtx(), mtx);
		PSMTXCopy(mtx, unk18->getModel()->unk20);

		if (unk50 == 0)
			PSMTXCopy(mtx, unk1C);

		f32 blend = unk64 - 0.01f;
		if (blend > 1.0f)
			blend = 1.0f;
		else if (blend < 0.0f)
			blend = 0.0f;
		unk64 = blend;

		if (unk18->unkC)
			unk18->unkC->setMotionBlendRatio(unk64);

		if (unk5C == 1 && unk18->curAnmEndsNext(0, nullptr)) {
			++unk6C;
			if (unk6C > 3) {
				resetBossEelEyeMotionBlend(this);
				resetBossEelEyeMotionBlend(unk68);
			}
		}
	}

	unk18->perform(flags, graphics);
	unk50 = -1;
}

TBossEelEye::TBossEelEye(const TLiveActor* owner, int index,
                         SDLModelData* model_data, u32 flags,
                         const char* name)
    : TSharedParts(owner, index, model_data, flags, name)
    , unk4C(nullptr)
    , unk50(-1)
    , unk54(0)
    , unk56(0)
    , unk58(0)
    , unk5A(50)
{
	unk4C = new SDLModel(model_data, flags, 1);
	unk4C->getModelData()->getMaterialName()->getIndex("_mat7");

	if (unk18->unkC)
		unk18->unkC->initNormalMotionBlend();

	resetBossEelEyeMotionBlend(this);
}

TBossEelSaveParams::TBossEelSaveParams()
    : TParams("/enemy/bosseel.prm")
    , PARAM_INIT(mSLInitTransYOffset, 0.0f)
    , PARAM_INIT(mSLAppearMoveDistY, 6000.0f)
    , PARAM_INIT(mSLBodyScale, 0.75f)
    , PARAM_INIT(mSLViewClipFar, 25000.0f)
    , PARAM_INIT(mSLViewClipRadius, 12000.0f)
    , PARAM_INIT(mSLBodyToHeadDistance, 6000.0f)
    , PARAM_INIT(mSLBodyAttackRadius, 2400.0f)
    , PARAM_INIT(mSLBodyAttackHeight, 6300.0f)
    , PARAM_INIT(mSLBodyDamageRadius, 2200.0f)
    , PARAM_INIT(mSLBodyDamageHeight, 6100.0f)
    , PARAM_INIT(mSLHeadAttackRadius, 3600.0f)
    , PARAM_INIT(mSLHeadAttackHeight, 5600.0f)
    , PARAM_INIT(mSLHeadDamageRadius, 3400.0f)
    , PARAM_INIT(mSLHeadDamageHeight, 5400.0f)
    , PARAM_INIT(mSLToothAttackRadius, 500.0f)
    , PARAM_INIT(mSLToothAttackHeight, 300.0f)
    , PARAM_INIT(mSLToothDamageRadius, 600.0f)
    , PARAM_INIT(mSLToothDamageHeight, 400.0f)
    , PARAM_INIT(mSLSpinAccel, 0.01f)
    , PARAM_INIT(mSLSpinMaxSpeed, 10.0f)
    , PARAM_INIT(mSLToothUpSpeed, 5.0f)
    , PARAM_INIT(mSLToothLiveHeight, 2000.0f)
    , PARAM_INIT(mSLToothMaxHitPoint, 50)
    , PARAM_INIT(mSLGenTearsTime, 100)
    , PARAM_INIT(mSLVortexAttackRadius, 500.0f)
    , PARAM_INIT(mSLVortexAttackHeight, 300.0f)
    , PARAM_INIT(mSLVortexDamageRadius, 600.0f)
    , PARAM_INIT(mSLVortexDamageHeight, 400.0f)
    , PARAM_INIT(mSLVortexLiveTimer, 300)
    , PARAM_INIT(mSLVortexScaleXZ, 1.0f)
    , PARAM_INIT(mSLVortexScaleY, 1.0f)
    , PARAM_INIT(mSLMouthOpenFrame, 500)
    , PARAM_INIT(mSLMouthOpenInterval, 2000)
    , PARAM_INIT(mSLCanEatFrame, 200)
    , PARAM_INIT(mSLBreathInPower, 10.0f)
{
	TParams::load(mPrmPath);
}

void TBossEelManager::clipEnemies(JDrama::TGraphics* graphics)
{
	clipActorsAux(graphics, mSaveParams.mSLViewClipFar.get(),
	              mSaveParams.mSLViewClipRadius.get());
}

void TBossEelManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "meoto_model.bmd", 0x10020000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBossEelCollision::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcEntryRadius();

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER))
				behaveToMario();
		}
	}

	if (flags & 2) {
		mPosition.x = unk68[0][3];
		mPosition.y = unk68[1][3];
		mPosition.z = unk68[2][3];
	}

	THitActor::perform(flags, graphics);
}

void TBossEelCollision::behaveToMario()
{
	JGeometry::TVec3<f32> velocity(0.0f, TBossEel::mForcePow, 0.0f);
	velocity.add(*gpMarioPos);
	SMS_MarioMoveRequest(velocity);

	TBossEel* eel = unk7C;
	if (eel == nullptr)
		return;

	bool canEat;
	if (*(u8*)((u8*)eel + 0x1C8)) {
		canEat = true;
	} else {
		MtxPtr mtx = eel->mMActor->getModel()
		                 ->mNodeMatrices[*(u16*)((u8*)eel + 0x1A0)];
		JGeometry::TVec3<f32> pos = *gpMarioPos;
		pos.x -= mtx[0][3];
		pos.y -= mtx[1][3];
		pos.z -= mtx[2][3];

		f32 dist = MsVECMag2(&pos);
		if (dist < eel->unk1D4 * eel->unk1D8)
			canEat = true;
		else
			canEat = false;
	}

	if (canEat) {
		TBossEel* parent = unk7C;
		if (parent->mSpine->getCurrentNerve()
		    != &TNerveBossEelEat::theNerve())
			parent->mSpine->pushNerve(&TNerveBossEelEat::theNerve());
	}
}

void TBossEelCollision::initCollision()
{
	initHitActor(0x08000023, 5, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelTearsRecoverCollision::perform(u32 flags,
                                            JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcEntryRadius();

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER))
				behaveToMario();
		}
	}

	THitActor::perform(flags, graphics);
}

void TBossEelTearsRecoverCollision::behaveToMario()
{
	SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
	unk80 = FALSE;
	onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBossEelTearsRecoverCollision::initCollision()
{
	unk74 = 400.0f;
	unk6C = 400.0f;
	unk78 = 400.0f;
	unk70 = 400.0f;
	initHitActor(0x2000002C, 3, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelBarrierCollision::behaveToMario()
{
	JGeometry::TVec3<f32> velocity(0.0f, TBossEel::mForcePow, 0.0f);
	velocity.add(*gpMarioPos);
	SMS_MarioMoveRequest(velocity);
}

void TBossEelBarrierCollision::initCollision()
{
	unk74 = 0.0f;
	unk6C = 0.0f;
	unk78 = 0.0f;
	unk70 = 0.0f;
	initHitActor(0x08000023, 5, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelAwaCollision::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcEntryRadius();

		if (gpMarioPos->y < mPosition.y + 500.0f)
			offHitFlag(HIT_FLAG_NO_COLLISION);

		if (gpMarioPos->y > mPosition.y + mAttackHeight)
			onHitFlag(HIT_FLAG_NO_COLLISION);

		for (int i = 0; i < mColCount; ++i) {
			if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER))
				behaveToMario();
		}
	}

	if (flags & 2) {
		mPosition.x = unk68[0][3];
		mPosition.y = unk68[1][3] + 11000.0f;
		mPosition.z = unk68[2][3];
	}

	THitActor::perform(flags, graphics);
}

void TBossEelAwaCollision::behaveToMario()
{
	JGeometry::TVec3<f32> velocity(0.0f, 10.0f, 0.0f);
	velocity.y = 15.0f;
	*gpMarioSpeedY = 0.0f;
	velocity.add(*gpMarioPos);
	SMS_MarioMoveRequest(velocity);
}

void TBossEelAwaCollision::initCollision()
{
	unk74 = 2000.0f;
	unk6C = 2000.0f;
	unk78 = 2000.0f;
	unk70 = 2000.0f;
	initHitActor(0x08000003, 2, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelBodyCollision::initCollision()
{
	unk6C = 2000.0f;
	unk74 = 2000.0f;
	unk70 = 5800.0f;
	unk78 = 5000.0f;
	initHitActor(0x08000023, 5, 0x80000000, unk6C, unk70, unk74, unk78);
}

void TBossEelVortex::reset()
{
	offHitFlag(HIT_FLAG_NO_COLLISION);
	onHitFlag(0x80000000);
	unk70 = 0;
}

void TBossEelVortex::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk6C)
		return;

	if (flags & 1) {
		TBossEelSaveParams* params = unk68->unk1E8;
		f32 scale                  = unk68->mScaling.x;
		f32 attackRadius           = params->mSLVortexAttackRadius.get() * scale;
		f32 attackHeight           = params->mSLVortexAttackHeight.get() * scale;
		f32 damageHeight           = params->mSLVortexDamageHeight.get() * scale;
		f32 damageRadius           = params->mSLVortexDamageRadius.get() * scale;
		mAttackRadius              = attackRadius;
		mAttackHeight              = attackHeight;
		mDamageRadius              = damageRadius;
		mDamageHeight              = damageHeight;
		calcEntryRadius();

		++unk70;
		if (unk70 > 30) {
			if (unk68->mMActor->checkCurBckFromIndex(14)
			    || unk68->mMActor->checkCurBckFromIndex(17)) {
				for (int i = 0; i < mColCount; ++i) {
					if (mCollisions[i]->isActorTypeOf(ACTOR_TYPE_PLAYER)) {
						JGeometry::TVec3<f32> velocity;
						velocity.x = unk68->mPosition.x - gpMarioPos->x;
						velocity.y = unk68->mPosition.y - gpMarioPos->y;
						velocity.z = unk68->mPosition.z - gpMarioPos->z;
						MsVECNormalize(&velocity, &velocity);

						f32 power = params->mSLBreathInPower.get();
						f32 pulse = fabsf(JMASin(unk70 * 0.9f));
						if (pulse > 1.0f)
							pulse = 1.0f;
						else if (pulse < 0.1f)
							pulse = 0.1f;

						power *= pulse;
						SMSRumbleMgr->start(8, &mPosition);

						if (unk68->mMActor->checkCurBckFromIndex(17))
							power *= 0.5f;

						velocity.x *= power;
						velocity.y *= power;
						velocity.z *= power;
						velocity.add(*gpMarioPos);
						SMS_MarioMoveRequest(velocity);
					}
				}
			} else {
				onHitFlag(HIT_FLAG_NO_COLLISION);
			}
		}
	}

	if (flags & 2) {
		mPosition.x
		    = unk68->mMActor->getModel()->mNodeMatrices[unk68->unk1A0[2]][0][3];
		mPosition.y = unk68->mMActor->getModel()
		                  ->mNodeMatrices[unk68->unk1A0[2]][1][3]
		            + 1000.0f;
		mPosition.z
		    = unk68->mMActor->getModel()->mNodeMatrices[unk68->unk1A0[2]][2][3];
	}

	THitActor::perform(flags, graphics);
}

void TBossEelTooth::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk6C->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT))
		return;

	if (unk70 == 0)
		return;

	if (flags & 1) {
		TBossEelSaveParams* params = unk6C->unk1E8;
		f32 scale                  = unk6C->mScaling.x;
		mAttackRadius              = params->mSLToothAttackRadius.value * scale;
		mAttackHeight              = params->mSLToothAttackHeight.value * scale;
		mDamageRadius              = params->mSLToothDamageRadius.value * scale;
		mDamageHeight              = params->mSLToothDamageHeight.value * scale;
		calcEntryRadius();

		for (int i = 0; i < mColCount; ++i) {
			THitActor* hit = mCollisions[i];
			if (unk6C->mSpine->getCurrentNerve()
			        != &TNerveBossEelEat::theNerve()
			    && hit->isActorTypeOf(ACTOR_TYPE_PLAYER) && unk70 > 1) {
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
			}
		}

		if (unk84 > 0) {
			--unk84;
		} else {
			MActor* actor = unk68->getMActor();
			if (actor->checkCurBckFromIndex(0x16)
			    && actor->curAnmEndsNext(0, nullptr))
				actor->setFrameRate(0.0f, 0);

			if (unk70 == 1) {
				if (unk74 == 1) {
					if (actor->checkCurBckFromIndex(0x16)) {
						actor->setBckFromIndex(0x14);

						JGeometry::TVec3<f32> pos;
						pos.x = unk88[0][3];
						pos.y = unk88[1][3] + unk7C;
						pos.z = unk88[2][3];

						TBEelTears* tears = (TBEelTears*)gpConductor
						    ->makeOneEnemyAppear(pos, cBossEelTearsManagerName,
						                         0);
						if (tears) {
							tears->unk16C->unk81 = FALSE;
							tears->unk16C->offHitFlag(HIT_FLAG_NO_COLLISION);
							tears->unk16C->unk80 = TRUE;
							tears->unk16C->mPosition = tears->mPosition;
							tears->unk16C->mPosition = tears->mPosition;
							tears->mSpine->initWith(
							    &TNerveBEelTearsMarioRecover::theNerve());
							tears->onLiveFlag(LIVE_FLAG_HIDDEN);
							tears->unk16C->unk81 = TRUE;
						}

						actor->setFrameRate(SMSGetAnmFrameRate(), 0);
					}

					if (actor->checkCurBckFromIndex(0x14)
					    && actor->curAnmEndsNext(0, nullptr))
						actor->setBckFromIndex(0x15);

					unk7C += unk6C->unk1E8->mSLToothUpSpeed.value;
					if (unk7C > unk6C->unk1E8->mSLToothLiveHeight.value
					    || mPosition.y > gpMarioPos->y + 2000.0f) {
						unk70 = 0;
						onHitFlag(HIT_FLAG_NO_COLLISION);
					}
				}
			}
		}
	}

	if (flags & 2) {
		Mtx local;
		if (unk70 > 1 || unk74 == 0 || unk74 == 2)
			PSMTXCopy(unk68->getConnectedMtx(), local);
		else
			PSMTXCopy(unk88, local);

		if (unk70 > 1)
			emitToothParticle(this, 0x19C);

		MActor* actor = unk68->getMActor();
		if (actor->checkCurBckFromIndex(0x16)
		    && actor->getFrameCtrl(0)->getRate() > 0.0f)
			emitToothParticle(this, 0x19A);

		if ((unk74 == 0 || unk74 == 2) && unk70 == 1)
			emitToothParticle(this, 0x19B);

		Mtx offset;
		PSMTXIdentity(offset);
		offset[2][3] = unk78;
		PSMTXConcat(local, offset, local);

		Mtx rot;
		MsMtxSetRotRPH(rot, unk80, unk80, 0.0f);
		PSMTXConcat(local, rot, local);

		local[1][3] += unk7C;
		mPosition.x = local[0][3];
		mPosition.y = local[1][3];
		mPosition.z = local[2][3];

		PSMTXCopy(local, actor->getModel()->getBaseTRMtx());
	}

	THitActor::perform(flags, graphics);
	unk68->getMActor()->perform(flags, graphics);
}

BOOL TBossEelTooth::receiveMessage(THitActor* sender, u32 message)
{
	if (message != HIT_MESSAGE_SPRAYED_BY_WATER)
		return FALSE;

	if (unk84 != 0)
		return TRUE;

	if (unk6C->unk1FD)
		return TRUE;

	if (unk70 <= 1)
		return TRUE;

	unk84 = 2;
	--unk70;
	unk68->getMActor()->setFrameRate(SMSGetAnmFrameRate(), 0);
	unk6C->unk1FC = TRUE;

	unkB8.a = (unk70 * 255) / unk6C->unk1E8->mSLToothMaxHitPoint.value;

	if (unk74 == 1 && unk70 % 20 == 1)
		unk6C->forceShedTears(unkBC);

	if (unk70 == 1) {
		unkB8.a = 0;
		PSMTXCopy(unk68->getConnectedMtx(), unk88);

		if (unk74 == 1) {
			playBossEelSound(0x8928, &mPosition);
			if (unkBC)
				playBossEelSound(0x892A, &unk6C->mPosition);
			else
				playBossEelSound(0x892B, &unk6C->mPosition);

			unk6C->unk1FD = TRUE;
			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emit(0xD3, &mPosition, 0, nullptr);
			setBossEelParticleScale(emitter, unk6C);
		} else {
			playBossEelSound(0x8929, &mPosition);
			if (unkBC)
				playBossEelSound(0x892C, &unk6C->mPosition);
			else
				playBossEelSound(0x892D, &unk6C->mPosition);
		}
	}

	return TRUE;
}

TBossEelTooth::TBossEelTooth(u8 tooth_id, TBossEel* boss,
                             const char* joint_name, SDLModelData* model_data,
                             const char* name)
    : THitActor(name)
    , unk68(nullptr)
    , unk6C(boss)
    , unk70(0)
    , unk74(tooth_id)
    , unk78(0.0f)
    , unk7C(0.0f)
    , unk80(0.0f)
    , unk84(0)
    , unkBC(TRUE)
{
	int jointIndex
	    = boss->mMActor->getModel()->getModelData()->getJointName()->getIndex(
	        joint_name);
	unk68 = new TSharedParts(boss, jointIndex, model_data, 0, "<TSharedParts>");

	MActor* actor = unk68->getMActor();
	actor->setLightType(1);
	actor->setBckFromIndex(0x16);
	actor->setFrameRate(0.0f, 0);

	for (u16 i = 0; i < actor->getModel()->getModelData()->getMaterialNum();
	     ++i) {
		SMS_InitPacket_OneTevKColorAndFog(actor->getModel(), i, GX_KCOLOR0,
		                                  &unkB8);
	}
	unkB8.a = 0xFF;

	TBossEelSaveParams* params = boss->unk1E8;
	unk70                       = params->mSLToothMaxHitPoint.value;
	initHitActor(0x08000022, 5, 0x81000000,
	             params->mSLToothAttackRadius.value,
	             params->mSLToothAttackHeight.value,
	             params->mSLToothDamageRadius.value,
	             params->mSLToothDamageHeight.value);

	TIdxGroupObj* group = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	list->insert(list->end(), this);
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

#define LOAD_BOSSEEL_PARTICLE(id, path)                                        \
	do {                                                                       \
		if (!gParticleFlagLoaded[id]) {                                        \
			gpResourceManager->load(path, id);                                 \
			gParticleFlagLoaded[id] = true;                                    \
		}                                                                      \
	} while (0)

void TBossEelManager::loadAfter()
{
	LOAD_BOSSEEL_PARTICLE(0xd3, "/scene/bosseel/jpa/ms_meo_awa_tooth.jpa");
	LOAD_BOSSEEL_PARTICLE(0xd4, "/scene/bosseel/jpa/ms_meo_awa_mouth.jpa");
	LOAD_BOSSEEL_PARTICLE(0x192, "/scene/bosseel/jpa/ms_meo_eyeblur.jpa");
	LOAD_BOSSEEL_PARTICLE(0x193, "/scene/bosseel/jpa/ms_meo_spin_smoke.jpa");
	LOAD_BOSSEEL_PARTICLE(0x194, "/scene/bosseel/jpa/ms_meo_spin_smoke_l.jpa");
	LOAD_BOSSEEL_PARTICLE(0x195, "/scene/bosseel/jpa/ms_meo_spin_awa.jpa");
	LOAD_BOSSEEL_PARTICLE(0x196, "/scene/bosseel/jpa/ms_meo_spin_awa_l.jpa");
	LOAD_BOSSEEL_PARTICLE(0x197, "/scene/bosseel/jpa/ms_meo_awa_body.jpa");
	LOAD_BOSSEEL_PARTICLE(0x198, "/scene/bosseel/jpa/ms_meo_awa_dead.jpa");
	LOAD_BOSSEEL_PARTICLE(0x199, "/scene/bosseel/jpa/ms_meo_suikomi.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19a, "/scene/bosseel/jpa/ms_meo_tooth_wash.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19b, "/scene/bosseel/jpa/ms_meo_tooth_kira.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19c, "/scene/bosseel/jpa/ms_meo_tooth_always.jpa");
}

TBEelTears::TBEelTears(const char* name)
    : TSpineEnemy(name)
    , unk15C(nullptr)
    , unk160(TRUE)
    , unk164(0)
    , unk168(nullptr)
    , unk16C(nullptr)
{
}

const char** TBEelTears::getBasNameTable() const
{
	return bossEelTears_bastable;
}

void TBEelTears::kill()
{
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk16C->onHitFlag(HIT_FLAG_NO_COLLISION);
	onLiveFlag(LIVE_FLAG_DEAD);
}

BOOL TBEelTears::receiveMessage(THitActor*, u32 message)
{
	if (message == 0xf) {
		unk164 = 60;

		if (mSpine->getCurrentNerve() == &TNerveBEelTearsMoveUp::theNerve()
		    || mSpine->getCurrentNerve() == &TNerveOilBallStay::theNerve())
			mSpine->pushNerve(&TNerveBEelTearsWaterHit::theNerve());

		if (mSpine->getCurrentNerve() == &TNerveBEelTearsWaterHit::theNerve()) {
			MActor* actor = mMActor;
			actor->setFrameRate(SMSGetAnmFrameRate(), 0);
		}

		return TRUE;
	}

	return FALSE;
}

void TBEelTears::reset()
{
	mActorType = 0x8000003;
	TSpineEnemy::reset();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());
	onLiveFlag(LIVE_FLAG_UNK10);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	offLiveFlag(LIVE_FLAG_UNK200);
	offLiveFlag(LIVE_FLAG_HIDDEN);
	mGroundPlane = TMap::getIllegalCheckData();
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

TBEelTearsManager::TBEelTearsManager(const char* name)
    : TEnemyManager(name)
{
}

void TBEelTearsManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBEelTearsSaveLoadParams("/enemy/bossEelTears.prm");
	TEnemyManager::load(stream);
}

void TBEelTearsManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		clipEnemies(graphics);

	for (int i = 0; i < getActiveObjNum(); ++i)
		getObj(i)->perform(flags, graphics);

	for (int i = 0; i < 30; ++i) {
		if (mDrops[i]->unk6C)
			mDrops[i]->perform(flags, graphics);
	}
}

TSpineEnemy* TBEelTearsManager::createEnemyInstance()
{
	return new TBEelTears("めおとウナギ涙");
}

void TBEelTearsManager::splitTears(JGeometry::TVec3<f32>& position)
{
	position.y += 600.0f;

	int splitNum = ((TBEelTearsSaveLoadParams*)unk38)->mSLTearsSplitNum.get();
	f32 minPos   = -250.0f;
	f32 maxPos   = 250.0f;

	for (int i = 0; i < 30; ++i) {
		TBEelTearsDrop* drop = mDrops[i];
		if (drop->unk6C)
			continue;

		JGeometry::TVec3<f32> dropPos = position;
		f32 offset = rand() * 0.000030517578f;
		offset *= maxPos - minPos;
		offset += minPos;
		dropPos.x += offset;
		offset = rand() * 0.000030517578f;
		offset *= maxPos - minPos;
		offset += minPos;
		dropPos.y += offset;
		offset = rand() * 0.000030517578f;
		offset *= maxPos - minPos;
		offset += minPos;
		dropPos.z += offset;

		--splitNum;
		drop->offHitFlag(HIT_FLAG_NO_COLLISION);
		drop->unk6C = TRUE;

		f32 minSpeed = 4.0f;
		f32 maxSpeed = 6.0f;
		f32 speed = rand() * 0.000030517578f;
		speed *= maxSpeed - minSpeed;
		speed += minSpeed;
		drop->unk70 = speed;
		drop->mPosition = dropPos;

		f32 minScale = 1.0f;
		f32 maxScale = 1.5f;
		f32 scale    = rand() * 0.000030517578f;
		scale *= maxScale - minScale;
		scale += minScale;

		TBEelTearsSaveLoadParams* params = drop->unk74->unk15C;
		scale = rand() * 0.000030517578f;
		scale *= params->mTearsDropScaleHigh - params->mTearsDropScaleLow;
		scale += params->mTearsDropScaleLow;
		drop->mScaling.x = scale;
		drop->mScaling.y = scale;
		drop->mScaling.z = scale;

		if (splitNum < 0)
			break;
	}
}

void TBEelTearsManager::createEnemies(int count)
{
	TEnemyManager::createEnemies(count);

	void* resource
	    = JKRFileLoader::getGlbResource("/scene/bossEelTears/tears_drop.bmd");
	SDLModelData* modelData
	    = new SDLModelData(J3DModelLoaderDataBase::load(resource, 0x11240000));
	TBEelTears* tears = (TBEelTears*)unk18[0];

	for (int i = 0; i < 30; ++i)
		mDrops[i] = new TBEelTearsDrop(tears, 0, modelData, "涙粒");
}

void TBEelTearsManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "tears.bmd", 0x11240000, 0 },
		{ "tears_waterhit.bmd", 0x11240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBEelTearsManager::loadAfter()
{
	LOAD_BOSSEEL_PARTICLE(0xd5,
	                      "/scene/bossEelTears/ms_meo_tear_bomb.jpa");
	LOAD_BOSSEEL_PARTICLE(0xd6,
	                      "/scene/bossEelTears/ms_meo_tear_awaget.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19d,
	                      "/scene/bossEelTears/ms_meo_tear_awa.jpa");
}

#undef LOAD_BOSSEEL_PARTICLE

TBEelTearsSaveLoadParams::TBEelTearsSaveLoadParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLTearsUpSpeed, 5.0f)
    , PARAM_INIT(mSLTearsDamageUpSpeed, 1.0f)
    , PARAM_INIT(mSLTearsLiveHeight, 2000.0f)
    , PARAM_INIT(mSLTearsSplitNum, 2)
    , PARAM_INIT(mSLTearsDamageRadius, 600)
    , PARAM_INIT(mSLTearsDamageHeight, 400)
    , PARAM_INIT(mSLTearsAttackRadius, 500)
    , PARAM_INIT(mSLTearsAttackHeight, 300)
    , PARAM_INIT(mSLTearsDropDamageRadius, 600)
    , PARAM_INIT(mSLTearsDropDamageHeight, 400)
    , PARAM_INIT(mSLTearsDropAttackRadius, 500)
    , PARAM_INIT(mSLTearsDropAttackHeight, 300)
    , PARAM_INIT(mSLHighPolyDistY, 5.0f)
    , PARAM_INIT(mSLHitAnmFrameRate, 1.0f)
    , PARAM_INIT(mSLBodyScaleLow, 1.0f)
    , PARAM_INIT(mSLBodyScaleHigh, 1.0f)
    , PARAM_INIT(mSLTearsDropScaleLow, 1.0f)
    , PARAM_INIT(mSLTearsDropScaleHigh, 1.0f)
    , mBodyScaleLow(0.0f)
    , mBodyScaleHigh(1.0f)
    , mTearsDropScaleLow(0.0f)
    , mTearsDropScaleHigh(1.0f)
{
	TParams::load(mPrmPath);
	mBodyScaleLow       = mSLBodyScaleLow.get();
	mBodyScaleHigh      = mSLBodyScaleHigh.get();
	mTearsDropScaleLow  = mSLTearsDropScaleLow.get();
	mTearsDropScaleHigh = mSLTearsDropScaleHigh.get();
}

void TOilBall::load(JSUMemoryInputStream& stream)
{
	unk160 = FALSE;
	TSpineEnemy::load(stream);
	unk150 = mPosition;
	reset();
}

void TOilBall::calcRootMatrix() { TSpineEnemy::calcRootMatrix(); }

void TOilBall::reset()
{
	mActorType = 0x08000003;
	TSpineEnemy::reset();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());
	onLiveFlag(LIVE_FLAG_UNK10);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	offLiveFlag(LIVE_FLAG_UNK200);
	offLiveFlag(LIVE_FLAG_HIDDEN);
	mGroundPlane = TMap::getIllegalCheckData();
	offHitFlag(HIT_FLAG_NO_COLLISION);
	mPosition = unk150;
	mSpine->initWith(&TNerveOilBallStay::theNerve());
	mMActor = mMActorKeeper->getMActor("tears.bmd");
	mMActor->setBckFromIndex(2);
	unk160 = FALSE;
}

void TOilBall::moveObject()
{
	TBEelTearsSaveLoadParams* params = unk15C;
	f32 scale                       = mScaling.x;
	mAttackRadius  = params->mSLTearsAttackRadius.get() * scale;
	mAttackHeight  = params->mSLTearsAttackHeight.get() * scale;
	mDamageRadius  = params->mSLTearsDamageRadius.get() * scale;
	mDamageHeight  = params->mSLTearsDamageHeight.get() * scale;
	calcEntryRadius();

	for (int i = 0; i < mColCount; ++i) {
		THitActor* hitActor = mCollisions[i];

		if (hitActor->isActorTypeOf(ACTOR_TYPE_PLAYER)) {
			if (mSpine->getCurrentNerve() == &TNerveBEelTearsMoveUp::theNerve()
			    || mSpine->getCurrentNerve() == &TNerveOilBallStay::theNerve()) {
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
				mSpine->pushNerve(&TNerveBEelTearsSplit::theNerve());
			}
		} else {
			JGeometry::TVec3<f32> velocity;
			velocity.zero();

			JGeometry::TVec3<f32> direction;
			direction.x = mPosition.x - hitActor->mPosition.x;
			direction.y = mPosition.y - hitActor->mPosition.y;
			direction.z = mPosition.z - hitActor->mPosition.z;

			if (direction.x == 0.0f && direction.y == 0.0f
			    && direction.z == 0.0f)
				direction.x += 1.0f;

			MsVECNormalize(&direction, &direction);
			direction.x *= 5.0f;
			direction.y *= 5.0f;
			direction.z *= 5.0f;
			velocity.add(direction);
			mLinearVelocity = velocity;
		}
	}

	TLiveActor::moveObject();
}

void TBEelTears::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("tears.bmd", 0);
	mMActorKeeper->createMActor("tears_waterhit.bmd", 0);
}

void TBEelTears::init(TLiveManager* manager)
{
	TSpineEnemy::init(manager);
	mActorType = 0x08000003;
	unk64 |= 0x80000000;
	setMActorAndKeeper();
	unk15C = (TBEelTearsSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());

	TScreenTexture* texture
	    = JDrama::TNameRefGen::search<TScreenTexture>("スクリーンテクスチャ");
	const ResTIMG* texInfo = texture->getTexture()->getTexInfo();
	J3DSkinDeform* skinDeform = new J3DSkinDeform;

	MActor* actor = mMActorKeeper->getMActor("tears.bmd");
	actor->getModel()->setSkinDeform(skinDeform,
	                                 J3D_DEFORM_ATTACH_FLAG_UNK_1);
	actor->resetDL();
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texInfo);
	actor->setLightType(3);

	actor = mMActorKeeper->getMActor("tears_waterhit.bmd");
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texInfo);
	actor->setLightType(3);

	onLiveFlag(LIVE_FLAG_DEAD);

	f32 scaleMin = unk15C->mBodyScaleLow;
	f32 scaleMax = unk15C->mBodyScaleHigh;
	mBodyScale   = scaleMin
	             + (scaleMax - scaleMin) * (rand() * 0.000030517578f);

	TIdxGroupObj* group = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	MtxPtr mtx          = mMActor->getModel()->mNodeMatrices[0];
	unk16C = new TBossEelTearsRecoverCollision(mtx, cTearsRecoverCollisionName);
	unk16C->initCollision();

	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	list->insert(list->end(), unk16C);
	unk16C->onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBEelTears::perform(u32 flags, JDrama::TGraphics* graphics)
{
	unk16C->perform(flags, graphics);

	if (mLiveFlag & LIVE_FLAG_DEAD)
		return;

	if (flags & 1)
		moveObject();

	u32 calcFlags = flags & 2;
	if (calcFlags) {
		Mtx mtx;
		SMS_GetLightPerspectiveForEffectMtx(mtx);
		setEffectMtxOnTex1(
		    mMActor->getModel()->getModelData()->getMaterialNodePointer(0),
		    mtx);
		updateAnmSound();
	}

	if (calcFlags)
		mMActor->frameUpdate();

	if (mLiveFlag & (LIVE_FLAG_HIDDEN | LIVE_FLAG_CLIPPED_OUT))
		return;

	if (calcFlags) {
		calcRootMatrix();
		mMActor->calc();
	}

	if (flags & 4)
		mMActor->viewCalc();

	if (flags & 0x200)
		mMActor->entry();

	THitActor::perform(flags, graphics);
}

void TBEelTears::calcRootMatrix()
{
	if (unk168) {
		if (mPosition.y < unk168[1][3])
			mPosition.y = unk168[1][3];

		TSpineEnemy::calcRootMatrix();

		if (mSpine->getCurrentNerve() == &TNerveBEelTearsGenerate::theNerve()) {
			f32 z = mPosition.z;
			f32 y = mPosition.y;
			f32 x = mPosition.x;
			TPosition3f mtx;

			mtx.translation(x, y, z);

			mtx.ref(0, 3) += 0.1f * (unk168[0][3] - mtx.ref(0, 3));
			mtx.ref(2, 3) += 0.1f * (unk168[2][3] - mtx.ref(2, 3));

			f32 scale = unk15C->mSLBodyScaleLow.get();
			mScaling.x = scale;
			mScaling.y = scale;
			mScaling.z = scale;
			mMActor->getModel()->setBaseScale(mScaling);

			PSMTXCopy(mtx.mMtx, mMActor->getModel()->getBaseTRMtx());
		}
	} else {
		TSpineEnemy::calcRootMatrix();
	}
}

void TBEelTears::moveObject()
{
	mVelocity.x *= 0.9f;
	mVelocity.z *= 0.9f;
	f32 liveHeight = unk15C->mSLTearsLiveHeight.get();

	mPosition.x += mVelocity.x;
	mPosition.z += mVelocity.z;

	if (((u8*)unk16C)[0x81] == FALSE && unk168) {
		if (mPosition.y > gpMarioPos->y + 3000.0f
		    || mPosition.y > unk168[1][3] + liveHeight) {
			kill();
			return;
		}
	}

	f32 scale      = mScaling.x;
	mAttackRadius  = unk15C->mSLTearsAttackRadius.get() * scale;
	mAttackHeight  = unk15C->mSLTearsAttackHeight.get() * scale;
	mDamageRadius  = unk15C->mSLTearsDamageRadius.get() * scale;
	mDamageHeight  = unk15C->mSLTearsDamageHeight.get() * scale;
	calcEntryRadius();

	for (int i = 0; i < mColCount; ++i) {
		THitActor* hitActor = mCollisions[i];

		if (hitActor->isActorTypeOf(ACTOR_TYPE_PLAYER)) {
			if (mSpine->getCurrentNerve() == &TNerveBEelTearsMoveUp::theNerve()) {
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
				mSpine->pushNerve(&TNerveBEelTearsSplit::theNerve());
			}
		} else {
			JGeometry::TVec3<f32> velocity;
			velocity.zero();

			JGeometry::TVec3<f32> direction;
			direction.x = mPosition.x - hitActor->mPosition.x;
			direction.y = mPosition.y - hitActor->mPosition.y;
			direction.z = mPosition.z - hitActor->mPosition.z;

			if (direction.x == 0.0f && direction.y == 0.0f
			    && direction.z == 0.0f)
				direction.x += 1.0f;

			MsVECNormalize(&direction, &direction);
			direction.x *= 5.0f;
			direction.y *= 5.0f;
			direction.z *= 5.0f;
			velocity.add(direction);
			mLinearVelocity = velocity;
		}
	}

	TLiveActor::moveObject();
}

void TBEelTearsDrop::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (flags & 1) {
		mPosition.y += unk70;
		if (mPosition.y > gpMarioPos->y + 2000.0f)
			unk6C = FALSE;
	}

	if (flags & 2) {
		Mtx mtx;
		MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y, mPosition.z,
		               mRotation.x, mRotation.y, mRotation.z);

		MActor* actor = unk68->getMActor();
		PSMTXCopy(mtx, actor->getModel()->getBaseTRMtx());

		f32 scale = unk74->unk15C->mSLTearsDropScaleLow.get();
		mScaling.x = scale;
		mScaling.y = scale;
		mScaling.z = scale;
		actor->getModel()->setBaseScale(mScaling);
	}

	unk68->getMActor()->perform(flags, graphics);
}

TBEelTearsDrop::TBEelTearsDrop(TBEelTears* tears, int index,
                               SDLModelData* model_data, const char* name)
    : THitActor(name)
    , unk68(nullptr)
    , unk74(tears)
{
	unk68 = new TSharedParts(tears, index, model_data, 0, "<TSharedParts>");

	TBEelTearsSaveLoadParams* params = tears->unk15C;
	initHitActor(0x2000002c, 3, 0x80000000,
	             params->mSLTearsDropAttackRadius.get(),
	             params->mSLTearsDropAttackHeight.get(),
	             params->mSLTearsDropDamageRadius.get(),
	             params->mSLTearsDropDamageHeight.get());

	TIdxGroupObj* group = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	list->insert(list->end(), this);
	unk6C = FALSE;

	TScreenTexture* texture
	    = JDrama::TNameRefGen::search<TScreenTexture>("スクリーンテクスチャ");
	new J3DSkinDeform;

	MActor* actor = unk68->getMActor();
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texture->getTexture()->getTexInfo());
	actor->setBckFromIndex(0);
	actor->setLightType(3);
}

DEFINE_NERVE(TNerveBossEelWaitAppear, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0)
		START_BOSS_EEL_BCK(eel, 10);

	if (spine->getTime() == 2500)
		gpMarDirector->mConsole->startAppearBalloon(0xE0012, true);

	Vec pos = *gpMarioPos;
	pos.y += 75.0f;
	if (eel->unk1AC->isInCube(pos, 0)) {
		spine->pushAfterCurrent(&TNerveBossEelFirstSpin::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelFirstSpin, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0) {
		eel->mTurnSpeed = 0.0f;
		if (gpMSound->gateCheck(0x8921)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x8921, &eel->mPosition, nullptr, 2.0f, 0, 0, nullptr, 0,
			    4);
		}

		START_BOSS_EEL_BCK(eel, 10);

		if (rand() * 0.000030517578f < 0.5f)
			eel->onLiveFlag(LIVE_FLAG_UNK10000);
		else
			eel->offLiveFlag(LIVE_FLAG_UNK10000);
	}

	f32 turnSpeed = eel->mTurnSpeed;
	CLBChaseGeneralConstantSpecifySpeed(
	    &turnSpeed, eel->unk1E8->mSLSpinMaxSpeed.value,
	    eel->unk1E8->mSLSpinAccel.value);
	eel->mTurnSpeed = turnSpeed;
	gpCameraShake->keepShake((EnumCamShakeMode)0x18, 1.0f);

	if (eel->mLiveFlag & LIVE_FLAG_UNK10000) {
		eel->mRotation.y -= turnSpeed;
		if (eel->mRotation.y <= 0.0f) {
			if (gpMSound->gateCheck(0x8921)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x8921, &eel->mPosition, nullptr, turnSpeed, 0, 0,
				    nullptr, 0, 4);
			}
		}
	} else {
		eel->mRotation.y += turnSpeed;
		if (eel->mRotation.y >= 360.0f) {
			if (gpMSound->gateCheck(0x8921)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x8921, &eel->mPosition, nullptr, turnSpeed, 0, 0,
				    nullptr, 0, 4);
			}
		}
	}

	eel->mRotation.y = callMsWrap(eel->mRotation.y, 0.0f, 360.0f);

	if (spine->getTime() >= 360) {
		spine->pushAfterCurrent(&TNerveBossEelAppear::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelSecondSpin, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0) {
		START_BOSS_EEL_BCK(eel, 10);

		TBossEelUnk1EC* spinState = eel->unk1EC;
		spinState->unk0           = 0;
		spinState->unk4
		    = (s32)(rand() * 0.000030517578f * 960.0f) + 241;
		eel->mTurnSpeed = 0.0f;

		if (gpMSound->gateCheck(0x8921)) {
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x8921, &eel->mPosition, nullptr, 2.0f, 0, 0, nullptr, 0,
			    4);
		}

		if (rand() * 0.000030517578f < 0.5f)
			eel->onLiveFlag(LIVE_FLAG_UNK10000);
		else
			eel->offLiveFlag(LIVE_FLAG_UNK10000);
	}

	f32 turnSpeed = eel->mTurnSpeed;
	CLBChaseGeneralConstantSpecifySpeed(
	    &turnSpeed, eel->unk1E8->mSLSpinMaxSpeed.value,
	    eel->unk1E8->mSLSpinAccel.value);
	eel->mTurnSpeed = turnSpeed;
	gpCameraShake->keepShake((EnumCamShakeMode)0x18, 1.0f);

	if (eel->mLiveFlag & LIVE_FLAG_UNK10000) {
		eel->mRotation.y -= turnSpeed;
		if (eel->mRotation.y <= 0.0f) {
			if (gpMSound->gateCheck(0x8921)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x8921, &eel->mPosition, nullptr, turnSpeed, 0, 0,
				    nullptr, 0, 4);
			}
		}
	} else {
		eel->mRotation.y += turnSpeed;
		if (eel->mRotation.y >= 360.0f) {
			if (gpMSound->gateCheck(0x8921)) {
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x8921, &eel->mPosition, nullptr, turnSpeed, 0, 0,
				    nullptr, 0, 4);
			}
		}
	}

	eel->mRotation.y = callMsWrap(eel->mRotation.y, 0.0f, 360.0f);

	TBossEelUnk1EC* spinState = eel->unk1EC;
	++spinState->unk0;
	bool finished = false;
	if (spinState->unk0 >= spinState->unk4) {
		spinState->unk0 = spinState->unk4;
		finished        = true;
	}

	if (finished) {
		spine->pushAfterCurrent(&TNerveBossEelAppear::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelOutWait, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	eel->unk200++;

	if (spine->getTime() == 0) {
		eel->unk1FC = FALSE;
		START_BOSS_EEL_BCK(eel, 16);
	}

	if (eel->checkCurAnmEnd(0)) {
		s32 mouthOpenInterval
		    = eel->unk1E8->mSLMouthOpenInterval.value;

		if (eel->mMActor->checkCurBckFromIndex(19)) {
			spine->setNext(&TNerveBossEelQuickBack::theNerve());
		} else if (eel->unk200 >= 3600) {
			spine->pushAfterCurrent(&TNerveBossEelSlowBack::theNerve());
			eel->unk200 = 0;
			return TRUE;
		} else if (spine->getTime() > mouthOpenInterval) {
			spine->pushAfterCurrent(&TNerveBossEelOutWait::theNerve());
			spine->pushAfterCurrent(&TNerveBossEelMouthOpenWait::theNerve());
			return TRUE;
		} else if (eel->unk1FD) {
			START_BOSS_EEL_BCK(eel, 19);
		} else if (eel->unk1FC) {
			eel->unk1FC = FALSE;
			START_BOSS_EEL_BCK(eel, 18);
		} else {
			START_BOSS_EEL_BCK(eel, 16);
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelAppear, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0) {
		playBossEelSound(0x8922, &eel->mPosition);

		*(u32*)((u8*)eel->unk210 + 0x64) &= ~1U;
		START_BOSS_EEL_BCK(eel, 15);
		eel->unk1F0 = TRUE;
		gpCameraShake->startShake((EnumCamShakeMode)0x19, 1.0f);

		f32 frameCount = eel->mMActor->getFrameCtrl(0)->getEnd() * 2;
		eel->unk1F8
		    = eel->unk1E8->mSLAppearMoveDistY.value / frameCount / frameCount;
		testHeight = 0.0f;
	}

	s32 frameCount = eel->mMActor->getFrameCtrl(0)->getEnd() * 2;
	if (spine->getTime() < frameCount) {
		eel->unk1F4 += eel->unk1E8->mSLAppearMoveDistY.value / frameCount;
		eel->mTurnSpeed *= 0.98f;

		if (eel->mLiveFlag & LIVE_FLAG_UNK10000)
			eel->mRotation.y -= eel->mTurnSpeed;
		else
			eel->mRotation.y += eel->mTurnSpeed;

		eel->mRotation.y = MsWrap(eel->mRotation.y, 0.0f, 360.0f);
	} else if (eel->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBossEelOutWait::theNerve());
		*(u32*)((u8*)eel->unk210 + 0x64) |= 1;

		if (eel->unk21D) {
			eel->unk21D = FALSE;
			gpMarDirector->mConsole->startAppearBalloon(0xE0013, true);
		}

		return TRUE;
	}

	return FALSE;
}

static BOOL ExecBackNerve_Sub(TSpineBase<TLiveActor>* spine, f32)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 1) {
		playBossEelSound(0x8923, &eel->mPosition);

		if (eel->unk1FD) {
			eel->unk1FD = FALSE;
			START_BOSS_EEL_BCK(eel, 6);
			gpCameraShake->startShake((EnumCamShakeMode)0x1a, 1.0f);
		} else {
			START_BOSS_EEL_BCK(eel, 9);
		}
	}

	J3DFrameCtrl* frameCtrl = eel->mMActor->getFrameCtrl(0);
	f32 frameEnd           = frameCtrl->getEnd() * 2;
	eel->unk1F4 -= eel->unk1E8->mSLAppearMoveDistY.value / frameEnd;

	if (eel->unk1F4 < 0.0f) {
		eel->unk1F4 = 0.0f;
		if (eel->checkCurAnmEnd(0)) {
			eel->unk1F0 = FALSE;
			spine->pushAfterCurrent(&TNerveBossEelSecondSpin::theNerve());
			if (eel->mMActor->checkCurBckFromIndex(6))
				spine->pushAfterCurrent(
				    &TNerveBossEelSleepOnBottom::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelSlowBack, TLiveActor)
{
	return ExecBackNerve_Sub(spine, 10.0f);
}

DEFINE_NERVE(TNerveBossEelQuickBack, TLiveActor)
{
	if (ExecBackNerve_Sub(spine, 40.0f)) {
		TBossEel* eel = (TBossEel*)spine->getBody();
		TBossEelTooth** teeth = (TBossEelTooth**)((u8*)eel + 0x16C);

		for (int i = 0; i < 8; ++i) {
			TBossEelTooth* tooth = teeth[i];
			if (tooth && tooth->unk70 > 1) {
				tooth->unk70 = tooth->unk6C->unk1E8->mSLToothMaxHitPoint.value;
				tooth->unkB8.a = 0xff;
			}
		}

		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelEat, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0)
		START_BOSS_EEL_BCK(eel, 17);

	if (eel->checkCurAnmEnd(0)) {
		if (eel->mMActor->checkCurBckFromIndex(17)) {
			u8 canEat = *(u8*)((u8*)eel + 0x1C8);
			if (!canEat) {
				JGeometry::TVec3<f32> pos = *gpMarioPos;
				MtxPtr mtx = eel->mMActor->getModel()
				                 ->mNodeMatrices[*(u16*)((u8*)eel + 0x1A0)];
				pos.x -= mtx[0][3];
				pos.y -= mtx[1][3];
				pos.z -= mtx[2][3];

				if (MsVECMag2(&pos) < eel->unk1D4 * eel->unk1D8)
					canEat = TRUE;
			}

			if (canEat) {
				START_BOSS_EEL_BCK(eel, 12);
				if (SMS_SendMessageToMario(eel, HIT_MESSAGE_TAKE))
					eel->mHeldObject = (TTakeActor*)SMS_GetMarioHitActor();

				if (!eel->unk21C) {
					gpMarDirector->mConsole->startAppearBalloon(0xE0015, true);
					gpMarDirector->fireStartDemoCamera(
					    "meoto_mogu_camera", &eel->mPosition, -1, 0.0f,
					    false, hoseiDiveCameraCallback, (u32)eel, nullptr,
					    JDrama::TFlagT<u16>(0));
					eel->unk21C = TRUE;
				}
			} else {
				START_BOSS_EEL_BCK(eel, 5);

				MtxPtr mtx = eel->mMActor->getModel()->mNodeMatrices[5];
				eel->unk204.x = mtx[0][3];
				eel->unk204.y = mtx[1][3];
				eel->unk204.z = mtx[2][3];

				JPABaseEmitter* emitter
				    = gpMarioParticleManager->emitAndBindToPosPtr(
				        0xD4, &eel->unk204, 0, nullptr);
				if (emitter)
					emitter->setScale(eel->mScaling);
			}
		} else if (eel->mMActor->checkCurBckFromIndex(12)) {
			if (SMS_SendMessageToMario(eel, HIT_MESSAGE_UNK8)) {
				eel->mHeldObject = nullptr;
				SMS_SendMessageToMario(eel, 0xE);
				gpMarDirector->fireEndDemoCamera();
				eel->unk21C = FALSE;
			}

			spine->reset();
			spine->setDefaultNext();
			spine->pushAfterCurrent(&TNerveBossEelQuickBack::theNerve());
			return TRUE;
		}
	}

	if (eel->mMActor->checkCurBckFromIndex(12)) {
		if (eel->mMActor->getFrameCtrl(0)->getFrame() < 75.0f)
			SMSRumbleMgr->start(8, &eel->mPosition);
		else
			SMSRumbleMgr->start(0x14, 10, (f32*)nullptr);

		gpMarioOriginal->mGamePad->onNeutralMarioKey();
		if (SMS_SendMessageToMario(eel, HIT_MESSAGE_TAKE)) {
			eel->mHeldObject = (TTakeActor*)SMS_GetMarioHitActor();
			if (!eel->unk21C) {
				gpMarDirector->mConsole->startAppearBalloon(0xE0015, true);
				gpMarDirector->fireStartDemoCamera(
				    "meoto_mogu_camera", &eel->mPosition, -1, 0.0f, false,
				    hoseiDiveCameraCallback, (u32)eel, nullptr,
				    JDrama::TFlagT<u16>(0));
				eel->unk21C = TRUE;
			}
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelDie, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0) {
		if (gpMSound->gateCheck(0x892F)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x892F, &eel->mPosition, 0, nullptr, 0, 4);
		}

		gpMarDirector->mConsole->startAppearBalloon(0xE0014, true);
		MSBgm::stopTrackBGMs(7, 10);
		gpCameraShake->startShake((EnumCamShakeMode)0x1e, 1.0f);
		START_BOSS_EEL_BCK(eel, 3);

		((THitActor*)eel->unk1B0)->onHitFlag(HIT_FLAG_NO_COLLISION);
		((THitActor*)eel->unk210)->onHitFlag(HIT_FLAG_NO_COLLISION);
		((THitActor*)*(void**)((u8*)eel + 0x18C))
		    ->onHitFlag(HIT_FLAG_NO_COLLISION);
		((THitActor*)eel->unk1A8)->onHitFlag(HIT_FLAG_NO_COLLISION);
		((THitActor*)eel->unk214)->offHitFlag(HIT_FLAG_NO_COLLISION);
		((TBossEelHeartCoin*)eel->unk218)->generate(eel->mPosition);

		if (SMS_SendMessageToMario(eel, HIT_MESSAGE_TAKE)) {
			eel->mHeldObject = (TTakeActor*)SMS_GetMarioHitActor();
			gpMarDirector->fireStartDemoCamera(
			    cBossEelEndCameraName, &eel->mPosition, -1, 0.0f, false,
			    nullptr, 0, nullptr, JDrama::TFlagT<u16>(0));
		}
	}

	if (eel->mMActor->checkCurBckFromIndex(3)) {
		if (eel->mMActor->getFrameCtrl(0)->checkPass(650.0f))
			((TBossEelHeartCoin*)eel->unk218)->getMActor()->setBckFromIndex(8);

		if (eel->checkCurAnmEnd(0)) {
			if (SMS_SendMessageToMario(eel, HIT_MESSAGE_UNK8)) {
				eel->mHeldObject = nullptr;
				SMS_SendMessageToMario(eel, HIT_MESSAGE_ATTACK);
				gpMarDirector->fireEndDemoCamera();
			}

			JGeometry::TVec3<f32> pos = *gpMarioPos;
			TBEelTears* tears = (TBEelTears*)gpConductor->makeOneEnemyAppear(
			    pos, cBossEelTearsManagerName, 0);
			if (tears) {
				tears->unk16C->unk81 = FALSE;
				tears->unk16C->offHitFlag(HIT_FLAG_NO_COLLISION);
				tears->unk16C->unk80      = TRUE;
				tears->unk16C->mPosition = tears->mPosition;
				tears->unk16C->mPosition = tears->mPosition;
				tears->mSpine->initWith(
				    &TNerveBEelTearsMarioRecover::theNerve());
				tears->onLiveFlag(LIVE_FLAG_HIDDEN);
				tears->unk16C->unk81 = TRUE;
			}

			s32 jointIndex = eel->mMActor->getModel()
			                     ->getModelData()
			                     ->getJointName()
			                     ->getIndex(cBossEelEnemyGroupJointName);
			MtxPtr mtx = eel->mMActor->getModel()->mNodeMatrices[jointIndex];
			gpItemManager->makeShineAppearWithDemo(
			    cBossEelShineName, cBossEelShineCameraName, mtx[0][3],
			    mtx[1][3], mtx[2][3]);

			void* part = *(void**)((u8*)eel + 0x184);
			*(s32*)((u8*)part + 0x70) = 0;
			((THitActor*)part)->onHitFlag(HIT_FLAG_NO_COLLISION);
			START_BOSS_EEL_BCK(eel, 4);
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelMouthOpenWait, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0) {
		if (eel->unk1FD)
			eel->unk1FD = FALSE;

		START_BOSS_EEL_BCK(eel, 13);
		gpCameraShake->startShake((EnumCamShakeMode)0x1b, 1.0f);
	} else if (eel->checkCurAnmEnd(0)) {
		if (eel->mMActor->checkCurBckFromIndex(13)) {
			START_BOSS_EEL_BCK(eel, 14);
			gpCameraShake->startShake((EnumCamShakeMode)0x1c, 1.0f);

			TBossEelVortex* vortex
			    = *(TBossEelVortex**)((u8*)eel + 0x18C);
			vortex->reset();

			MtxPtr mtx
			    = eel->mMActor->getModel()->mNodeMatrices[eel->unk1A0[2]];
			vortex->mPosition.x = mtx[0][3];
			vortex->mPosition.y = mtx[1][3];
			vortex->mPosition.z = mtx[2][3];
			vortex->unk6C       = FALSE;
			vortex->mScaling.x  = eel->unk1E8->mSLVortexScaleXZ.value;
			vortex->mScaling.y  = eel->unk1E8->mSLVortexScaleY.value;
			vortex->mScaling.z  = eel->unk1E8->mSLVortexScaleXZ.value;
			eel->offHitFlag(1);
		} else {
			s32 mouthOpenFrame = eel->unk1E8->mSLMouthOpenFrame.value;
			s32 canEatFrame   = eel->unk1E8->mSLCanEatFrame.value;

			if (spine->getTime() > mouthOpenFrame - canEatFrame) {
				u8 canEat = *(u8*)((u8*)eel + 0x1C8);
				if (!canEat) {
					JGeometry::TVec3<f32> pos = *gpMarioPos;
					MtxPtr mtx = eel->mMActor->getModel()
					                 ->mNodeMatrices[*(u16*)((u8*)eel + 0x1A0)];
					pos.x -= mtx[0][3];
					pos.y -= mtx[1][3];
					pos.z -= mtx[2][3];

					if (MsVECMag2(&pos) < eel->unk1D4 * eel->unk1D8)
						canEat = TRUE;
				}

				if (canEat) {
					spine->pushAfterCurrent(&TNerveBossEelEat::theNerve());
					return TRUE;
				}
			}

			if (spine->getTime() > mouthOpenFrame) {
				if (eel->mMActor->checkCurBckFromIndex(2))
					return TRUE;

				gpCameraShake->startShake((EnumCamShakeMode)0x1d, 1.0f);
				START_BOSS_EEL_BCK(eel, 2);
			}
		}
	}

	if (eel->mMActor->checkCurBckFromIndex(14))
		eel->mRotation.y += TBossEel::mOpenRollSpeed;

	return FALSE;
}

DEFINE_NERVE(TNerveBossEelSleepOnBottom, TLiveActor)
{
	TBossEel* eel = (TBossEel*)spine->getBody();

	if (spine->getTime() == 0) {
		eel->unk200 = 0;
		START_BOSS_EEL_BCK(eel, 10);
	}

	if (spine->getTime() % 100 == 1)
		eel->forceShedTears(spine->getTime() % 2 != 0);

	if (spine->getTime() > 1000 && eel->checkCurAnmEnd(0))
		return TRUE;

	return FALSE;
}
