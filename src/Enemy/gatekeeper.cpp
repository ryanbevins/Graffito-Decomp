#include <Enemy/GateKeeper.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <Map/PollutionManager.hpp>
#include <Strategic/Spine.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <dolphin/mtx.h>

const char* gatekeeper_bastable[] = {
	"/scene/gatekeeper/bas/gene_pakkun_appear1.bas",
	nullptr,
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_damage1.bas",
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_dead1.bas",
	"/scene/gatekeeper/bas/gene_pakkun_dive1.bas",
	"/scene/gatekeeper/bas/gene_pakkun_iyaiya1.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_wait2.bas",
	nullptr,
	nullptr,
	"/scene/gatekeeper/bas/gene_pakkun_wait2_loop.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

TBiancoGateKeeper::TBiancoGateKeeper(const char* name)
    : TGateKeeperBase(name)
    , unk170(0)
    , unk174(0)
    , unk178(0)
    , unk17C(0)
    , unk17E(0)
    , unk180(0.0f)
    , unk184(0)
    , unk288(0)
    , unk28A(0)
    , unk28C(0)
    , unk290(0)
    , unk292(2)
    , unk293(0)
    , unk294(0)
    , unk296(0)
    , unk298(0)
    , unk29C(0.0f)
{
}

void TBiancoGateKeeper::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TGateKeeperBase::perform(flags, graphics);

	if (flags & 1)
		controlCollision();

	if (flags & 2)
		emitParticles();
}

void TBiancoGateKeeper::controlCollision()
{
	// TODO: recover the head/obstacle collision state transitions.
}

void TBiancoGateKeeper::emitParticles()
{
	// TODO: recover the bound saliva/smoke/death particle emitters.
}

const char** TBiancoGateKeeper::getBasNameTable() const
{
	return gatekeeper_bastable;
}

BOOL TBiancoGateKeeper::isDamageFogSituation() const
{
	// TODO: recover the fog/damage predicate used by the awake nerves.
	return false;
}

BOOL TBiancoGateKeeper::isHeadHitActive() const
{
	// TODO: recover the animation-window predicate.
	return false;
}

f32 TBiancoGateKeeper::getRumblePow()
{
	// TODO: recover the distance-based rumble falloff.
	return 0.0f;
}

void TBiancoGateKeeper::launchNamekuri()
{
	// TODO: recover Namekuri/Gorogoro launch selection and spawn positions.
}

void TBiancoGateKeeper::changeBck(int index)
{
	if (mMActor)
		mMActor->setBckFromIndex(index);
}

void TBiancoGateKeeper::kill()
{
	TGateKeeperBase::kill();
}

void TBiancoGateKeeper::init(TLiveManager* manager)
{
	TSpineEnemy::init(manager);
}

void TBGKMtxCalc::calc(u16 joint_no)
{
	M3UMtxCalcSIAnmBlendQuat::calc(joint_no);
}

void TBiancoGateKeeperManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "gene_pakkun_model1.bmd", 0x11210000, 0 },
		{ "stamp_keeper_model1.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBiancoGateKeeperManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBiancoGateKeeperParams("/enemy/gatekeeper.prm");
	TEnemyManager::load(stream);
	initJParticle();
}

void TBiancoGateKeeperManager::initJParticle()
{
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_dead.jpa", 0xA7);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_deadsmoke.jpa", 0xA8);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_bota.jpa", 0x140);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_yodare_s.jpa", 0x141);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_yodare_l.jpa", 0x142);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_kemuri.jpa", 0x1DF);
	SMS_LoadParticle("/scene/gatekeeper/jpa/ms_gkpa_kemuri_w.jpa", 0x1E0);
}

TBiancoGateKeeperManager::TBiancoGateKeeperManager(const char* name)
    : TEnemyManager(name)
{
}

void TGateKeeperBase::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (flags & 2)
		unk15C->update();

	if (flags & 1) {
		f32 blend;
		if (mMActor->unkC)
			blend = mMActor->unkC->getMotionBlendRatio();
		else
			blend = 0.0f;

		blend -= unk158;
		if (blend < 0.0f)
			blend = 0.0f;

		if (mMActor->unkC)
			mMActor->unkC->setMotionBlendRatio(blend);
	}

	TSpineEnemy::perform(flags, graphics);

	if (unk160 && unk150) {
		if (flags & 2) {
			J3DModel* model     = getModel();
			J3DModel* partModel = unk150->getModel();
			partModel->setBaseScale(unk164);
			PSMTXCopy(model->getBaseTRMtx(), partModel->getBaseTRMtx());
			unk150->calcAnm();
		}

		if (flags & 0x200)
			gpPollution->stampModel(unk150->getModel());
	}

	if (flags & 1)
		unk154 = 0;
}

BOOL TGateKeeperBase::receiveMessage(THitActor* sender, u32 message)
{
	if ((sender->mActorType - 0x01000000) == 1) {
		if (unk161 && message == HIT_MESSAGE_SPRAYED_BY_WATER)
			unk154 += 1;

		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);
		return true;
	}

	return false;
}

void TGateKeeperBase::kill() { onLiveFlag(LIVE_FLAG_DEAD); }

void TGKHitObj::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk68->checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (flags & 2) {
		if (!unk68->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
			MtxPtr mtx = unk68->mMActor->unk4->getAnmMtx(unk6C);
			mPosition.set(mtx[0][3], mtx[1][3], mtx[2][3]);
		} else {
			mPosition = unk68->mPosition;
		}
	}

	THitActor::perform(flags, graphics);
}

BOOL TGKHitObj::receiveMessage(THitActor* sender, u32 message)
{
	if ((sender->mActorType - 0x01000000) == 1
	    && message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (unk70)
			unk68->unk154 += 1;

		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);
		return true;
	}

	return unk68->receiveMessage(sender, message);
}
