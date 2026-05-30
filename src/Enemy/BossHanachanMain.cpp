#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossHanachanSaveParams.hpp>
#include <Enemy/BossHanachanSub.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAttach.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTexture.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Binder.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <dolphin/mtx.h>

const char* cSandPillarModelName = "sunabashira.bmd";
const char* cHitPoint1_RailName  = "bosshanachan2";
const char* cHitPoint2_RailName  = "bosshanachan1";
const char* cSandTextureName     = "suna";
const char* cDummyTextureName    = "M_dummy";

static const char* sChangeSaveFileName[] = {
	"/enemy/bosshanachan0.prm",
	"/enemy/bosshanachan1.prm",
	"/enemy/bosshanachan2.prm",
};

static const char* sCommonSaveFileName = "/enemy/bosshanachanCommon.prm";

static f32 getRotFromXZ(f32 x, f32 z)
{
	if (z == 0.0f) {
		if (x >= 0.0f)
			return 90.0f;
		return -90.0f;
	}
	if (z >= 0.0f)
		return matan(z, x) * (360.0f / 65536.0f);
	return 180.0f - matan(-z, x) * (360.0f / 65536.0f);
}

BOOL TBossHanachan::hasMapCollision() const { return TRUE; }

void TBossHanachan::removeAllMapCollision()
{
	mHead->mMapCollision->remove();
	for (int i = 0; i < 8; ++i)
		mBody[i]->mMapCollision->remove();
}

void TBossHanachan::execDamage()
{
	mSpine->reset();

	if (mHitPoints != 0)
		--mHitPoints;

	if (mHitPoints == 0) {
		((TBossHanachanPartsHead*)mHead)->mWaterHit->onHitFlag(1);
		for (int i = 0; i < 8; ++i) {
			TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
			body->mWaterHit->onHitFlag(1);
			body->mFeet[0]->onHitFlag(1);
			body->mFeet[1]->onHitFlag(1);
		}

		mSpine->setNext(&TNerveBossHanachanDead::theNerve());
		setAnmTimerWhenDead();
		unk1AC = *gpMarioPos;
		if (gpMSound->gateCheck(0x28E6))
			MSoundSESystem::MSoundSE::startSoundActor(0x28E6, &unk1AC, 0,
			                                          nullptr, 0, 4);
	} else {
		mSpine->setNext(&TNerveBossHanachanDamage::theNerve());
		setAnmTimerWhenDamage();

		TBossHanachanManager* manager = (TBossHanachanManager*)mManager;
		mChangeParams                 = manager->mChangeParams[3 - mHitPoints];

		const char* railName = cHitPoint1_RailName;
		if (mHitPoints == 2)
			railName = cHitPoint2_RailName;

		unk124->setGraph(gpConductor->getGraphByName(railName));
		mLiveFlag |= 0x20000;

		Vec* pos = (Vec*)&((TBossHanachanPartsBody*)mBody[unk174])->unk154;
		if (gpMSound->gateCheck(0x280F))
			MSoundSESystem::MSoundSE::startSoundActor(0x280F, pos, 0,
			                                          nullptr, 0, 4);
	}
}

void TBossHanachan::goToInitialRecoverGraphNode()
{
	unk124->mPrevIdx = -1;
	unk124->mCurrIdx = -1;

	int index = unk124->getGraph()->findNearestVisibleIndex(
	    mPosition, mRotation.y, mParams->mSLRecoverSearchDist.value,
	    mParams->mSLRecoverSearchDegree.value, 0xffffffff);
	if (index < 0) {
		goToShortestNextGraphNode();
	} else {
		unk124->setTo(index);
		setGoalPathFromGraph();
		unk128 = 0;
		unk12C = 0.0f;
	}
}

void TBossHanachan::execSlip()
{
	CLBChaseGeneralConstantSpecifySpeed<f32>(
	    &mMarchSpeed, 0.0f, mChangeParams->mSLMarchDecrease.value);
	mTurnSpeed = 0.1f;

	if (mMarchSpeed <= 0.001f)
		return;

	JGeometry::TVec3<f32> dir = unk188;
	if (mMarchSpeed > 4.0f) {
		f32 maxRot = getBodyMaxRotateZ();
		f32 yForce = 0.0f;
		f32 xSide  = 1.0f;
		if (maxRot > 0.0f) {
			yForce = -0.0f;
			xSide  = -1.0f;
		}

		s16 angle = CLBRoundf<s16>(mRotation.y * (65536.0f / 360.0f));
		f32 sinV = jmaSinTable[(u16)angle >> jmaSinShift];
		f32 cosV = jmaCosTable[(u16)angle >> jmaSinShift];
		f32 speed = 0.005f * mMarchSpeed;
		dir.x += (xSide * cosV + yForce * sinV) * speed;
		dir.y += yForce * speed;
		dir.z += (-xSide * sinV + yForce * cosV) * speed;
	}

	if (dir.squared() > 0.0000038146973f) {
		MsVECNormalize(&dir, &dir);
		dir.scale(500.0f);

		JGeometry::TVec3<f32> target = mPosition;
		target.add(dir);
		setGoalPath(TPathNode(target));
		walkToCurPathNode(mMarchSpeed, mTurnSpeed, 0.0f);
	}

	gpCameraShake->keepShake((EnumCamShakeMode)9, 1.0f);
	if (SMS_IsMarioTouchGround4cm() && mSpine->getTime() < 120)
		SMSRumbleMgr->start(0x16, (f32*)nullptr);
}

void TBossHanachan::execWalk(bool walk)
{
	if (walk) {
		CLBChaseGeneralConstantSpecifySpeed<f32>(
		    &mMarchSpeed, mChangeParams->mSLMaxMarchSpeed.value,
		    mChangeParams->mSLMarchAccel.value);
	} else {
		CLBChaseGeneralConstantSpecifySpeed<f32>(
		    &mMarchSpeed, 0.0f, mChangeParams->mSLMarchDecrease.value);
	}
	mTurnSpeed = mChangeParams->mSLWalkTurnSpeed.value;

	const JGeometry::TVec3<f32>& point = unkF4.getPoint();
	JGeometry::TVec3<f32> diff(point);
	diff.sub(mPosition);

	if (diff.squared() >= CLBSquared<f32>(10.0f))
		walkToCurPathNode(mMarchSpeed, mTurnSpeed, 0.0f);

	const JGeometry::TVec3<f32>& curPoint = unkF4.getPoint();
	JGeometry::TVec3<f32> curDiff(curPoint);
	curDiff.x -= mPosition.x;
	curDiff.y = 0.0f;
	curDiff.z -= mPosition.z;

	if (curDiff.squared() < CLBSquared<f32>(100.0f)) {
		if (!unk114.empty()) {
			unkF4 = unk114.pop();
		} else {
			goToDirLimitedNextGraphNode(90.0f);
		}
	}
}

f32 TBossHanachan::getBodyMaxRotateZ() const
{
	f32 maxRot = 0.0f;
	for (int i = 0; i < 8; ++i) {
		f32 rot = mBody[i]->mRotation.z;
		if (__fabsf(rot) > __fabsf(maxRot))
			maxRot = rot;
	}
	return maxRot;
}

bool TBossHanachan::checkFallDecideAndSetup()
{
	for (int i = 0; i < 8; ++i) {
		TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
		f32 absRot = body->mRotation.z;
		if (absRot < 0.0f)
			absRot = -absRot;

		if (absRot > mChangeParams->mSLFallDecideRotateZ.value) {
			emitOneTimeSandPillar_(body);
			if (body->mRotation.z > 0.0f)
				unk194 = 179.0f;
			else
				unk194 = -179.0f;

			f32 diff = body->unk13C - body->mRotation.z;
			if (diff < 0.0f)
				diff = -diff;
			unk198 = mChangeParams->mSLWaveFallDownSpeed.value * diff;
			if (unk198 < mChangeParams->mSLFallDecideMinSpeed.value)
				unk198 = mChangeParams->mSLFallDecideMinSpeed.value;
			return true;
		}
	}
	return false;
}

bool TBossHanachan::isTumbleCompletelyAllBody() const
{
	f32 rot = mBody[0]->mRotation.z;
	if (rot != -179.0f && rot != 179.0f)
		return false;

	for (int i = 1; i < 8; ++i)
		if (mBody[i]->mRotation.z != rot)
			return false;

	return true;
}

void TBossHanachan::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSpineEnemy::perform(flags, graphics);
}

void TBossHanachan::moveObject()
{
	updateSquareToMario();
	unk188 = mLinearVelocity;
	TLiveActor::moveObject();

	if (mSpine->getLatestNerve() != &TNerveBossHanachanGetUp::theNerve()) {
		CLBChaseDecrease(&mRotation.z, mBody[0]->mRotation.z, 0.04f, 0.0f);
	}

	mHead->mPosition    = mPosition;
	mHead->mRotation    = mRotation;
	mHead->mGroundPlane = mGroundPlane;
}

void TBossHanachan::bind()
{
	if (checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	if (mBinder != nullptr) {
		mBinder->bind(this);
		return;
	}

	JGeometry::TVec3<f32> oldPosition = mPosition;
	JGeometry::TVec3<f32> next        = mPosition;
	next.add(mLinearVelocity);
	next.add(mVelocity);

	mVelocity.y -= getGravityY();
	if (mVelocity.y < TLiveActor::mVelocityMinY)
		mVelocity.y = TLiveActor::mVelocityMinY;

	unk17C = next;
	f32 revX;
	f32 revZ;
	BHSCalcRevisionDistXZByRotateZ(mRotation.y, mSphereLink->m14,
	                               mRotation.z, &revX, &revZ);
	unk17C.x += revX;
	unk17C.z += revZ;

	mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
	    unk17C.x, unk17C.y + mHeadHeight, unk17C.z, &mGroundPlane);
	mGroundHeight += 1.0f;

	if (unk17C.y <= mGroundHeight + 0.05f) {
		if (mGroundPlane && !mGroundPlane->isIllegalData()) {
			offLiveFlag(LIVE_FLAG_AIRBORNE);
			mVelocity.set(0.0f, 0.0f, 0.0f);
			unk17C.y = mGroundHeight;
		}
	} else {
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	gpMap->isTouchedOneWallAndMoveXZ(&unk17C.x, unk17C.y + mHeadHeight,
	                                 &unk17C.z, mBodyRadius);

	JGeometry::TVec3<f32> moved = unk17C;
	moved.sub(oldPosition);
	JGeometry::TVec3<f32> oldToCur = oldPosition;
	oldToCur.sub(mPosition);
	mLinearVelocity = oldToCur;
	mLinearVelocity.add(moved);
}

void TBossHanachan::kill() { }

static void CalcRevisionPosByRotateZ(const JGeometry::TVec3<f32>& rot,
                                     f32 scale, Vec* pos)
{
	f32 absZ = __fabsf(rot.z);
	pos->y += scale * absZ;
	if (absZ > 90.0f) {
		f32 mag = 7.0f * (absZ - 90.0f);
		if (rot.z > 0.0f)
			mag = -mag;

		s16 angle = CLBRoundf<s16>(rot.y * (65536.0f / 360.0f));
		f32 sinV = jmaSinTable[(u16)angle >> jmaSinShift];
		f32 cosV = jmaCosTable[(u16)angle >> jmaSinShift];
		pos->x += mag * cosV + 0.0f * sinV;
		pos->z += -mag * sinV + 0.0f * cosV;
	}
}

void TBossHanachan::throwMario_(THitActor* hit_actor)
{
	JGeometry::TVec3<f32> throwVec = *gpMarioPos;
	throwVec.sub(hit_actor->mPosition);

	f32 throwPower;
	if (throwVec.squared() <= 0.0000038146973f) {
		throwVec.set(0.0f, 1.0f, 0.0f);
		throwPower = mMarchSpeed * mChangeParams->mSLThrowTotalPower.value;
	} else {
		f32 oldAngle = getRotFromXZ(unk188.x, unk188.z);
		f32 newAngle = getRotFromXZ(throwVec.x, throwVec.z);
		s16 oldShort = CLBRoundf<s16>(oldAngle * (65536.0f / 360.0f));
		s16 newShort = CLBRoundf<s16>(newAngle * (65536.0f / 360.0f));
		s16 diff     = oldShort - newShort;
		if (diff < 0)
			diff = -diff;

		throwPower = mMarchSpeed * mChangeParams->mSLThrowTotalPower.value;
		f32 ratio  = 1.0f - (diff * (1.0f / 32768.0f));
		f32 scale  = ratio * mChangeParams->mSLThrowMoveDirPower.value;
		JGeometry::TVec3<f32> add = unk188;
		add.scale(scale);
		throwVec.add(add);
		throwVec.y = mChangeParams->mSLThrowVecY.value;
	}

	if (throwPower > mChangeParams->mSLThrowSpeedMax.value)
		throwPower = mChangeParams->mSLThrowSpeedMax.value;
	else if (throwPower < mChangeParams->mSLThrowSpeedMin.value)
		throwPower = mChangeParams->mSLThrowSpeedMin.value;

	SMS_SendMessageToMario(mHead, HIT_MESSAGE_ATTACK);
	SMS_SendMessageToMario(mHead, HIT_MESSAGE_UNK7);
	SMS_ThrowMario(throwVec, throwPower);
	((TBossHanachanPartsHead*)mHead)->mWaterHit->onWaterHitCounter();
}

void TBossHanachan::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager, 10);
	mEffectActor  = mMActorKeeper->createMActor(cSandPillarModelName, 0);

	TBossHanachanManager* bossManager = (TBossHanachanManager*)manager;
	mParams                       = bossManager->mCommonParams;
	mChangeParams                 = bossManager->mChangeParams[0];
	mBodyScale                    = 1.0f;
	mBodyRadius                   = 350.0f;
	mWallRadius                   = mBodyRadius;
	mHeadHeight                   = 500.0f;
	mMarchSpeed                   = 0.0f;
	mGravity                      = 2.0f;
	mHitPoints                    = 3;
	mScaledBodyRadius             = 0.0f;
	mLiveFlag |= 0x1008;

	mSpine->initWith(&TNerveBossHanachanGraphWander::theNerve());
	unk124->mPrevIdx = -1;
	goToShortestNextGraphNode();

	initHitActor(0x08000014, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	onHitFlag(1);

	for (int i = 0; i < 8; ++i) {
		mBody[i] = new TBossHanachanPartsBody(this, "ボスハナチャンの体");
		((TBossHanachanPartsBody*)mBody[i])->unk114 = i;
	}
	mHead = new TBossHanachanPartsHead(this, "ボスハナチャンの頭");
	mMActor = mHead->mMActor;

	unk17C = mPosition;
	JGeometry::TVec3<f32> linkPos = unk17C;
	s16 angle = CLBRoundf<s16>(mRotation.y * (65536.0f / 360.0f));
	f32 sinV = jmaSinTable[(u16)angle >> jmaSinShift];
	f32 cosV = jmaCosTable[(u16)angle >> jmaSinShift];
	linkPos.x -= sinV * mParams->mSLBodyLength.value;
	linkPos.z -= cosV * mParams->mSLBodyLength.value;

	mSphereLink = new TSphereLink(8, linkPos, mParams->mSLBodyLength.value,
	                              mChangeParams->mSLSandSlopeForce.value, 0.2f,
	                              -2.0f, -3.5f, mRotation.y);

	mHead->mPosition    = mPosition;
	mHead->mRotation    = mRotation;
	mHead->mGroundPlane = mGroundPlane;

	for (int i = 0; i < 8; ++i) {
		TSpherePoint& point = mSphereLink->mPoints[i + 1];
		TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
		body->mPosition              = point.mPos;
		body->unk124                 = point.mPos.x;
		body->unk128                 = point.mPos.y;
		body->unk12C                 = point.mPos.z;
		body->unk130                 = body->unk124;
		body->unk134                 = body->unk128;
		body->unk138                 = body->unk12C;
		body->mRotation              = mRotation;
	}

	setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_OFF);

	JGeometry::TVec3<f32> headPos = mPosition;
	CalcRevisionPosByRotateZ(mRotation, mParams->mSLHeadPlusYByRotateZ.value,
	                         &headPos);
	CLBCalcRotateZXYTranslateMatrix(
	    mHead->mMActor->getModel()->getBaseTRMtx(), mRotation, headPos);
	mHead->mMActor->calc();

	for (int i = 0; i < 8; ++i) {
		TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
		JGeometry::TVec3<f32> pos   = body->mPosition;
		CalcRevisionPosByRotateZ(body->mRotation,
		                         mParams->mSLBodyPlusYByRotateZ.value, &pos);
		Mtx mtx;
		CLBCalcRotateZXYTranslateMatrix(mtx, body->mRotation, pos);
		PSMTXCopy(mtx, body->mMActor->getModel()->getBaseTRMtx());
		body->mMActor->calc();
	}

	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	mHead->initMapCollisionAndHitActor_(group);
	for (int i = 0; i < 8; ++i) {
		((TBossHanachanPartsBody*)mBody[i])
		    ->initMapCollisionAndHitActor_(group);
		((TBossHanachanPartsBody*)mBody[i])->initFootHitActor_(group);
	}
}

void TBossHanachan::setRandomWeakBodyIndex()
{
	unk174 = MsRandF() * 8.0f;
}

TBossHanachan::TBossHanachan(const char* name)
    : TSpineEnemy(name)
    , unk174(0)
    , mSphereLink(nullptr)
    , unk17C(0.0f, 0.0f, 0.0f)
    , unk188(0.0f, 0.0f, 0.0f)
    , unk194(0.0f)
    , unk198(0.0f)
    , mEffectActor(nullptr)
    , mEffectPos(0.0f, 0.0f, 0.0f)
    , unk1AC(0.0f, 0.0f, 0.0f)
    , unk1B8(-1)
    , mParams(nullptr)
    , mChangeParams(nullptr)
{
	setRandomWeakBodyIndex();
}

BOOL TBossHanachanManager::hasMapCollision() const { return TRUE; }

void TBossHanachanManager::clipEnemies(JDrama::TGraphics* graphics)
{
	clipActorsAux(graphics, mCommonParams->mSLViewClipFar.value,
	              mCommonParams->mSLViewClipRadius.value);
}

void TBossHanachanManager::loadAfter()
{
	J3DMaterialTable* table = gpMapObjManager->unkC0;
	int index              = table->getTextureName()->getIndex(cSandTextureName);
	ResTIMG* texture       = table->getTexture()->getResTIMG(index);

	for (int i = 0; i < 2; ++i) {
		SDLModelData* data = getModelDataKeeper()->getNthData(i);
		SMS_ChangeTextureAll(data->getModelData(), cDummyTextureName, *texture);
	}
}

void TBossHanachanManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "hanabody_model.bmd", 0x10300000, 0 },
		{ "hanahead_model.bmd", 0x10100000, 0 },
		{ nullptr, 0x10010000, 0 },
		{ nullptr, 0, 0 },
	};
	entry[2].unk0 = cSandPillarModelName;
	createModelDataArray(entry);
}

TBossHanachanManager::TBossHanachanManager(const char* name)
    : TEnemyManager(name)
    , mCommonParams(nullptr)
{
	mCommonParams = new TBossHanachanCommonSaveParams(sCommonSaveFileName);
	for (int i = 0; i < 3; ++i)
		mChangeParams[i]
		    = new TBossHanachanChangeSaveParams(sChangeSaveFileName[i]);
}
