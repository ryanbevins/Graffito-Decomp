#include <Enemy/FruitsBoat.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
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
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <JSystem/JMath.hpp>
#include <dolphin/mtx.h>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

static inline JGeometry::TVec3<f32> makeVec3(f32 x, f32 y, f32 z)
{
	return JGeometry::TVec3<f32>(x, y, z);
}

static const f32 dummy2333[3] = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2335[3] = { 1.0f, 1.0f, 1.0f };

DEFINE_NERVE(TNerveFruitsBoatBckTrace, TLiveActor)
{
	TFruitsBoat* self = (TFruitsBoat*)spine->getBody();

	self->mBckFrameCtrl->update();
	self->mBckAnm->setFrame(self->mBckFrameCtrl->getFrame());

	J3DTransformInfo info1;
	J3DTransformInfo info0;

	self->mBckAnm->getTransform(0, &info0);
	self->mBckAnm->getTransform(1, &info1);

	self->mPosition.x = info0.mTranslate.x + info1.mTranslate.x;
	self->mPosition.y = info0.mTranslate.y + info1.mTranslate.y;
	self->mPosition.z = info0.mTranslate.z + info1.mTranslate.z;

	self->mRotation.x
	    = (info0.mRotation.x + info1.mRotation.x) * (1.0f / 182.04445f);
	self->mRotation.y
	    = (info0.mRotation.y + info1.mRotation.y) * (1.0f / 182.04445f);
	self->mRotation.z
	    = (info0.mRotation.z + info1.mRotation.z) * (1.0f / 182.04445f);

	self->mScaling.x = info0.mScale.x * info1.mScale.x;
	self->mScaling.y = info0.mScale.y * info1.mScale.y;
	self->mScaling.z = info0.mScale.z * info1.mScale.z;

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

		JGeometry::TVec3<f32> dir(1.0f * JMASin(self->mRotation.y), 0.0f,
		                          1.0f * JMACos(self->mRotation.y));
		self->goToDirectedNextGraphNode(dir);

		if (self->mLiveFlag & 0x10000) {
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
				f32 w = callMsWrap(self->mRotation.y + 180.0f, 0.0f,
				                    360.0f);
				self->mRotation.y = w;
			}
		} else {
			self->walkToCurPathNode(self->mMarchSpeed, self->mTurnSpeed, 0.0f);
		}

		if (gpMSound->gateCheck(0x302e))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x302e, (Vec*)&self->mPosition.x, 0, nullptr, 0, 4);

		spine->pushAfterCurrent(&TNerveFruitsBoatGraphWander::theNerve());
		return TRUE;
	}

	if (self->mLiveFlag & 0x10000)
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
				f32 w = callMsWrap(self->mRotation.y + 180.0f, 0.0f,
				                    360.0f);
				self->mRotation.y = w;
			}
		} else {
			self->walkToCurPathNode(self->mMarchSpeed, self->mTurnSpeed, 0.0f);
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
	switch (mBoatId) {
	case 0: {
		static const TModelDataLoadEntry entry[] = {
			{ "ShipDolpic.bmd", 0x10210000, 0 },
			{ nullptr, 0, 0 },
		};
		createModelDataArray(entry);
		break;
	}
	case 1: {
		static const TModelDataLoadEntry entry[] = {
			{ "ShipDolpic2.bmd", 0x10210000, 0 },
			{ nullptr, 0, 0 },
		};
		createModelDataArray(entry);
		break;
	}
	case 2: {
		static const TModelDataLoadEntry entry[] = {
			{ "ShipDolpic3.bmd", 0x10210000, 0 },
			{ nullptr, 0, 0 },
		};
		createModelDataArray(entry);
		break;
	}
	case 3:
	default: {
		static const TModelDataLoadEntry entry[] = {
			{ "ShipDolpic4.bmd", 0x10210000, 0 },
			{ nullptr, 0, 0 },
		};
		createModelDataArray(entry);
		break;
	}
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

	int i = 0;
	while (i < table->unk0) {
		if (strcmp(name, table->unk8[i]) == 0) {
			mBckAnm = table->getAnmPtr(i);

			mBckFrameCtrl = new J3DFrameCtrl(0);
			mBckFrameCtrl->init(((s16*)mBckAnm)[1]);
			mBckFrameCtrl->setAttribute(((u8*)mBckAnm)[0]);

			f32 r = getSaveParam2()->mSLBckMoveSpeed.get();
			mBckFrameCtrl->setRate(r * SMSGetAnmFrameRate());
			return 0;
		}
		i += 1;
	}
	return -1;
}

void TFruitsBoat::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	char buf1[256];
	stream.readString(buf1, 256);
	JDrama::TNameRef* root = JDrama::TNameRefGen::instance->mRootNameRef;
	JDrama::TNameRef* host
	    = root->searchF(JDrama::TNameRef::calcKeyCode(buf1), buf1);

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
	offHitFlag(1);

	getTracer()->mPrevIdx = -1;
	goToShortestNextGraphNode();

	{
		TGraphTracer* tr = getTracer();
		TGraphWeb* gr    = tr->getGraph();
		if (gr->unk14) {
			mPosition = gr->indexToPoint(tr->mCurrIdx);

			TGraphTracer* tr2 = getTracer();
			tr2->moveTo(tr2->unk0->getShortestNextIndex(
			    tr2->mCurrIdx, tr2->mPrevIdx, 0xffffffff));
		}
	}

	TFruitsBoatManager* mgr = (TFruitsBoatManager*)mManager;
	switch (mgr->mBoatId) {
	case 0:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboat", this);
		mMActor = mMActorKeeper->createMActor("ShipDolpic.bmd", 0);
		mMapCollisionManager->init("ShipDolpic.col", 1, nullptr);
		setAttackRadius(850.0f);
		setAttackHeight(600.0f);
		break;
	case 1:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboatb", this);
		mMActor = mMActorKeeper->createMActor("ShipDolpic2.bmd", 0);
		mMapCollisionManager->init("ShipDolpic2.col", 1, nullptr);
		setAttackRadius(750.0f);
		setAttackHeight(480.0f);
		break;
	case 2:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboatc", this);
		mMActor = mMActorKeeper->createMActor("ShipDolpic3.bmd", 0);
		mMapCollisionManager->init("ShipDolpic3.col", 1, nullptr);
		setAttackRadius(1000.0f);
		setAttackHeight(300.0f);
		break;
	default:
		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/fruitsboatd", this);
		mMActor = mMActorKeeper->createMActor("ShipDolpic4.bmd", 0);
		mMapCollisionManager->init("ShipDolpic4.col", 1, nullptr);
		setAttackRadius(760.0f);
		setAttackHeight(270.0f);
		break;
	}

	{
		TMapCollisionManager* mgr = mMapCollisionManager;
		Mtx mtx;
		MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
		            mRotation.y, mRotation.z, mScaling.x, mScaling.y,
		            mScaling.z);
		TMapCollisionBase* base = mgr->getUnk8();
		PSMTXCopy(mtx, base->unk20);
		base->setUp();
	}

	mSpine->initWith(&TNerveFruitsBoatGraphWander::theNerve());

	{
		TGraphTracer* tr = getTracer();
		TGraphWeb* g     = tr->unk0;
		if (!g)
			mLiveFlag |= 0x10000;
		else if (g->unk0[tr->mCurrIdx].unk0->mFlags & 0x80)
			mLiveFlag |= 0x10000;
		else
			mLiveFlag &= ~0x10000;
	}

	mMarchSpeed = getSaveParam2()->mSLMoveSpeed.get();
	mTurnSpeed  = getSaveParam2()->mSLRotSpeed.get();

	mLiveFlag &= ~0x4;
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

	*(Vec*)((u8*)model + 0x14) = (Vec&)mScaling;
}

void TFruitsBoat::setGroundCollision()
{
	JGeometry::TVec3<f32> diff = mPosition;
	diff.x -= gpMarioPos->x;
	diff.y -= gpMarioPos->y;
	diff.z -= gpMarioPos->z;

	if (mColCount == 0) {
		f32 dist = diff.length();
		if (!(dist < 1000.0f)) {
			void* y = SMS_GetYoshi();
			int hasYoshi;
			if (!*(u8*)y)
				hasYoshi = 0;
			else
				hasYoshi = 1;

			if (hasYoshi) {
				if (!(mPosition.x - 1000.0f
				      < *(f32*)((u8*)SMS_GetYoshi() + 0x20)))
					return;
				if (!(mPosition.x + 1000.0f
				      > *(f32*)((u8*)SMS_GetYoshi() + 0x20)))
					return;
				if (!(mPosition.z - 1000.0f
				      < *(f32*)((u8*)SMS_GetYoshi() + 0x28)))
					return;
				if (!(mPosition.z + 1000.0f
				      > *(f32*)((u8*)SMS_GetYoshi() + 0x28)))
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
	u32 f = mLiveFlag;
	if (f & 0xb)
		return;

	if ((f & 0x204) == 0 || (f & 0x400) != 0) {
		TCircleShadowRequest req;
		req.unk0  = mPosition;
		req.unkC  = unk154;
		req.unk10 = unk158;
		req.unk1C = 3;
		req.unk14 = (s16)(s32)mRotation.y;

		if (mLiveFlag & 0x400) {
			u32 actorType = mActorType;
			gpBindShadowManager->forceRequest(req, actorType);
		} else {
			u32 actorType = mActorType;
			gpBindShadowManager->request(req, actorType);
		}
	}

	if (mLiveFlag & 0x204)
		return;

	bool hasOwner;
	if (mActorType & 0x40000000)
		hasOwner = true;
	else
		hasOwner = false;
	if (hasOwner)
		return;

	gpQuestionManager->request(mPosition, mScaledBodyRadius);
}

static JGeometry::TVec3<f32> up;
static JGeometry::TVec3<f32> up2733;
static bool upInitialized;
static bool up2733Initialized;

void TFruitsBoat::moveObject()
{
	// 1) Tilt from wave heights at two probe points along the boat's yaw.
	JGeometry::TVec3<f32> dirVec;
	f32 yawShort = mRotation.y * 182.04445f;
	dirVec = makeVec3(JMASSin((s16)(s32)yawShort) * 300.0f, 0.0f,
	                  JMASCos((s16)(s32)yawShort) * 300.0f);

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
	f32 wrapped = callMsWrap(rotXDeg, mRotation.x - 180.0f,
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

	if (!(mLiveFlag & 0x20000)) {
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
				if (!upInitialized) {
					up.set(0.0f, 1.0f, 0.0f);
					upInitialized = true;
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

				mWaveNormalX = nv.x;
				mWaveNormalY = nv.y;
				mWaveNormalZ = nv.z;
				mSwayVel += 0.0003f * len;
				mLiveFlag |= 0x20000;
				mLiveFlag &= ~0x10000;

				if ((gp->mBGType == 7 || gp->mBGType == 0x8007) ? true
				                                                   : false) {
					const char* bck = nullptr;
					switch (((TFruitsBoatManager*)mManager)->mBoatId) {
					case 0:
						bck = "shipdolpic";
						break;
					case 1:
						bck = "shipdolpic2";
						break;
					case 2:
						bck = "shipdolpic3";
						break;
					}

					if (bck) {
						if (!mMActor->checkCurAnm(bck, 0)
						    || mMActor->curAnmEndsNext(0, nullptr))
							mMActor->setBck(bck);
					}
					mLiveFlag &= ~0x20000;
				}
			}
		}
	} else {
		if (!(gp && gp->mActor == this && SMS_IsMarioTouchGround4cm()))
			mLiveFlag &= ~0x20000;
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
			if (!up2733Initialized) {
				up2733.set(0.0f, 1.0f, 0.0f);
				up2733Initialized = true;
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
	mSwayVel     = 0.01f * -sinShort + mSwayVel;
	mSwayAngle += mSwayVel;
	if (mSwayAngle < -10.0f || mSwayAngle > 10.0f)
		mSwayVel = -mSwayVel;
	mSwayVel *= 0.99f;

	TLiveActor::moveObject();
}
