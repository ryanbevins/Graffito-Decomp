#include <Enemy/Koopa.hpp>
#include <Enemy/BathtubBinder.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <System/Particles.hpp>
#include <math.h>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static const char* koopajr_bastable[] = {
	"/scene/koopajr/bas/koopajr_damage.bas",
	"/scene/koopajr/bas/koopajr_shoot.bas",
	nullptr,
	"/scene/koopajr/bas/koopajr_yahoo.bas",
};

static const char* TKoopaJr_jointNameTable[] = {
	"KoopaJr_null",
	"killer_null00",
	"killer_null03",
	"killer_null01",
	"killer_null04",
};

static s32 TKoopaJr_jointIndexTable[5];
static const char* koopajrsubmarine_bastable = nullptr;

DEFINE_NERVE(TNerveKoopaJrWait, TLiveActor)
{
	TKoopaJr* actor = (TKoopaJr*)spine->getBody();
	if (spine->getTime() == 0) {
		actor->mMActor->setBckFromIndex(2);

		const char** bas = actor->getBasNameTable();
		actor->setAnmSound(!bas ? nullptr : bas[2]);

		actor->unk154 = actor->getSaveParam2()->mSLLaunchKillerPeriod.get();
		actor->unk158
		    = actor->getSaveParam2()->mSLLaunchKillerPeriodFast.get();
	}

	return FALSE;
}

DEFINE_NERVE(TNerveKoopaJrSubmarineCannonOpenClose, TLiveActor)
{
	TKoopaJrSubmarine* actor = (TKoopaJrSubmarine*)spine->getBody();
	if (spine->getTime() == 0) {
		actor->mMActor->setBckFromIndex(0);

		const char** bas = actor->getBasNameTable();
		actor->setAnmSound(!bas ? nullptr : bas[0]);
		f32 rate = actor->unk188;
		actor->mMActor->getFrameCtrl(0)->setRate(rate);
	}

	return actor->mMActor->isCurAnmAlreadyEnd(0) ? TRUE : FALSE;
}

DEFINE_NERVE(TNerveKoopaJrLaunch, TLiveActor)
{
	TKoopaJr* actor = (TKoopaJr*)spine->getBody();
	if (spine->getTime() == 0) {
		actor->mMActor->setBckFromIndex(1);

		const char** bas = actor->getBasNameTable();
		actor->setAnmSound(!bas ? nullptr : bas[1]);
	}

	return actor->mMActor->isCurAnmAlreadyEnd(0) ? TRUE : FALSE;
}

TDirectionCalc::TDirectionCalc()
    : mDirection(0.0f)
{
}

TDirectionCalc::TDirectionCalc(f32 direction)
    : mDirection(direction)
{
}

TDirectionCalc::TDirectionCalc(JGeometry::TVec3<f32> direction)
{
	makeDirection(direction);
}

f32 TDirectionCalc::r2d(f32 radians)
{
	return 180.0f * radians / 3.1415927f;
}

f32 TDirectionCalc::d2r(f32 degrees)
{
	return degrees * 3.1415927f / 180.0f;
}

f32 TDirectionCalc::absDirection(f32 direction)
{
	mDirection = JGeometry::TUtil<f32>::mod(6.2831855f + (mDirection - 0.0f),
	                                        6.2831855f)
	             + 0.0f;

	if (direction >= mDirection) {
		f32 diff = direction - mDirection;
		if (6.2831855f - diff < diff)
			direction -= 6.2831855f;
	} else {
		f32 diff = mDirection - direction;
		if (6.2831855f - diff < diff)
			direction += 6.2831855f;
	}

	return __fabsf(mDirection - direction);
}

JGeometry::TVec3<f32> TDirectionCalc::calcDirectionVector()
{
	f32 z = cosf(mDirection);
	f32 x = sinf(mDirection);
	return JGeometry::TVec3<f32>(x, 0.0f, z);
}

void TDirectionCalc::makeDirection(JGeometry::TVec3<f32> direction)
{
	mDirection = atan2f(direction.z, direction.x);
}

f32 TDirectionCalc::calcTurnDirection(f32 direction, f32 maxTurn)
{
	mDirection = std::fmodf(6.2831855f + (mDirection - 0.0f), 6.2831855f)
	             + 0.0f;
	mDirection = JGeometry::TUtil<f32>::mod(6.2831855f + (mDirection - 0.0f),
	                                        6.2831855f)
	             + 0.0f;

	if (direction >= mDirection) {
		f32 diff = direction - mDirection;
		if (6.2831855f - diff < diff)
			direction -= 6.2831855f;
	} else {
		f32 diff = mDirection - direction;
		if (6.2831855f - diff < diff)
			direction += 6.2831855f;
	}

	if (direction > mDirection) {
		f32 diff = direction - mDirection;
		if (diff < maxTurn)
			maxTurn = diff;
		return mDirection + maxTurn;
	}

	f32 diff = mDirection - direction;
	if (diff < maxTurn)
		maxTurn = diff;
	return mDirection - maxTurn;
}

f32 TDirectionCalc::sub(f32 direction)
{
	mDirection = JGeometry::TUtil<f32>::mod(6.2831855f + (mDirection - 0.0f),
	                                        6.2831855f)
	             + 0.0f;

	if (direction >= mDirection) {
		f32 diff = direction - mDirection;
		if (6.2831855f - diff < diff)
			direction -= 6.2831855f;
	} else {
		f32 diff = mDirection - direction;
		if (6.2831855f - diff < diff)
			direction += 6.2831855f;
	}

	return mDirection - direction;
}

f32 TDirectionCalc::calcNearerDirection(f32 direction)
{
	mDirection = std::fmodf(6.2831855f + (mDirection - 0.0f), 6.2831855f)
	             + 0.0f;

	if (direction >= mDirection) {
		f32 diff = direction - mDirection;
		if (6.2831855f - diff < diff)
			direction -= 6.2831855f;
	} else {
		f32 diff = mDirection - direction;
		if (6.2831855f - diff < diff)
			direction += 6.2831855f;
	}

	return direction;
}

TKoopaJrParams::TKoopaJrParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLLaunchKillerLimit, 10000.0f)
    , PARAM_INIT(mSLDamageRadius, 1000.0f)
    , PARAM_INIT(mSLDamageHeight, 4000.0f)
    , PARAM_INIT(mSLKoopaJrScale, 1.6f)
    , PARAM_INIT(mSLFastLaunchDistance, 10000.0f)
    , PARAM_INIT(mSLDamagePeriod, 360)
    , PARAM_INIT(mSLLaunchKillerPeriod, 1200)
    , PARAM_INIT(mSLLaunchKillerPeriodFast, 360)
{
	TParams::load(mPrmPath);

	mSLLaunchKillerLimit.set(4200.0f);
	mSLDamageRadius.set(100.0f);
	mSLDamageHeight.set(300.0f);
	mSLKoopaJrScale.set(2.0f);
	mSLFastLaunchDistance.set(4600.0f);
	mSLDamagePeriod.set(240);
	mSLLaunchKillerPeriod.set(840);
	mSLLaunchKillerPeriodFast.set(360);
}

TKoopaJrSubmarineParams::TKoopaJrSubmarineParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(killerTargetDistanceMin, 500.0f)
    , PARAM_INIT(killerTargetDistance, 500.0f)
    , PARAM_INIT(bottomHeight, 0.0f)
    , PARAM_INIT(centerZ, 0.0f)
    , PARAM_INIT(aboidKoopaFlameAngle, 0.31415927f)
    , PARAM_INIT(traceMarioAngle, 0.31415927f)
    , PARAM_INIT(mSLWavePhaseVelocity, 0.31415927f)
    , PARAM_INIT(mSLWaveAmplitudeMin, 0.37699112f)
    , PARAM_INIT(mSLWaveAmplitudeMaxLaunch, 0.37699112f)
    , PARAM_INIT(mSLWaveAmplitudeMax, 0.37699112f)
    , PARAM_INIT(mSLSwingPhaseVelocity, 0.31415927f)
    , PARAM_INIT(mSLSwingAmplitudeMin, 0.37699112f)
    , PARAM_INIT(mSLSwingAmplitudeMax, 0.37699112f)
    , PARAM_INIT(mSLRoundAngleVelocity, 0.05f)
    , PARAM_INIT(mSLRoundDistance, 1.0f)
    , PARAM_INIT(mSLAcceleration, 1.0f)
    , PARAM_INIT(mSLRotationSpeed, 1.0f)
    , PARAM_INIT(mSLSpeedMax, 8.0f)
    , PARAM_INIT(mSLKoopaJrSubmarineScale, 1.6f)
    , PARAM_INIT(mSLDamageRadius, 1000.0f)
    , PARAM_INIT(mSLDamageHeight, 4000.0f)
    , PARAM_INIT(shineKillerProbability0, 0.0f)
    , PARAM_INIT(shineKillerProbability1, 0.0f)
    , PARAM_INIT(mSLKillerIntervalFast, 30)
    , PARAM_INIT(mSLKillerInterval, 30)
{
	TParams::load(mPrmPath);

	killerTargetDistanceMin.set(500.0f);
	killerTargetDistance.set(700.0f);
	bottomHeight.set(0.0f);
	centerZ.set(200.0f);
	aboidKoopaFlameAngle.set(0.62831855f);
	traceMarioAngle.set(0.31415927f);
	mSLWavePhaseVelocity.set(0.09424778f);
	mSLWaveAmplitudeMin.set(0.12566371f);
	mSLWaveAmplitudeMaxLaunch.set(0.37699112f);
	mSLWaveAmplitudeMax.set(0.31415927f);
	mSLSwingPhaseVelocity.set(0.18849556f);
	mSLSwingAmplitudeMin.set(0.12566371f);
	mSLSwingAmplitudeMax.set(0.5654867f);
	mSLRoundAngleVelocity.set(0.12f);
	mSLRoundDistance.set(2000.0f);
	mSLAcceleration.set(1.0f);
	mSLRotationSpeed.set(1.0f);
	mSLSpeedMax.set(5.0f);
	mSLKoopaJrSubmarineScale.set(2.0f);
	mSLDamageRadius.set(240.0f);
	mSLDamageHeight.set(120.0f);
	shineKillerProbability0.set(0.5f);
	shineKillerProbability1.set(0.125f);
	mSLKillerIntervalFast.set(30);
	mSLKillerInterval.set(90);
}

TKoopaJrManager::TKoopaJrManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TKoopaJrManager::createEnemyInstance() { return nullptr; }

void TKoopaJrManager::loadAfter()
{
	const char* filename = "/scene/koopajr/jpa/ms_koopajr_killer.jpa";
	if (!gParticleFlagLoaded[0xef]) {
		gpResourceManager->load(filename, 0xef);
		gParticleFlagLoaded[0xef] = true;
	}
}

void TKoopaJrManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TKoopaJrParams("/enemy/koopajr.prm");
}

void TKoopaJrManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "koopajr_model.bmd", 0x54220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TKoopaJrSubmarineManager::TKoopaJrSubmarineManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TKoopaJrSubmarineManager::createEnemyInstance()
{
	return new TKoopaJrSubmarine("クッパジュニアサブマリン");
}

void TKoopaJrSubmarineManager::loadAfter()
{
	JDrama::TNameRef::loadAfter();
}

void TKoopaJrSubmarineManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TKoopaJrSubmarineParams("/enemy/koopajrsubmarine.prm");
}

void TKoopaJrSubmarineManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "LastKoopaJrSubmarine.bmd", 0x54220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TKoopaJr::TKoopaJr(const char* name)
    : TSpineEnemy(name)
    , unk15C(nullptr)
    , unk160(nullptr)
    , unk164(nullptr)
    , unk168(nullptr)
    , unk16C(nullptr)
{
	mLiveFlag |= 0x10;
	mLiveFlag &= ~0x100;
}

const char** TKoopaJr::getBasNameTable() const { return koopajr_bastable; }

void TKoopaJr::reset()
{
	TSpineEnemy::reset();
	mSpine->reset();
	unk150 = 0;
	unk154 = 0;
	unk158 = 0;
	unk154 = getSaveParam2()->mSLLaunchKillerPeriod.get();
	unk158 = 0;
}

void TKoopaJr::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("koopajr_model.bmd", 0);
	mMActor->setLightType(1);

	initAnmSound();

	f32 damageHeight = getSaveParam2()->mSLDamageHeight.get();
	f32 damageRadius = getSaveParam2()->mSLDamageRadius.get();
	initHitActor(0x8000028, 1, 0, 0.0f, 0.0f, damageRadius,
	             damageHeight);
	offHitFlag(1);

	mSpine->initWith(&TNerveKoopaJrWait::theNerve());

	const char* killerManagerName = "バスタブキラーマネージャー";
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	unk16C = (TEnemyManager*)root->searchF(
	    JDrama::TNameRef::calcKeyCode(killerManagerName), killerManagerName);

	if (!unk168) {
		const char* submarineManagerName
		    = "クッパジュニアサブマリンマネージャー";
		root = JDrama::TNameRefGen::getInstance()->getRootNameRef();
		unk168 = (TEnemyManager*)root->searchF(
		    JDrama::TNameRef::calcKeyCode(submarineManagerName),
		    submarineManagerName);
	}

	f32 scale = getSaveParam2()->mSLKoopaJrScale.get();
	mScaling.x = scale;
	mScaling.y = scale;
	mScaling.z = scale;

	mSpine->reset();
	unk150 = 0;
	unk154 = 0;
	unk158 = 0;
	unk154 = getSaveParam2()->mSLLaunchKillerPeriod.get();
	unk158 = 0;
}

void TKoopaJr::calcRootMatrix()
{
	J3DModel* model = getModel();
	if (unk15C->getUnk29A()) {
		MtxPtr mtx       = unk15C->getKoopaJrMtxInDemo();
		J3DModel* model2 = getModel();
		PSMTXCopy(mtx, model2->getBaseTRMtx());
	} else {
		unk164->getJointTransByIndex(TKoopaJr_jointIndexTable[0], &mPosition);
		MsMtxSetXYZRPH(model->getBaseTRMtx(), mPosition.x, mPosition.y,
		                mPosition.z, mRotation.x, mRotation.y, mRotation.z);
	}
	model->setBaseScale(mScaling);
}

void TKoopaJr::checkNerveKillerLaunchFast()
{
	if (unk158 > 0)
		return;

	int num = unk15C->getNumKillerBurstable();
	if (num == 0)
		return;
	TKoopaJrSubmarine* submarine = unk164;
	if (num > 8)
		num = 8;

	submarine->unk180 = 0;
	submarine->unk184 = num;
	for (int i = 0; i < submarine->unk184; ++i)
		submarine->unk178[i] = 2;

	if (submarine->appearShineKiller(submarine->unk184))
		submarine->unk178[submarine->unk184 - 1] = 1;

	mSpine->pushNerve(&TNerveKoopaJrLaunch::theNerve());
	submarine->mSpine->pushNerve(
	    &TNerveKoopaJrSubmarineCannonOpenClose::theNerve());
}

void TKoopaJr::checkNerveKillerLaunchNormal()
{
	if (unk154 > 0)
		return;

	int num = unk15C->getNumKillerLaunchable();
	if (num == 0)
		return;
	TKoopaJrSubmarine* submarine = unk164;
	if (num > 8)
		num = 8;

	submarine->unk180 = 0;
	submarine->unk184 = num;
	for (int i = 0; i < submarine->unk184; ++i)
		submarine->unk178[i] = 0;

	if (submarine->appearShineKiller(submarine->unk184))
		submarine->unk178[submarine->unk184 - 1] = 1;

	mSpine->pushNerve(&TNerveKoopaJrLaunch::theNerve());
	submarine->mSpine->pushNerve(
	    &TNerveKoopaJrSubmarineCannonOpenClose::theNerve());
}

TKoopaJrSubmarine::TKoopaJrSubmarine(const char* name)
    : TSpineEnemy(name)
    , unk164(0.0f)
    , unk16C(0.0f)
    , unk188(0.0f)
    , unk1A0(nullptr)
{
	mLiveFlag &= ~0x10;
	mLiveFlag &= ~0x100;
}

const char** TKoopaJrSubmarine::getBasNameTable() const
{
	return &koopajrsubmarine_bastable;
}

void TKoopaJrSubmarine::resetKoopaJrSubmarine()
{
	mSpine->reset();
	unk150 = 0;
	mMActor->setBckFromIndex(0);

	const char** table = getBasNameTable();
	setAnmSound(!table ? nullptr : table[0]);

	unk188 = mMActor->getFrameCtrl(0)->getRate();
	unk180 = 0;
	unk184 = 0;
	for (int i = 0; i < 8; ++i)
		unk178[i] = 0;

	unk154 = 0.0f;
	unk158 = 0.0f;
	unk15C = 0.0f;
	unk160 = 1.0f;
	unk16C = 0.0f;
	unk164 = 0.0f;
	unk170 = 0;
	unk18C = 0;
	unk190 = 0.0f;
	unk194 = 0.0f;
	unk198 = 0.0f;
	unk19C = 0.0f;

	if (unk174)
		unk174->init(100.0f, 3.1415927f, 100.0f, 3.1415927f,
		             getSaveParam2()->bottomHeight.get());
}

void TKoopaJrSubmarine::reset()
{
	TSpineEnemy::reset();
	resetKoopaJrSubmarine();
}

BOOL TCallbackHitActor::receiveMessage(THitActor* sender, u32 message)
{
	return unk68->receiveMessage(sender, message);
}
