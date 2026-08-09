#include <Enemy/DebuTelesa.hpp>
#include <Enemy/Conductor.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/Camera.hpp>
#include <System/Particles.hpp>
#include <Strategic/Spine.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static const char* DebuTelesa_bastable[] = {
	"/scene/DebuTelesa/bas/debuTelesa_wait.bas",
};

TDebuTelesa::TDebuTelesa(const char* name)
    : TSmallEnemy(name)
{
	onLiveFlag(LIVE_FLAG_UNK10);
}

void TDebuTelesa::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	setMActorAndKeeper();
	mSpine->initWith(&TNerveDebuTelesaWait::theNerve());

	initHitActor(0x10000033, 1, 0x80000000, 10.0f, 10.0f, 10.0f, 10.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	initAnmSound();

	mJntNullYodare
	    = getModel()->getModelData()->getJointName()->getIndex("null_yodare");
	mJntRhand
	    = getModel()->getModelData()->getJointName()->getIndex("jnt_Rhand");
}

void TDebuTelesa::reset() { }

void TDebuTelesa::calcRootMatrix()
{
	TSpineEnemy::calcRootMatrix();

	if (isTaken())
		return;

	if (mSpine->getLatestNerve() == &TNerveSmallEnemyDie::theNerve())
		return;

	MtxPtr mtx = getModel()->getAnmMtx(mJntRhand);
	mTipPos.set<f32>(mtx[0][3], mtx[1][3], mtx[2][3]);
	SMS_EasyEmitParticle(PARTICLE_MS_POI_ZZZ, &mTipPos, this,
	                     JGeometry::TVec3<f32>(1.5f, 1.5f, 1.5f));

	SMS_EasyEmitParticle((E_SMS_EFFECT_LOOP_NORMAL)0x187,
	                     getModel()->getAnmMtx(mJntNullYodare), this,
	                     JGeometry::TVec3<f32>(2.3f, 2.3f, 2.3f));
}

void TDebuTelesa::kill() { TSmallEnemy::kill(); }

BOOL TDebuTelesa::receiveMessage(THitActor* sender, u32 message)
{
	switch (message) {
	case 0:
	case 1:
	case 12:
		return 0;
	case 11:
		if (gpMSound->gateCheck(0x2938)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x2938, &mPosition, 0,
			                                          nullptr, 0, 4);
		}
		break;
	}
	return TSmallEnemy::receiveMessage(sender, message);
}

const char** TDebuTelesa::getBasNameTable() const { return DebuTelesa_bastable; }

void TDebuTelesa::behaveToWater(THitActor*) { }

void TDebuTelesa::attackToMario() { sendAttackMsgToMario(); }

bool TDebuTelesa::isCollidMove(THitActor*) { return false; }

bool TDebuTelesa::doKeepDistance() { return true; }

void TDebuTelesa::setDeadAnm() { mMActor->getFrameCtrl(0)->init(1); }

TDebuTelesaManager::TDebuTelesaManager(const char* name)
    : TSmallEnemyManager(name)
{
}

void TDebuTelesaManager::load(JSUMemoryInputStream& stream)
{
	TDebuTelesaParams* params
	    = new TDebuTelesaParams("/enemy/debuTelesa.prm");
	unk38 = params;

	params->mSLAttackRadius.set(0xF0);
	params->mSLAttackHeight.set(0x14A);
	params->mSLDamageRadius.set(0xDC);
	params->mSLDamageHeight.set(0x12C);
	params->mBodyScaleRange.mMin = 1.0f;
	params->mBodyScaleRange.mMax = 1.0f;

	TSmallEnemyManager::load(stream);
	unk5C = 0;
}

void TDebuTelesaManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "debuTelesa.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TDebuTelesaManager::clipEnemies(JDrama::TGraphics* graphics)
{
	SetViewFrustumClipCheckPerspective(
	    gpCamera->getFovy(), gpCamera->getAspect(), 350.0f,
	    ((TSmallEnemyParams*)unk38)->mSLFarClip.get());

	s32 num = mObjNum;
	for (s32 i = 0; i < num; ++i) {
		TLiveActor* actor = (TLiveActor*)unk18[i];
		Vec pos           = actor->mPosition;

		if (actor->checkLiveFlag(LIVE_FLAG_UNK2000)) {
			if (SMS_IsInOtherFastCube(pos)) {
				actor->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
				continue;
			}
		}

		if (ViewFrustumClipCheck(graphics, &actor->mPosition, unk3C)) {
			actor->offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		} else {
			actor->onLiveFlag(LIVE_FLAG_CLIPPED_OUT);
		}
	}
}

DEFINE_NERVE(TNerveDebuTelesaWait, TLiveActor)
{
	TDebuTelesa* self = (TDebuTelesa*)spine->getBody();

	if (spine->getTime() == 0) {
		self->getMActor()->setBck("debutelesa_wait");
		self->setCurAnmSound();
	}

	return false;
}
