#include <Enemy/FruitsBoat.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Map/MapCollisionManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/question.hpp>
#include <System/Application.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/JMath.hpp>
#include <dolphin/mtx.h>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

DEFINE_NERVE(TNerveFruitsBoatBckTrace, TLiveActor)
{
	TFruitsBoat* self = (TFruitsBoat*)spine->getBody();

	self->mBckFrameCtrl->update();
	// J3DAnm-base: write current frame to offset 0x10 (mFrame surrogate)
	// the structurally-correct field on the anim copies the frame down.
	*(f32*)((u8*)self->mBckAnm + 0x4) = self->mBckFrameCtrl->getFrame();

	JGeometry::TVec3<f32> v0;
	JGeometry::TVec3<f32> v1;
	S16Vec r0;
	S16Vec r1;

	// virtual call vtable[3] (offset 0xc): getTransform(idx, vec, s16vec)
	{
		typedef void (*GetT)(J3DAnmBase*, int, JGeometry::TVec3<f32>*,
		                     S16Vec*);
		J3DAnmBase* anm = self->mBckAnm;
		GetT fn         = (GetT)((u32*)(*(u32**)anm))[3];
		fn(anm, 0, &v0, &r0);
		fn(anm, 1, &v1, &r1);
	}

	self->mPosition.x = v0.x + v1.x;
	self->mPosition.y = v0.y + v1.y;
	self->mPosition.z = v0.z + v1.z;

	self->mRotation.x = (s16)(r0.x + r1.x) * (1.0f / 182.04445f);
	self->mRotation.y = (s16)(r0.y + r1.y) * (1.0f / 182.04445f);
	self->mRotation.z = (s16)(r0.z + r1.z) * (1.0f / 182.04445f);

	self->mScaling.x = v0.x * v1.x;
	self->mScaling.y = v0.y * v1.y;
	self->mScaling.z = v0.z * v1.z;

	return FALSE;
}

DEFINE_NERVE(TNerveFruitsBoatGraphWander, TLiveActor)
{
	TFruitsBoat* self = (TFruitsBoat*)spine->getBody();
	TGraphWeb* graph  = self->getTracer()->getGraph();

	if (!graph || graph->isDummy())
		return FALSE;

	if (self->isReachedToGoal()) {
		TGraphTracer* tr  = self->getTracer();
		TGraphNode& node  = tr->getCurrent();
		TRailNode* rn     = node.unk0;

		if (rn->mFlags & 0x100)
			self->mLiveFlag |= 0x10000;

		if (rn->mFlags & 0x400) {
			s16 v             = self->mAttrFlag;
			self->mAttrFlag   = (u16)(v ^ 1);
		}

		JGeometry::TVec3<f32> dir;
		dir.x = 1.0f * JMASin(self->mRotation.y);
		dir.y = 0.0f;
		dir.z = 1.0f * JMACos(self->mRotation.y);
		self->goToDirectedNextGraphNode(dir);

		if (self->mLiveFlag & 0x8000) {
			// skip
		} else if (graph->unk14) {
			f32 sp    = tr->calcSplineSpeed(self->mMarchSpeed);
			f32 saved = sp;
			tr->traceSpline(sp);

			JGeometry::TVec3<f32> p;
			JGeometry::TVec3<f32> r;
			graph->unk14->getPosAndRot(*(f32*)((u8*)tr + 0x14), &p, &r);
			p.x -= self->mPosition.x;
			p.y -= self->mPosition.y;
			p.z -= self->mPosition.z;

			self->mLinearVelocity.x += p.x;
			self->mLinearVelocity.y += p.y;
			self->mLinearVelocity.z += p.z;

			self->mRotation.y = r.x;

			if (saved < 0.0f) {
				f32 w = MsWrap<f32>(self->mRotation.y + 180.0f, 0.0f,
				                    360.0f);
				self->mRotation.y = w;
			}
		} else {
			self->walkToCurPathNode(self->mTurnSpeed, 0.0f, 0.0f);
		}

		if (gpMSound->gateCheck(0x302e))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x302e, (Vec*)&self->mPosition.x, 0, nullptr, 0, 4);

		spine->pushAfterCurrent(&TNerveFruitsBoatGraphWander::theNerve());
		return TRUE;
	}

	if (self->mLiveFlag & 0x8000)
		return FALSE;

	{
		TGraphTracer* tr = self->getTracer();
		if (graph->unk14) {
			f32 sp    = tr->calcSplineSpeed(self->mMarchSpeed);
			f32 saved = sp;
			tr->traceSpline(sp);

			JGeometry::TVec3<f32> p;
			JGeometry::TVec3<f32> r;
			graph->unk14->getPosAndRot(*(f32*)((u8*)tr + 0x14), &p, &r);
			p.x -= self->mPosition.x;
			p.y -= self->mPosition.y;
			p.z -= self->mPosition.z;

			self->mLinearVelocity.x += p.x;
			self->mLinearVelocity.y += p.y;
			self->mLinearVelocity.z += p.z;

			self->mRotation.y = r.x;

			if (saved < 0.0f) {
				f32 w = MsWrap<f32>(self->mRotation.y + 180.0f, 0.0f,
				                    360.0f);
				self->mRotation.y = w;
			}
		} else {
			self->walkToCurPathNode(self->mTurnSpeed, 0.0f, 0.0f);
		}
	}

	if (gpMSound->gateCheck(0x302e))
		MSoundSESystem::MSoundSE::startSoundActor(
		    0x302e, (Vec*)&self->mPosition.x, 0, nullptr, 0, 4);

	return FALSE;
}

TFruitsBoatParams::TFruitsBoatParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLMoveSpeed, 4.0f)
    , PARAM_INIT(mSLRotSpeed, 0.1f)
    , PARAM_INIT(mSLBckMoveSpeed, 0.2f)
{
	TParams::load(mPrmPath);
}

TFruitsBoatManager::TFruitsBoatManager(int id, const char* name)
    : TEnemyManager(name)
    , mBoatId(id)
{
}

void TFruitsBoatManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TFruitsBoatParams("/enemy/fruitsBoat.prm");
	TEnemyManager::load(stream);
}

void TFruitsBoatManager::createModelData()
{
	static const TModelDataLoadEntry entry0[] = {
		{ "ShipDolpic.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	static const TModelDataLoadEntry entry1[] = {
		{ "ShipDolpic2.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	static const TModelDataLoadEntry entry2[] = {
		{ "ShipDolpic3.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	static const TModelDataLoadEntry entry3[] = {
		{ "ShipDolpic4.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};

	switch (mBoatId) {
	case 0:
		createModelDataArray(entry0);
		break;
	case 1:
		createModelDataArray(entry1);
		break;
	case 2:
		createModelDataArray(entry2);
		break;
	default:
		createModelDataArray(entry3);
		break;
	}
}

TSpineEnemy* TFruitsBoatManager::createEnemyInstance() { return nullptr; }

TFruitsBoat::TFruitsBoat(const char* name)
    : TSpineEnemy(name)
    , mAttrFlag(0)
    , unk154(800.0f)
    , unk158(800.0f)
    , mBckAnm(nullptr)
    , mBckFrameCtrl(nullptr)
    , mWaveNormalX(1.0f)
    , mWaveNormalY(0.0f)
    , mWaveNormalZ(0.0f)
    , mSwayAngle(0.0f)
    , mSwayVel(0.0f)
{
	mLiveFlag |= 0x10;
}

BOOL TFruitsBoat::receiveMessage(THitActor* sender, u32 message) { return FALSE; }

Mtx* TFruitsBoat::getRootJointMtx() const
{
	return (Mtx*)(*(u8**)((u8*)mMActor->unk4 + 0x58));
}

int TFruitsBoat::setBckTrack(const char* name)
{
	MActorAnmDataEach<J3DAnmTransformKey>* table
	    = mManager->getMActorAnmData()->getUnk2C();

	int i;
	for (i = 0; i < table->unk0; i++) {
		if (strcmp(name, table->unk8[i]) == 0)
			break;
	}
	if (i >= table->unk0)
		return -1;

	if (i < table->unk0)
		mBckAnm = table->unkC[i];
	else
		mBckAnm = nullptr;

	mBckFrameCtrl = new J3DFrameCtrl(0);
	mBckFrameCtrl->init(((s16*)mBckAnm)[1]);
	mBckFrameCtrl->setAttribute(((u8*)mBckAnm)[0]);

	f32 anmRate = SMSGetAnmFrameRate();
	mBckFrameCtrl->setRate(getSaveParam2()->mSLBckMoveSpeed.get() * anmRate);
	return 0;
}

void TFruitsBoat::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	char buf1[256];
	stream.readString(buf1, 256);
	JDrama::TNameRef* host
	    = JDrama::TNameRefGen::instance->mRootNameRef->searchF(
	        JDrama::TNameRef::calcKeyCode(buf1), buf1);

	char buf2[256];
	stream.readString(buf2, 256);

	TGraphWeb* graph = gpConductor->getGraphByName(buf2);
	getTracer()->setGraph(graph);
	mGroundPlane = TMap::getIllegalCheckData();

	init((TLiveManager*)host);

	if (!graph || graph->isDummy()) {
		if (setBckTrack(buf2) == 0)
			mSpine->initWith(&TNerveFruitsBoatBckTrace::theNerve());
	}
}

void TFruitsBoat::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(mManager, 1);

	initHitActor(0x4000007b, 1, 0xc0000000, 0.0f, 0.0f, 0.0f, 0.0f);
	mActorType &= ~1;

	getTracer()->mPrevIdx = -1;
	goToShortestNextGraphNode();

	if (getTracer()->getGraph()) {
		JGeometry::TVec3<f32> pt
		    = getTracer()->getGraph()->indexToPoint(getTracer()->mCurrIdx);
		mPosition = pt;

		TGraphTracer* tr = getTracer();
		tr->moveTo(tr->unk0->getShortestNextIndex(tr->mCurrIdx, tr->mPrevIdx,
		                                          0xffffffff));
	}

	TFruitsBoatManager* mgr = (TFruitsBoatManager*)mManager;
	switch (mgr->mBoatId) {
	case 0:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboat", this);
		mMActor = mMActorKeeper->createMActor("shipdolpic", 0);
		mMapCollisionManager->init("ShipDolpic.col", 1, nullptr);
		setDamageRadius(850.0f);
		setDamageHeight(600.0f);
		break;
	case 1:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboatb", this);
		mMActor = mMActorKeeper->createMActor("shipdolpic2", 0);
		mMapCollisionManager->init("ShipDolpic2.col", 1, nullptr);
		setDamageRadius(750.0f);
		setDamageHeight(480.0f);
		break;
	case 2:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboatc", this);
		mMActor = mMActorKeeper->createMActor("shipdolpic3", 0);
		mMapCollisionManager->init("ShipDolpic3.col", 1, nullptr);
		setDamageRadius(1000.0f);
		setDamageHeight(300.0f);
		break;
	default:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboatd", this);
		mMActor = mMActorKeeper->createMActor("shipdolpic4", 0);
		mMapCollisionManager->init("ShipDolpic4.col", 1, nullptr);
		setDamageRadius(760.0f);
		setDamageHeight(270.0f);
		break;
	}

	{
		Mtx mtx;
		MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
		            mRotation.y, mRotation.z, mScaling.x, mScaling.y,
		            mScaling.z);
		PSMTXCopy(mtx,
		          *(MtxPtr*)((u8*)mMapCollisionManager->getUnk8() + 0x20));
		// call vtable[6] of TMapCollisionBase (calc)
		typedef void (*FN)(void*);
		void* base = mMapCollisionManager->getUnk8();
		FN fn      = (FN)(((u32*)(*(u32**)base))[6]);
		fn(base);
	}

	mSpine->initWith(&TNerveFruitsBoatGraphWander::theNerve());

	mMarchSpeed = getSaveParam2()->mSLMoveSpeed.get();
	mTurnSpeed  = getSaveParam2()->mSLRotSpeed.get();

	mLiveFlag &= ~0x8;
	mLiveFlag |= 0x20;
	mLiveFlag &= ~0x100;

	mMActor->setLightType(2);

	calcRootMatrix();
	mMActor->calc();
}

void TFruitsBoat::calcRootMatrix()
{
	J3DModel* model = getModel();
	MtxPtr mtx      = (MtxPtr)((u8*)model + 0x20);
	MsMtxSetRotRPH(mtx, mRotation.x, mRotation.y, mRotation.z);

	Mtx tmp;
	f32 deg2rad = mSwayAngle * 0.017453292f;
	PSMTXRotAxisRad(tmp, (Vec*)&mWaveNormalX, deg2rad);
	PSMTXConcat(tmp, mtx, mtx);
	PSMTXTransApply(mtx, mtx, mPosition.x, mPosition.y, mPosition.z);

	// copy scale into model[0x14..0x1c]
	*(u32*)((u8*)model + 0x14) = *(u32*)&mScaling.x;
	*(u32*)((u8*)model + 0x18) = *(u32*)&mScaling.y;
	*(u32*)((u8*)model + 0x1c) = *(u32*)&mScaling.z;
}

void TFruitsBoat::setGroundCollision()
{
	JGeometry::TVec3<f32> diff;
	diff.x = mPosition.x - gpMarioPos->x;
	diff.y = mPosition.y - gpMarioPos->y;
	diff.z = mPosition.z - gpMarioPos->z;

	if (mColCount == 0) {
		f32 lenSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		f32 dist  = JGeometry::TUtil<f32>::sqrt(lenSq);
		if (dist >= 1000.0f) {
			void* y = SMS_GetYoshi();
			bool hasYoshi;
			if (!*(u8*)y)
				hasYoshi = false;
			else
				hasYoshi = true;

			if (hasYoshi) {
				if (mPosition.x - 1000.0f >= *(f32*)((u8*)SMS_GetYoshi() + 0x20))
					return;
				if (mPosition.x + 1000.0f <= *(f32*)((u8*)SMS_GetYoshi() + 0x20))
					return;
				if (mPosition.z - 1000.0f >= *(f32*)((u8*)SMS_GetYoshi() + 0x28))
					return;
				if (mPosition.z + 1000.0f <= *(f32*)((u8*)SMS_GetYoshi() + 0x28))
					return;
			}
		}
	}

	{
		J3DModel* model = getModel();
		MtxPtr modelMtx = *(MtxPtr*)((u8*)model + 0x58);
		void* base      = mMapCollisionManager->getUnk8();
		if (base) {
			typedef void (*FN)(void*, MtxPtr);
			FN fn = (FN)(((u32*)(*(u32**)base))[5]);
			fn(base, modelMtx);
		}
	}
}

void TFruitsBoat::requestShadow()
{
	if (mLiveFlag & 0xb)
		return;

	if ((mLiveFlag & 0x204) == 0 || (mLiveFlag & 0x400) == 0) {
		TCircleShadowRequest req;
		req.unk0.set(0.0f, 0.0f, 0.0f);
		req.unk0  = mPosition;
		req.unkC  = unk154;
		req.unk10 = unk158;
		req.unk14 = (s16)(s32)mRotation.x;
		req.unk1C = 3;

		if (mLiveFlag & 0x400)
			gpBindShadowManager->forceRequest(req, mActorType);
		else
			gpBindShadowManager->request(req, mActorType);
	}

	if (mLiveFlag & 0x204)
		return;

	if (mActorType & 0x40000000)
		return;

	JGeometry::TVec3<f32> pos = mPosition;
	gpQuestionManager->request(pos, mScaledBodyRadius);
}

static JGeometry::TVec3<f32> up;
static JGeometry::TVec3<f32> up2733;

void TFruitsBoat::moveObject()
{
	// 1) Tilt from wave heights at two probe points along the boat's yaw.
	JGeometry::TVec3<f32> dirVec;
	f32 yawShort = mRotation.y * 182.04445f;
	dirVec.set(0.01f * JMASSin((s16)(s32)yawShort) * 300.0f, 0.0f,
	           0.01f * JMASCos((s16)(s32)yawShort) * 300.0f);

	JGeometry::TVec3<f32> dirCopy = dirVec;
	(void)dirCopy;
	JGeometry::TVec3<f32> p1 = mPosition;
	p1.x += dirVec.x;
	p1.y += dirVec.y;
	p1.z += dirVec.z;
	JGeometry::TVec3<f32> p2 = mPosition;
	p2.x -= dirVec.x;
	p2.y -= dirVec.y;
	p2.z -= dirVec.z;

	p1.y = gpMapObjWave->getWaveHeight(p1.x, p1.z);
	p2.y = gpMapObjWave->getWaveHeight(p2.x, p2.z);

	JGeometry::TVec3<f32> delta = p1;
	delta.sub(p2);

	JGeometry::TVec3<f32> rot = MsGetRotFromZaxis(delta);

	f32 rotXDeg = rot.x * 0.005493164f;
	f32 wrapped = MsWrap<f32>(rotXDeg, mRotation.x - 180.0f,
	                          mRotation.x + 180.0f);
	f32 diffAng = wrapped - mRotation.x;
	f32 clamped;
	if (diffAng > 1.0f)
		clamped = 1.0f;
	else if (diffAng < -1.0f)
		clamped = -1.0f;
	else
		clamped = diffAng;
	mRotation.x = mRotation.x + clamped;

	// 2) Mario-on-boat detection / wave-normal update.
	const TBGCheckData* gp = SMS_GetMarioGrPlane();

	if (!(mLiveFlag & 0x40000)) {
		if (gp && gp->mActor == this && SMS_IsMarioTouchGround4cm()) {
			JGeometry::TVec3<f32> mp = *gpMarioPos;
			mp.x -= mPosition.x;
			mp.y -= mPosition.y;
			mp.z -= mPosition.z;
			mp.y      = 0.0f;
			f32 lenSq = mp.x * mp.x + mp.y * mp.y + mp.z * mp.z;
			f32 len;
			if (lenSq <= 0.0f) {
				len = lenSq;
			} else {
				len = lenSq * JGeometry::TUtil<f32>::inv_sqrt(lenSq);
			}
			if (len != 0.0f) {
				if (!*(u8*)&up.x) {
					up.set(0.0f, 1.0f, 0.0f);
					*(u8*)&up.x = 1;
				}
				if (lenSq < 0.0000038146973f) {
					mp.set(0.0f, 0.0f, 0.0f);
				} else {
					f32 inv = JGeometry::TUtil<f32>::inv_sqrt(lenSq);
					mp.x *= inv;
					mp.y *= inv;
					mp.z *= inv;
				}
				// Cross product up x mp
				JGeometry::TVec3<f32> nv;
				nv.x = up.y * mp.z - up.z * mp.y;
				nv.y = up.z * mp.x - up.x * mp.z;
				nv.z = up.x * mp.y - up.y * mp.x;

				f32 nvLen = nv.x * nv.x + nv.y * nv.y + nv.z * nv.z;
				if (nvLen < 0.0000038146973f) {
					nv.set(0.0f, 0.0f, 0.0f);
				} else {
					f32 inv = JGeometry::TUtil<f32>::inv_sqrt(nvLen);
					nv.x *= inv;
					nv.y *= inv;
					nv.z *= inv;
				}

				mSwayAngle += 0.0003f * len;
				mLiveFlag |= 0x20000;
				mLiveFlag &= ~0x10000;
			}
		}
	} else {
		if (!(gp && gp->mActor == this && SMS_IsMarioTouchGround4cm()))
			mLiveFlag &= ~0x40000;
	}

	if (mLiveFlag & 0x20000) {
		JGeometry::TVec3<f32> mp = *gpMarioPos;
		mp.x -= mPosition.x;
		mp.y -= mPosition.y;
		mp.z -= mPosition.z;
		mp.y      = 0.0f;
		f32 lenSq = mp.x * mp.x + mp.y * mp.y + mp.z * mp.z;
		f32 len;
		if (lenSq <= 0.0f) {
			len = lenSq;
		} else {
			len = lenSq * JGeometry::TUtil<f32>::inv_sqrt(lenSq);
		}
		if (len != 0.0f) {
			if (lenSq < 0.0000038146973f) {
				mp.set(0.0f, 0.0f, 0.0f);
			} else {
				f32 inv = JGeometry::TUtil<f32>::inv_sqrt(lenSq);
				mp.x *= inv;
				mp.y *= inv;
				mp.z *= inv;
			}
			if (!*(u8*)&up2733.x) {
				up2733.set(0.0f, 1.0f, 0.0f);
				*(u8*)&up2733.x = 1;
			}
			JGeometry::TVec3<f32> nv;
			nv.x = up2733.y * mp.z - up2733.z * mp.y;
			nv.y = up2733.z * mp.x - up2733.x * mp.z;
			nv.z = up2733.x * mp.y - up2733.y * mp.x;

			f32 nvLenSq = nv.x * nv.x + nv.y * nv.y + nv.z * nv.z;
			if (nvLenSq < 0.0000038146973f) {
				nv.set(0.0f, 0.0f, 0.0f);
			} else {
				f32 r = JGeometry::TUtil<f32>::inv_sqrt(nvLenSq);
				nv.x *= r;
				nv.y *= r;
				nv.z *= r;
			}

			mWaveNormalX += (nv.x - mWaveNormalX) * 0.1f;
			mWaveNormalY += (nv.y - mWaveNormalY) * 0.1f;
			mWaveNormalZ += (nv.z - mWaveNormalZ) * 0.1f;
		}
	}

	// 3) Sway integration (pendulum)
	f32 sinShort = JMASSin((s16)(s32)(mSwayAngle * 182.04445f));
	mSwayVel     = 0.0003f * -sinShort + mSwayVel;
	mSwayAngle += mSwayVel;
	if (mSwayAngle < -10.0f || mSwayAngle > 10.0f)
		mSwayVel = -mSwayVel;
	mSwayVel *= 0.99f;

	TLiveActor::moveObject();
}

