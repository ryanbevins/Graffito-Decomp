#define J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#define J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#include <MoveBG/MapObjSirena.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Enemy/Conductor.hpp>
#include <Strategic/Strategy.hpp>
#include <Player/MarioAccess.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
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
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#undef J3DMTXCALC_MAYA_INIT_OUT_OF_LINE
#undef J3DMTXCALC_BASIC_INIT_OUT_OF_LINE
#include <JSystem/JMath.hpp>
#include <Enemy/Telesa.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/ItemManager.hpp>
#include <System/MarioGamePad.hpp>
#include <stdlib.h>

// rogue includes needed for matching sinit
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
// Subset of InfectiousStrings -- without MtxCalcTypeName (matches original .o)
static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";
static const char* MtxCalcTypeName_Basic
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char* MtxCalcTypeName_Softimage
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char* MtxCalcTypeName_MotionBlend
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char* MtxCalcTypeName_User
    = "MActorMtxCalcType_User ユーザー定義";

static void* gpCurObject;


static int partsRollCallback(J3DNode* node, int param)
{
	if (param != 0)
		return 1;
	if (gpCurObject == nullptr)
		return 1;
	u16 jointIdx = ((J3DJoint*)node)->mJntNo;
	TSirenaRollMapObj* obj = (TSirenaRollMapObj*)gpCurObject;
	MtxPtr localMtx = obj->getModel()->mNodeMatrices[jointIdx];
	jointIdx -= 1;

	Mtx scaleMtx;
	scaleMtx[0][0] = obj->mScaling.x;
	scaleMtx[0][1] = 0.0f;
	scaleMtx[0][2] = 0.0f;
	scaleMtx[0][3] = 0.0f;
	scaleMtx[1][0] = 0.0f;
	scaleMtx[1][1] = obj->mScaling.y;
	scaleMtx[1][2] = 0.0f;
	scaleMtx[1][3] = 0.0f;
	scaleMtx[2][0] = 0.0f;
	scaleMtx[2][1] = 0.0f;
	scaleMtx[2][2] = obj->mScaling.z;
	scaleMtx[2][3] = 0.0f;

	Mtx rotMtx;
	f32 angZ = obj->getRollAngZ(jointIdx);
	f32 angY = obj->getRollAngY(jointIdx);
	MsMtxSetRotRPH(rotMtx, obj->getRollAngX(jointIdx), angY, angZ);

	PSMTXConcat(localMtx, rotMtx, localMtx);
	PSMTXConcat(localMtx, scaleMtx, localMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, rotMtx, J3DSys::mCurrentMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, scaleMtx, J3DSys::mCurrentMtx);

	return 1;
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
	if (mLifeTimer > 0)
		r3v = 1;
	else
		r3v = 0;
	if (r3v)
		return;
	f32 dx = sender->mPosition.x - mPosition.x;
	f32 dy = sender->mPosition.y - mPosition.y;
	f32 dz = sender->mPosition.z - mPosition.z;
	f32 sq = dz * dz + (dx * dx + dy * dy);
	if (sq > 0.0f)
		sq = JGeometry::TUtil<f32>::sqrt(sq);
	if (sq < 200.0f) {
		mLifeTimer = 0x3C;
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
			setUpCurrentMapCollision();
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
	unk148.r = 0;
	unk148.g = 0;
	unk148.b = 0;
	unk148.a = 0xFF;
	if (gpApplication.mCurrArea.getStage() == 14
	    && gpMarDirector->getCurrentStage() == 1) {
		unk141 = 1;
		unk148.b = 0xFF;
	}
	if (gpApplication.mCurrArea.getStage() == 56) {
		unk148.b = 0xFF;
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
		JGeometry::TVec3<f32>* mario = gpMarioPos;
		f32 ground = SMS_GetMarioGrLevel();
		ground = 20.0f + ground;
		if (mario->y < ground && unk13C != 0.0f) {
			unk150->unk6C = 0;
			unk13C = 0.0f;
			unk148.r = 0;
			unk148.g = 0;
			unk148.b = 0;
			if (gpMSound->gateCheck(0x2924))
				MSoundSESystem::MSoundSE::startSoundActor(0x2924, mPosition, 0,
				                                          nullptr, 0, 4);
		}
		if (unk150->unk6C != 0 && unk141 != 0) {
			unk150->unk6C = 0;
			unk148.r = 0;
			unk148.g = 0;
			unk148.b = 0;
			if (gpMSound->gateCheck(0x2924))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2924, mPosition, 0, nullptr, 0, 4);
			unk140 = 1;
		}
	}
}

void TRoulette::setRollSp(f32 sp)
{
	unk13C = sp;
	unk148.r = 0;
	unk148.g = 0;
	unk148.b = 0xFF;
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
	for (u16 j = 1; j <= unk148; ++j) {
		mMActor->setJointCallback(j, partsRollCallback);
	}
}

// Stub implementations -- partial matches expected, started for completeness

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
	if (unk16C != 0) {
		if (mMActor->checkCurAnm("closetopen", 0) == 0) {
			++unk16D;
			if (unk16D == 60) {
				mMActor->setBck("closetopen");
				setAnmSound("/scene/mapObj/closetopen.bas");
			}
			return;
		}
	}
	for (s32 i = 0; i < unk148; ++i) {
		f32 cur = unk138[i];
		if (cur == 0.0f)
			continue;
		if (fabsf(cur) > unk160) {
			unk13C[i] += cur;
			if (cur > 0.0f)
				unk138[i] = cur - unk15C;
			else
				unk138[i] = cur + unk15C;
			bool changed = false;
			if (unk13C[i] >= 360.0f) {
				unk13C[i] -= 360.0f;
				changed = true;
			}
			if (unk13C[i] < 0.0f) {
				unk13C[i] += 360.0f;
				changed = true;
			}
			if (changed) {
				if (gpMSound->gateCheck(0x389C))
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x389C, &mPosition, nullptr, fabsf(unk138[i]),
					    0, 0, nullptr, 0, 4);
			}
			continue;
		}
		unk13C[i] += cur;
		bool changed = false;
		if (unk13C[i] >= 360.0f) {
			unk13C[i] -= 360.0f;
			changed = true;
		}
		if (unk13C[i] < 0.0f) {
			unk13C[i] += 360.0f;
			changed = true;
		}
		if (changed) {
			if (gpMSound->gateCheck(0x389C))
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x389C, &mPosition, nullptr, fabsf(unk138[i]), 0, 0,
				    nullptr, 0, 4);
		}
		s32 angInt = (s32)fabsf(unk13C[i]);
		if ((angInt % 180) != 0)
			continue;
		unk138[i] = 0.0f;
		if (unk13C[i] > 180.0f && unk13C[i] < 360.0f)
			continue;
		for (s32 j = 0; j < unk148; ++j) {
			if (j == i)
				continue;
			if (unk138[j] != 0.0f)
				return;
			if (unk13C[j] >= 180.0f && unk13C[j] < 360.0f)
				return;
		}
		unk16C = 1;
		if (gpMSound->gateCheck(0x484D))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x484D, 0,
			                                             nullptr, 0);
	}
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
		dir = 0;
		if (mRotation.y < 0.0f)
			dir = 3;
	} else if (senderZ < myZ) {
		dir = 1;
		if (mRotation.y < 0.0f)
			dir = 2;
	} else if (senderZ < myZ + hZ) {
		dir = 2;
		if (mRotation.y < 0.0f)
			dir = 1;
	} else {
		dir = 3;
		if (mRotation.y < 0.0f)
			dir = 0;
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
	mPosition.y = unk150 - unk14C;
	if (unk16D != 0) {
		J3DFrameCtrl* fc = mMActor->getFrameCtrl(0);
		f32 endFrame = (f32)*((s16*)fc + 4);
		f32 curFrame = fc->getFrame();
		if (curFrame < endFrame - 8.0f) {
			if (gpMSound->gateCheck(0x4058))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x4058, 0, nullptr,
				                                             0);
		}
		if (unk16C == 0) {
			if (fc->checkPass(endFrame - 2.0f)) {
				unk16C = 1;
				if (gpMSound->gateCheck(0x484D))
					MSoundSESystem::MSoundSE::startSoundSystemSE(0x484D, 0,
					                                             nullptr, 0);
			}
		}
		return;
	}

	bool allStopped = true;
	for (s32 i = 0; i < unk148; ++i) {
		f32 cur = unk138[i];
		if (cur == 0.0f) {
			if (unk13C[i] < 180.0f)
				allStopped = false;
			continue;
		}
		if (fabsf(cur) > unk160) {
			unk13C[i] += cur;
			if (cur > 0.0f)
				unk138[i] = cur - unk15C;
			else
				unk138[i] = cur + unk15C;
			bool changed = false;
			if (unk13C[i] >= 360.0f) {
				unk13C[i] -= 360.0f;
				changed = true;
			}
			if (unk13C[i] < 0.0f) {
				unk13C[i] += 360.0f;
				changed = true;
			}
			if (changed) {
				if (gpMSound->gateCheck(0x389E))
					MSoundSESystem::MSoundSE::startSoundActorWithInfo(
					    0x389E, &mPosition, nullptr, fabsf(unk138[i]),
					    0, 0, nullptr, 0, 4);
			}
			continue;
		}
		unk13C[i] += cur;
		bool changed = false;
		if (unk13C[i] >= 360.0f) {
			unk13C[i] -= 360.0f;
			changed = true;
		}
		if (unk13C[i] < 0.0f) {
			unk13C[i] += 360.0f;
			changed = true;
		}
		if (changed) {
			if (gpMSound->gateCheck(0x389E))
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x389E, &mPosition, nullptr, fabsf(unk138[i]), 0, 0,
				    nullptr, 0, 4);
		}
		s32 angInt = (s32)fabsf(unk13C[i]);
		if ((angInt % 180) == 0) {
			if (unk13C[i] < 180.0f || unk13C[i] >= 360.0f)
				unk13C[i] = 0.0f;
			else
				unk13C[i] = 180.0f;
			unk138[i] = 0.0f;
		}
	}
	if (allStopped) {
		unk16D = 1;
		mMActor->setBck("pazul");
		MSBgm::startBGM(0x80010025);
		if (gpMSound->gateCheck(0x4849))
			MSoundSESystem::MSoundSE::startSoundActor(0x4849, mPosition, 0,
			                                          nullptr, 0, 4);
		removeMapCollision();
	}
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
	f32 hY = unk144;
	f32 hX = unk140;
	f32 dx = sender->mPosition.x - mPosition.x;
	int dir;
	if (sender->mPosition.y > mPosition.y + 3.0f * hY) {
		if (dx < -hX)
			dir = 0xC;
		else if (dx < 0.0f)
			dir = 0xD;
		else if (dx > hX)
			dir = 0xF;
		else
			dir = 0xE;
		if (sender->mPosition.y < mPosition.y + 3.5f * hY)
			unk164 = -1;
	} else if (sender->mPosition.y > mPosition.y + 2.0f * hY) {
		if (dx < -hX)
			dir = 8;
		else if (dx < 0.0f)
			dir = 9;
		else if (dx > hX)
			dir = 0xB;
		else
			dir = 0xA;
		if (sender->mPosition.y < mPosition.y + 2.5f * hY)
			unk164 = -1;
	} else if (sender->mPosition.y > mPosition.y + hY) {
		if (dx < -hX)
			dir = 4;
		else if (dx < 0.0f)
			dir = 5;
		else if (dx > hX)
			dir = 7;
		else
			dir = 6;
		if (sender->mPosition.y < mPosition.y + 1.5f * hY)
			unk164 = -1;
	} else {
		if (dx < -hX)
			dir = 0;
		else if (dx < 0.0f)
			dir = 1;
		else if (dx > hX)
			dir = 3;
		else
			dir = 2;
		if (sender->mPosition.y < mPosition.y + 0.5f * hY)
			unk164 = -1;
	}
	f32 sign = (f32)(s16)unk164;
	unk138[dir] += unk154 * sign;
	if (fabsf(unk138[dir]) > unk158) {
		unk138[dir] = unk158 * sign;
	}
	return 1;
}

void TSlotDrum::moveObject()
{
	TLiveActor::moveObject();
	mPosition.y = unk150 + unk14C;
	for (s32 i = 0; i < unk148; ++i) {
		f32 cur = unk138[i];
		if (cur == 0.0f)
			continue;
		f32* abs_counter = (f32*)((u8*)this + 0x188 + i * 4);
		*abs_counter += fabsf(cur);
		if (*abs_counter > 360.0f / (f32)unk168) {
			*abs_counter = 0.0f;
			if (i == 1) {
				if (gpMSound->gateCheck(0x3890))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x3890, mPosition, 0, nullptr, 0, 4);
			} else if (i < 1) {
				if (gpMSound->gateCheck(0x388E))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x388E, mPosition, 0, nullptr, 0, 4);
			} else if (i < 3) {
				if (gpMSound->gateCheck(0x388F))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x388F, mPosition, 0, nullptr, 0, 4);
			}
		}
		if (fabsf(cur) > unk160) {
			unk13C[i] += cur;
			if (cur > 0.0f)
				unk138[i] = cur - unk15C;
			else
				unk138[i] = cur + unk15C;
			if (unk13C[i] >= 360.0f)
				unk13C[i] -= 360.0f;
			if (unk13C[i] < 0.0f)
				unk13C[i] += 360.0f;
			continue;
		}
		unk13C[i] += cur;
		if (unk13C[i] >= 360.0f)
			unk13C[i] -= 360.0f;
		if (unk13C[i] < 0.0f)
			unk13C[i] += 360.0f;
		s32 angInt = (s32)fabsf(unk13C[i]);
		if ((angInt % unk168) != 0)
			continue;
		unk138[i] = 0.0f;
		if (unk13C[i] < (f32)unk168) {
			unk170[i].r = 0xFF;
			unk170[i].g = 0xFF;
			unk170[i].b = 0x46;
			if (gpMSound->gateCheck(0x4849))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x4849, mPosition, 0, nullptr, 0, 4);
		} else {
			unk170[i].r = 0x78;
			unk170[i].g = 0xE6;
			unk170[i].b = 0xFF;
		}
		if (unk13C[i] >= (f32)unk168 && unk13C[i] < 360.0f)
			return;
		for (s32 j = 0; j < unk148; ++j) {
			if (j == i)
				continue;
			if (unk138[j] != 0.0f)
				return;
			if (unk13C[j] >= (f32)unk168 && unk13C[j] < 360.0f)
				return;
		}
		MSBgm::startBGM(0x80010025);
		unk194 = 1;
	}
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
		dir = 2;
		if (mRotation.y < 0.0f)
			dir = 0;
	} else if (sender->mPosition.z > mPosition.z + hZ) {
		dir = 0;
		if (mRotation.y < 0.0f)
			dir = 2;
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
	unk168 = 0;
	unk138 = (f32*)operator new[](unk148 * sizeof(f32));
	unk13C = (f32*)operator new[](unk148 * sizeof(f32));
	for (s32 i = 0; i < unk148; ++i) {
		unk138[i] = 0.0f;
		unk13C[i] = (f32)(i + 1) * 90.0f;
		*(u8*)((u8*)this + 0x188 + i) = 0;
	}
	TMapObjBase::initMapObj();
	for (u8 k = 0; k < getModel()->getModelData()->getJointNum(); ++k) {
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
	const char* names[] = { "_NEON_C", "_NEON_B", "_NEON_A" };
	for (int i = 0; i < 3; ++i) {
		unk170[i].r = 0x78;
		unk170[i].g = 0xE6;
		unk170[i].b = 0xFF;
		unk170[i].a = 0xFF;
		J3DModelData* modelData = getModel()->mModelData;
		J3DModel* model = mMActor->getModel();
		u16 idx = modelData->mMaterialName->getIndex(names[i]);
		SMS_InitPacket_OneTevColor(model, idx,
		                           (GXTevRegID)1, &unk170[i]);
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
			TMsRange<s32> indexRange(0, 2);
			s32 r = indexRange.rand();
			*((u8*)this + 0x19C + r) = 1;

			TMsRange<f32> resultRange(0.0f, 100.0f);
			f32 picked = resultRange.rand();
			if (picked < unk1A8 && unk1A8 > 10.0f) {
				unk198 = 0;
			} else if (picked < 30.0f) {
				unk198 = 2;
			} else if (picked < 70.0f) {
				unk198 = 3;
			} else {
				unk198 = 1;
			}
			unk1A8 += 2.0f;
		}
	}
	for (s32 i = 0; i < unk148; ++i) {
		if (*((u8*)this + 0x19C + i) != 0) {
			if ((u32)getForcastResult(i) == unk198) {
				*((u8*)this + 0x19C + i) = 0;
				*((u8*)this + 0x19F + i) = 0;
			}
		}
		f32 cur = unk138[i];
		if (cur == 0.0f)
			continue;
		if (fabsf(cur) > unk160) {
			unk13C[i] += cur;
			if (*((u8*)this + 0x19F + i) == 0) {
				if (cur > 0.0f)
					unk138[i] = cur - unk15C;
				else
					unk138[i] = cur + unk15C;
			}
			if (unk13C[i] >= 360.0f)
				unk13C[i] -= 360.0f;
			if (unk13C[i] < 0.0f)
				unk13C[i] += 360.0f;
			continue;
		}
		unk13C[i] += cur;
		if (unk13C[i] >= 360.0f)
			unk13C[i] -= 360.0f;
		if (unk13C[i] < 0.0f)
			unk13C[i] += 360.0f;
		if (*((u8*)this + 0x19F + i) != 0)
			continue;
		s32 angInt = (s32)fabsf(unk13C[i]);
		if ((angInt % unk168) != 0)
			continue;
		unk13C[i] = (f32)((s32)(unk13C[i] / (f32)unk168) * unk168);
		unk138[i] = 0.0f;
		if (gpMSound->gateCheck(0x292C))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x292C, mPosition, 0, nullptr, 0, 4);
		bool allStopped = (unk138[0] == 0.0f) && (unk138[1] == 0.0f)
		    && (unk138[2] == 0.0f);
		if (allStopped) {
			unk1A2 = 1;
			generateItem();
		}
		for (s32 j = 0; j < unk148; ++j) {
			if (*((u8*)this + 0x19F + j) == 0)
				continue;
			TMsRange<f32> retryRange(0.0f, 1.0f);
			f32 picked = retryRange.rand();
			if (picked < 0.9f) {
				*((u8*)this + 0x19C + j) = 1;
				continue;
			}
			*((u8*)this + 0x19F + j) = 0;
		}
		if (unk13C[i] < (f32)unk168) {
			unk170[i].r = 0xFF;
			unk170[i].g = 0xFF;
			unk170[i].b = 0x46;
			if (gpMSound->gateCheck(0x4849))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x4849, mPosition, 0, nullptr, 0, 4);
		} else {
			unk170[i].r = 0x78;
			unk170[i].g = 0xE6;
			unk170[i].b = 0xFF;
		}
	}
}
void TItemSlotDrum::calcRootMatrix()
{
	gpCurObject = this;
	u8 anyNonZero = 0;
	for (int i = 0; i < 3; ++i) {
		if (0.0f != unk138[i])
			anyNonZero = 1;
	}
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
	u32 result = 1;
	if (unk194 || !unk1A2)
		return result;

	unk1A4 = TMsRange<s32>(100, 150).rand();
	for (s32 i = 0; i < unk148; ++i) {
		*((u8*)this + 0x19F + i) = 1;
		*((u8*)this + 0x19C + i) = 0;
		unk138[i] = unk158 * TMsRange<f32>(0.5f, 0.8f).rand();
	}
	unk1A2 = 0;

	return result;
}
void TItemSlotDrum::generateItem()
{
	// Check for all-zero result (jackpot)
	int result = getResultFromAng(unk13C[0]);
	for (int i = 1; i < 3; ++i) {
		if (getResultFromAng(unk13C[i]) != result) {
			result = -1;
			break;
		}
	}
	if (result == 0) {
		MSBgm::startBGM(0x80010025);
		unk194 = 1;
		return;
	}

	// Check for all-one result (enemy spawn)
	result = getResultFromAng(unk13C[0]);
	for (int i = 1; i < 3; ++i) {
		if (getResultFromAng(unk13C[i]) != result) {
			result = -1;
			break;
		}
	}
	if (result == 1) {
		TSpineEnemy* enemy
		    = gpConductor->makeOneEnemyAppear(mPosition, "テレサマネージャー", 1);
		if (!enemy)
			return;
		Mtx rot;
		Vec offset;
		offset.x = 0.0f;
		offset.y = -350.0f;
		offset.z = 300.0f;
		s16 angleY = (s16)(mRotation.y * 182.04445f);
		f32 sinY = JMASSin(angleY);
		f32 cosY = JMASCos(angleY);
		rot[0][0] = cosY;
		rot[0][1] = 0.0f;
		rot[0][2] = sinY;
		rot[0][3] = 0.0f;
		rot[1][0] = 0.0f;
		rot[1][1] = 1.0f;
		rot[1][2] = 0.0f;
		rot[1][3] = 0.0f;
		rot[2][0] = -sinY;
		rot[2][1] = 0.0f;
		rot[2][2] = cosY;
		rot[2][3] = 0.0f;
		PSMTXMultVec(rot, &offset, &offset);
		enemy->mPosition.x += offset.x;
		enemy->mPosition.y += offset.y;
		enemy->mPosition.z += offset.z;
		((TTelesa*)enemy)->initItemAttacker(this);
		return;
	}

	result = getResultFromAng(unk13C[0]);
	for (int i = 1; i < 3; ++i) {
		if (getResultFromAng(unk13C[i]) != result) {
			result = -1;
			break;
		}
	}
	if (result == -1) {
		if (gpMSound->gateCheck(0x483D))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x483D, mPosition, 0, nullptr, 0, 4);
		return;
	}

	int count = 1;
	f32 angIncr = 0.0f;
	if (result == 2) {
		count = 3;
		angIncr = 20.0f;
	}
	for (int i = 0; i < count; ++i) {
		f32 ang = mRotation.y + angIncr * ((f32)i - 1.0f);
		s16 angleY = (s16)(ang * 182.04445f);
		f32 sinY = JMASSin(angleY);
		f32 cosY = JMASCos(angleY);
		Mtx rot;
		rot[0][0] = cosY;
		rot[0][1] = 0.0f;
		rot[0][2] = sinY;
		rot[0][3] = 0.0f;
		rot[1][0] = 0.0f;
		rot[1][1] = 1.0f;
		rot[1][2] = 0.0f;
		rot[1][3] = 0.0f;
		rot[2][0] = -sinY;
		rot[2][1] = 0.0f;
		rot[2][2] = cosY;
		rot[2][3] = 0.0f;
		Vec offset;
		offset.x = 0.0f;
		offset.y = -350.0f;
		offset.z = 200.0f;
		PSMTXMultVec(rot, &offset, &offset);
		TMapObjBase* item = (TMapObjBase*)gpItemManager->makeObjAppear(
		    mPosition.x + offset.x, mPosition.y,
		    mPosition.z + offset.z, 0x2000000E, false);
		if (!item)
			continue;
		item->mPosition.x += offset.x;
		item->mPosition.y += offset.y;
		item->mPosition.z += offset.z;
		MsVECNormalize(&offset, &offset);
		TMsRange<f32> ySpeedRange(5.0f, 10.0f);
		f32 vz = offset.z * 12.0f;
		item->mVelocity.x = offset.x * 12.0f;
		item->mVelocity.y = ySpeedRange.rand();
		item->mVelocity.z = vz;
		item->mLiveFlag &= ~0x10;
	}
}
int TItemSlotDrum::getForcastResult(int idx)
{
	f32 angle = unk13C[idx];
	f32 speed = unk138[idx];
	for (;;) {
		if (fabsf(speed) > unk160) {
			angle += speed;
			if (speed > 0.0f)
				speed -= unk15C;
			else
				speed += unk15C;
			if (angle >= 360.0f)
				angle -= 360.0f;
			if (angle <= 0.0f)
				angle += 360.0f;
		} else {
			angle += speed;
			if (angle >= 360.0f)
				angle -= 360.0f;
			if (angle <= 0.0f)
				angle += 360.0f;
			if ((int)fabsf(angle) % unk168 == 0)
				break;
		}
	}
	f32 resultAngle = (f32)(unk168 * (int)(angle / (f32)unk168));
	if (resultAngle < 89.0f)
		return 0;
	if (resultAngle < 179.0f)
		return 1;
	if (resultAngle < 269.0f)
		return 2;
	return 3;
}

void TRoulette::moveObject()
{
	TLiveActor::moveObject();
	if (unk142 != 0) {
		mRotation.y += unk13C;
	}
	if (unk141 != 0 && unk140 != 0) {
		gpMarioOriginal->mGamePad->onNeutralMarioKey();
		gpMarioOriginal->mGamePad->mDisabledFrames = 5;
		if (gpMSound->gateCheck(0x2125))
			MSoundSESystem::MSoundSE::startSoundActor(0x2125, mPosition, 0,
			                                          nullptr, 0, 4);
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
void TRoulette::initMapObj()
{
	mPosition.y += 500.0f;
	TMapObjBase::initMapObj();
	for (u16 i = 0; i < getModel()->mModelData->mMaterialNum; ++i) {
		const char* name = getModel()->mModelData->mMaterialName->getName(i);
		if (strstr(name, "_switch")) {
			SMS_InitPacket_OneTevColor(getMActor()->getModel(), i,
			                           (GXTevRegID)1, &unk148);
		}
	}
	TRouletteSw* sw = new TRouletteSw("ルーレットスイッチ");
	sw->unk68 = this;
	unk150 = sw;
	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("オブジェクトグループ");
	group->getChildren().insert(group->getChildren().end(),
	                            (THitActor*)unk150);
	if (gpApplication.mCurrArea.getStage() == 14) {
		unk150->initHitActor(0x4000019A, 2, 0x80000000, 40.0f, 80.0f, 40.0f,
		                    80.0f);
	} else {
		unk150->initHitActor(0x4000019A, 2, 0x80000000, 500.0f, 100.0f,
		                    500.0f, 100.0f);
	}
	unk150->unk64 &= ~1;
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
