#define MSL_STDFMODF_OUT_OF_LINE

#include <Enemy/BathtubPeach.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Camera/cameralib.hpp>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

#include <dolphin/mtx.h>

namespace std {
float fmodf(float, float);
}

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]   = "メモリが足りません\n";
static const char MtxCalcTypeName0[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeName1[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeName2[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeName3[]
    = "MActorMtxCalcType_User ユーザー定義";
static const f32 dummy2850[3] = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2852[3] = { 1.0f, 1.0f, 1.0f };

static const char* bathtubpeach_bastable[] = {
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr,
	nullptr,
	"/scene/bathtubpeach/bas/peach_wait.bas",
	nullptr,
};

// reverse-order definitions for -inline deferred TU

DEFINE_NERVE(TNervePeachStagger, TLiveActor)
{
	TBathtubPeach* peach = (TBathtubPeach*)spine->getBody();
	if (!peach->getMActor()->checkCurBckFromIndex(0)) {
		peach->getMActor()->setBckFromIndex(0);
		const char** table = peach->getBasNameTable();
		peach->setAnmSound(!table ? nullptr : table[0]);
	}
	if (peach->getMActor()->getCurAnmIdx(3) != 0)
		peach->getMActor()->setBtpFromIndex(0);
	J3DFrameCtrl* frameCtrl = peach->getMActor()->getFrameCtrl(0);
	frameCtrl->setRate(0.5f * (2.0f * SMSGetAnmFrameRate()));
	return peach->getMActor()->curAnmEndsNext(0, nullptr) ? TRUE : FALSE;
}

DEFINE_NERVE(TNervePeachEscape, TLiveActor)
{
	TBathtubPeach* peach   = (TBathtubPeach*)spine->getBody();
	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	TBathtub* bathtub      = (TBathtub*)root->searchF(
        JDrama::TNameRef::calcKeyCode("バスタブ"), "バスタブ");
	if (bathtub->getUnk29A())
		return FALSE;

	if (!(spine->getTime() & 4)) {
		if (bathtub->getUnk1D4())
			spine->pushNerve(&TNervePeachStagger::theNerve());
		return FALSE;
	}

	if (!peach->getMActor()->checkCurBckFromIndex(1)) {
		peach->getMActor()->setBckFromIndex(1);
		const char** table = peach->getBasNameTable();
		peach->setAnmSound(!table ? nullptr : table[1]);
	}
	if (peach->getMActor()->getCurAnmIdx(3) != 1)
		peach->getMActor()->setBtpFromIndex(1);
	J3DFrameCtrl* frameCtrl = peach->getMActor()->getFrameCtrl(0);
	frameCtrl->setRate(0.5f * (2.0f * SMSGetAnmFrameRate()));

	Mtx* btMtx   = bathtub->getRootJointMtx();
	f32 bathtubX = (*btMtx)[0][3];
	f32 bathtubZ = (*btMtx)[2][3];

	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	f32 marioDeg = SHORTANGLE2DEG(
	    (s16)matan(marioPos.z - bathtubZ, marioPos.x - bathtubX));

	f32 peachDeg = SHORTANGLE2DEG((s16)matan(peach->mPosition.z - bathtubZ,
	                                         peach->mPosition.x - bathtubX));
	f32 angleDiff
	    = std::fmodf(360.0f + peachDeg - marioDeg - -180.0f, 360.0f) + -180.0f;

	TBathtubPeachParams* params
	    = (TBathtubPeachParams*)((TEnemyManager*)peach->getManager())->getSaveParam();
	f32 angleParam = params->angle.value;
	f32 newAngle;
	if (angleDiff < 0.0f) {
		newAngle
		    = std::fmodf(360.0f + (marioDeg - angleParam) - -180.0f, 360.0f)
		      + -180.0f;
	} else {
		newAngle
		    = std::fmodf(360.0f + (marioDeg + angleParam) - -180.0f, 360.0f)
		      + -180.0f;
	}

	TBathtubPeachParams* params2
	    = (TBathtubPeachParams*)((TEnemyManager*)peach->getManager())->getSaveParam();
	f32 radius  = params2->radius.value;
	f32 targetX = bathtubX + radius * JMASin(newAngle);
	f32 targetZ = bathtubZ + radius * JMACos(newAngle);

	JGeometry::TVec2<f32> delta;
	delta.x = targetX - peach->mPosition.x;
	delta.y = targetZ - peach->mPosition.z;

	TBathtubPeachParams* params3
	    = (TBathtubPeachParams*)((TEnemyManager*)peach->getManager())->getSaveParam();
	f32 speed = params3->speed.value;
	if (delta.x * delta.x + delta.y * delta.y >= speed * speed) {
		f32 lenSq = delta.dot(delta);
		if (lenSq <= 3.8146973e-6f) {
			delta.y = 0.0f;
			delta.x = 0.0f;
		} else {
			f32 invLen = JGeometry::TUtil<f32>::inv_sqrt(lenSq);
			delta.x *= speed * invLen;
			delta.y *= speed * invLen;
		}
	}

	peach->mPosition.x += delta.x;
	peach->mPosition.z += delta.y;

	f32 dx                       = gpMarioPos->x - peach->mPosition.x;
	f32 dz                       = gpMarioPos->z - peach->mPosition.z;
	TBathtubPeachParams* params4
	    = (TBathtubPeachParams*)((TEnemyManager*)peach->getManager())->getSaveParam();
	f32 turnSpeed2 = params4->turnSpeed2.value;
	if (dx * dx + dz * dz > 3.8146973e-6f) {
		f32 targetRot
		    = SHORTANGLE2DEG((s16)matan(dz, dx)) - 90.0f;
		f32 diff = std::fmodf(
		               360.0f + (targetRot - peach->mRotation.y) - -180.0f,
		               360.0f)
		           + -180.0f;
		if (diff < -turnSpeed2) {
			peach->mRotation.y
			    = std::fmodf(
			          360.0f + (peach->mRotation.y - turnSpeed2) - -180.0f,
			          360.0f)
			      + -180.0f;
		} else if (diff > turnSpeed2) {
			peach->mRotation.y
			    = std::fmodf(
			          360.0f + (peach->mRotation.y + turnSpeed2) - -180.0f,
			          360.0f)
			      + -180.0f;
		} else {
			peach->mRotation.y = targetRot;
		}
	}

	return FALSE;
}

TBathtubPeach::TBathtubPeach(const char* name)
    : TSpineEnemy(name)
{
	onLiveFlag(LIVE_FLAG_AIRBORNE);
	offLiveFlag(LIVE_FLAG_UNK100);
	offLiveFlag(LIVE_FLAG_UNK10);
}

const char** TBathtubPeach::getBasNameTable() const
{
	return bathtubpeach_bastable;
}

void TBathtubPeach::init(TLiveManager* manager)
{
	TSpineEnemy::init(manager);
	mSpine->initWith(&TNervePeachEscape::theNerve());
	initAnmSound();
	reset();
	mScaling.x = 2.0f;
	mScaling.y = 2.0f;
	mScaling.z = 2.0f;
}

void TBathtubPeach::reset()
{
	mPosition.x *= 0.21f;
	mPosition.y *= 0.21f;
	mPosition.z *= 0.21f;
	mBinder.init(50.0f, 50.0f, 50.0f, 50.0f, 0.0f);
	unk130     = 0;
	mScaling.x = 1.5f;
	mScaling.y = 1.5f;
	mScaling.z = 1.5f;
	TLiveActor::mBinder = &mBinder;
	TSpineEnemy::reset();

	if (!mMActor->checkCurBckFromIndex(1)) {
		mMActor->setBckFromIndex(1);
		const char** table = getBasNameTable();
		setAnmSound(!table ? nullptr : table[1]);
	}
	if (mMActor->getCurAnmIdx(3) != 1)
		mMActor->setBtpFromIndex(1);
	J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
	frameCtrl->setRate(0.5f * (2.0f * SMSGetAnmFrameRate()));
}

void TBathtubPeach::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSpineEnemy::perform(flags, graphics);
}

Mtx* TBathtubPeach::getRootJointMtx() const
{
	return (Mtx*)((u8*)getModel() + 0x20);
}

BOOL TBathtubPeach::receiveMessage(THitActor* sender, u32 message)
{
	return TSpineEnemy::receiveMessage(sender, message);
}

void TBathtubPeach::calcRootMatrix()
{
	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	Mtx* dst;
	JDrama::TNameRef* bathtubRef = root->searchF(
        JDrama::TNameRef::calcKeyCode("バスタブ"), "バスタブ");
	TBathtub* bathtub = (TBathtub*)bathtubRef;
	if (bathtubRef && bathtub->getUnk29A()) {
		dst = (Mtx*)((u8*)getModel() + 0x20);
		PSMTXCopy(bathtub->getPeachMtxInDemo(), *dst);
	} else {
		TLiveActor::calcRootMatrix();
	}
}

TBathtubPeachManager::TBathtubPeachManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TBathtubPeachManager::createEnemyInstance() { return nullptr; }

void TBathtubPeachManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "ahiru_peach.bmd", 0x14240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBathtubPeachManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38 = new TBathtubPeachParams("/enemy/bathtubpeach.prm");
}
