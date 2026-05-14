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
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DPacket.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <stdlib.h>

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
void TWaterHitPictureHideObj::afterFinishedAnim() { }
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

void TPictureTelesa::touchActor(THitActor* sender)
{
	TWaterHitPictureHideObj::touchActor(sender);
	u8 r1, r2, r3v;
	if ((sender->mActorType - 0x40000000) == 0x1A2)
		r1 = 1;
	else
		r1 = 0;
	if (!r1)
		return;
	if (unk174 != 0)
		return;
	if (mState == 3)
		r2 = 1;
	else
		r2 = 0;
	if (!r2)
		return;
	if (unk104 > 0)
		r3v = 1;
	else
		r3v = 0;
	if (r3v)
		return;
	f32 dx = sender->mPosition.x - mPosition.x;
	f32 dy = sender->mPosition.y - mPosition.y;
	f32 dz = sender->mPosition.z - mPosition.z;
	f32 sq = dx * dx + dy * dy + dz * dz;
	if (sq > 0.0f)
		sq = JGeometry::TUtil<f32>::sqrt(sq);
	if (sq < 200.0f) {
		unk104 = 0x3C;
		if (gpMSound->gateCheck(0x28D5))
			MSoundSESystem::MSoundSE::startSoundActor(0x28D5, mPosition, 0,
			                                          nullptr, 0, 4);
		unk174 = 1;
	}
}

void TPictureTelesa::afterFinishedAnim()
{
	TWaterHitPictureHideObj::afterFinishedAnim();
	u8 result;
	if ((mActorType - 0x40000000) == 0x1A2)
		result = 1;
	else
		result = 0;
	if (result) {
		if (gpMSound->gateCheck(0x484D))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x484D, 0, nullptr, 0);
		if (gpMSound->gateCheck(0x28D9))
			MSoundSESystem::MSoundSE::startSoundActor(0x28D9, mPosition, 0,
			                                          nullptr, 0, 4);
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
	J3DModel* model = getModel();
	Mtx mtx;
	MtxPtr mtxPtr = mtx;
	MsMtxSetXYZRPH(mtxPtr, mPosition.x, mPosition.y + unk140, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	PSMTXCopy(mtxPtr, model->unk20);
	model->unk14 = (Vec&)mScaling;
	mtxPtr[1][3] += unk140;

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

void TRoulette::switchStop()
{
	if (unk150->unk6C != 0) {
		f32 ground = SMS_GetMarioGrLevel();
		if (gpMarioPos->y < ground + 20.0f && unk13C != 0.0f) {
			unk150->unk6C = 0;
			unk13C = 0.0f;
			unk148 = 0;
			unk14A = 0;
			unk14C = 0;
			if (gpMSound->gateCheck(0x2924))
				MSoundSESystem::MSoundSE::startSoundActor(0x2924, mPosition, 0,
				                                          nullptr, 0, 4);
		}
	}
	if (unk150->unk6C != 0 && unk141 != 0) {
		unk150->unk6C = 0;
		unk148 = 0;
		unk14A = 0;
		unk14C = 0;
		if (gpMSound->gateCheck(0x2924))
			MSoundSESystem::MSoundSE::startSoundActor(0x2924, mPosition, 0,
			                                          nullptr, 0, 4);
		unk140 = 1;
	}
}

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

#pragma dont_inline on
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
#pragma dont_inline off

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

// Stub implementations — partial matches expected, started for completeness

void TDonchou::calcRootMatrix()
{
	J3DModel* model = getModel();
	Mtx mtx;
	MtxPtr mtxPtr = mtx;
	MsMtxSetXYZRPH(mtxPtr, mPosition.x, mPosition.y + unk140, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	PSMTXCopy(mtxPtr, model->unk20);
	model->unk14 = (Vec&)mScaling;
	mtxPtr[1][3] += unk140;

	if (unk144 != nullptr && *((u8*)unk144 + 0x194)
	    && unk148 != nullptr && *((u8*)unk148 + 0x194)) {
		unk13C = 1;
	}
	if (unk13C != 0) {
		++unk14C;
		if (unk14C > 100) {
			if (mMActor->checkCurAnm("donchou", 0)) {
				if (mMActor->curAnmEndsNext(0, 0)) {
					unk138->remove();
				}
			} else {
				if (gpMSound->gateCheck(0x4857))
					MSoundSESystem::MSoundSE::startSoundActor(0x4857, mPosition,
					                                          0, nullptr, 0, 4);
				mMActor->setBck("donchou");
				JDrama::TFlagT<u16> flag;
				flag = 0;
				gpMarDirector->fireStartDemoCamera("どん帳カメラ", &mPosition,
				                                   -1, 0.0f, true, nullptr, 0,
				                                   nullptr, flag);
				J3DFrameCtrl* fc = mMActor->getFrameCtrl(0);
				*((f32*)fc + 3) *= 0.5f;
			}
		}
	}
}

void TCloset::moveObject()
{
	TLiveActor::moveObject();
}
void TCloset::calcRootMatrix()
{
	gpCurObject = this;
	J3DModel* model = getModel();
	Mtx mtx;
	MtxPtr mtxPtr = mtx;
	MsMtxSetXYZRPH(mtxPtr, mPosition.x, mPosition.y + unk14C, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	PSMTXCopy(mtxPtr, model->unk20);
	model->unk14 = (Vec&)mScaling;
	mtxPtr[1][3] += unk14C;
	if (unk16C != 0) {
		if (mMActor->checkCurAnm("closetopen", 0)) {
			if (mMActor->curAnmEndsNext(0, 0)) {
				unk168->remove();
			}
		}
	}
}
u32 TCloset::touchWater(THitActor* sender)
{
	if (unk16C != 0)
		return 0;
	if (fabsf(mPosition.x - sender->mPosition.x) >= 50.0f)
		return 0;
	f32 hZ = 1.1f * unk140;
	f32 senderZ = sender->mPosition.z;
	f32 myZ = mPosition.z;
	int dir;
	if (senderZ < myZ - hZ) {
		dir = (mRotation.y < 0.0f) ? 0 : 3;
	} else if (senderZ < myZ) {
		dir = (mRotation.y < 0.0f) ? 1 : 2;
	} else if (senderZ < myZ + hZ) {
		dir = (mRotation.y < 0.0f) ? 2 : 1;
	} else {
		dir = (mRotation.y < 0.0f) ? 3 : 0;
	}
	unk164 = 1;
	f32 sign = (f32)(s16)unk164;
	unk138[dir] += unk154 * sign;
	if (fabsf(unk138[dir]) > unk158) {
		unk138[dir] = unk158 * sign;
	}
	return 1;
}

void TCasinoPanelGate::moveObject()
{
	TLiveActor::moveObject();
}
void TCasinoPanelGate::calcRootMatrix()
{
	gpCurObject = this;
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(model->unk20, mPosition.x, mPosition.y + unk14C, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	model->unk14 = (Vec&)mScaling;
	unk140 = 0.25f * mDamageRadius;
	unk144 = 0.25f * mDamageHeight;
}
u32 TCasinoPanelGate::touchWater(THitActor* sender)
{
	if (unk16D != 0)
		return 1;
	if (fabsf(mPosition.z - sender->mPosition.z) >= 50.0f)
		return 0;
	unk164 = 1;
	return 1;
}

void TSlotDrum::moveObject()
{
	TLiveActor::moveObject();
	mPosition.y = unk150 + unk14C;
}
void TSlotDrum::calcRootMatrix()
{
	gpCurObject = this;
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(model->unk20, mPosition.x, mPosition.y - unk14C, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	model->unk14 = (Vec&)mScaling;
}
u32 TSlotDrum::touchWater(THitActor* sender)
{
	if (unk194 != 0)
		return 1;
	if (fabsf(mPosition.x - sender->mPosition.x) >= 150.0f)
		return 0;
	f32 hZ = 0.6f * unk140;
	int dir;
	if (sender->mPosition.z < mPosition.z - hZ) {
		dir = (mRotation.y < 0.0f) ? 0 : 2;
	} else if (sender->mPosition.z > mPosition.z + hZ) {
		dir = (mRotation.y < 0.0f) ? 2 : 0;
	} else {
		dir = 1;
	}
	unk164 = 1;
	f32 sign = (f32)(s16)unk164;
	unk138[dir] += unk154 * sign;
	if (fabsf(unk138[dir]) > unk158) {
		unk138[dir] = unk158 * sign;
	}
	return 1;
}
void TSlotDrum::initMapObj()
{
	unk148 = 3;
	unk14C = 400.0f;
	unk150 = mPosition.y;
	unk194 = 0;
	unk154 = 2.0f;
	unk158 = 10.0f;
	unk15C = 0.1f;
	unk160 = 0.5f;
	unk164 = 0;
	*(u32*)(unkSlotDrum + 0) = 0; // unk168
	unk138 = (f32*)operator new[](unk148 * sizeof(f32));
	unk13C = (f32*)operator new[](unk148 * sizeof(f32));
	for (s32 i = 0; i < unk148; ++i) {
		unk138[i] = 0.0f;
		unk13C[i] = (f32)(i + 1) * 90.0f;
		*(u8*)((u8*)this + 0x188 + i) = 0;
	}
	TMapObjBase::initMapObj();
	for (u8 k = 0; k < *(u16*)((u8*)getModel() + 0x1C); ++k) {
	}
	for (s32 j = 1; j <= unk148; ++j) {
		mMActor->setJointCallback(j, partsRollCallback);
	}
	unk140 = mDamageRadius / 3.0f;
	unk144 = mDamageHeight;
	initNeonMatColor();
}
void TSlotDrum::initNeonMatColor()
{
	static const char* names[] = { "_NEON_C", "_NEON_B", "_NEON_A" };
	for (int i = 0; i < 3; ++i) {
		GXColorS10* col = (GXColorS10*)((u8*)this + 0x170 + i * 8);
		col->r = 0x78;
		col->g = 0xE6;
		col->b = 0xFF;
		col->a = 0xFF;
		J3DModel* model = mMActor->getModel();
		u16 idx = model->getModelData()->getMaterialName()->getIndex(names[i]);
		SMS_InitPacket_OneTevColor(model, idx, (GXTevRegID)1, col);
	}
}

void TItemSlotDrum::moveObject()
{
	TLiveActor::moveObject();
	mPosition.y = unk150 + unk14C;
	if (unk1A4 > 0) {
		++unk1A4;
		if (unk1A4 > 160) {
			unk1A4 = 0;
		}
	}
}
void TItemSlotDrum::calcRootMatrix()
{
	gpCurObject = this;
	u8 anyNonZero = 0;
	if (unk138[0] != 0.0f)
		anyNonZero = 1;
	if (unk138[1] != 0.0f)
		anyNonZero = 1;
	if (unk138[2] != 0.0f)
		anyNonZero = 1;
	if (anyNonZero) {
		if (gpMSound->gateCheck(0x308D))
			MSoundSESystem::MSoundSE::startSoundActor(0x308D, mPosition, 0,
			                                          nullptr, 0, 4);
	}
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(model->unk20, mPosition.x, mPosition.y - unk14C, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	model->unk14 = (Vec&)mScaling;
}
u32 TItemSlotDrum::touchWater(THitActor* sender)
{
	if (unk194 != 0 || unk1A2 != 0)
		return 1;
	int range = 0x96 - 0x64;
	unk1A4 = 0x64 + (s32)((f32)rand() * (1.0f / 32768.0f) * (f32)range);
	return 0;
}
void TItemSlotDrum::generateItem()
{
	int firstResult = getResultFromAng(unk13C[0]);
	int allMatch = firstResult;
	for (int i = 1; i < 3; ++i) {
		if (getResultFromAng(unk13C[i]) != firstResult) {
			allMatch = -1;
			break;
		}
	}
	if (allMatch == 0) {
		MSBgm::startBGM(0x80010025);
		unk194 = 1;
	}
}
int TItemSlotDrum::getForcastResult(int idx)
{
	f32 ang = unk13C[idx];
	f32 cur = unk138[idx];
	while (fabsf(cur) > unk160) {
		ang += cur;
		if (cur > 0.0f)
			cur -= unk15C;
		else
			cur += unk15C;
		while (ang >= 360.0f)
			ang -= 360.0f;
		while (ang < 0.0f)
			ang += 360.0f;
	}
	ang += cur;
	while (ang >= 360.0f)
		ang -= 360.0f;
	while (ang < 0.0f)
		ang += 360.0f;
	return getResultFromAng(ang);
}

void TRoulette::moveObject()
{
	TLiveActor::moveObject();
	if (unk142 != 0) {
		mRotation.y += unk13C;
	}
	if (unk141 != 0 && unk140 != 0) {
		// stub for game-pad / sound triggers
		mPosition.y -= 1.0f;
	}
	J3DModel* model = mMActor->getModel();
	MtxPtr mtx = (MtxPtr)((u8*)model->mNodeMatrices + 0x30);
	unk150->mPosition.x = mtx[0][3];
	unk150->mPosition.y = mPosition.y - 100.0f;
	unk150->mPosition.z = mtx[2][3];
}
void TRoulette::calcRootMatrix()
{
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(model->unk20, mPosition.x, mPosition.y, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	model->unk14 = (Vec&)mScaling;
}
void TRoulette::initMapObj() { TMapObjBase::initMapObj(); }

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
