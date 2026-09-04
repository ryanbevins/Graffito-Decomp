#include <Camera/CameraShake.hpp>
#include <Enemy/TinKoopa.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/CoasterKiller.hpp>
#include <Enemy/EffectObj.hpp>
#include <Enemy/Graph.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <Strategic/ObjManager.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>

static const char* tinkoopa_bastable[] = {
	"/scene/tinkoopa/bas/tinkoopa_break1.bas",
	"/scene/tinkoopa/bas/tinkoopa_break2.bas",
	"/scene/tinkoopa/bas/tinkoopa_break3.bas",
	"/scene/tinkoopa/bas/tinkoopa_break4.bas",
	nullptr,
	"/scene/tinkoopa/bas/tinkoopa_damage1.bas",
	"/scene/tinkoopa/bas/tinkoopa_damage2.bas",
	"/scene/tinkoopa/bas/tinkoopa_damage3.bas",
	"/scene/tinkoopa/bas/tinkoopa_damage4.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/tinkoopa/bas/tinkoopa_wait1.bas",
	"/scene/tinkoopa/bas/tinkoopa_wait2.bas",
	"/scene/tinkoopa/bas/tinkoopa_wait3.bas",
	"/scene/tinkoopa/bas/tinkoopa_wait4.bas",
	"/scene/tinkoopa/bas/tinkoopa_wait5.bas",
};

static const char* TTinKoopa_jointNameTable[] = {
	"jnt_head",      "jnt_breast",   "jnt_stomach",  "jnt_rarm",
	"jnt_larm",      "jnt_leg",      "jnt_leye",     "jnt_reye",
	"fire_null",     "fire_col_null", "jnt_femur",    "killer_null1",
	"killer_null2",  "killer_null3",  "killer_null4",
};

static u32 TTinKoopa_jointIndexTable[15];

static const char* partsCollisionFileTable[] = {
	"/scene/tinkoopa/head_col.col",
	"/scene/tinkoopa/breast_col.col",
	"/scene/tinkoopa/stomach_col.col",
	"/scene/tinkoopa/rarm_col.col",
	"/scene/tinkoopa/larm_col.col",
	"/scene/tinkoopa/leg_col.col",
};

static const char* partsBreakModelTable[] = {
	nullptr,
	"tinkoopa_breast.bmd",
	"tinkoopa_stomach.bmd",
	"tinkoopa_rarm.bmd",
	"tinkoopa_larm.bmd",
	nullptr,
};

static int partsBreakBckTable[] = { 0, 4, 11, 9, 10, 0 };

static u32 partsHitActorTypeTable[] = {
	0x08000019, 0x0800001A, 0x0800001B,
	0x0800001D, 0x0800001C, 0x0800001E,
};

static int waitBckTable[] = { 12, 13, 14, 15, 16 };

static int breakBckTable[] = { 0, 1, 2, 3, 16 };

static int damageBckTable[] = { 5, 6, 7, 8, 16 };

static int breakStartFrameTable[] = { 100, 134, 134, 100, 0 };

static int breakPartIndexTable[] = { 2, 3, 4, 1, 2 };

static const char* breastTrackJointNameTable[] = {
	"breast_1", "breast_2", "breast_3", "breast_4", "breast_5", "breast_6",
};

static const char* bellyTrackJointNameTable[] = {
	"stomach_1", "stomach_2", "stomach_3",
	"stomach_4", "stomach_5", "stomach_6",
};

static const char* rightArmTrackJointNameTable[] = {
	"rarm_1",
	"rarm_2",
	"rarm_3",
	"rarm_4",
};

static const char* leftArmTrackJointNameTable[] = {
	"larm_1",
	"larm_2",
	"larm_3",
	"larm_4",
};

inline const TNerveTinKoopaWait& TNerveTinKoopaWait::theNerve()
{
	static TNerveTinKoopaWait instance;
	return instance;
}

inline const TNerveTinKoopaDamage& TNerveTinKoopaDamage::theNerve()
{
	static TNerveTinKoopaDamage instance;
	return instance;
}

inline const TNerveTinKoopaBreak& TNerveTinKoopaBreak::theNerve()
{
	static TNerveTinKoopaBreak instance;
	return instance;
}

BOOL TNerveTinKoopaWait::execute(TSpineBase<TLiveActor>* spine) const
{
	TTinKoopa* self = (TTinKoopa*)spine->getBody();
	if (spine->getTime() == 0) {
		int bck = waitBckTable[self->unk150];
		self->mMActor->setBckFromIndex(bck);
		const char** bas = self->getBasNameTable();
		self->setAnmSound(!bas ? nullptr : bas[bck]);

		TTinKoopaParams* params = (TTinKoopaParams*)self->getSaveParam();
		self->unk180 = params->mSLDefeatWaitTime.get();
	}

	if (self->unk150 == 4 && self->unk180 <= 0)
		TFlagManager::smInstance->setBool(true, 0x5000a);

	return false;
}

BOOL TNerveTinKoopaDamage::execute(TSpineBase<TLiveActor>* spine) const
{
	TTinKoopa* self = (TTinKoopa*)spine->getBody();
	if (spine->getTime() == 0) {
		int bck = damageBckTable[self->unk150];
		self->mMActor->setBckFromIndex(bck);
		const char** bas = self->getBasNameTable();
		self->setAnmSound(bas ? bas[bck] : nullptr);

		u32 jointIndex = TTinKoopa_jointIndexTable[0];
		MtxPtr mtx     = self->getModel()->getAnmMtx(jointIndex);
		gpMarioParticleManager->emitAndBindToMtxPtr(0xee, mtx, 0, this);
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK6, 1.0f);
	}

	int damageBck = damageBckTable[self->unk150];
	if (self->mMActor->checkCurBckFromIndex(damageBck)
	    && self->mMActor->curAnmEndsNext(0, 0)) {
		int waitBck = waitBckTable[self->unk150];
		self->mMActor->setBckFromIndex(waitBck);
		const char** bas = self->getBasNameTable();
		self->setAnmSound(bas ? bas[waitBck] : nullptr);
		return true;
	}

	return false;
}

BOOL TNerveTinKoopaBreak::execute(TSpineBase<TLiveActor>* spine) const
{
	TTinKoopa* self = (TTinKoopa*)spine->getBody();
	if (spine->getTime() == 0) {
		int bck = breakBckTable[self->unk150];
		self->mMActor->setBckFromIndex(bck);
		const char** bas = self->getBasNameTable();
		self->setAnmSound(bas ? bas[bck] : nullptr);

		u32 jointIndex = TTinKoopa_jointIndexTable[0];
		MtxPtr mtx     = self->getModel()->getAnmMtx(jointIndex);
		gpMarioParticleManager->emitAndBindToMtxPtr(0xee, mtx, 0, this);
		gpCameraShake->startShake(CAM_SHAKE_MODE_UNK6, 1.0f);

		if (self->unk150 == 3) {
			((TCoasterKillerManager*)self->unk1F0)->unk60 = true;
			MSBgm::stopTrackBGMs(7, 10);
		}
	}

	int breakBck = breakBckTable[self->unk150];
	if (self->mMActor->checkCurBckFromIndex(breakBck)) {
		TTinKoopaPartsBase* part
		    = self->unk1CC[breakPartIndexTable[self->unk150]];
		if (self->mMActor->curAnmEndsNext(0, 0)) {
			part->unkF4->remove();
			++self->unk150;

			if (self->unk150 == 0) {
				TTinKoopaParams* params
				    = (TTinKoopaParams*)self->getSaveParam();
				f32 height = params->mSLDamageHeight0.get();
				params     = (TTinKoopaParams*)self->getSaveParam();
				self->mAttackRadius = 0.0f;
				self->mAttackHeight = 0.0f;
				self->mDamageRadius = params->mSLDamageRadius.get();
				self->mDamageHeight = height;
				self->calcEntryRadius();
			} else if (self->unk150 == 1) {
				TTinKoopaParams* params
				    = (TTinKoopaParams*)self->getSaveParam();
				f32 height = params->mSLDamageHeight1.get();
				params     = (TTinKoopaParams*)self->getSaveParam();
				self->mAttackRadius = 0.0f;
				self->mAttackHeight = 0.0f;
				self->mDamageRadius = params->mSLDamageRadius.get();
				self->mDamageHeight = height;
				self->calcEntryRadius();
			} else if (self->unk150 == 2) {
			}

			TTinKoopaFlame* flame = self->unk160;
			if (flame->unk68->unk150 == 0) {
				TTinKoopaParams* params
				    = (TTinKoopaParams*)flame->unk68->getSaveParam();
				f32 height = params->mSLFlameDamageHeight0.get();
				params = (TTinKoopaParams*)flame->unk68->getSaveParam();
				flame->mAttackRadius = 0.0f;
				flame->mAttackHeight = 0.0f;
				flame->mDamageRadius
				    = params->mSLFlameDamageRadius0.get();
				flame->mDamageHeight = height;
				flame->calcEntryRadius();
			} else if (flame->unk68->unk150 == 1) {
				TTinKoopaParams* params
				    = (TTinKoopaParams*)flame->unk68->getSaveParam();
				f32 height = params->mSLFlameDamageHeight1.get();
				params = (TTinKoopaParams*)flame->unk68->getSaveParam();
				flame->mAttackRadius = 0.0f;
				flame->mAttackHeight = 0.0f;
				flame->mDamageRadius
				    = params->mSLFlameDamageRadius1.get();
				flame->mDamageHeight = height;
				flame->calcEntryRadius();
			}

			int waitBck = waitBckTable[self->unk150];
			self->mMActor->setBckFromIndex(waitBck);
			const char** bas = self->getBasNameTable();
			self->setAnmSound(bas ? bas[waitBck] : nullptr);

			TTinKoopaParams* params = (TTinKoopaParams*)self->getSaveParam();
			self->unk1C8            = params->mSLPartsHP.get();
			return true;
		}

		if (!part->unkF8) {
			int frame = breakStartFrameTable[self->unk150];
			if (self->mMActor->getFrameCtrl(0)->checkPass((f32)frame)) {
				self->unk1E4 = part;
				self->unk1E4->startBreaking();
			}
		}
	}

	return false;
}

TTinKoopaManager::TTinKoopaManager(const char* name)
    : TEnemyManager(name)
{
}

void TTinKoopaManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "tinkoopa_body.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};

	createModelDataArray(entry);
}

void TTinKoopaManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TTinKoopaParams("/enemy/tinkoopa.prm");
}

void TTinKoopaManager::loadAfter()
{
	static const char* onetimeFilenames[] = {
		"/scene/tinkoopa/jpa/ms_mkp_hibana_d1he.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_killer.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_smoke1.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_parge_b14.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_parge_b23.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_flame_yuge.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_parts.jpa",
	};
	static const char* loopFilenames[] = {
		"/scene/tinkoopa/jpa/ms_mkp_hibana_w1br.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_hibana_w3ar.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_hibana_w4ar.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_biri_w1st.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_biri_w1ar.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_biri_w1fe.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_biri_w1he.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_biri_d1br_a.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_biri_d1br_b.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_b1ar.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_w2br_a.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_w2br_b.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_b1he.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_b1fe_l.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_kemu_b1fe_r.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_fire_a.jpa",
		"/scene/tinkoopa/jpa/ms_mkp_fire_b.jpa",
	};
	static const char* loopIndirectFilenames[] = {
		"/scene/tinkoopa/jpa/ms_mkp_fire_c.jpa",
	};
	for (int i = 0; i < 7; ++i) {
		u16 id = 0xee + i;
		const char* filename = onetimeFilenames[i];
		bool* particleFlag = &gParticleFlagLoaded[id];
		if (!*particleFlag) {
			gpResourceManager->load(filename, id);
			*particleFlag = true;
		}
	}

	for (int i = 0; i < 17; ++i) {
		u16 id = 0x1ac + i;
		const char* filename = loopFilenames[i];
		bool* particleFlag = &gParticleFlagLoaded[id];
		if (!*particleFlag) {
			gpResourceManager->load(filename, id);
			*particleFlag = true;
		}
	}

	const char* filename = loopIndirectFilenames[0];
	if (!gParticleFlagLoaded[0x1f2]) {
		gpResourceManager->load(filename, 0x1f2);
		gParticleFlagLoaded[0x1f2] = true;
	}
}

TSpineEnemy* TTinKoopaManager::createEnemyInstance() { return nullptr; }

BOOL TTinKoopaManager::hasMapCollision() const { return true; }

TTinKoopaParams::TTinKoopaParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLPartsHP, 2)
    , PARAM_INIT(mSLFlameHP, 10)
    , PARAM_INIT(mSLFlameRevivalTime, 10)
    , PARAM_INIT(mSLFlameDamageRadius0, 100.0f)
    , PARAM_INIT(mSLFlameDamageHeight0, 400.0f)
    , PARAM_INIT(mSLFlameDamageRadius1, 100.0f)
    , PARAM_INIT(mSLFlameDamageHeight1, 400.0f)
    , PARAM_INIT(mSLDamageRadius, 1000.0f)
    , PARAM_INIT(mSLDamageHeight0, 4000.0f)
    , PARAM_INIT(mSLDamageHeight1, 4000.0f)
    , PARAM_INIT(mSLKillerInterval, 30)
    , PARAM_INIT(mSLDefeatWaitTime, 600)
    , PARAM_INIT(mSLKillerApproachingDistance, 6000.0f)
{
	TParams::load(mPrmPath);

	mSLPartsHP.set(1);
	mSLFlameHP.set(50);
	mSLFlameRevivalTime.set(1200);
	mSLFlameDamageRadius0.set(500.0f);
	mSLFlameDamageHeight0.set(300.0f);
	mSLFlameDamageRadius1.set(700.0f);
	mSLFlameDamageHeight1.set(400.0f);
	mSLKillerInterval.set(60);
	mSLDamageRadius.set(800.0f);
	mSLDamageHeight0.set(4000.0f);
	mSLDamageHeight1.set(3400.0f);
	mSLDefeatWaitTime.set(160);
	mSLKillerApproachingDistance.set(2000.0f);
}

TTinKoopa::TTinKoopa(const char* name)
    : TSpineEnemy(name)
    , unk1F0(nullptr)
    , unk1F8(nullptr)
{
	mLiveFlag |= LIVE_FLAG_UNK10;
	mLiveFlag &= ~LIVE_FLAG_UNK100;
	mScaledBodyRadius = 2000.0f;
}

BOOL TTinKoopa::receiveMessage(THitActor* sender, u32)
{
	if (sender->mActorType == 0x1000002b) {
		hitParts();
		return true;
	}

	return false;
}

const char** TTinKoopa::getBasNameTable() const { return tinkoopa_bastable; }

BOOL TTinKoopa::hasMapCollision() const { return true; }

void TTinKoopa::launchKiller(int direction)
{
	u32 jointIndex;
	TCoasterKiller* killer = (TCoasterKiller*)unk1F0->getDeadEnemy();
	if (!killer)
		return;

	killer->reset();

	s32 jointNo;
	if (direction == 1)
		jointNo = unk174 + 11;
	else
		jointNo = 14 - unk174;

	jointIndex = TTinKoopa_jointIndexTable[jointNo];
	getJointTransByIndex(jointIndex, &killer->mPosition);
	killer->mPathDir = direction;

	MtxPtr mtx = getModel()->getAnmMtx(jointIndex);
	gpMarioParticleManager->emitAndBindToMtxPtr(0xef, mtx, 0, this);

	if (gpMSound->gateCheck(0x285d)) {
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x285d, (const Vec*)&killer->mPosition, 0, nullptr, 0, 4);
	}
}

#define ASSERT_MSG(msg, line) (void)((msg), (line))
#define ASSERT_TEST(expr)                                                      \
	(void)((expr) ? true : (ASSERT_MSG(__FILE__, __LINE__), false));

void TTinKoopa::resetTinKoopa()
{
	if (!unk1F0) {
		unk1F0 = JDrama::TNameRefGen::search<TEnemyManager>(
		    "コースターキラーマネージャー");
	}
	ASSERT_TEST(unk1F0->unk38);

	unk164 = gpMarioOriginal->mKoopaRail;
	unk150 = 0;
	unk154 = 0;
	unk158 = 0;
	unk15C = 0;
	unk168 = 0;

	TTinKoopaParams* params = (TTinKoopaParams*)getSaveParam();
	unk1C8                 = params->mSLPartsHP.get();
	unk1E4                 = 0;
	unk170                 = 0;
	unk174                 = 0;
	unk169[0]              = 0;
	unk169[1]              = 0;
	unk169[2]              = 0;
	unk169[3]              = 0;
	unk178                 = 0;
	unk17C                 = 0;
	unk180                 = 0;
	unk178                 = 0;
	unk1B4                 = 0.0f;
	unk1B8                 = 30.0f;
	unk1BC                 = 15.0f;
	unk1C0                 = 45.0f;
}

void TTinKoopa::reset()
{
	TSpineEnemy::reset();

	TTinKoopaFlame* flame = unk160;
	TTinKoopaParams* params
	    = (TTinKoopaParams*)flame->unk68->getSaveParam();
	flame->unk70 = (s16)params->mSLFlameHP.get();
	flame->unk6C = 1.0f;
	flame->unk72 = 0;

	for (int i = 0; i < 6; ++i)
		unk1CC[i]->reset();

	if (unk150 == 0) {
		params       = (TTinKoopaParams*)getSaveParam();
		f32 height   = params->mSLDamageHeight0.get();
		params       = (TTinKoopaParams*)getSaveParam();
		setHitParams(0.0f, 0.0f, params->mSLDamageRadius.get(), height);
	} else if (unk150 == 1) {
		params       = (TTinKoopaParams*)getSaveParam();
		f32 height   = params->mSLDamageHeight1.get();
		params       = (TTinKoopaParams*)getSaveParam();
		setHitParams(0.0f, 0.0f, params->mSLDamageRadius.get(), height);
	}

	flame = unk160;
	if (flame->unk68->unk150 == 0) {
		params       = (TTinKoopaParams*)flame->unk68->getSaveParam();
		f32 height   = params->mSLFlameDamageHeight0.get();
		params       = (TTinKoopaParams*)flame->unk68->getSaveParam();
		flame->setHitParams(0.0f, 0.0f,
		                    params->mSLFlameDamageRadius0.get(), height);
	} else if (flame->unk68->unk150 == 1) {
		params       = (TTinKoopaParams*)flame->unk68->getSaveParam();
		f32 height   = params->mSLFlameDamageHeight1.get();
		params       = (TTinKoopaParams*)flame->unk68->getSaveParam();
		flame->setHitParams(0.0f, 0.0f,
		                    params->mSLFlameDamageRadius1.get(), height);
	}

	resetTinKoopa();

	int bck = waitBckTable[unk150];
	mMActor->setBckFromIndex(bck);
	const char** bas = getBasNameTable();
	setAnmSound(bas ? bas[bck] : nullptr);
}

void TTinKoopa::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2) {
		emitTinKoopaEffects();
		if (mSpine->getCurrentNerve() == &TNerveTinKoopaWait::theNerve()) {
			if (unk174 < unk170 && unk178 <= 0) {
				launchKiller(unk169[unk174]);
				++unk174;
				TTinKoopaParams* params = (TTinKoopaParams*)getSaveParam();
				unk178 = params->mSLKillerInterval.get();
			}
		}
	}

	if (flags & 1) {
		if (unk178 > 0)
			--unk178;
		if (unk17C > 0)
			--unk17C;
		if (unk180 > 0)
			--unk180;

		if (unk164) {
			J3DFrameCtrl* ctrl = unk164->getFrameCtrl(0);
			if (ctrl->checkPass((f32)(ctrl->getEnd() - 1))) {
				++unk15C;
				if (unk15C > 2)
					unk15C = 0;
			}
		}

		TTinKoopaLaunchOrderTable* orders = unk1F4;
		for (int i = 0; i < orders->unk0; ++i)
			orders->unk8[i]->checkOrder();

		checkTinKoopaKillerApproachingMessage();

		if (unk164) {
			J3DFrameCtrl* ctrl = unk164->getFrameCtrl(0);
			if (unk15C == 0 && ctrl->checkPass(300.0f))
				gpMarDirector->getConsole()->startAppearBalloon(0xe000a,
				                                                true);
		}

		checkTinKoopaFirstFlameMessage();
	}

	TSpineEnemy::perform(flags, graphics);
	unk160->perform(flags, graphics);

	for (int i = 0; i < 6; ++i)
		unk1CC[i]->perform(flags, graphics);
}

static inline MtxPtr getTinKoopaJointMtx(TTinKoopa* koopa, int jointNo)
{
	return koopa->getModel()->getAnmMtx(TTinKoopa_jointIndexTable[jointNo]);
}

static inline void copyTinKoopaJointPos(TTinKoopa* koopa, int jointNo,
                                        JGeometry::TVec3<f32>* out)
{
	MtxPtr mtx = getTinKoopaJointMtx(koopa, jointNo);
	out->set(mtx[0][3], mtx[1][3], mtx[2][3]);
}

static inline void emitTinKoopaMtxParticle(TTinKoopa* koopa, u32 particleId,
                                           int jointNo, u8 kind,
                                           const void* user)
{
	gpMarioParticleManager->emitAndBindToMtxPtr(
	    particleId, getTinKoopaJointMtx(koopa, jointNo), kind, user);
}

static inline void emitTinKoopaPosParticle(TTinKoopa* koopa, u32 particleId,
                                           JGeometry::TVec3<f32>* pos, u8 kind,
                                           const void* user)
{
	gpMarioParticleManager->emitAndBindToPosPtr(particleId, pos, kind, user);
}

static inline BOOL isTinKoopaBreakFrame(TTinKoopa* koopa, f32 frame)
{
	return koopa->mMActor->getFrameCtrl(0)->checkPass(frame);
}

void TTinKoopa::emitTinKoopaEffects()
{
	JGeometry::TVec3<f32>* effectPos = (JGeometry::TVec3<f32>*)unk184;
	copyTinKoopaJointPos(this, 0, &effectPos[0]);
	copyTinKoopaJointPos(this, 1, &effectPos[1]);
	copyTinKoopaJointPos(this, 3, &effectPos[2]);
	copyTinKoopaJointPos(this, 4, &effectPos[3]);

	const void* otherUser = this + 1;

	emitTinKoopaPosParticle(this, 0x1ac, &effectPos[1], 1, this);

	if (unk150 > 1)
		emitTinKoopaMtxParticle(this, 0x1ad, 3, 1, this);

	if (unk150 > 2)
		emitTinKoopaMtxParticle(this, 0x1ae, 4, 1, this);

	BOOL isWait = mSpine->getCurrentNerve() == &TNerveTinKoopaWait::theNerve();
	BOOL isDamage
	    = mSpine->getCurrentNerve() == &TNerveTinKoopaDamage::theNerve();
	BOOL isBreak
	    = mSpine->getCurrentNerve() == &TNerveTinKoopaBreak::theNerve();

	if (unk150 <= 0 && isWait)
		emitTinKoopaMtxParticle(this, 0x1af, 2, 1, this);

	emitTinKoopaMtxParticle(this, 0x1b0, 3, 1, this);
	emitTinKoopaMtxParticle(this, 0x1b0, 4, 1, otherUser);
	emitTinKoopaMtxParticle(this, 0x1b1, 10, 1, this);
	emitTinKoopaMtxParticle(this, 0x1b2, 0, 1, this);

	if (isDamage || isBreak) {
		emitTinKoopaPosParticle(this, 0x1b3, &effectPos[1], 1, this);
		emitTinKoopaPosParticle(this, 0x1b3, &effectPos[1], 1, otherUser);
		emitTinKoopaPosParticle(this, 0x1b4, &effectPos[1], 1, this);
		emitTinKoopaPosParticle(this, 0x1b4, &effectPos[1], 1, otherUser);
	}

	if ((isWait && unk150 > 1) || isDamage || isBreak)
		emitTinKoopaPosParticle(this, 0x1b5, &effectPos[2], 1, this);

	if ((isWait && unk150 > 2) || isDamage || isBreak)
		emitTinKoopaPosParticle(this, 0x1b5, &effectPos[3], 1, otherUser);

	if (((isWait || isDamage) && unk150 > 0)
	    || (isBreak && (unk150 == 1 || unk150 == 2))) {
		emitTinKoopaMtxParticle(this, 0x1b6, 1, 1, this);
		emitTinKoopaMtxParticle(this, 0x1b7, 1, 1, this);
	}

	if ((isWait && unk150 > 2) || isDamage || isBreak)
		emitTinKoopaPosParticle(this, 0x1b8, &effectPos[0], 1, this);

	if (isDamage || isBreak) {
		emitTinKoopaMtxParticle(this, 0x1ba, 10, 1, this);
		emitTinKoopaMtxParticle(this, 0x1b9, 10, 1, this);
	}

	if (isBreak) {
		if (unk150 == 0 || unk150 == 3) {
			if (isTinKoopaBreakFrame(this, 100.0f))
				gpCameraShake->startShake(CAM_SHAKE_MODE_UNK6, 1.0f);
		}

		if (unk150 == 1 || unk150 == 2) {
			if (isTinKoopaBreakFrame(this, 104.0f))
				gpCameraShake->startShake(CAM_SHAKE_MODE_UNK6, 1.0f);
		}

		if (unk150 == 0 || unk150 == 3) {
			if (isTinKoopaBreakFrame(this, 108.0f))
				emitTinKoopaMtxParticle(this, 0xf0, 2, 0, this);
		}

		if (unk150 == 0) {
			if (isTinKoopaBreakFrame(this, 100.0f))
				emitTinKoopaMtxParticle(this, 0xf1, 2, 0, this);
		}

		if (unk150 == 3) {
			if (isTinKoopaBreakFrame(this, 100.0f))
				emitTinKoopaMtxParticle(this, 0xf1, 1, 0, this);
		}

		if (unk150 == 1) {
			if (isTinKoopaBreakFrame(this, 106.0f))
				emitTinKoopaMtxParticle(this, 0xf2, 3, 0, this);
		}

		if (unk150 == 2) {
			if (isTinKoopaBreakFrame(this, 106.0f))
				emitTinKoopaMtxParticle(this, 0xf2, 4, 0, this);
		}
	}

	if (unk1E4)
		unk1E4->emitPartsDisappearEffects();
}

void TTinKoopa::checkTinKoopaFirstFlameMessage()
{
	if (!unk164)
		return;
	if (unk168)
		return;
	if (mSpine->getCurrentNerve() != &TNerveTinKoopaWait::theNerve())
		return;

	J3DFrameCtrl* ctrl = unk164->getFrameCtrl(0);
	if (unk150 == 0) {
		if (ctrl->checkPass(2600.0f)) {
			gpMarDirector->getConsole()->startAppearBalloon(0xe000b, true);
			unk168 = true;
		}
	} else {
		if (ctrl->checkPass(3100.0f)) {
			gpMarDirector->getConsole()->startAppearBalloon(0xe000b, true);
			unk168 = true;
		}
	}
}

void TTinKoopa::checkTinKoopaKillerApproachingMessage()
{
	u32 messageId = 0xe0009;
	for (int i = 0; i < unk1F0->getActiveObjNum(); ++i) {
		TCoasterKiller* killer = (TCoasterKiller*)unk1F0->getObj(i);
		if (killer->checkLiveFlag(LIVE_FLAG_DEAD))
			continue;

		TTinKoopaParams* params = (TTinKoopaParams*)getSaveParam();
		f32 messageDistance
		    = params->mSLKillerApproachingDistance.get();
		JGeometry::TVec3<f32> marioPos = *gpMarioPos;

		bool approaching;
		if (killer->checkLiveFlag(LIVE_FLAG_DEAD)) {
			approaching = false;
		} else if (killer->mPathDir != 0) {
			approaching = false;
		} else {
			int marioNode = unk1EC->findNearestNodeIndex(marioPos, -1);
			f32 distance;
			if (marioNode >= killer->mPathIdx) {
				distance = calcCoasterDistance(killer->mPathIdx, marioNode);
			} else {
				distance = calcCoasterDistance(killer->mPathIdx,
				                               unk1EC->unk8 - 1);
				distance += calcCoasterDistance(0, marioNode);
			}
			approaching = distance <= messageDistance;
		}

		if (approaching)
			gpMarDirector->getConsole()->startAppearBalloon(messageId,
			                                                true);
	}
}

#pragma dont_inline on
void TTinKoopa::hitParts()
{
	if (mSpine->getCurrentNerve() == &TNerveTinKoopaBreak::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveTinKoopaDamage::theNerve())
		return;
	if (unk150 == 4)
		return;

	gpMarDirector->getConsole()->startAppearBalloon(0xe0024, true);
	--unk1C8;

	if (unk1C8 <= 0)
		mSpine->pushNerve(&TNerveTinKoopaBreak::theNerve());
	else
		mSpine->pushNerve(&TNerveTinKoopaDamage::theNerve());
}
#pragma dont_inline off

#pragma dont_inline on
f32 TTinKoopa::calcCoasterDistance(int start, int end)
{
	f32 distance = 0.0f;
	for (int i = start; i < end; ++i)
		distance += unk1E8[i];
	return distance;
}
#pragma dont_inline off

void TTinKoopaMtxCalc::calc(u16 index)
{
	M3UMtxCalcSIAnmBlendQuat::calc(index);
}

void TTinKoopaLaunchOrder::checkOrder()
{
	if (unk0->unk15C != unk4)
		return;

	s32 frame = unk8;
	bool passed;
	if (!unk0->unk164) {
		passed = false;
	} else {
		if (unk0->unk164->getFrameCtrl(0)->checkPass((f32)frame))
			passed = true;
		else
			passed = false;
	}

	if (!passed)
		return;

	s32 count = 1;
	if (unkC == -1) {
		if (unk0->unk150 == 0) {
			count = 1;
		} else if (unk0->unk150 == 1) {
			count = 1;
		} else if (unk0->unk150 == 2) {
			count = 2;
		} else if (unk0->unk150 == 3) {
			count = 3;
		}
	} else {
		count = unkC;
	}

	s8 direction = unkD;
	if (direction == 1) {
		s32 nextCount;
		if (count <= 2)
			nextCount = count;
		else
			nextCount = 2;
		count = nextCount;
	}

	s32 cappedCount = count;
	TTinKoopa* koopa = unk0;
	if (count > 4)
		cappedCount = 4;

	koopa->unk174 = 0;
	koopa->unk170 = cappedCount;
	for (int i = 0; i < koopa->unk170; ++i)
		koopa->unk169[i] = direction;
	koopa->unk178 = 0;
}

void TTinKoopa::makeLaunchSchedule()
{
	int index = 0;
	TTinKoopaLaunchOrder* order = unk1F4->unk8[index++];
	order->unk4 = 0;
	order->unk8 = 0x258;
	order->unkC = -1;
	order->unkD = 0;

	order = unk1F4->unk8[index++];
	order->unk4 = 0;
	order->unk8 = 0x4ce;
	order->unkC = -1;
	order->unkD = 1;

	order = unk1F4->unk8[index++];
	order->unk4 = 0;
	order->unk8 = 0x992;
	order->unkC = -1;
	order->unkD = 1;

	order = unk1F4->unk8[index++];
	order->unk4 = 1;
	order->unk8 = 0xf0;
	order->unkC = -1;
	order->unkD = 0;

	order = unk1F4->unk8[index++];
	order->unk4 = 1;
	order->unk8 = 0x118;
	order->unkC = -1;
	order->unkD = 1;

	order = unk1F4->unk8[index++];
	order->unk4 = 1;
	order->unk8 = 0x276;
	order->unkC = -1;
	order->unkD = 0;

	order = unk1F4->unk8[index++];
	order->unk4 = 1;
	order->unk8 = 0x384;
	order->unkC = -1;
	order->unkD = 0;

	order = unk1F4->unk8[index++];
	order->unk4 = 1;
	order->unk8 = 0x4b0;
	order->unkC = -1;
	order->unkD = 1;

	order = unk1F4->unk8[index++];
	order->unk4 = 2;
	order->unk8 = 0x235;
	order->unkC = -1;
	order->unkD = 0;

	order = unk1F4->unk8[index++];
	order->unk4 = 2;
	order->unk8 = 0x2bc;
	order->unkC = -1;
	order->unkD = 0;

	order = unk1F4->unk8[index++];
	order->unk4 = 2;
	order->unk8 = 0x8ac;
	order->unkC = -1;
	order->unkD = 1;
}

inline TTinKoopaFlame::TTinKoopaFlame(const char* name)
    : THitActor(name)
{
}

inline TTinKoopaPartsBase::TTinKoopaPartsBase(const char* name, s32 partIndex,
                                              TTinKoopa* owner)
    : TLiveActor(name)
    , unkF8(0)
    , unkFC(partIndex)
    , unk100(owner)
    , unk104(0)
{
}

inline TTinKoopaLaunchOrder::TTinKoopaLaunchOrder(TTinKoopa* owner, s8 phase,
                                                  s32 frame, s8 count,
                                                  s8 direction)
    : unk0(owner)
    , unk4(phase)
    , unk8(frame)
    , unkC(count)
    , unkD(direction)
{
}

void TTinKoopa::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	mSpine->initWith(&TNerveTinKoopaWait::theNerve());

	mMActorKeeper = new TMActorKeeper(mManager, 7);
	mMActor       = mMActorKeeper->createMActor("tinkoopa_body.bmd", 0);

	JUTNameTab* jointNames = getModel()->getModelData()->getJointName();
	for (int i = 0; i < 15; ++i)
		TTinKoopa_jointIndexTable[i]
		    = jointNames->getIndex(TTinKoopa_jointNameTable[i]);

	TTinKoopaFlame* flame = new TTinKoopaFlame("flame");
	if (flame) {
		flame->unk68 = this;

		TTinKoopaParams* params
		    = (TTinKoopaParams*)flame->unk68->getSaveParam();
		f32 height = params->mSLFlameDamageHeight0.get();
		params     = (TTinKoopaParams*)flame->unk68->getSaveParam();
		flame->initHitActor(0x08000027, 0, 0, 0.0f, 0.0f,
		                    params->mSLFlameDamageRadius0.get(), height);
		flame->offHitFlag(HIT_FLAG_NO_COLLISION);

		JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")->add(flame);

		params       = (TTinKoopaParams*)flame->unk68->getSaveParam();
		flame->unk70 = (s16)params->mSLFlameHP.get();
		flame->unk6C = 1.0f;
		flame->unk72 = 0;
	}
	unk160 = flame;

	for (int i = 0; i < 6; ++i) {
		TTinKoopaPartsBase* part
		    = new TTinKoopaPartsBase(partsCollisionFileTable[i], i, this);
		unk1CC[i] = part;
		part->initTinKoopaPartsBase();
	}

	TTinKoopaParams* params = (TTinKoopaParams*)getSaveParam();
	f32 height             = params->mSLDamageHeight0.get();
	params                 = (TTinKoopaParams*)getSaveParam();
	initHitActor(0x08000018, 1, 0, 0.0f, 0.0f,
	             params->mSLDamageRadius.get(), height);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	mMActor->setLightType(1);

	J3DModel* model = mMActor->getModel();
	if (!model->getSkinDeform())
		model->setSkinDeform(new J3DSkinDeform, J3D_DEFORM_ATTACH_FLAG_UNK_1);

	calcRootMatrix();
	mMActor->calc();

	unk1EC = gpConductor->getGraphByName("killer");
	unk1E8 = new f32[unk1EC->unk8 - 1];

	JGeometry::TVec3<f32> prev = unk1EC->indexToPoint(0);
	f32 total                  = 0.0f;
	for (int i = 0; i < unk1EC->unk8 - 1; ++i) {
		JGeometry::TVec3<f32> current = unk1EC->indexToPoint(i + 1);
		JGeometry::TVec3<f32> diff;
		diff.sub(prev, current);
		unk1E8[i] = diff.length();
		total += unk1E8[i];
		prev = current;
	}

	unk1F4 = new TTinKoopaLaunchOrderTable;
	if (unk1F4) {
		unk1F4->unk0 = 11;
		unk1F4->unk4 = this;
		unk1F4->unk8 = new TTinKoopaLaunchOrder*[unk1F4->unk0];

		for (int i = 0; i < unk1F4->unk0; ++i)
			unk1F4->unk8[i] = new TTinKoopaLaunchOrder(this, 0, 0, 0, 0);
	}

	makeLaunchSchedule();
	initAnmSound();
	resetTinKoopa();
}

BOOL TTinKoopaPartsBase::receiveMessage(THitActor* sender, u32)
{
	if (sender->mActorType == 0x1000002b) {
		unk100->hitParts();
		return true;
	}

	return false;
}

void TTinKoopaPartsBase::reset()
{
	unkF8 = 0;
	JGeometry::TVec3<f32> zero(0.0f, 0.0f, 0.0f);
	unkF4->setUpTrans(zero);
}

BOOL TTinKoopaFlame::receiveMessage(THitActor*, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (unk68->unk17C <= 0) {
			if (unk70 > 0)
				--unk70;

			if (unk70 <= 0) {
				TTinKoopaParams* params
				    = (TTinKoopaParams*)unk68->getSaveParam();
				unk70 = (s16)params->mSLFlameHP.get();
				params = (TTinKoopaParams*)unk68->getSaveParam();
				unk68->unk17C
				    = (s16)params->mSLFlameRevivalTime.get();
				onHitFlag(HIT_FLAG_NO_COLLISION);
			}

			if (!unk72) {
				unk72 = 1;
				gpMarioParticleManager->emitAndBindToPosPtr(
				    0xf3, &mPosition, 0, this);
			}
		}

		return true;
	}

	return false;
}

void TTinKoopaFlame::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (flags & 1) {
		if (unk68->unk17C <= 0 && unk68->unk150 != 4) {
			s32 frame = unk68->unk150 == 0 ? 0xabe : 0xd48;
			bool passed;
			if (!unk68->unk164) {
				passed = false;
			} else {
				if (unk68->unk164->getFrameCtrl(0)->checkPass((f32)frame))
					passed = true;
				else
					passed = false;
			}
			if (passed)
				SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
		}
	}

	if (flags & 2) {
		MtxPtr mtx = unk68->getModel()->getAnmMtx(TTinKoopa_jointIndexTable[9]);
		mPosition.x = mtx[0][3];
		mPosition.y = mtx[1][3];
		mPosition.z = mtx[2][3];

		emitFlameEffects();
		if (unk68->unk17C <= 0)
			offHitFlag(HIT_FLAG_NO_COLLISION);
		unk72 = 0;
	}
}

void TTinKoopaFlame::emitFlameEffects()
{
	if (unk68->mSpine->getCurrentNerve() != &TNerveTinKoopaWait::theNerve())
		return;

	MtxPtr mtx = unk68->getModel()->getAnmMtx(TTinKoopa_jointIndexTable[8]);

	f32 baseScale = 1.0f;
	if (unk68->unk150 != 0)
		baseScale = 1.6f;

	if (unk68->unk17C > 0) {
		unk6C -= 0.05f;
		if (unk6C < 0.3f)
			unk6C = 0.3f;
	} else {
		unk6C += 0.05f;
		if (unk6C > 1.0f)
			unk6C = 1.0f;
	}

	f32 yScale;
	f32 xzScale;
	yScale = xzScale = unk6C * baseScale;
	if (unk68->unk17C > 0)
		yScale *= 3.0f;

	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emitAndBindToMtxPtr(0x1bb, mtx, 1, this);
	if (emitter) {
		emitter->unk154.x = xzScale;
		emitter->unk154.y = yScale;
		emitter->unk154.z = xzScale;
		emitter->unk174.x = xzScale;
		emitter->unk174.y = yScale;
		emitter->unk174.z = xzScale;
	}

	emitter
	    = gpMarioParticleManager->emitAndBindToMtxPtr(0x1bc, mtx, 1, this);
	if (emitter) {
		emitter->unk154.x = xzScale;
		emitter->unk154.y = yScale;
		emitter->unk154.z = xzScale;
		emitter->unk174.x = xzScale;
		emitter->unk174.y = yScale;
		emitter->unk174.z = xzScale;
	}

	emitter
	    = gpMarioParticleManager->emitAndBindToMtxPtr(0x1f2, mtx, 3, this);
	if (emitter) {
		emitter->unk154.x = xzScale;
		emitter->unk154.y = yScale;
		emitter->unk154.z = xzScale;
		emitter->unk174.x = xzScale;
		emitter->unk174.y = yScale;
		emitter->unk174.z = xzScale;
	}

	if (gpMSound->gateCheck(0x8135)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x8135, (const Vec*)&mPosition, nullptr, xzScale, 0, 0, nullptr, 0,
		    4);
	}
}

void TTinKoopaPartsBase::perform(u32 flags, JDrama::TGraphics* graphics)
{
	JDrama::TGraphics* performGraphics = graphics;
	TLiveActor::perform(flags, performGraphics);

	if (flags & 1) {
		u32 jointIndex = TTinKoopa_jointIndexTable[unkFC];
		MtxPtr mtx     = unk100->getModel()->getAnmMtx(jointIndex);
		unkF4->moveMtx(mtx);
	}

	if (flags & 2) {
		if (unkF8 && unk104) {
			if (unk104->curAnmEndsNext(0, nullptr))
				unkF8 = 0;
		}
	}

	if (unkF8 && unk104)
		unk104->perform(flags, performGraphics);
}

void TTinKoopaPartsBase::emitPartsDisappearEffects(const char** jointNames,
                                                   int count, f32 scale)
{
	JGeometry::TVec3<f32> effectScale = mScaling;
	effectScale *= scale;

	JUTNameTab* jointNamesTable
	    = unk104->getModel()->getModelData()->getJointName();
	for (int i = 0; i < count; ++i) {
		s32 jointIndex = jointNamesTable->getIndex(jointNames[i]);
		if (jointIndex < 0)
			break;

		MtxPtr mtx = unk104->getModel()->getAnmMtx(jointIndex);
		unk108[i].set(mtx[0][3], mtx[1][3], mtx[2][3]);

		TEffectExplosion* effect
		    = (TEffectExplosion*)gpConductor->makeOneEnemyAppear(
		        unk108[i], "エフェクト爆発マネージャー", 1);
		if (!effect)
			break;

		effect->generate(unk108[i], effectScale);
	}
}

void TTinKoopaPartsBase::emitPartsDisappearEffects()
{
	if (!unk104)
		return;

	if (!unk104->checkCurBckFromIndex(partsBreakBckTable[unkFC]))
		return;

	s32 frame = 60;
	if (!unk104->getFrameCtrl(0)->checkPass((f32)frame))
		return;

	if (unkFC == 1) {
		emitPartsDisappearEffects(breastTrackJointNameTable, 6, 4.0f);
	} else if (unkFC == 2) {
		emitPartsDisappearEffects(bellyTrackJointNameTable, 6, 4.0f);
	} else if (unkFC == 3) {
		emitPartsDisappearEffects(rightArmTrackJointNameTable, 4, 3.0f);
	} else if (unkFC == 4) {
		emitPartsDisappearEffects(leftArmTrackJointNameTable, 4, 3.0f);
	}

	unk100->unk1E4 = 0;
}

#pragma dont_inline on
void TTinKoopaPartsBase::emitPartsTrackEffects(const char** jointNames,
                                               int count)
{
	unk104->getModel()->calc();

	JUTNameTab* jointNamesTable
	    = unk104->getModel()->getModelData()->getJointName();
	for (int i = 0; i < count; ++i) {
		s32 jointIndex = jointNamesTable->getIndex(jointNames[i]);
		if (jointIndex < 0)
			break;

		MtxPtr mtx = unk104->getModel()->getAnmMtx(jointIndex);
		unk108[i].set(mtx[0][3], mtx[1][3], mtx[2][3]);

		gpMarioParticleManager->emitAndBindToPosPtr(
		    0xf4, &unk108[i], 0, unk100);
	}
}
#pragma dont_inline off

void TTinKoopaPartsBase::startBreaking()
{
	unkF8 = 1;

	u32 jointIndex = TTinKoopa_jointIndexTable[unkFC];
	MtxPtr mtx     = unk100->getModel()->getAnmMtx(jointIndex);
	mPosition.x    = mtx[0][3];
	mPosition.y    = mtx[1][3];
	mPosition.z    = mtx[2][3];

	if (!unk104)
		return;

	unk104->setBckFromIndex(partsBreakBckTable[unkFC]);

	J3DModel* model = unk104->getModel();
	MtxPtr baseMtx  = model->getBaseTRMtx();
	baseMtx[0][3]   = mPosition.x;
	baseMtx[1][3]   = mPosition.y;
	baseMtx[2][3]   = mPosition.z;
	unk104->getModel()->setBaseTRMtx(baseMtx);

	if (!unk104)
		return;

	if (unkFC == 1) {
		emitPartsTrackEffects(breastTrackJointNameTable, 6);
	} else if (unkFC == 2) {
		emitPartsTrackEffects(bellyTrackJointNameTable, 6);
	} else if (unkFC == 3) {
		emitPartsTrackEffects(rightArmTrackJointNameTable, 4);
	} else if (unkFC == 4) {
		emitPartsTrackEffects(leftArmTrackJointNameTable, 4);
	}
}

#pragma dont_inline on
void TTinKoopaPartsBase::initTinKoopaPartsBase()
{
	initHitActor(partsHitActorTypeTable[unkFC], 0, 0, 0.0f, 0.0f, 0.0f,
	             0.0f);
	unk64 |= 1;

	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	group->getChildren().push_back(this);

	unkF4 = new TMapCollisionMove();
	unkF4->init(partsCollisionFileTable[unkFC], 0, this);

	JGeometry::TVec3<f32> zero(0.0f);
	unkF4->setUpTrans(zero);

	if (partsBreakModelTable[unkFC] != nullptr) {
		unk104 = unk100->getActorKeeper()->createMActor(
		    partsBreakModelTable[unkFC], 0);
		unk104->setLightType(1);
	}

	unkF8 = 0;
	unkF4->setUpTrans(zero);
}
#pragma dont_inline off
