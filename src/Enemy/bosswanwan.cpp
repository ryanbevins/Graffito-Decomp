#include <Enemy/BossWanwan.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>

static const char* bwanwan_bastable[] = {
	"/scene/bwanwan/bas/bwanwan_bark.bas",
	nullptr,
	"/scene/bwanwan/bas/bwanwan_shake.bas",
	nullptr,
	"/scene/bwanwan/bas/bwanwan_wait.bas",
	"/scene/bwanwan/bas/bwanwan_wait2.bas",
	nullptr,
};

static const TModelDataLoadEntry sModelDataEntries[] = {
	{ "bwanwan_body.bmd", 0x10220000, 0 },
	{ "bwanwan_chain.bmd", 0x10220000, 0 },
	{ "bwanwan_picket.bmd", 0x10220000, 0 },
	{ nullptr, 0, 0 },
};

static JGeometry::TVec3<f32> BW_BATH_POS(-1000.0f, 4.5f, -6217.2f);
static JGeometry::TVec3<f32> BW_PICKET_START(6012.84f, 0.0f, 7323.15f);
static JGeometry::TVec3<f32> BW_HEAD_START(5741.72f, -100.0f, 6311.62f);

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

TBWParams::TBWParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLMarchSpeed, 6.0f)
    , PARAM_INIT(mSLTurnSpeed, 1.0f)
    , PARAM_INIT(mSLLeashNodeLen, 120.0f)
    , PARAM_INIT(mSLPicketHeight, 100.0f)
    , PARAM_INIT(mSLPicketRadius, 100.0f)
    , PARAM_INIT(mSLChainHitHeight, 100.0f)
    , PARAM_INIT(mSLChainHitRadius, 100.0f)
    , PARAM_INIT(mSLChainGroundRadius, 60.0f)
    , PARAM_INIT(mSLPullLimit, 1.0f)
    , PARAM_INIT(mSLAttackSpeed, 10.0f)
    , PARAM_INIT(mSLStunTimer, 4000)
    , PARAM_INIT(mSLSearchLength, 10000.0f)
    , PARAM_INIT(mSLSearchAngle, 60.0f)
    , PARAM_INIT(mSLBWHitPointMax, (u8)0xff)
    , PARAM_INIT(mSLHeadGap, 150.0f)
    , PARAM_INIT(mSLShakeLengthMax, 3000.0f)
    , PARAM_INIT(mSLShakeLengthMaxHP0, 2000.0f)
{
	TParams::load(mPrmPath);
}

void TBWLeashNode::calcTemperature()
{
	if (mIndex == 0)
		return;

	int prevIndex     = mIndex - 1;
	TBWLeashNode* prev = mLeash->mNodes[prevIndex];
	f32 diff           = prev->unk74 - unk74;
	f32 step;
	if (diff < 0.0f) {
		if (diff < -0.1f)
			step = -0.02f;
		else
			step = -0.005f;
	} else {
		if (diff > 0.1f)
			step = 0.02f;
		else
			step = 0.005f;
	}

	unk74 += step;
	if (unk74 < 0.0f)
		unk74 = 0.0f;
	if (unk74 > 1.0f)
		unk74 = 1.0f;
}

void TBWLeashNode::calcMatrix()
{
	TRope* rope       = mLeash->mRope;
	TRopePoint* point = &rope->mPoints[mIndex];
	JGeometry::TVec3<f32> position = point->mPosition;
	MtxPtr mtx = mMActor->getModel()->getBaseTRMtx();

	JGeometry::TVec3<f32> direction;
	if (mIndex < rope->mNumPoints - 1) {
		direction = rope->mPoints[mIndex + 1].mPosition;
		direction.sub(position);
	} else {
		direction = rope->mPoints[mIndex - 1].mPosition;
		direction.sub(position);
		direction.negate();
	}

	PSVECNormalize((Vec*)&direction, (Vec*)&direction);

	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	JGeometry::TVec3<f32> side;
	side.cross(up, direction);
	PSVECNormalize((Vec*)&side, (Vec*)&side);
	up.cross(direction, side);
	PSVECNormalize((Vec*)&up, (Vec*)&up);

	mtx[0][2] = direction.x;
	mtx[1][2] = direction.y;
	mtx[2][2] = direction.z;

	if (mIndex & 1) {
		mtx[0][0] = side.x;
		mtx[1][0] = side.y;
		mtx[2][0] = side.z;
		mtx[0][1] = up.x;
		mtx[1][1] = up.y;
		mtx[2][1] = up.z;
	} else {
		mtx[0][0] = up.x;
		mtx[1][0] = up.y;
		mtx[2][0] = up.z;
		mtx[0][1] = side.x;
		mtx[1][1] = side.y;
		mtx[2][1] = side.z;
	}

	mtx[0][3] = position.x;
	mtx[1][3] = position.y + 30.0f;
	mtx[2][3] = position.z;
	mPosition = position;
}

void TBWLeashNode::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		calcTemperature();
		calcMatrix();

		if (mLeash->mOwner->mHitPoints != 0 && mIndex < 8) {
			for (int i = 0; i < mColCount; ++i) {
				THitActor* actor = mCollisions[i];
				if (actor->getActorType() == 0x80000001)
					actor->receiveMessage(this, HIT_MESSAGE_UNKA);
			}
		}
	}

	if (flags & 2) {
		J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(5);
		if (frameCtrl != nullptr) {
			f32 frame = unk74 * (f32)(frameCtrl->getEnd() - 1);
			f32 scale;
			if (mIndex < 5) {
				TBossWanwan* owner = mLeash->mOwner;
				u8 maxHP = ((TBWParams*)owner->getSaveParam())
				               ->mSLBWHitPointMax.get();
				scale = (f32)mIndex * 0.25f
				        + (f32)owner->mHitPoints / (f32)maxHP;
			} else {
				int pointCount = mLeash->mRope->mNumPoints;
				if (mIndex >= pointCount - 10)
					scale = (f32)(pointCount - mIndex) / 10.0f;
				else
					scale = 1.0f;
			}

			if (scale > 1.0f)
				scale = 1.0f;
			else if (scale < 0.0f)
				scale = 0.0f;

			frameCtrl->setFrame(frame * scale);
			frameCtrl->setRate(0.0f);
		}
	}

	if (mIndex < mLeash->mRope->mNumPoints - 1)
		mMActor->perform(flags, graphics);
}

TBWLeash::TBWLeash(TBossWanwan* owner, int node_count, const char* name)
    : JDrama::TViewObj(name)
    , mOwner(owner)
    , mRope(nullptr)
    , mNodes(nullptr)
{
	mRope = new TRope(node_count, mOwner->mPosition,
	                  ((TBWParams*)mOwner->getSaveParam())
	                      ->mSLLeashNodeLen.get(),
	                  mOwner->mTurnSpeed, 0.0f, 0.0f);
	mNodes = new TBWLeashNode*[node_count];
	for (int i = 0; i < node_count; ++i)
		mNodes[i] = new TBWLeashNode(this, i, "鎖部");
}

void TBWLeash::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		JGeometry::TVec3<f32> headPos;
		mOwner->getJointTransByIndex(5, &headPos);
		mRope->moveHead(headPos);

		mOwner->unk188 = 0;
		if (mOwner->unk17C != 0) {
			JGeometry::TVec3<f32> headBefore = mRope->mPoints[0].mPosition;
			mRope->constraintTail(mOwner->mPicket->mPosition);

			JGeometry::TVec3<f32> pull = mRope->mPoints[0].mPosition;
			pull.sub(headBefore);
			mOwner->unk15C = pull;

			JGeometry::TVec3<f32> toHead = mRope->mPoints[0].mPosition;
			JGeometry::TVec3<f32> ownerPos = mOwner->mPosition;
			ownerPos.y += 500.0f;
			toHead.sub(ownerPos);

			if (PSVECMag((Vec*)&toHead) > 650.0f) {
				PSVECNormalize((Vec*)&toHead, (Vec*)&toHead);
				toHead.scale(20.0f);
				toHead.y = 0.0f;
				mOwner->mPosition.add(toHead);
				mOwner->unk188 = 1;
			}

			f32 targetYaw;
			if (toHead.z == 0.0f) {
				if (toHead.x >= 0.0f)
					targetYaw = 90.0f;
				else
					targetYaw = -90.0f;
			} else if (toHead.z >= 0.0f) {
				targetYaw = matan(toHead.z, toHead.x) * (360.0f / 65536.0f);
			} else {
				f32 yaw = matan(-toHead.z, toHead.x) * (360.0f / 65536.0f);
				targetYaw = 180.0f - yaw;
			}

			targetYaw += 180.0f;
			while (targetYaw >= 360.0f)
				targetYaw -= 360.0f;
			while (targetYaw < 0.0f)
				targetYaw += 360.0f;

			f32 wrappedYaw = callMsWrap(mOwner->mRotation.y, targetYaw - 180.0f,
			                            targetYaw + 180.0f);
			f32 turn       = targetYaw - wrappedYaw;
			if (turn > 0.0f) {
				f32 turnMax = 1.5f * mOwner->mTurnSpeed;
				if (turn > turnMax)
					turn = turnMax;
			} else {
				f32 turnMin = 1.5f * -mOwner->mTurnSpeed;
				if (turn <= turnMin)
					turn = turnMin;
			}

			f32 newYaw = mOwner->mRotation.y + turn;
			while (newYaw >= 360.0f)
				newYaw -= 360.0f;
			while (newYaw < 0.0f)
				newYaw += 360.0f;
			mOwner->mRotation.y = newYaw;
		}
	}

	if (flags & 2) {
		JGeometry::TVec3<f32> bodyPos;
		mOwner->getJointTransByIndex(1, &bodyPos);
		for (int i = 0; i < 15; ++i) {
			TRopePoint& point = mRope->mPoints[i];
			JGeometry::TVec3<f32> offset = point.mPosition;
			offset.sub(bodyPos);
			f32 distance = PSVECMag((Vec*)&offset);
			if (distance < 500.0f) {
				offset.scale(500.0f / distance);
				JGeometry::TVec3<f32> position = bodyPos;
				position.add(offset);
				point.mVelocity.zero();
				point.mPosition = position;
				point.mPrevPos  = point.mPosition;
			}
		}
	}

	for (int i = 0; i < mRope->mNumPoints; ++i)
		mNodes[i]->testPerform(flags, graphics);
}

BOOL TBWPicket::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->getActorType() != 0x80000001)
		return FALSE;

	if (message == HIT_MESSAGE_HIP_DROP) {
		mOwner->unk17C = 1;
		mOwner->unk184 = 0;
		if (gpMSound->gateCheck(0x28C0))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28C0, &mPosition, 0, nullptr, 0, 4);
		return TRUE;
	}

	if (message == HIT_MESSAGE_TAKE) {
		if (mOwner->unk17C != 0) {
			JPABaseEmitter* emitter = gpMarioParticleManager->emit(
			    0xAE, &mOwner->mPicket->mPosition, 0, nullptr);
			if (emitter != nullptr) {
				JGeometry::TVec3<f32> scale(0.3f, 0.5f, 0.3f);
				emitter->setScale(scale);
			}
		}
		mOwner->unk194 = 0;
		mOwner->unk17C = 0;
		mHolder        = (TTakeActor*)sender;
		return TRUE;
	}

	if (message == HIT_MESSAGE_UNK7 || message == HIT_MESSAGE_UNK8) {
		mHolder = nullptr;
		return TRUE;
	}

	return FALSE;
}

bool TBWPicket::moveRequest(const JGeometry::TVec3<f32>& position)
{
	if (mOwner->mSpine->getLatestNerve()
	        == &TNerveBWJumpToBath::theNerve()
	    || mOwner->mSpine->getLatestNerve() == &TNerveBWDie::theNerve())
		return false;

	if (mOwner->mHitPoints != 0)
		return false;

	TRope* rope = mOwner->mLeash->mRope;
	JGeometry::TVec3<f32> tailBefore = rope->mPoints[0].mPosition;
	rope->constraintTail(position);
	mOwner->unk15C = rope->mPoints[0].mPosition;
	mOwner->unk15C.sub(tailBefore);
	return true;
}

MtxPtr TBWPicket::getTakingMtx() { return unk74; }

void TBWPicket::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		TRope* rope = mOwner->mLeash->mRope;
		mPosition  = rope->mPoints[rope->mNumPoints - 1].mPosition;

		ensureTakeSituation();
		if (mHolder != nullptr) {
			JGeometry::TVec3<f32> direction;
			direction.x = mHolder->mPosition.x;
			direction.y = mHolder->mPosition.y;
			direction.z = mHolder->mPosition.z;
			direction.sub(rope->mPoints[0].mPosition);

			JGeometry::TVec3<f32> up;
			up.x = 0.0f;
			up.y = 1.0f;
			up.z = 0.0f;

			JGeometry::TVec3<f32> side;
			side.cross(up, direction);
			PSVECNormalize((Vec*)&side, (Vec*)&side);
			up.cross(direction, side);
			PSVECNormalize((Vec*)&up, (Vec*)&up);
			direction.cross(side, up);
			PSVECNormalize((Vec*)&direction, (Vec*)&direction);

			unk74.mMtx[0][0] = side.x;
			unk74.mMtx[1][0] = side.y;
			unk74.mMtx[2][0] = side.z;
			unk74.mMtx[0][1] = up.x;
			unk74.mMtx[1][1] = up.y;
			unk74.mMtx[2][1] = up.z;
			unk74.mMtx[0][2] = direction.x;
			unk74.mMtx[1][2] = direction.y;
			unk74.mMtx[2][2] = direction.z;
			unk74.mMtx[0][3] = mPosition.x;
			unk74.mMtx[1][3] = mPosition.y;
			unk74.mMtx[2][3] = mPosition.z;

			mHolder->moveRequest(mPosition);
		}
	}

	if (flags & 2) {
		MtxPtr mtx = mMActor->getModel()->getBaseTRMtx();
		PSMTXIdentity(mtx);

		TRope* rope = mOwner->mLeash->mRope;
		JGeometry::TVec3<f32> direction;
		direction.x = mPosition.x;
		direction.y = mPosition.y;
		direction.z = mPosition.z;
		direction.sub(rope->mPoints[rope->mNumPoints - 3].mPosition);

		if (direction.squared() <= 0.0000038146973f) {
			direction.x = 0.0f;
			direction.y = 0.0f;
			direction.z = 1.0f;
		}

		JGeometry::TVec3<f32> up;
		up.x = 0.0f;
		up.y = 1.0f;
		up.z = 0.0f;

		JGeometry::TVec3<f32> side;
		side.cross(up, direction);
		PSVECNormalize((Vec*)&side, (Vec*)&side);
		up.cross(direction, side);
		PSVECNormalize((Vec*)&up, (Vec*)&up);
		direction.cross(side, up);
		PSVECNormalize((Vec*)&direction, (Vec*)&direction);

		mtx[0][0] = side.x;
		mtx[1][0] = side.y;
		mtx[2][0] = side.z;
		mtx[0][1] = up.x;
		mtx[1][1] = up.y;
		mtx[2][1] = up.z;
		mtx[0][2] = direction.x;
		mtx[1][2] = direction.y;
		mtx[2][2] = direction.z;
		mtx[0][3] = mPosition.x;
		mtx[1][3] = mPosition.y;
		mtx[2][3] = mPosition.z;

		if (mOwner->unk17C == 0)
			mtx[1][3] += 70.0f;

		if (mHolder != nullptr) {
			mtx[0][3] -= 50.0f * direction.x;
			mtx[2][3] -= 50.0f * direction.z;
		} else {
			mtx[0][3] -= 60.0f * direction.x;
			mtx[2][3] -= 60.0f * direction.z;
		}
	}

	mMActor->perform(flags, graphics);
	THitActor::perform(flags, graphics);
}

BOOL TBWHit::receiveMessage(THitActor* sender, u32 message)
{
	return mOwner->receiveMessage(sender, message);
}

void TBWHit::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		if (mJointIndex >= 0)
			mOwner->getJointTransByIndex(mJointIndex, &mPosition);

		for (int i = 0; i < mColCount; ++i) {
			THitActor* actor = mCollisions[i];
			if (mOwner->mHitPoints != 0
			    && actor->getActorType() == 0x80000001)
				actor->receiveMessage(mOwner, HIT_MESSAGE_UNKA);
		}
	}

	THitActor::perform(flags, graphics);
}

void TBWBinder::bind(TLiveActor* actor)
{
	actor->mPosition.add(actor->mLinearVelocity);
	actor->mPosition.add(actor->mVelocity);
}

void TBossWanwanMtxCalc::calc(u16 joint_no)
{
	if (joint_no == 0) {
		bool airborne;
		if (mOwner->checkLiveFlag(LIVE_FLAG_AIRBORNE))
			airborne = true;
		else
			airborne = false;

		if (airborne) {
			J3DTransformInfo info;

			j3dSys.setCurrentMtxCalc(this);
			if (unk54 != nullptr) {
				unk54->getTransform(joint_no, &info);
			} else {
				info = j3dSys.getModel()
				           ->getModelData()
				           ->getJointNodePointer(joint_no)
				           ->getTransformInfo();
			}

			info.mTranslate.x = 0.0f;
			info.mTranslate.y = 0.0f;
			info.mTranslate.z = 0.0f;
			calcTransform(joint_no, info);
			return;
		}
	}

	M3UMtxCalcSIAnmBlendQuat::calc(joint_no);

	if (joint_no == 1) {
		Mtx mtx;
		MtxPtr mtxPtr = mtx;
		f32 zero      = 0.0f;
		mtxPtr[2][3]  = zero;
		mtxPtr[1][3]  = zero;
		mtxPtr[0][3]  = zero;
		mtxPtr[2][2]  = zero;
		mtxPtr[1][2]  = zero;
		mtxPtr[0][2]  = zero;
		mtxPtr[2][1]  = zero;
		mtxPtr[1][1]  = zero;
		mtxPtr[0][1]  = zero;
		mtxPtr[2][0]  = zero;
		mtxPtr[1][0]  = zero;
		mtxPtr[0][0]  = zero;

		s16 angle = static_cast<s16>(mOwner->unk168 * (65536.0f / 360.0f));
		f32 sin   = JMASSin(angle);
		f32 cos   = JMASCos(angle);

		mtxPtr[0][0] = 1.0f;
		mtxPtr[0][1] = zero;
		mtxPtr[0][2] = zero;
		mtxPtr[0][3] = zero;
		mtxPtr[1][0] = zero;
		mtxPtr[1][1] = cos;
		mtxPtr[1][2] = -sin;
		mtxPtr[1][3] = zero;
		mtxPtr[2][0] = zero;
		mtxPtr[2][1] = sin;
		mtxPtr[2][2] = cos;
		mtxPtr[2][3] = zero;

		MtxPtr jointMtx = mOwner->getModel()->getAnmMtx(joint_no);
		PSMTXConcat(jointMtx, mtxPtr, jointMtx);
		PSMTXCopy(jointMtx, J3DSys::mCurrentMtx);
	}
}

TBossWanwan::TBossWanwan(const char* name)
    : TSpineEnemy(name)
    , mMtxCalc(nullptr)
    , mLeash(nullptr)
    , mPicket(nullptr)
    , unk168(0.0f)
    , unk16C(0)
    , mHeadHit(nullptr)
    , mBodyHit(nullptr)
    , unk17C(0)
    , unk180(0)
    , unk184(0)
    , unk188(0)
    , unk18C(0)
    , unk18D(0)
    , unk190(0)
    , unk194(1)
    , unk195(0)
    , unk198(0)
    , unk19C(0)
    , unk1A0(0)
    , unk1A4(0.0f, 0.0f, 0.0f)
    , unk1B0(0)
    , unk1B4(0)
{
	mBinder = new TBWBinder;
}

void TBossWanwan::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager, 0x11);
	mMActor       = mMActorKeeper->createMActor("bwanwan_body.bmd", 0);

	TGraphWeb* web = gpConductor->getGraphByName("bwanwan");
	web->initGoalIndex(BW_BATH_POS);
	unk124->setGraph(web);

	mLeash   = new TBWLeash(this, 15, "ボスワンワン鎖");
	mPicket  = new TBWPicket(this, "ボスワンワンつかみ");
	mSpine->initWith(&TNerveBWGraphWander::theNerve());

	TBWParams* params = (TBWParams*)getSaveParam();
	mMarchSpeed       = params->mSLMarchSpeed.get();
	mTurnSpeed        = params->mSLTurnSpeed.get();
	mPosition         = BW_HEAD_START;
	reset();

	mPicket->initHitActor(0x0800000D, 1, 0x80000000,
	                      params->mSLPicketRadius.get(),
	                      params->mSLPicketHeight.get(),
	                      params->mSLPicketRadius.get(),
	                      params->mSLPicketHeight.get());
	TIdxGroupObj* shadowGroup
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("影グループ");
	shadowGroup->add(mPicket);
	mPicket->offHitFlag(HIT_FLAG_NO_COLLISION);
	mPicket->mMActor
	    = mPicket->mOwner->mMActorKeeper->createMActor("bwanwan_picket.bmd", 0);
	mPicket->mPosition = BW_PICKET_START;

	unk17C = 1;
	unk184 = 0;
	goToRandomNextGraphNode();
	initHitActor(0x0800000B, 1, 0x80000000, 0.0f, 0.0f, 0.0f, 0.0f);

	mHeadHit = new TBWHit(this, 3, "ボスワンワンヒット");
	mHeadHit->initHitActor(0x0800000B, 3, 0xA0000000, 500.0f, 500.0f,
	                       450.0f, 500.0f);
	mBodyHit = new TBWHit(this, -1, "ボスワンワンヒット");
	mBodyHit->initHitActor(0x0800000B, 3, 0xA0000000, 300.0f, 500.0f,
	                       270.0f, 500.0f);

	shadowGroup->add(mHeadHit);
	mHeadHit->offHitFlag(HIT_FLAG_NO_COLLISION);
	shadowGroup->add(mBodyHit);
	mBodyHit->offHitFlag(HIT_FLAG_NO_COLLISION);

	initAnmSound();
	mScaledBodyRadius = 500.0f;

	mMtxCalc = new TBossWanwanMtxCalc(this);
	mMActor->setCalcForBck(mMtxCalc);
	mMActor->calc();
	offLiveFlag(LIVE_FLAG_UNK100);

	J3DAnmTransform* waitAnm
	    = mMActorKeeper->getMActorAnmData()->getUnk2C()->getAnmPtr(4);
	if (mMtxCalc->unk54 != waitAnm) {
		mMtxCalc->unk58 = mMtxCalc->unk54;
		mMtxCalc->unk54 = waitAnm;
		mMtxCalc->unk50 = 1.0f;
	}
	mMActor->getAnmBck()->setFrameCtrl(4);
	J3DFrameCtrl* frameCtrl = mMActor->getFrameCtrl(0);
	unk178 = 10.0f / (f32)frameCtrl->getEnd();
	setAnmSound(bwanwan_bastable[4]);
	mMActor->setBrkFromIndex(0);
	mMActor->setLightType(1);

	if (mMActor->getModel()->getSkinDeform() == nullptr)
		mMActor->getModel()->setSkinDeform(new J3DSkinDeform,
		                                    J3D_DEFORM_ATTACH_FLAG_UNK_1);

	mHitPoints = params->mSLBWHitPointMax.get();
	unk15C.zero();

	mMapCollisionManager = new TMapCollisionManager(1, "/scene/bwanwan", this);
	mMapCollisionManager->init("bwanwan_ofuro_col.col", 2, nullptr);

	TMapCollisionBase* collision = mMapCollisionManager->unk8;
	Mtx mtx;
	MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	            mRotation.y, mRotation.z, mScaling.x, mScaling.y, mScaling.z);
	PSMTXCopy(mtx, collision->unk20);
	collision->setUp();
	collision = mMapCollisionManager->unk8;
	if (collision != nullptr)
		collision->remove();
}

void TBossWanwan::shakeCamera(int mode)
{
	if (!SMS_IsMarioTouchGround4cm())
		return;

	f32 marioDist = JGeometry::TUtil<f32>::sqrt(mDistToMarioSquared);
	TBWParams* params = (TBWParams*)getSaveParam();
	f32 lengthMax     = params->mSLShakeLengthMax.get();
	f32 lengthMaxHP0  = params->mSLShakeLengthMaxHP0.get();
	f32 ratio;

	if (mMActor->checkCurBckFromIndex(0)) {
		ratio = 1.0f;
	} else {
		ratio = (f32)mHitPoints / (f32)params->mSLBWHitPointMax.get();
	}

	f32 length = lengthMax * ratio + lengthMaxHP0 * (1.0f - ratio);
	f32 power  = length - marioDist;
	if (power < 0.0f)
		return;

	power /= length;
	if (power > 1.0f)
		power = 1.0f;

	power *= ratio;
	gpCameraShake->startShake((EnumCamShakeMode)mode, power);
	SMSRumbleMgr->start(8, &mPosition);
}

BOOL TBossWanwan::receiveMessage(THitActor* sender, u32 message)
{
	u32 actorType = sender->getActorType();
	if (actorType == 0x80000001)
		return FALSE;

	if (actorType == 0x1000001) {
		if (unk18C)
			return TRUE;

		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);

		if (mHitPoints == 0) {
			if (gpMSound->gateCheck(0x28D1))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28D1, &mPosition, 0, nullptr, 0, 4);
		} else if (mHitPoints == 1) {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0xB0, getModel()->mNodeMatrices[1], 0, nullptr);
			if (gpMSound->gateCheck(0x28C5))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28C5, &mPosition, 0, nullptr, 0, 4);
		} else {
			if (gpMSound->gateCheck(0x28BE))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28BE, &mPosition, 0, nullptr, 0, 4);
		}

		if (mHitPoints != 0)
			--mHitPoints;

		++unk190;
		return TRUE;
	}

	if (actorType == 0x4000005A) {
		sender->receiveMessage(this, HIT_MESSAGE_HIP_DROP);
		mHitPoints = 0;
		++unk190;

		if (!unk1A0)
			++unk1A0;

		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0xB0, getModel()->mNodeMatrices[1], 0, nullptr);
		if (gpMSound->gateCheck(0x28C5))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x28C5, &mPosition, 0, nullptr, 0, 4);
	}

	return TSpineEnemy::receiveMessage(sender, message);
}

void TBossWanwan::calcRootMatrix()
{
	J3DModel* model = getModel();
	model->unk14.x  = mScaling.x;
	model->unk14.y  = mScaling.y;
	model->unk14.z  = mScaling.z;

	MsMtxSetXYZRPH(getModel()->getBaseTRMtx(), mPosition.x,
	               mPosition.y + 500.0f, mPosition.z, mRotation.x,
	               mRotation.y, mRotation.z);
}

void TBossWanwan::slideToCurPathNode(f32 march_speed, f32 turn_speed)
{
	JGeometry::TVec3<f32> pathDir = getUnkF4().getPoint();
	pathDir.sub(mPosition);

	f32 distance = PSVECMag((Vec*)&pathDir);

	f32 targetYaw;
	if (pathDir.z == 0.0f) {
		if (pathDir.x >= 0.0f)
			targetYaw = 90.0f;
		else
			targetYaw = -90.0f;
	} else if (pathDir.z >= 0.0f) {
		targetYaw = matan(pathDir.z, pathDir.x) * (360.0f / 65536.0f);
	} else {
		f32 yaw  = matan(-pathDir.z, pathDir.x) * (360.0f / 65536.0f);
		targetYaw = 180.0f - yaw;
	}

	while (targetYaw >= 360.0f)
		targetYaw -= 360.0f;
	while (targetYaw < 0.0f)
		targetYaw += 360.0f;

	f32 wrappedYaw
	    = callMsWrap(mRotation.y, targetYaw - 180.0f, targetYaw + 180.0f);
	f32 turn = targetYaw - wrappedYaw;
	if (turn > 0.0f) {
		if (turn > turn_speed)
			turn = turn_speed;
	} else {
		f32 turnMin = -turn_speed;
		if (turn <= turnMin)
			turn = turnMin;
	}

	f32 newYaw = mRotation.y + turn;
	while (newYaw >= 360.0f)
		newYaw -= 360.0f;
	while (newYaw < 0.0f)
		newYaw += 360.0f;
	mRotation.y = newYaw;

	JGeometry::TVec3<f32> velocity = mLinearVelocity;
	if (distance > 0.0f)
		pathDir.scale(march_speed / distance);

	velocity.add(pathDir);
	mLinearVelocity = velocity;
}

void TBossWanwan::control()
{
	TLiveActor::control();

	if (unk17C != 0
	    || (mPicket->isTaken()
	        && unk15C.x * unk15C.x + unk15C.y * unk15C.y + unk15C.z * unk15C.z
	               >= ((TBWParams*)getSaveParam())->mSLPullLimit.get())) {
		mLinearVelocity.x += unk15C.x;
		mLinearVelocity.y += unk15C.y;
		mLinearVelocity.z += unk15C.z;

		JGeometry::TVec3<f32> ropeDir = mPosition;
		ropeDir.sub(mLeash->mRope->mPoints[3].mPosition);

		f32 targetYaw;
		if (ropeDir.z == 0.0f) {
			if (ropeDir.x >= 0.0f)
				targetYaw = 90.0f;
			else
				targetYaw = -90.0f;
		} else if (ropeDir.z >= 0.0f) {
			targetYaw = matan(ropeDir.z, ropeDir.x) * (360.0f / 65536.0f);
		} else {
			f32 yaw = matan(-ropeDir.z, ropeDir.x) * (360.0f / 65536.0f);
			targetYaw = 180.0f - yaw;
		}

		while (targetYaw >= 360.0f)
			targetYaw -= 360.0f;
		while (targetYaw < 0.0f)
			targetYaw += 360.0f;

		f32 wrappedYaw
		    = callMsWrap(mRotation.y, targetYaw - 180.0f, targetYaw + 180.0f);
		f32 turn = targetYaw - wrappedYaw;
		if (turn > 0.0f) {
			f32 turnMax = 4.0f * mTurnSpeed;
			if (turn > turnMax)
				turn = turnMax;
		} else {
			f32 turnMin = 4.0f * -mTurnSpeed;
			if (turn <= turnMin)
				turn = turnMin;
		}

		f32 newYaw = mRotation.y + turn;
		while (newYaw >= 360.0f)
			newYaw -= 360.0f;
		while (newYaw < 0.0f)
			newYaw += 360.0f;

		mRotation.y = newYaw;
	}

	unk15C.z = 0.0f;
	unk15C.y = 0.0f;
	unk15C.x = 0.0f;

	updateSquareToMario();
}

void TBossWanwan::emitEffects()
{
	BOOL emitLanding = FALSE;
	if (mMActor->checkCurBckFromIndex(4) || mMActor->checkCurBckFromIndex(5)) {
		if (mMActor->checkBckPass(8.0f))
			emitLanding = TRUE;
	} else if (mMActor->checkCurBckFromIndex(2)) {
		if (mMActor->checkBckPass(38.0f))
			emitLanding = TRUE;
	}

	if (emitLanding) {
		gpMarioParticleManager->emit(0xAD, &mPosition, 0, nullptr);
		gpMarioParticleManager->emit(0xAE, &mPosition, 0, nullptr);

		if (mHitPoints == 0) {
			const JGeometry::TVec3<f32>* chainPos
			    = &mLeash->mRope->mPoints[0].mPosition;
			if (gpMSound->gateCheck(0x2975))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2975, chainPos, 0, nullptr, 0, 4);

			const JGeometry::TVec3<f32>* picketPos = &mPicket->mPosition;
			if (gpMSound->gateCheck(0x2976))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2976, picketPos, 0, nullptr, 0, 4);
		} else {
			const JGeometry::TVec3<f32>* chainPos
			    = &mLeash->mRope->mPoints[0].mPosition;
			if (gpMSound->gateCheck(0x2973))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2973, chainPos, 0, nullptr, 0, 4);

			const JGeometry::TVec3<f32>* picketPos = &mPicket->mPosition;
			if (gpMSound->gateCheck(0x2974))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2974, picketPos, 0, nullptr, 0, 4);
		}
	}

	BOOL emitBodySmoke = FALSE;
	if (mMActor->checkCurBckFromIndex(0)) {
		if (mMActor->checkBckPass(72.0f))
			emitBodySmoke = TRUE;
	} else if (mMActor->checkCurBckFromIndex(4)
	           || mMActor->checkCurBckFromIndex(5)) {
		if (mMActor->checkBckPass(6.0f) || mMActor->checkBckPass(12.0f))
			emitBodySmoke = TRUE;
	} else if (mMActor->checkCurBckFromIndex(2)) {
		if (mMActor->checkBckPass(4.0f))
			emitBodySmoke = TRUE;
	}

	if (emitBodySmoke)
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0xAF, getModel()->mNodeMatrices[1], 0, this);

	if (mMActor->checkCurBckFromIndex(4) || mMActor->checkCurBckFromIndex(5)) {
		if (mMActor->checkBckPass(10.0f))
			shakeCamera(0x16);
	}

	if (mMActor->checkCurBckFromIndex(0)) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
		if (ctrl->checkPass(60.0f) || ctrl->checkPass(127.0f)) {
			shakeCamera(0x16);
			gpMarioParticleManager->emit(0xAD, &mPosition, 0, nullptr);
			gpMarioParticleManager->emit(0xAE, &mPosition, 0, nullptr);
		}

		if (ctrl->checkPass(202.0f)) {
			shakeCamera(0x17);
			gpMarioParticleManager->emit(0xAD, &mPosition, 0, nullptr);
			gpMarioParticleManager->emit(0xAE, &mPosition, 0, nullptr);
		}
	}

	if (mMActor->checkCurBckFromIndex(2) && mMActor->checkBckPass(40.0f))
		shakeCamera(0x16);

	if (mHitPoints != 0)
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x1EE, getModel()->mNodeMatrices[1], 3, this);

	if (unk190 != 0 && mHitPoints != 0) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x167, getModel()->mNodeMatrices[1], 1, this);
		unk190 = 0;
	}
}

void TBossWanwan::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((s8)unk18C != 0) {
		mHeadHit->perform(flags, graphics);
		TSpineEnemy::perform(flags, graphics);

		if (flags & 1) {
			mMtxCalc->unk50 -= unk178;
			if (mMtxCalc->unk50 < 0.0f)
				mMtxCalc->unk50 = 0.0f;
			else if (mMtxCalc->unk50 > 1.0f)
				mMtxCalc->unk50 = 1.0f;
		}

		if (flags & 0x200) {
			J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(5);
			if (ctrl != nullptr
			    && ctrl->getFrame() > 0.5f * (f32)ctrl->getEnd()) {
				unk1A4 = mPosition;
				unk1A4.y += 500.0f;
				gpMarioParticleManager->emitAndBindToPosPtr(
				    0x168, &unk1A4, 1, this);
			}
		}
		return;
	}

	if (flags & 2) {
		J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(5);
		TBWParams* params  = (TBWParams*)getSaveParam();
		u8 maxHP           = params->mSLBWHitPointMax.get();
		f32 ratio          = (f32)mHitPoints / (f32)maxHP;
		ctrl->setFrame(ratio * (f32)(ctrl->getEnd() - 1));
		ctrl->setRate(0.0f);

		if (mHitPoints == 0 || mHitPoints == maxHP)
			mLeash->mNodes[0]->unk74 = ratio;

		emitEffects();
	}

	mHeadHit->perform(flags, graphics);

	BOOL moveStep = flags & 1;
	if (moveStep) {
		mBodyHit->mPosition = mHeadHit->mPosition;
		mBodyHit->mPosition.y -= 500.0f;
	}
	mBodyHit->perform(flags, graphics);

	if (moveStep && (s8)unk194 == 0) {
		++unk19C;
		if (unk19C < 0)
			unk19C = 1;

		if (unk19C > 600) {
			if ((unk198 & 1) == 0)
				gpMarDirector->mConsole->startAppearBalloon(0xE001A, true);
			unk198 |= 1;
		}
	}

	if (moveStep && (s8)unk1A0 == 0 && gpMarDirector->unk58 >= 0x7080) {
		if ((unk198 & 8) == 0)
			gpMarDirector->mConsole->startAppearBalloon(0xE001D, true);
		unk198 |= 8;
	}

	if (moveStep
	    && mSpine->getLatestNerve() != &TNerveBWDie::theNerve()
	    && mSpine->getLatestNerve() != &TNerveBWJumpToBath::theNerve()) {
		if (mHitPoints != 0) {
			if (unk17C != 0 && (s8)unk194 == 0) {
				++unk184;
				if (unk184 > 600) {
					mSpine->pushNerve(&TNerveBWShake::theNerve());
					unk184 = 0;
				}
			}

			if (gpMarDirector->unk58 % 20 == 0) {
				u8 maxHP
				    = ((TBWParams*)getSaveParam())->mSLBWHitPointMax.get();
				if (mHitPoints < maxHP)
					++mHitPoints;
			}
			unk180 = 0;
		} else {
			unk184 = 0;

			if (mSpine->getLatestNerve() != &TNerveBWBark::theNerve()) {
				++unk180;
				if (unk180 > 2400
				    && mSpine->getLatestNerve() != &TNerveBWBark::theNerve()) {
					mSpine->setNext(&TNerveBWBark::theNerve());
				}
			} else {
				if (gpMarDirector->unk58 % 20 == 0) {
					u8 maxHP = ((TBWParams*)getSaveParam())
					               ->mSLBWHitPointMax.get();
					if (mHitPoints < maxHP)
						++mHitPoints;
				}
				unk180 = 0;
			}
		}
	}

	if (moveStep
	    && mSpine->getLatestNerve() == &TNerveBWGraphWander::theNerve()
	    && mPicket->isTaken()) {
		f32 pull = unk15C.length();
		if (gpMSound->gateCheck(0x20D2))
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x20D2, &mLeash->mRope->mPoints[6].mPosition, nullptr, pull,
			    0, 0, nullptr, 0, 4);
	}

	TSpineEnemy::perform(flags, graphics);
	mLeash->testPerform(flags, graphics);
	mPicket->testPerform(flags, graphics);

	if (moveStep) {
		mMtxCalc->unk50 -= unk178;
		if (mMtxCalc->unk50 < 0.0f)
			mMtxCalc->unk50 = 0.0f;
		else if (mMtxCalc->unk50 > 1.0f)
			mMtxCalc->unk50 = 1.0f;
	}
}

void TBossWanwan::kill() { }

TBossWanwanManager::TBossWanwanManager(const char* name)
    : TEnemyManager(name)
{
}

TSpineEnemy* TBossWanwanManager::createEnemyInstance()
{
	return new TBossWanwan("ボスワンワン");
}

void TBossWanwanManager::createModelData() { createModelDataArray(sModelDataEntries); }

void TBossWanwanManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBWParams("/enemy/bosswanwan.prm");

	TEnemyManager::load(stream);

	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_jump_rock.jpa", 0xad);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_jump_smoke.jpa", 0xae);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_downyuge.jpa", 0xb0);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_hibana.jpa", 0xaf);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_deadyuge.jpa", 0xb1);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_yugami.jpa", 0x1ee);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_hityuge.jpa", 0x167);
	SMS_LoadParticle("/scene/bwanwan/jpa/ms_bwan_kira.jpa", 0x168);
}

DEFINE_NERVE(TNerveBWGraphWander, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0)
		self->mMActor->setBck(bwanwan_bastable[4]);

	self->slideToCurPathNode(((TBWParams*)self->getSaveParam())->mSLMarchSpeed.get(),
	                         ((TBWParams*)self->getSaveParam())->mSLTurnSpeed.get());
	return false;
}

DEFINE_NERVE(TNerveBWRoll, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		J3DFrameCtrl* frameCtrl = self->mMActor->getFrameCtrl(0);
		frameCtrl->setFrame(0.0f);
		frameCtrl->setRate(0.0f);
		self->unk16C = 1;
	}

	if (self->isReachedToGoal()) {
		spine->pushAfterCurrent(&TNerveBWFall::theNerve());
		J3DFrameCtrl* frameCtrl = self->mMActor->getFrameCtrl(0);
		frameCtrl->setRate(SMSGetAnmFrameRate());
		return true;
	}

	self->walkToCurPathNode(
	    ((TBWParams*)self->getSaveParam())->mSLAttackSpeed.get(),
	    self->mTurnSpeed, 0.0f);
	return false;
}

DEFINE_NERVE(TNerveBWBark, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0)
		self->mMActor->setBck(bwanwan_bastable[0]);
	return false;
}

DEFINE_NERVE(TNerveBWJump, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWStun, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWWakeup, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWJumpToBath, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWDie, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWJumpAway, TLiveActor)
{
	return false;
}

DEFINE_NERVE(TNerveBWShake, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0)
		self->mMActor->setBck(bwanwan_bastable[2]);
	return false;
}

DEFINE_NERVE(TNerveBWFall, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> velocity = self->calcVelocityToJumpToY(
		    self->mPicket->mPosition, 500.0f, self->getGravityY());

		TPathNode node(self->mPicket->mPosition);
		self->setGoalPath(node);
		self->mVelocity = velocity;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->unk16C = 0;
	}

	if (self->isReachedToGoal()) {
		self->mPosition = self->mPicket->mPosition;
		self->unk124->mPrevIdx = -1;
		self->unk124->mCurrIdx = -1;
		self->goToShortestNextGraphNode();
		spine->pushAfterCurrent(&TNerveBWFall::theNerve());
		return true;
	}

	return false;
}
