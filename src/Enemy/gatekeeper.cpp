#include <Enemy/GateKeeper.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSound.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <System/FlagManager.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <dolphin/mtx.h>
#include <string.h>

DEFINE_NERVE(TNerveBGKSleep, TLiveActor)
{
	// TODO: recover the sleeping gatekeeper state.
	return false;
}

DEFINE_NERVE(TNerveBGKAwakeDamage, TLiveActor)
{
	// TODO: recover the awake damage gatekeeper state.
	return false;
}

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
	const TNerveBase<TLiveActor>* awakeDamage
	    = &TNerveBGKAwakeDamage::theNerve();

	if (mSpine->getLatestNerve() == awakeDamage && mSpine->getTime() <= 110)
		return true;

	if (mMActor->checkCurBckFromIndex(0x12)) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		f32 frame          = ctrl->getFrame();
		if (!(50.0f < frame && frame < 160.0f))
			return false;
	}

	if (mMActor->checkCurBckFromIndex(0x0B)
	    || mMActor->checkCurBckFromIndex(0x11)
	    || mMActor->checkCurBckFromIndex(0x0D)
	    || mMActor->checkCurBckFromIndex(0x07)
	    || mMActor->checkCurBckFromIndex(0x06))
		return false;

	if (unk17C > 0)
		return true;

	return false;
}

BOOL TBiancoGateKeeper::isHeadHitActive() const
{
	J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);

	if (mMActor->checkCurBckFromIndex(0x12)) {
		f32 frame = ctrl->getFrame();
		if (50.0f < frame && frame < 160.0f)
			return true;
		return false;
	}

	if (mMActor->checkCurBckFromIndex(0x0F)) {
		if (ctrl->getFrame() > 12.0f)
			return true;
		return false;
	}

	if (mMActor->checkCurBckFromIndex(0x10)
	    || mMActor->checkCurBckFromIndex(0x0C))
		return true;

	return false;
}

f32 TBiancoGateKeeper::getRumblePow()
{
	JGeometry::TVec3<f32> delta = mPosition;
	delta.sub(*gpMarioPos);

	f32 distance = delta.length();
	if (distance == 0.0f)
		return 1.0f;

	f32 power = 2000.0f / distance;
	if (power > 1.0f)
		power = 1.0f;

	return power;
}

void TBiancoGateKeeper::launchNamekuri()
{
	// TODO: recover Namekuri/Gorogoro launch selection and spawn positions.
}

void TBiancoGateKeeper::changeBck(int index)
{
	int curBck = mMActor->getCurAnmIdx(MActor::ANM_TYPE_BCK);

	if ((curBck == 0x11 && index == 0x0B)
	    || (curBck == 0x11 && index == 0x12)
	    || (curBck == 0x0B && index == 0x12)
	    || (curBck == 0x12 && index == 0x0B)
	    || (curBck == 0x07 && index == 0x07)
	    || (curBck == 0x07 && index == 0x12)
	    || (curBck == 0x0F && index == 0x10)
	    || (curBck == 0x10 && index == 0x0C)) {
		TBGKMtxCalc* calc = unk178;
		MActorAnmDataEach<J3DAnmTransformKey>* data
		    = calc->unk64->mMActorKeeper->getMActorAnmData()->getUnk2C();
		J3DAnmTransform* nextAnm = data->getAnmPtr(index);

		calc->unk54 = nextAnm;
		calc->unk58 = nullptr;
		calc->unk50 = 0.0f;

		MActorAnmBck* bck = mMActor->unkC;
		bck->unk0         = index;
		if (index >= 0) {
			bck->unk24 = bck->getData()->getAnmPtr(index);
			bck->unk4.init(bck->unk24->getFrameMax());
			bck->unk4.setAttribute(bck->unk24->getAttribute());
			bck->unk4.setRate(SMSGetAnmFrameRate());
		}
		unk158 = 0.0f;
	} else {
		TBGKMtxCalc* calc = unk178;
		MActorAnmDataEach<J3DAnmTransformKey>* data
		    = calc->unk64->mMActorKeeper->getMActorAnmData()->getUnk2C();
		J3DAnmTransform* nextAnm = data->getAnmPtr(index);

		if (calc->unk54 != nextAnm) {
			calc->unk58 = calc->unk54;
			calc->unk54 = nextAnm;
			calc->unk50 = 1.0f;
		}

		MActorAnmBck* bck = mMActor->unkC;
		bck->unk0         = index;
		if (index >= 0) {
			bck->unk24 = bck->getData()->getAnmPtr(index);
			bck->unk4.init(bck->unk24->getFrameMax());
			bck->unk4.setAttribute(bck->unk24->getAttribute());
			bck->unk4.setRate(SMSGetAnmFrameRate());
		}

		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
		if (ctrl) {
			f32 blendFrames = 0.2f * ctrl->getEnd();
			if (blendFrames < 1.0f)
				unk158 = 1.0f;
			else
				unk158 = 1.0f / blendFrames;
		} else {
			unk158 = 1.0f;
		}
	}

	const char** basTable = getBasNameTable();
	const char* basName;
	if (basTable == nullptr)
		basName = nullptr;
	else
		basName = basTable[index];
	setAnmSound(basName);
}

void TBiancoGateKeeper::kill()
{
	TGateKeeperBase::kill();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk174->onHitFlag(HIT_FLAG_NO_COLLISION);
	unk28C->onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBiancoGateKeeper::init(TLiveManager* manager)
{
	static const char enemyGroupName[]
	    = "\x93\x47\x83\x4F\x83\x8B\x81\x5B\x83\x76";
	static const char riccoGateKeeperName[]
	    = "\x83\x51\x81\x5B\x83\x67\x83\x4C\x81\x5B\x83\x70\x81\x5B\x81\x69"
	      "\x83\x8A\x83\x52\x81\x6A";
	static const char mammaGateKeeperName[]
	    = "\x83\x51\x81\x5B\x83\x67\x83\x4C\x81\x5B\x83\x70\x81\x5B\x81\x69"
	      "\x83\x7D\x83\x93\x83\x7D\x81\x6A";
	static const char obstacleName[] = "TBGKObstacle";
	static const char headHitName[]
	    = "\x93\xAA\x83\x71\x83\x62\x83\x67";

	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 3);
	mMActor       = mMActorKeeper->createMActor("gene_pakkun_model1.bmd", 3);

	if (strcmp(mName, riccoGateKeeperName) == 0) {
		unk292 = 3;
		if (!TFlagManager::smInstance->getBool(0x50001))
			onLiveFlag(LIVE_FLAG_DEAD);
	} else if (strcmp(mName, mammaGateKeeperName) == 0) {
		unk292 = 4;
		if (!TFlagManager::smInstance->getBool(0x50002))
			onLiveFlag(LIVE_FLAG_DEAD);
	} else if (gpMarDirector->mMap == 0) {
		unk292 = 0;
	} else if (gpMarDirector->mMap == 1) {
		unk292 = 2;
	} else {
		unk292 = 1;
	}

	if (gpMarDirector->mMap == 1) {
		unk150
		    = mMActorKeeper->createMActor("stamp_keeper_model1.bmd", 3);
		unk150->setBckFromIndex(19);

		if (unk292 == 2)
			unk164.scale(0.8f);
		else if (unk292 == 3)
			unk164.scale(1.2f);
		else if (unk292 == 4)
			unk164.scale(1.4f);
	}

	initHitActor(0x10000022, 5, 0x81000000, 400.0f, 150.0f, 400.0f,
	             150.0f);
	JDrama::TNameRefGen::search<TIdxGroupObj>(enemyGroupName)->add(this);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	mSpine->initWith(&TNerveBGKSleep::theNerve());

	J3DModel* model = mMActor->getModel();
	if (!model->getSkinDeform())
		model->setSkinDeform(new J3DSkinDeform, J3D_DEFORM_ATTACH_FLAG_UNK_1);

	unk178 = new TBGKMtxCalc(this);
	mMActor->setCalcForBck((J3DMtxCalc*)unk178);

	mHitPoints = 3;
	mMActor->offMakeDL();

	unk15C = new TMultiBtk(2, getModel()->getModelData());
	MActorAnmDataEach<J3DAnmTextureSRTKey>* btkData
	    = mMActorKeeper->getMActorAnmData()->getUnk38();
	for (int i = 0; i < 2; ++i)
		unk15C->setNthData(i, btkData->getAnmPtr(i));

	unk28C = new TBGKObstacle(obstacleName);
	unk28C->mPosition = mPosition;
	unk28C->mPosition.y -= 1000.0f;
	unk28C->initHitActor(0x10000022, 1, 0x80000000, 800.0f, 800.0f,
	                      800.0f, 800.0f);
	unk28C->offHitFlag(HIT_FLAG_NO_COLLISION);
	JDrama::TNameRefGen::search<TIdxGroupObj>(enemyGroupName)->add(unk28C);

	ResTIMG* tex = (ResTIMG*)JKRFileLoader::getGlbResource(
	    "/scene/map/pollution/H_ma_rak.bti");
	if (tex)
		SMS_ChangeTextureAll(mMActor->getModel()->getModelData(),
		                     "Q_kepper_dummy_128IA4", *tex);

	initAnmSound();
	(void)getBasNameTable();

	unk174        = new TGKHitObj(headHitName);
	unk174->unk68 = this;
	unk174->unk6C = 10;
	unk174->unk70 = 0;
	unk174->initHitActor(0x10000022, 1, 0x80000000, 0.0f, 0.0f, 150.0f,
	                      200.0f);
	unk174->offHitFlag(HIT_FLAG_NO_COLLISION);
	JDrama::TNameRefGen::search<TIdxGroupObj>(enemyGroupName)->add(unk174);

	mMActor->calc();
	if (unk150)
		unk150->calc();

	if (checkLiveFlag(LIVE_FLAG_DEAD)) {
		onHitFlag(HIT_FLAG_NO_COLLISION);
		unk174->onHitFlag(HIT_FLAG_NO_COLLISION);
		unk28C->onHitFlag(HIT_FLAG_NO_COLLISION);
	}
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
