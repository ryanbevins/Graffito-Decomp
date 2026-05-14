#include <MoveBG/MapObjSirena.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Enemy/Conductor.hpp>
#include <Player/MarioAccess.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <M3DUtil/MActorData.hpp>
#include <Strategic/ObjModel.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <MoveBG/MapObjManager.hpp>

// rogue includes needed for matching sinit
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static void* gpCurObject;

static int partsRollCallback(J3DNode*, int) { return 1; }

TWaterHitPictureHideObj::~TWaterHitPictureHideObj() { }
#pragma dont_inline on
void TWaterHitPictureHideObj::control() { }
void TWaterHitPictureHideObj::touchActor(THitActor*) { }
#pragma dont_inline off

Vec* TWaterHitPictureHideObj::getObjAppearPos() const
{
	return (Vec*)&mPosition;
}

void TPictureTelesa::control()
{
	TWaterHitPictureHideObj::control();
	if (unk174 != 0 && mColCount == 0) {
		unk174 = 0;
	}
}

void TChestRevolve::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		if (animIsFinished()) {
			mState = 1;
			setUpMapCollision(0);
		}
		break;
	}
}

u32 TChestRevolve::touchWater(THitActor* sender)
{
	if (isState(1)) {
		mState = 2;
		startAnim(1);
		setUpMapCollision(1);
	}
	return 1;
}

void TPanelRevolve::someAction() { }

void TPanelRevolve::touchPlayer(THitActor* sender)
{
	if (marioHipAttack() && isState(1)) {
		if (gpMSound->gateCheck(0x385D)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x385D, mPosition, 0,
			                                          nullptr, 0, 4);
		}
		mState = 2;
		startAnim(1);
		removeMapCollision();
	}
}

BOOL TPanelRevolve::receiveMessage(THitActor* sender, u32 msg)
{
	if (isState(1)) {
		if (gpMSound->gateCheck(0x385D)) {
			MSoundSESystem::MSoundSE::startSoundActor(0x385D, mPosition, 0,
			                                          nullptr, 0, 4);
		}
		mState = 2;
		startAnim(1);
		removeMapCollision();
	}
	return TRUE;
}

void TPanelRevolve::control()
{
	TMapObjBase::control();
	switch (mState) {
	case 0:
		break;
	case 1:
		break;
	case 2:
		if (animIsFinished()) {
			mState = 1;
			someAction();
		}
		break;
	}
}

TWarpAreaActor::TWarpAreaActor(const char* name)
    : THitActor(name)
{
	unk68 = -1;
	unk6A = -1;
}

void TWarpAreaActor::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	s32 tmp;
	stream.read(&tmp, 4);
	unk68 = (s16)tmp;
	stream.read(&tmp, 4);
	unk6A = (s16)tmp;

	initHitActor(0x4000019d, 1, 0x80000000, mScaling.x * 100.0f,
	             mScaling.y * 100.0f, 0.0f, 0.0f);
	unk64 &= ~1;
	gpConductor->registerOtherObj(this);
}

void TWarpAreaActor::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (!(flags & 1))
		return;
	if (mColCount == 0)
		return;

	if (*gpMarioSpeedY > 0.0f && unk68 != -1)
		gpMap->changeModel(unk68);
	if (*gpMarioSpeedY < 0.0f && unk6A != -1)
		gpMap->changeModel(unk6A);
}

u32 TSirenaCasinoRoof::getSDLModelFlag() const { return 0; }

void TSirenaCasinoRoof::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		unk138->update();
	TMapObjBase::perform(flags, graphics);
}

void TSirenaCasinoRoof::initMapObj()
{
	TMapObjBase::initMapObj();
	mMActor->offMakeDL();

	MActorAnmData* anmData = mMActorKeeper->getMActorAnmData();
	unk138 = new TMultiBtk(3, getModel()->getModelData());
	for (int i = 0; i <= 2; ++i) {
		MActorAnmDataEach<J3DAnmTextureSRTKey>* btk = anmData->getUnk38();
		J3DAnmTextureSRTKey* anm;
		if (i < btk->unk0)
			anm = (J3DAnmTextureSRTKey*)btk->unkC[i];
		else
			anm = nullptr;
		unk138->setNthData(i, anm);
	}
	mMActor->setBrk("casino_lighting");
}

u32 TSirenabossWall::getSDLModelFlag() const { return 0; }

void TSirenabossWall::drawObject(JDrama::TGraphics* gfx)
{
	mMActor->getModel()->entry();
}

void TSirenabossWall::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		unk138->update();
	TMapObjBase::perform(flags, graphics);
}

void TSirenabossWall::initMapObj()
{
	TMapObjBase::initMapObj();
	mMActor->offMakeDL();

	MActorAnmData* anmData = mMActorKeeper->getMActorAnmData();
	unk138 = new TMultiBtk(3, getModel()->getModelData());
	for (int i = 0; i <= 2; ++i) {
		MActorAnmDataEach<J3DAnmTextureSRTKey>* btk = anmData->getUnk38();
		J3DAnmTextureSRTKey* anm;
		if (i < btk->unk0)
			anm = (J3DAnmTextureSRTKey*)btk->unkC[i];
		else
			anm = nullptr;
		unk138->setNthData(i, anm);
	}
}

TSakuCasino::TSakuCasino(const char* name)
    : TMapObjBase(name)
    , unk138(nullptr)
    , unk13C(0)
    , unk140(0.0f)
    , unk144(nullptr)
{
}

void TSakuCasino::loadAfter()
{
	TMapObjBase::loadAfter();
	unk144 = JDrama::TNameRefGen::search<THitActor>("pazul");
}

void TSakuCasino::initMapObj()
{
	unk140 = 0.0f;
	unk13C = 0;
	TMapObjBase::initMapObj();
	getModel();

	Mtx mtx;
	MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y + 2.0f * unk140, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	unk138 = new TMapCollisionWarp;
	unk138->init("/mapObj/SakuCasino", 0, this);
	TMapCollisionWarp* warp = unk138;
	PSMTXCopy(mtx, warp->unk20);
	warp->setUp();
}

void TSakuCasino::calcRootMatrix()
{
	Mtx mtx;
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y + unk140, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	PSMTXCopy(mtx, model->unk20);
	model->unk14 = (Vec&)mScaling;
	mtx[1][3] += unk140;

	if (unk144 != nullptr && *((const u8*)unk144 + 0x16D)) {
		unk13C = 1;
		unk138->remove();
	}
	if (unk13C != 0) {
		unk140 -= 1.0f;
		mScaling.y *= 0.99f;
	}
}

TDonchou::TDonchou(const char* name)
    : TMapObjBase(name)
    , unk138(nullptr)
    , unk13C(0)
    , unk140(0.0f)
    , unk14C(0)
{
}

u32 TDonchou::touchWater(THitActor* sender)
{
	if (fabsf(mPosition.z - sender->mPosition.z) < 50.0f)
		return 1;
	return 0;
}

void TDonchou::loadAfter()
{
	TMapObjBase::loadAfter();
	if (gpApplication.mCurrArea.getStage() == 14
	    && gpMarDirector->getCurrentStage() == 0) {
		unk144 = JDrama::TNameRefGen::search<THitActor>("srotdram");
		unk148 = JDrama::TNameRefGen::search<THitActor>("itemsrotdram");
	}
}

void TDonchou::initMapObj()
{
	unk140 = 0.0f;
	unk13C = 0;
	unk148 = nullptr;
	unk144 = nullptr;
	TMapObjBase::initMapObj();
	getModel();

	Mtx mtx;
	MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y + 2.0f * unk140, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	unk138 = new TMapCollisionWarp;
	unk138->init("/mapObj/Donchou", 0, this);
	TMapCollisionWarp* warp = unk138;
	PSMTXCopy(mtx, warp->unk20);
	warp->setUp();
}

TSirenaRollMapObj::TSirenaRollMapObj(const char* name)
    : TMapObjBase(name)
    , unk138(nullptr)
    , unk13C(nullptr)
    , unk148(0)
    , unk14C(0.0f)
    , unk150(0.0f)
    , unk154(1.0f)
    , unk158(10.0f)
    , unk15C(0.1f)
    , unk160(0.2f)
    , unk164(1)
{
	gpCurObject = nullptr;
}

TCloset::TCloset(const char* name)
    : TSirenaRollMapObj(name)
    , unk168(0)
    , unk16C(0)
    , unk16D(0)
{
}

TCasinoPanelGate::TCasinoPanelGate(const char* name)
    : TSirenaRollMapObj(name)
    , unk168(0)
    , unk16C(0)
    , unk16D(0)
{
}

TSlotDrum::TSlotDrum(const char* name)
    : TSirenaRollMapObj(name)
    , unk194(0)
{
}

TRoulette::TRoulette(const char* name)
    : TMapObjBase(name)
    , unk138(500.0f)
    , unk13C(0.0f)
    , unk140(0)
    , unk141(0)
    , unk142(0)
    , unk144(0.2f)
{
	unk150 = nullptr;
	unk148 = 0;
	unk14A = 0;
	unk14C = 0;
	unk14E = 0xFF;
	if (gpApplication.mCurrArea.getStage() == 14
	    && gpMarDirector->getCurrentStage() == 1) {
		unk141 = 1;
		unk14C = 0xFF;
	}
	if (gpApplication.mCurrArea.getStage() == 56) {
		unk14C = 0xFF;
		unk142 = 1;
	}
}

void TRoulette::perform(u32 flags, JDrama::TGraphics* gfx)
{
	TMapObjBase::perform(flags, gfx);
	unk150->perform(flags, gfx);
}

void TRouletteSw::perform(u32 flags, JDrama::TGraphics* gfx)
{
	THitActor::perform(flags, gfx);
	((TRoulette*)unk68)->switchStop();
}

BOOL TRouletteSw::receiveMessage(THitActor* sender, u32 message)
{
	if (message == 1) {
		unk6C = 1;
		return TRUE;
	}
	return FALSE;
}

void TRoulette::switchStop() { }

void TRoulette::setRollSp(f32 sp)
{
	unk13C = sp;
	unk148 = 0;
	unk14A = 0;
	unk14C = 0xFF;
	unk150->unk6C = 0;
}

void TItemSlotDrum::loadAfter()
{
	TMapObjBase::loadAfter();
	for (int i = 0; i < 6; ++i) {
		TMapObjBaseManager::newAndRegisterObj("coin");
	}
}

int TItemSlotDrum::getResultFromAng(f32 ang)
{
	if (ang < 89.0f)
		return 0;
	if (ang < 179.0f)
		return 1;
	if (ang < 269.0f)
		return 2;
	return 3;
}

void TCasinoPanelGate::initMapObj()
{
	unk148 = 16;
	unk14C = 410.0f;
	unk150 = mPosition.y;
	unk154 = 4.0f;
	unk158 = 15.0f;
	unk15C = 0.2f;
	unk160 = 1.0f;
	unk164 = 0;
	unk138 = new f32[unk148];
	unk13C = new f32[unk148];
	for (int i = 0; i < unk148; ++i) {
		unk138[i] = 0.0f;
		unk13C[i] = 0.0f;
	}
	TMapObjBase::initMapObj();
	for (u16 j = 1; (s32)j <= unk148; ++j) {
		mMActor->setJointCallback(j, partsRollCallback);
	}
}

void TCloset::initMapObj()
{
	unk148 = 4;
	unk14C = 0.0f;
	unk150 = mPosition.y;
	unk154 = 2.5f;
	unk158 = 8.0f;
	unk15C = 0.1f;
	unk160 = 0.5f;
	unk164 = 0;
	unk16C = 0;
	unk16D = 0;
	unk138 = new f32[unk148];
	unk13C = new f32[unk148];
	for (int i = 0; i < unk148; ++i) {
		unk138[i] = 0.0f;
		unk13C[i] = 180.0f;
	}
	TMapObjBase::initMapObj();
	for (u16 j = 1; (s32)j <= unk148; ++j) {
		mMActor->setJointCallback(j, partsRollCallback);
	}
	unk140 = 0.25f * mDamageRadius;
	unk144 = mDamageHeight;
	getModel();

	Mtx mtx;
	MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y + 2.0f * unk140, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	unk168 = new TMapCollisionWarp;
	unk168->init("/mapObj/Closet", 0, this);
	TMapCollisionWarp* warp = unk168;
	PSMTXCopy(mtx, warp->unk20);
	warp->setUp();
	initAnmSound();
}

f32 TCloset::getRollAngY(int i) const { return unk13C[i]; }
f32 TCasinoPanelGate::getRollAngX(int i) const { return unk13C[i]; }
f32 TSlotDrum::getRollAngX(int i) const { return unk13C[i]; }

TItemSlotDrum::TItemSlotDrum(const char* name)
    : TSlotDrum(name)
    , unk198(0)
    , unk1A4(0)
    , unk1A8(5.0f)
{
	unk19C = 0;
	unk19F = 1;
	unk19D = 0;
	unk1A0 = 1;
	unk19E = 0;
	unk1A1 = 1;
	unk1A2 = 1;
}
