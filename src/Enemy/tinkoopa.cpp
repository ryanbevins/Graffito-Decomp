#include <Enemy/TinKoopa.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
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

static const int partsBreakBckTable[] = { 0, 4, 11, 9, 10, 0 };

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

static const char* loopIndirectFilenames
    = "/scene/tinkoopa/jpa/ms_mkp_fire_c.jpa";

TTinKoopaManager::TTinKoopaManager(const char* name)
    : TEnemyManager(name)
{
}

void TTinKoopaManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
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
	for (int i = 0; i < 7; ++i) {
		u16 id = 0xee + i;
		if (!gParticleFlagLoaded[id]) {
			gpResourceManager->load(onetimeFilenames[i], id);
			gParticleFlagLoaded[id] = true;
		}
	}

	for (int i = 0; i < 17; ++i) {
		u16 id = 0x1ac + i;
		if (!gParticleFlagLoaded[id]) {
			gpResourceManager->load(loopFilenames[i], id);
			gParticleFlagLoaded[id] = true;
		}
	}

	if (!gParticleFlagLoaded[0x1f2]) {
		gpResourceManager->load(loopIndirectFilenames, 0x1f2);
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

void TTinKoopaMtxCalc::calc(u16 index)
{
	M3UMtxCalcSIAnmBlendQuat::calc(index);
}

void TTinKoopaLaunchOrder::checkOrder()
{
	TTinKoopa* koopa = unk0;
	if (koopa->unk15C != unk4)
		return;

	bool passed = false;
	if (koopa->unk164)
		passed = koopa->unk164->getFrameCtrl(0)->checkPass((f32)unk8);

	if (!passed)
		return;

	s32 count = 1;
	if (unkC == -1) {
		switch (koopa->unk150) {
		case 2:
			count = 2;
			break;
		case 3:
			count = 3;
			break;
		default:
			count = 1;
			break;
		}
	} else {
		count = unkC;
	}

	if (unkD == 1 && count > 2)
		count = 2;

	if (count > 4)
		count = 4;

	koopa->unk174 = 0;
	koopa->unk170 = count;
	for (int i = 0; i < koopa->unk170; ++i)
		koopa->unk169[i] = unkD;
	koopa->unk178 = 0;
}

void TTinKoopa::makeLaunchSchedule()
{
	TTinKoopaLaunchOrder** order = unk1F4->unk8;

	order[0]->unk4 = 0;
	order[0]->unk8 = 0x258;
	order[0]->unkC = -1;
	order[0]->unkD = 0;

	order[1]->unk4 = 0;
	order[1]->unk8 = 0x4ce;
	order[1]->unkC = -1;
	order[1]->unkD = 1;

	order[2]->unk4 = 0;
	order[2]->unk8 = 0x992;
	order[2]->unkC = -1;
	order[2]->unkD = 1;

	order[3]->unk4 = 1;
	order[3]->unk8 = 0xf0;
	order[3]->unkC = -1;
	order[3]->unkD = 0;

	order[4]->unk4 = 1;
	order[4]->unk8 = 0x118;
	order[4]->unkC = -1;
	order[4]->unkD = 1;

	order[5]->unk4 = 1;
	order[5]->unk8 = 0x276;
	order[5]->unkC = -1;
	order[5]->unkD = 0;

	order[6]->unk4 = 1;
	order[6]->unk8 = 0x384;
	order[6]->unkC = -1;
	order[6]->unkD = 0;

	order[7]->unk4 = 1;
	order[7]->unk8 = 0x4b0;
	order[7]->unkC = -1;
	order[7]->unkD = 1;

	order[8]->unk4 = 2;
	order[8]->unk8 = 0x235;
	order[8]->unkC = -1;
	order[8]->unkD = 0;

	order[9]->unk4 = 2;
	order[9]->unk8 = 0x2bc;
	order[9]->unkC = -1;
	order[9]->unkD = 0;

	order[10]->unk4 = 2;
	order[10]->unk8 = 0x8ac;
	order[10]->unkC = -1;
	order[10]->unkD = 1;
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
			if (unk68->unk164) {
				passed = unk68->unk164->getFrameCtrl(0)->checkPass((f32)frame);
			} else {
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

void TTinKoopaPartsBase::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TLiveActor::perform(flags, graphics);

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
		unk104->perform(flags, graphics);
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

	switch (unkFC) {
	case 1:
		emitPartsTrackEffects(breastTrackJointNameTable, 6);
		break;
	case 2:
		emitPartsTrackEffects(bellyTrackJointNameTable, 6);
		break;
	case 3:
		emitPartsTrackEffects(rightArmTrackJointNameTable, 4);
		break;
	case 4:
		emitPartsTrackEffects(leftArmTrackJointNameTable, 4);
		break;
	default:
		break;
	}
}
