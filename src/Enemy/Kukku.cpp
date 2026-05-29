#include <Enemy/Kukku.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <Map/PollutionManager.hpp>
#include <System/Particles.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static char* tori_bastable[] = {
	(char*)"/scene/tori/bas/tori_back.bas",
	(char*)"/scene/tori/bas/tori_down.bas",
	(char*)0,
	(char*)"/scene/tori/bas/tori_hit.bas",
	(char*)"/scene/tori/bas/tori_wait.bas",
	(char*)"/scene/tori/bas/tori_fall_end.bas",
};

DEFINE_NERVE(TNerveKukkuFall, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("tori_wait");
		self->setCurAnmSound();
		J3DFrameCtrl* fc = self->getMActor()->getFrameCtrl(0);
		fc->setRate(2.0f * SMSGetAnmFrameRate());

		JGeometry::TVec3<f32> vel(0.0f, -self->getSaveParam2()->mWaterPowerY.get(),
		                          0.0f);
		self->mVelocity = vel;
		self->dropCoins();
	}

	if (!self->checkLiveFlag(LIVE_FLAG_AIRBORNE)) {
		JGeometry::TVec3<f32> scale(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    (E_SMS_EFFECT_ONETIME_NORMAL)0xA1, &self->mPosition, nullptr, scale);
		JGeometry::TVec3<f32> scale2(1.0f, 1.0f, 1.0f);
		SMS_EasyEmitParticle<E_SMS_EFFECT_ONETIME_NORMAL>(
		    (E_SMS_EFFECT_ONETIME_NORMAL)0xA2, &self->mPosition, nullptr, scale2);
		spine->pushAfterCurrent(&TNerveSmallEnemyDie::theNerve());
		return TRUE;
	}

	JGeometry::TVec3<f32> vel = self->mVelocity;
	f32 fric                  = self->getSaveParam2()->mAirFric.get();
	vel.x *= fric;
	vel.y *= fric;
	vel.z *= fric;
	vel.y += self->getSaveParam2()->mUpperVelocityY.get();

	bool landed = false;
	if (-0.1f < vel.y) {
		vel.y  = 0.0f;
		landed = true;
	}
	self->mVelocity = vel;

	if (landed) {
		spine->pushAfterCurrent(&TNerveKukkuRecoverGraph::theNerve());
		return TRUE;
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKukkuPostFall, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("tori_back");
		self->setCurAnmSound();
		JGeometry::TVec3<f32> vel(0.0f, 0.0f, 0.0f);
		self->mVelocity = vel;
	}

	if (self->checkCurAnmEnd(0)) {
		if (self->getSaveParam2()->mHabatakiTimer.get() < spine->getTime()) {
			spine->pushAfterCurrent(&TNerveKukkuGraphWander::theNerve());
			return TRUE;
		}
	}
	return FALSE;
}

DEFINE_NERVE(TNerveKukkuRecoverGraph, TLiveActor)
{
	TKukku* self = (TKukku*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("tori_back");
		self->setCurAnmSound();
		JGeometry::TVec3<f32> vel(0.0f, 0.0f, 0.0f);
		self->mVelocity = vel;
	}

	if (self->getSaveParam2()->mHabatakiTimer.get() < spine->getTime()) {
		spine->pushAfterCurrent(&TNerveKukkuGraphWander::theNerve());
		return TRUE;
	}

	self->updateRotation();
	self->mLinearVelocity
	    = self->calcMomentum(self->getSaveParam2()->mMarchSpeed.get());
	return FALSE;
}

DEFINE_NERVE(TNerveKukkuGraphWander, TLiveActor) { return FALSE; }

void TKukkuManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
	    { "tori.bmd", 0x10210000, 0 },
	    { nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TKukkuParams::TKukkuParams(const char* path)
    : TSmallEnemyParams(path)
    , PARAM_INIT(mMarchSpeed, 3.0f)
    , PARAM_INIT(mTurnSpeed, 0.2f)
    , PARAM_INIT(mWaterPowerY, 12.0f)
    , PARAM_INIT(mShootSpeed, 1.0f)
    , PARAM_INIT(mShootInterval, 60)
    , PARAM_INIT(mSearchRange, 800.0f)
    , PARAM_INIT(mHabatakiTimer, 45)
    , PARAM_INIT(mAirFric, 0.97f)
    , PARAM_INIT(mUpperVelocityY, 0.0f)
    , PARAM_INIT(mDropSpeed, 5.0f)
    , PARAM_INIT(mDropAngleX, -0.25f)
{
	TParams::load(mPrmPath);
}

void TKukkuManager::load(JSUMemoryInputStream& stream)
{
	TKukkuParams* params = new TKukkuParams("/enemy/kukku.prm");
	unk38                = params;
	params->mSLAttackRadius.set(30);
	params->mSLAttackHeight.set(30);
	params->mSLDamageRadius.set(100);
	params->mSLDamageHeight.set(100);
	params->mSLBodyRadius.set(5.0f);
	TSmallEnemyManager::load(stream);
}

TKukkuManager::TKukkuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

const char** TKukku::getBasNameTable() const
{
	return (const char**)tori_bastable;
}

void TKukku::setDeadAnm()
{
	getMActor()->setBck("tori_down");
	setCurAnmSound();
}

void TKukku::setAfterDeadEffect()
{
	TSmallEnemy::setAfterDeadEffect();
	gpPollution->stamp(((TSmallEnemyManager*)mManager)->getUnk58(), mPosition.x,
	                   mPosition.y, mPosition.z, 1000.0f);
}

void TKukku::bind() { TLiveActor::bind(); }

void TKukku::control()
{
	if (getUnk1A4() > 0)
		unk1A4 -= 1;
	TLiveActor::control();
}

void TKukku::reset()
{
	unk1A4 = 0;
	unk1AC = 0;
	mGravity = 0.0f;
	unk1B0 = 0;
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	mScaledBodyRadius = 75.0f;
}

BOOL TKukku::receiveMessage(THitActor* sender, u32 message)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return FALSE;

	if ((s32)message < 2 && (s32)message >= 0) {
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		return TRUE;
	}
	return TSmallEnemy::receiveMessage(sender, message);
}

void TKukku::perform(u32 action, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(action, graphics);

	for (void** ball = &unk194[0]; ball != &unk1A0; ++ball) {
		THitActor* a = (THitActor*)*ball;
		a->perform(action, graphics);
	}
}

TKukku::TKukku(const char* name)
    : TSmallEnemy(name)
{
	unk1A0 = nullptr;
}
