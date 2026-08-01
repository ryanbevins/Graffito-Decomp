#define JGEOMETRY_ROTATION3_SETQUAT_OUT_OF_LINE
#include <Enemy/LimitKoopaJr.hpp>
#undef JGEOMETRY_ROTATION3_SETQUAT_OUT_OF_LINE
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static const f32 dummy2659[3] = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2661[3] = { 1.0f, 1.0f, 1.0f };

static const char* koopajr_bastable[] = {
	"/scene/koopajr/bas/koopajr_damage.bas",
	"/scene/koopajr/bas/koopajr_shoot.bas",
	nullptr,
	"/scene/koopajr/bas/koopajr_yahoo.bas",
};

DEFINE_NERVE(TNerveLimitKoopaJrYahoo, TLiveActor)
{
	TLimitKoopaJr* self = (TLimitKoopaJr*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBckFromIndex(3);
		const char** table = self->getBasNameTable();
		self->setAnmSound(!table ? nullptr : table[3]);
	}

	if (self->mMActor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveLimitKoopaJrWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaJrLaunch, TLiveActor)
{
	TLimitKoopaJr* self = (TLimitKoopaJr*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBckFromIndex(1);
		const char** table = self->getBasNameTable();
		self->setAnmSound(!table ? nullptr : table[1]);
		self->mShotDoodleTimer = self->getSaveParam2()->mSLShotDoodlePeriod.get();
	}

	if (self->mMActor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveLimitKoopaJrWait::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaJrWait, TLiveActor)
{
	TLimitKoopaJr* self = (TLimitKoopaJr*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBckFromIndex(2);
		const char** table = self->getBasNameTable();
		self->setAnmSound(!table ? nullptr : table[2]);
	}

	{
		TDirectionCalc tdc;
		JGeometry::TVec3<f32>& tp = self->mTargetActor->mPosition;
		JGeometry::TVec3<f32> diff;
		diff.x = tp.x - gpMarioPos->x;
		diff.y = tp.y - gpMarioPos->y;
		diff.z = tp.z - gpMarioPos->z;
		diff.y = 0.0f;
		tdc.makeDirection(diff);

		f32 absDir = self->mDirection1.absDirection(tdc.mDirection);
		bool turned;
		if (absDir <= 0.62831855f)
			turned = false;
		else
			turned = true;
		if (turned) {
			spine->pushAfterCurrent(&TNerveLimitKoopaJrRun::theNerve());
			return TRUE;
		}
	}

	if (SMS_IsMarioStatusTypeJumping()) {
		spine->pushAfterCurrent(&TNerveLimitKoopaJrYahoo::theNerve());
		return TRUE;
	}

	if (self->mShotDoodleTimer <= 0) {
		spine->pushAfterCurrent(&TNerveLimitKoopaJrLaunch::theNerve());
		return TRUE;
	}

	{
		JGeometry::TVec3<f32> diff;
		diff.x = gpMarioPos->x - self->mPosition.x;
		diff.y = gpMarioPos->y - self->mPosition.y;
		diff.z = gpMarioPos->z - self->mPosition.z;
		diff.y = 0.0f;
		diff.setLength(diff, 1.0f);

		TDirectionCalc tdc(diff);
		f32 turn = TDirectionCalc::d2r(
		    self->getSaveParam2()->mSLRotationSpeed.get());
		self->mDirection2.mDirection
		    = self->mDirection2.calcTurnDirection(tdc.mDirection, turn);
	}

	return FALSE;
}

DEFINE_NERVE(TNerveLimitKoopaJrRun, TLiveActor)
{
	TLimitKoopaJr* self = (TLimitKoopaJr*)spine->getBody();

	if (spine->getTime() == 0) {
		self->mMActor->setBckFromIndex(2);
		const char** table = self->getBasNameTable();
		self->setAnmSound(!table ? nullptr : table[2]);
	}

	{
		TDirectionCalc tdc;
		JGeometry::TVec3<f32>& tp = self->mTargetActor->mPosition;
		JGeometry::TVec3<f32> diff;
		diff.x = tp.x - gpMarioPos->x;
		diff.y = tp.y - gpMarioPos->y;
		diff.z = tp.z - gpMarioPos->z;
		diff.y = 0.0f;
		tdc.makeDirection(diff);

		f32 targetDirection = tdc.mDirection;
		f32 absDir = self->mDirection1.absDirection(targetDirection);
		bool turned;
		if (absDir <= 0.62831855f)
			turned = false;
		else
			turned = true;
		if (!turned) {
			spine->pushAfterCurrent(&TNerveLimitKoopaJrWait::theNerve());
			return TRUE;
		}
	}

	self->moveRun();
	return FALSE;
}

void TLimitKoopaJr::moveRun()
{
	f32 rotSpeedRad
	    = 0.017453294f * getSaveParam2()->mSLRoundAngleVelocity.get();

	TDirectionCalc tdc;
	JGeometry::TVec3<f32>& tp = mTargetActor->mPosition;
	JGeometry::TVec3<f32> diff;
	diff.x = tp.x - gpMarioPos->x;
	diff.y = tp.y - gpMarioPos->y;
	diff.z = tp.z - gpMarioPos->z;
	diff.y = 0.0f;
	tdc.makeDirection(diff);

	f32 direction
	    = mDirection1.calcTurnDirection(tdc.mDirection, rotSpeedRad);
	f32 turnVal = TDirectionCalc(direction).sub(mDirection1.mDirection);
	mDirection1.mDirection = direction;

	mRoundRadius = getSaveParam2()->mSLRoundRadius.get();
	JGeometry::TVec3<f32> dirVec = mDirection1.calcDirectionVector();
	dirVec.scale(mRoundRadius);

	mPosition.x = tp.x + dirVec.x;
	mPosition.y = tp.y + dirVec.y;
	mPosition.z = tp.z + dirVec.z;
	mPosition.y = getSaveParam2()->mSLRoundHeight.get();

	// Normalize the direction vector (with degenerate-case fallback)
	f32 sq = dirVec.x * dirVec.x + dirVec.y * dirVec.y + dirVec.z * dirVec.z;
	if (sq <= 0.0000038146973f) {
		dirVec.x = 0.0f;
		dirVec.y = 0.0f;
		dirVec.z = 0.0f;
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(sq);
		dirVec.x *= inv;
		dirVec.y *= inv;
		dirVec.z *= inv;
	}

	// Cross with up vector to get perpendicular
	JGeometry::TVec3<f32> cross;
	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	cross.cross(up, dirVec);

	f32 csq = cross.x * cross.x + cross.y * cross.y + cross.z * cross.z;
	if (csq <= 0.0000038146973f) {
		cross.x = 0.0f;
		cross.y = 0.0f;
		cross.z = 0.0f;
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(csq);
		cross.x *= inv;
		cross.y *= inv;
		cross.z *= inv;
	}

	if (turnVal < 0.0f) {
		cross.x = -cross.x;
		cross.y = -cross.y;
		cross.z = -cross.z;
	}

	// dot-and-scale via setLength
	JGeometry::TVec3<f32> tmp = cross;
	f32 dotProd = tmp.dot(tmp);
	if (dotProd <= 0.0000038146973f) {
		tmp.x = 0.0f;
		tmp.y = 0.0f;
		tmp.z = 0.0f;
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(dotProd);
		tmp.scale(1.0f * inv, tmp);
	}

	TDirectionCalc tdc2(tmp);
	f32 turnRot = TDirectionCalc::d2r(getSaveParam2()->mSLRotationSpeed.get());
	mDirection2.mDirection
	    = mDirection2.calcTurnDirection(tdc2.mDirection, turnRot);
}

BOOL TLimitKoopaJr::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 0xF)
		return TRUE;
	return FALSE;
}

const char** TLimitKoopaJr::getBasNameTable() const { return koopajr_bastable; }

void TLimitKoopaJr::calcRootMatrix()
{
	f32 half = 0.5f * mDirection2.mDirection;
	f32 s    = sinf(half);
	f32 c    = cosf(half);

	Mtx tmp;
	JGeometry::TQuat4<f32> q;
	q.x = 0.0f;
	q.y = s;
	q.z = 0.0f;
	q.w = c;
	((JGeometry::TRotation3<JGeometry::TMatrix34<JGeometry::SMatrix34C<f32> > >*)
	    &tmp)
	    ->setQuat(q);

	MtxPtr transformMtx = tmp;
	transformMtx[0][3]  = mPosition.x;
	transformMtx[1][3]  = mPosition.y;
	transformMtx[2][3]  = mPosition.z;

	PSMTXCopy(transformMtx, (MtxPtr)((u8*)getModel() + 0x20));

	getModel()->setBaseScale(mScaling);
}

void TLimitKoopaJr::bind() { }

void TLimitKoopaJr::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mKoopaMgrRef == nullptr) {
		JDrama::TNameRef* root
		    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
		JDrama::TNameRef* mgr = root->searchF(
		    JDrama::TNameRef::calcKeyCode("クッパマネージャー"),
		    "クッパマネージャー");
		mKoopaMgrRef = (*(TLiveActor***)((u8*)mgr + 0x18))[0];
	}

	if (mTargetActor == nullptr) {
		JDrama::TNameRef* root
		    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
		mTargetActor = (TLiveActor*)root->searchF(
		    JDrama::TNameRef::calcKeyCode("バスタブ"), "バスタブ");
	}

	if (flags & 1) {
		if (mDamageTimer > 0)
			mDamageTimer -= 1;
		if (mShotDoodleTimer > 0)
			mShotDoodleTimer -= 1;

		if (mSpine->getCurrentNerve()
		    != &TNerveLimitKoopaJrWait::theNerve()) {
			(void)&TNerveLimitKoopaJrRun::theNerve();
		}
	}

	TSpineEnemy::perform(flags, graphics);
}

void TLimitKoopaJr::reset()
{
	TSpineEnemy::reset();
	mSpine->reset();
	mDamageTimer     = 0;
	mShotDoodleTimer = 0;
	mShotDoodleTimer = getSaveParam2()->mSLShotDoodlePeriod.get();
	mShotVelocity.x  = 0.0f;
	mShotVelocity.y  = 0.0f;
	mShotVelocity.z  = 0.0f;
	unk16C           = 1.0f;
	mDirection2.mDirection = 0.0f;
	mDirection1.mDirection = 0.0f;
}

void TLimitKoopaJr::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("koopajr_model.bmd", 0);
	mMActor->setLightType(1);

	initAnmSound();
	f32 dmgHeight  = getSaveParam2()->mSLDamageHeight.get();
	f32 dmgRadius  = getSaveParam2()->mSLDamageRadius.get();
	initHitActor(0x800002E, 1, 0, 0.0f, 0.0f, dmgRadius, dmgHeight);
	offHitFlag(0x1);

	mSpine->initWith(&TNerveLimitKoopaJrRun::theNerve());

	f32 scale = getSaveParam2()->mSLKoopaJrScale.get();
	mScaling.x = scale;
	mScaling.y = scale;
	mScaling.z = scale;
	mSpine->reset();
	mDamageTimer     = 0;
	mShotDoodleTimer = 0;
	mShotDoodleTimer = getSaveParam2()->mSLShotDoodlePeriod.get();

	mShotVelocity.x = 0.0f;
	mShotVelocity.y = 0.0f;
	mShotVelocity.z = 0.0f;
	unk16C          = 1.0f;
	mDirection2.mDirection = 0.0f;
	mDirection1.mDirection = 0.0f;

	J3DModelData* md = getModel()->getModelData();
	for (u16 i = 0; i < md->getShapeNum(); ++i) {
		md->getShapeNodePointer(i)->unk8 |= 1;
	}
}

TLimitKoopaJr::TLimitKoopaJr(const char* name)
    : TSpineEnemy(name)
    , mTargetActor(nullptr)
{
	mLiveFlag |= 0x10;
	mLiveFlag &= ~0x100;
}

TLimitKoopaJrParams::TLimitKoopaJrParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLAcceleration, 1.0f)
    , PARAM_INIT(mSLRotationSpeed, 1.0f)
    , PARAM_INIT(mSLSpeedMax, 8.0f)
    , PARAM_INIT(mSLRoundAngleVelocity, 0.05f)
    , PARAM_INIT(mSLRoundRadius, 2000.0f)
    , PARAM_INIT(mSLRoundHeight, 0.0f)
    , PARAM_INIT(mSLDamageRadius, 1000.0f)
    , PARAM_INIT(mSLDamageHeight, 4000.0f)
    , PARAM_INIT(mSLKoopaJrScale, 1.6f)
    , PARAM_INIT(mSLShotDoodlePeriod, 1200)
    , PARAM_INIT(mSLDamagePeriod, 360)
{
	TParams::load(mPrmPath);

	mSLAcceleration.set(1.0f);
	mSLRotationSpeed.set(1.0f);
	mSLSpeedMax.set(15.0f);
	mSLRoundAngleVelocity.set(0.08f);
	mSLRoundRadius.set(3300.0f);
	mSLRoundHeight.set(5400.0f);
	mSLDamageRadius.set(100.0f);
	mSLDamageHeight.set(300.0f);
	mSLKoopaJrScale.set(2.0f);
	mSLDamagePeriod.set(120);
	mSLShotDoodlePeriod.set(600);
}

TLimitKoopaJrManager::TLimitKoopaJrManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TLimitKoopaJrManager::createEnemyInstance() { return nullptr; }

void TLimitKoopaJrManager::loadAfter() { }

void TLimitKoopaJrManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TLimitKoopaJrParams("/enemy/limitkoopajr.prm");
}

void TLimitKoopaJrManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "koopajr_model.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}
