#include <Enemy/TinKoopa.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
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
