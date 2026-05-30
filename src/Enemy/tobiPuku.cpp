#include <Enemy/TobiPuku.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>
#include <math.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static const char* pukupuku_bastable[] = {
	nullptr,
	"/scene/pukupuku/bas/pukupuku_death.bas",
	"/scene/pukupuku/bas/pukupuku_down_air.bas",
	"/scene/pukupuku/bas/pukupuku_down_land.bas",
	nullptr,
	"/scene/pukupuku/bas/pukupuku_fall_end_land.bas",
	nullptr,
	nullptr,
	"/scene/pukupuku/bas/pukupuku_pitipiti.bas",
	"/scene/pukupuku/bas/pukupuku_swim.bas",
	nullptr,
};

static const char* moepuku_bastable[] = {
	nullptr,
	nullptr,
	"/scene/moepuku/bas/moepuku_down_air.bas",
	"/scene/moepuku/bas/moepuku_down_land.bas",
	nullptr,
	"/scene/moepuku/bas/moepuku_fall_end_land.bas",
	nullptr,
	nullptr,
	"/scene/moepuku/bas/moepuku_pitipiti.bas",
	nullptr,
	nullptr,
};

static TTobiPuku* gpCurTobiPuku;

f32 TTobiPuku::mLandAngle      = 90.0f;
u8 TTobiPuku::mBoundSw         = 1;
f32 TTobiPuku::mBoundVelocityY = 0.8f;
u8 TTobiPuku::mReturnLaunchSw  = 1;

static inline TTobiPuku* tobiPuku(TSpineBase<TLiveActor>* spine)
{
	return (TTobiPuku*)spine->getBody();
}

BOOL TNerveTobiPukuSwimWander::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);

	if (spine->getTime() == 0) {
		self->setSwimAnm();
		self->goToShortestNextGraphNode();
	}

	self->walkBehavior(0, self->getMarchSpeed());
	self->swimEffect();
	if (self->isReachedToGoalXZ()) {
		self->goToShortestNextGraphNode();
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuReturnLaunch::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);

	if (spine->getTime() == 0) {
		self->setFallAnm();
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	if (self->mLaunchPad) {
		JGeometry::TVec3<f32> toPad;
		toPad.sub(self->mLaunchPad->mPosition, self->mPosition);
		if (!toPad.isZero())
			toPad.setLength(self->getTobiPukuParams()->mSLPowerFromWater.get());
		self->mVelocity.add(toPad);
	}

	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuGenerate::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuPrepareFly::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setJumpStartAnm();

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTobiPukuFly::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuBound::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->setJumpAnm();
		self->mVelocity.y = TTobiPuku::mBoundVelocityY;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuLand::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuLand::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setDownLandAnm();

	if (self->checkCurAnmEnd(0)) {
		if (TTobiPuku::mReturnLaunchSw && self->mLaunchPad)
			spine->pushAfterCurrent(&TNerveTobiPukuReturnLaunch::theNerve());
		else
			spine->pushAfterCurrent(&TNerveTobiPukuSwimWander::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuDie::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setDeadAnm();

	if (self->checkCurAnmEnd(0)) {
		self->kill();
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuPitiPiti::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setPichiAnm();

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTobiPukuFall::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuFall::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->setFallAnm();
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuLand::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuHitWater::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->hitWater();
		self->setDownAirAnm();
	}

	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuLand::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuAttack::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setAttackAnm();

	self->attackToMario();
	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTobiPukuSwimWander::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuFly::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0)
		self->setJumpAnm();

	self->mVelocity = self->mLaunchVelocity;
	if (!self->isAirborne()) {
		spine->pushAfterCurrent(&TNerveTobiPukuLand::theNerve());
		return TRUE;
	}

	return FALSE;
}

BOOL TNerveTobiPukuGenerate::execute(TSpineBase<TLiveActor>* spine) const
{
	TTobiPuku* self = tobiPuku(spine);
	if (spine->getTime() == 0) {
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->setJumpStartAnm();
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveTobiPukuFly::theNerve());
		return TRUE;
	}

	return FALSE;
}

const char** TMoePuku::getBasNameTable() const { return moepuku_bastable; }

void TMoePuku::generateEffectColumWater()
{
	TTobiPuku::generateEffectColumWater();
}

void TMoePuku::setJumpStartAnm() { TTobiPuku::setJumpStartAnm(); }
void TMoePuku::setFallEndLandAnm() { setBckAnm(5); }
void TMoePuku::setDeadAnm() { setBckAnm(1); }
void TMoePuku::setDownLandAnm() { setBckAnm(3); }
void TMoePuku::setDownAirAnm() { setBckAnm(2); }
void TMoePuku::setFallAnm() { setBckAnm(4); }
void TMoePuku::setPichiAnm() { setBckAnm(8); }
void TMoePuku::setAttackAnm() { setBckAnm(0); }
void TMoePuku::setSwimAnm() { setBckAnm(9); }
void TMoePuku::setJumpAnm() { setBckAnm(6); }
bool TMoePuku::isJumpStartBck() { return isBckAnm(7); }
bool TMoePuku::isFallEndLandBck() { return isBckAnm(5); }
bool TMoePuku::isAttackBck() { return isBckAnm(0); }
bool TMoePuku::isDeadBck() { return isBckAnm(1); }
bool TMoePuku::isJumpBck() { return isBckAnm(6); }
bool TMoePuku::isPichiEffect() { return isBckAnm(8); }

void TMoePuku::hitWater()
{
	TTobiPuku::hitWater();
}

void TMoePuku::calcRootMatrix()
{
	TTobiPuku::calcRootMatrix();
}

TPukuPuku::TPukuPuku(const char* name)
    : TTobiPuku(name)
{
}

void TPukuPuku::reset()
{
	TTobiPuku::reset();
	if (mSpine)
		mSpine->initWith(&TNerveTobiPukuSwimWander::theNerve());
}

void TPukuPuku::init(TLiveManager* manager)
{
	TTobiPuku::init(manager);
	if (mSpine)
		mSpine->initWith(&TNerveTobiPukuSwimWander::theNerve());
	gpCurTobiPuku = nullptr;
}

void TPukuPuku::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	unk1AC = 0;
}

const char** TTobiPuku::getBasNameTable() const { return pukupuku_bastable; }

void TTobiPuku::scalingChangeActor()
{
	TSmallEnemy::scalingChangeActor();
}

void TTobiPuku::initAttacker(THitActor* actor)
{
	TWalkerEnemy::initAttacker(actor);
}

void TTobiPuku::changeOut()
{
	TSmallEnemy::changeOut();
}

void TTobiPuku::genEventCoin()
{
	TSmallEnemy::genEventCoin();
}

void TTobiPuku::forceKill()
{
	kill();
}

void TTobiPuku::kill()
{
	onLiveFlag(LIVE_FLAG_DEAD);
	TSmallEnemy::kill();
}

void TTobiPuku::hitWater()
{
	unk194 = 1;
	mVelocity.y += getTobiPukuParams()->mSLPowerFromWater.get();
	onLiveFlag(LIVE_FLAG_AIRBORNE);
}

f32 TTobiPuku::getGravityY() const
{
	if (unk194)
		return 0.0f;
	return mTobiPukuParams->mSLFlyGravityY.get();
}

void TTobiPuku::attackToMario()
{
	TSmallEnemy::attackToMario();
}

void TTobiPuku::generateEffectColumWater()
{
	TSmallEnemy::generateEffectColumWater();
}

bool TTobiPuku::isReachedToGoalXZ()
{
	return TWalkerEnemy::isReachedToGoalXZ();
}

void TTobiPuku::swimEffect()
{
}

void TTobiPuku::walkBehavior(int walk_state, float speed)
{
	TWalkerEnemy::walkBehavior(walk_state, speed);
}

void TTobiPuku::behaveToWater(THitActor* actor)
{
	TSmallEnemy::behaveToWater(actor);
	if (mSpine)
		mSpine->pushAfterCurrent(&TNerveTobiPukuHitWater::theNerve());
}

void TTobiPuku::setJumpStartAnm()
{
	if (isJumpStartBck())
		setBckAnm(7);
}

void TTobiPuku::setFallEndLandAnm() { setBckAnm(5); }
void TTobiPuku::setDeadAnm() { setBckAnm(1); }
void TTobiPuku::setDownLandAnm() { setBckAnm(3); }
void TTobiPuku::setDownAirAnm() { setBckAnm(2); }
void TTobiPuku::setFallAnm() { setBckAnm(4); }
void TTobiPuku::setPichiAnm() { setBckAnm(8); }
void TTobiPuku::setAttackAnm() { setBckAnm(0); }
void TTobiPuku::setSwimAnm() { setBckAnm(9); }
void TTobiPuku::setJumpAnm() { setBckAnm(6); }
bool TTobiPuku::isJumpStartBck() { return isBckAnm(7); }
bool TTobiPuku::isFallEndLandBck() { return isBckAnm(5); }
bool TTobiPuku::isAttackBck() { return isBckAnm(0); }
bool TTobiPuku::isDeadBck() { return isBckAnm(1); }
bool TTobiPuku::isJumpBck() { return isBckAnm(6); }
bool TTobiPuku::isPichiEffect() { return isBckAnm(8); }

void TTobiPuku::calcRootMatrix()
{
	TWalkerEnemy::calcRootMatrix();
}

void TTobiPuku::hitWall()
{
	if (mBoundSw) {
		mVelocity.y = mBoundVelocityY;
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}
}

void TTobiPuku::moveObject()
{
	mTurnSpeed = getTobiPukuParams()->mSLTurnSpeedLow.get();
	if (mBoundSw && checkLiveFlag(LIVE_FLAG_AIRBORNE))
		hitWall();
	TWalkerEnemy::moveObject();
}

void TTobiPuku::reset()
{
	gpCurTobiPuku = this;
	TWalkerEnemy::reset();
	if (mSpine)
		mSpine->initWith(&TNerveTobiPukuGenerate::theNerve());

	unk1AD  = 1;
	unk194  = 0;
	unk1C4  = mPosition;
	unk1B8  = unk1C4;
	unk1E0  = mPosition.y;
}

void TTobiPuku::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType      = 0x10000012;
	unk150          = 0x31;
	mTobiPukuParams = getTobiPukuParams();
	if (getMActor())
		getMActor()->setJointCallback(1, TobiPukuRollCallback);
}

TTobiPuku::TTobiPuku(const char* name)
    : TWalkerEnemy(name)
    , unk194(0)
    , unk198(nullptr)
    , mTobiPukuParams(nullptr)
    , unk1AC(1)
    , unk1AD(1)
    , unk1AE(0)
    , unk1B0(30.0f)
    , unk1B4(30.0f)
    , mLaunchPad(nullptr)
    , unk1E0(30.0f)
    , unk1E4(30.0f)
    , unk1E8(30.0f)
    , unk1EC(30.0f)
{
	gpCurTobiPuku = nullptr;
}

void TMoePukuLaunchPad::launch()
{
	TTobiPuku* puku = nullptr;
	if (gpConductor)
		puku = (TTobiPuku*)gpConductor->makeOneEnemyAppear(
		    mPosition, "モエプクマネージャー", 1);
	if (puku) {
		forceLaunch(puku);
		mLaunchedPuku = puku;
	}
}

void TTobiPukuLaunchPad::forceLaunch(TTobiPuku* puku)
{
	if (!puku)
		return;

	puku->reset();
	puku->mPosition = mPosition;
	puku->mRotation = mRotation;
	puku->mLaunchPad = this;

	f32 angle = mRotation.y * (3.1415927f / 180.0f);
	f32 speed = mLaunchSpeed;
	if (mLaunchParams)
		speed = mLaunchParams->mSLFlySpeed.get();

	JGeometry::TVec3<f32> velocity(sinf(angle) * speed,
	                               mLaunchParams
	                                   ? mLaunchParams->mSLLaunchVelocityY.get()
	                                   : 12.0f,
	                               cosf(angle) * speed);
	puku->mLaunchVelocity = velocity;
	puku->mVelocity       = velocity;
	puku->unk1B0          = mPosition.y;
	puku->unk1B4          = mRotation.y;
	puku->onLiveFlag(LIVE_FLAG_AIRBORNE);
	if (puku->mSpine)
		puku->mSpine->initWith(&TNerveTobiPukuFly::theNerve());
}

void TTobiPukuLaunchPad::launch()
{
	TTobiPuku* puku = nullptr;
	if (gpConductor)
		puku = (TTobiPuku*)gpConductor->makeOneEnemyAppear(
		    mPosition, "とびプクマネージャー", 1);
	if (puku) {
		forceLaunch(puku);
		mLaunchedPuku = puku;
	}
}

void TTobiPukuLaunchPad::reset()
{
	TSmallEnemy::reset();
	mTimer        = 0;
	mLaunchedPuku = nullptr;
}

void TTobiPukuLaunchPad::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	s32 speed = 0;
	stream.read(speed);
	mLaunchSpeed = speed;
}

void TTobiPukuLaunchPad::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType     = 0x10000012;
	mLaunchParams  = (TTobiPukuLaunchPadParams*)getSaveParam();
}

void TTobiPukuLaunchPad::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
	if (!(flags & 1))
		return;
	if (checkLiveFlag(LIVE_FLAG_HIDDEN | LIVE_FLAG_DEAD))
		return;

	if (TTobiPuku::mReturnLaunchSw) {
		if (!mLaunchedPuku || mLaunchedPuku->checkLiveFlag(LIVE_FLAG_DEAD))
			launch();
		return;
	}

	mTimer++;
	if (mLaunchParams && mTimer > mLaunchParams->mSLLaunchInterval.get()) {
		mTimer = 0;
		launch();
	}
}

TTobiPukuLaunchPad::TTobiPukuLaunchPad(const char* name)
    : TSmallEnemy(name)
    , mTimer(0)
    , mLaunchParams(nullptr)
    , mLaunchSpeed(30.0f)
    , mLaunchedPuku(nullptr)
{
}

TSpineEnemy* TMoePukuManager::createEnemyInstance()
{
	return new TMoePuku("モエプク");
}

TSpineEnemy* TTobiPukuManager::createEnemyInstance()
{
	return new TTobiPuku("とびプク");
}

void TTobiPukuManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TTobiPukuParams("/enemy/tobipuku.prm");
	TSmallEnemyManager::load(stream);
}

TTobiPukuManager::TTobiPukuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

TSpineEnemy* TMoePukuLaunchPadManager::createEnemyInstance()
{
	return new TMoePukuLaunchPad("モエプク発射台");
}

void TTobiPukuLaunchPadManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	for (int i = 0; i < getActiveObjNum(); ++i)
		getObj(i)->perform(flags, graphics);
}

TSpineEnemy* TTobiPukuLaunchPadManager::createEnemyInstance()
{
	return new TTobiPukuLaunchPad("とびプク発射台");
}

void TTobiPukuLaunchPadManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TTobiPukuLaunchPadParams("/enemy/tobipukulaunch.prm");
	TSmallEnemyManager::load(stream);
}

TTobiPukuLaunchPadManager::TTobiPukuLaunchPadManager(const char* name)
    : TSmallEnemyManager(name)
    , mForceJumpToPad(0)
{
}

int TobiPukuRollCallback(J3DNode*, int)
{
	return 1;
}

void TMoePuku::swimEffect()
{
}

BOOL TTobiPuku::isInhibitedForceMove()
{
	return checkLiveFlag(LIVE_FLAG_AIRBORNE) ? TRUE : FALSE;
}
