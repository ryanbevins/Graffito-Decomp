#include <Enemy/BossWanwan.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EffectObj.hpp>
#include <Enemy/Graph.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapData.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
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
	                  ((TBWParams*)mOwner->getSaveParam())
	                      ->mSLChainGroundRadius.get(),
	                  0.7f, -2.0f);
	mNodes = new TBWLeashNode*[node_count];
	for (int i = 0; i < node_count; ++i) {
		TBWLeashNode* node = new TBWLeashNode(this, i, "鎖部");
		node->mMActor = mOwner->mMActorKeeper->createMActor(
		    "bwanwan_chain.bmd", 0);
		node->mMActor->setBrkFromIndex(2);

		TBWParams* params = (TBWParams*)mOwner->getSaveParam();
		f32 radius        = params->mSLChainHitRadius.get();
		f32 height        = params->mSLChainHitHeight.get();
		node->initHitActor(0x0800000C, 1, 0x80000000, radius * 1.2f,
		                   height * 1.2f, radius, height);
		mNodes[i] = node;
	}

	TIdxGroupObj* enemyGroup
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	for (int i = 0; i < node_count; ++i) {
		enemyGroup->getChildren().push_back(mNodes[i]);
		if (i < node_count - 2)
			mNodes[i]->offHitFlag(HIT_FLAG_NO_COLLISION);
		else
			mNodes[i]->onHitFlag(HIT_FLAG_NO_COLLISION);

		if (i < 2)
			mRope->mPoints[i].mFlags |= 1;
		else
			mRope->mPoints[i].mFlags &= ~1;
	}
}

void TBWLeash::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		JGeometry::TVec3<f32> headPos;
		mOwner->getJointTransByIndex(5, &headPos);
		mRope->moveHead(headPos);

		mOwner->unk188 = 0;
		if (mOwner->unk17C != 0) {
			const JGeometry::TVec3<f32>& picketPos = mOwner->mPicket->mPosition;
			JGeometry::TVec3<f32> headBefore = mRope->mPoints[0].mPosition;
			mRope->constraintTail(picketPos);

			headBefore.sub(mRope->mPoints[0].mPosition);
			headBefore.negate();
			mOwner->unk15C = headBefore;

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
	if (sender->getActorType() == 0x80000001) {
		if (message == HIT_MESSAGE_HIP_DROP) {
			TBossWanwan* owner = mOwner;
			owner->unk17C      = 1;
			owner->unk184      = 0;
			if (gpMSound->gateCheck(0x28C0))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x28C0, &mPosition, 0, nullptr, 0, 4);
			return TRUE;
		}

		if (message == HIT_MESSAGE_TAKE) {
			TBossWanwan* owner = mOwner;
			if (owner->unk17C != 0) {
				JPABaseEmitter* emitter = gpMarioParticleManager->emit(
				    0xAE, &owner->mPicket->mPosition, 0, nullptr);
				if (emitter != nullptr) {
					JGeometry::TVec3<f32> scale(0.3f, 0.5f, 0.3f);
					emitter->setScale(scale);
				}
			}
			owner->unk194 = 0;
			owner->unk17C = 0;
			mHolder       = (TTakeActor*)sender;
			return TRUE;
		}

		if (message == HIT_MESSAGE_UNK7 || message == HIT_MESSAGE_UNK8) {
			mHolder = nullptr;
			return TRUE;
		}
	}

	return FALSE;
}

BOOL TBWPicket::moveRequest(const JGeometry::TVec3<f32>& position)
{
	if (mOwner->mSpine->getLatestNerve()
	        == &TNerveBWJumpToBath::theNerve()
	    || mOwner->mSpine->getLatestNerve() == &TNerveBWDie::theNerve())
		return false;

	if (mOwner->mHitPoints != 0)
		return false;

	TBWLeash* leash = mOwner->mLeash;
	JGeometry::TVec3<f32> tailBefore
	    = leash->mRope->mPoints[0].mPosition;
	leash->mRope->constraintTail(position);
	tailBefore.sub(leash->mRope->mPoints[0].mPosition);
	tailBefore.negate();
	leash->mOwner->unk15C = tailBefore;
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
	TBossWanwan* self = (TBossWanwan*)actor;
	JGeometry::TVec3<f32> nextPos = actor->mPosition;
	nextPos += actor->mLinearVelocity;

	if (actor->isAirborne()) {
		nextPos += actor->mVelocity;
		actor->mVelocity.y -= actor->getGravityY();
		if (actor->mVelocity.y < TLiveActor::mVelocityMinY)
			actor->mVelocity.y = TLiveActor::mVelocityMinY;
	}

	if (actor->mSpine->getLatestNerve() == &TNerveBWJumpToBath::theNerve()
	    || actor->mSpine->getLatestNerve() == &TNerveBWDie::theNerve()) {
		actor->mLinearVelocity = nextPos;
		actor->mLinearVelocity -= actor->mPosition;
		return;
	}

	if (actor->isAirborne()) {
		const TBGCheckData* groundPlane;
		f32 groundY = gpMap->checkGround(
		    nextPos.x, nextPos.y + actor->mHeadHeight, nextPos.z, &groundPlane);
		groundY += 1.0f;

		if (actor->mPosition.y - nextPos.y > 0.0f
		    && !groundPlane->isEnemyThrough()) {
			const TBGCheckData* groundPlane2;
			f32 groundY2 = gpMap->checkGround(
			    nextPos.x, actor->mPosition.y + actor->mHeadHeight, nextPos.z,
			    &groundPlane2);
			groundY2 += 1.0f;
			if (groundY2 > groundY) {
				groundY     = groundY2;
				groundPlane = groundPlane2;
			}
		}

		if (nextPos.y <= groundY
		    && !groundPlane->checkFlag(BG_CHECK_FLAG_ILLEGAL)
		    && !groundPlane->isEnemyThrough()) {
			nextPos.y = groundY;
			actor->mVelocity.zero();
			actor->offLiveFlag(LIVE_FLAG_AIRBORNE);
			actor->offLiveFlag(LIVE_FLAG_UNK8000);
		} else {
			actor->onLiveFlag(LIVE_FLAG_AIRBORNE);
		}

		actor->mGroundHeight = groundY;
		actor->mGroundPlane  = groundPlane;
	}

	JGeometry::TVec3<f32> movement = nextPos;
	movement -= actor->mPosition;

	if (!actor->isAirborne()) {
		TGraphTracer* tracer = self->unk124;
		if (tracer->unk0 != nullptr && tracer->mCurrIdx >= 0
		    && tracer->mPrevIdx >= 0 && tracer->mCurrIdx != tracer->mPrevIdx) {
			JGeometry::TVec3<f32> currPoint;
			JGeometry::TVec3<f32> prevPoint;
			tracer->unk0->getGraphNode(tracer->mCurrIdx).getPoint(&currPoint);
			tracer->unk0->getGraphNode(tracer->mPrevIdx).getPoint(&prevPoint);

			JGeometry::TVec3<f32> linkDir = currPoint;
			linkDir -= prevPoint;
			PSVECNormalize(&linkDir, &linkDir);

			f32 len = linkDir.squared();
			f32 step;
			if (len == 0.0f) {
				step = 0.0f;
			} else {
				step = movement.dot(linkDir) / len;
			}

			if (step < 0.0f) {
				if (step > -3.0f)
					step = -3.0f;
			} else if (step > 0.0f) {
				if (step < 3.0f)
					step = 3.0f;
			}

			movement = linkDir;
			movement *= step;
		}
	}

	if (!actor->isAirborne()) {
		JGeometry::TVec3<f32> movementForRoll = movement;
		f32 moveLength = PSVECMag(&movementForRoll);
		if (moveLength != 0.0f) {
			f32 twistStep = 360.0f * (moveLength / 3141.5928f);
			s16 yaw       = (s16)(self->mRotation.y * (65536.0f / 360.0f));
			JGeometry::TVec3<f32> facing(JMASSin(yaw), 0.0f, JMASCos(yaw));
			if (movement.dot(facing) < 0.0f)
				twistStep = -twistStep;

			twistStep *= 2.0f;
			if (self->unk16C != 0) {
				self->unk168 = callMsWrap(self->unk168 + twistStep, 0.0f,
				                          360.0f);
			} else if (self->unk168 != 0.0f) {
				f32 twist = self->unk168 + twistStep;
				if (twist > 360.0f)
					twist = 0.0f;
				self->unk168 = twist;
			}
		}
	}

	if (self->unk17C != 0) {
		JGeometry::TVec3<f32> oldPos = actor->mPosition;
		JGeometry::TVec3<f32> clampedPos = oldPos;
		clampedPos += movement;

		JGeometry::TVec3<f32> toRope = clampedPos;
		JGeometry::TVec3<f32> ropePos
		    = self->mLeash->mRope->mPoints[3].mPosition;
		toRope -= ropePos;
		if (PSVECMag(&toRope) > 860.0f) {
			PSVECNormalize(&toRope, &toRope);
			toRope *= 860.0f;
			clampedPos = ropePos;
			clampedPos += toRope;
			movement = clampedPos;
			movement -= oldPos;
		}
	}

	if (!actor->isAirborne()) {
		TGraphTracer* tracer = self->unk124;
		TGraphWeb* graph     = tracer->unk0;
		JGeometry::TVec3<f32> currPoint;
		JGeometry::TVec3<f32> prevPoint;
		graph->getGraphNode(tracer->mCurrIdx).getPoint(&currPoint);
		graph->getGraphNode(tracer->mPrevIdx).getPoint(&prevPoint);

		JGeometry::TVec3<f32> foot
		    = MsPerpendicFootToLineR(prevPoint, currPoint, actor->mPosition);
		JGeometry::TVec3<f32> correction = foot;
		correction -= actor->mPosition;
		f32 dist = PSVECMag(&correction);
		if (dist > 10.0f)
			dist = 10.0f;
		if (dist < 0.0001f) {
			correction.zero();
		} else {
			PSVECNormalize(&correction, &correction);
			correction *= dist;
		}
		movement += correction;
	}

	actor->mLinearVelocity = movement;
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
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
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

	f32 marioDist = mDistToMarioSquared;
	if (marioDist > 0.0f) {
		f64 guess = __frsqrte((f64)marioDist);
		volatile f32 rounded
		    = (f32)((f64)marioDist
		            * (0.5 * guess
		               * -((f64)marioDist * (guess * guess) - 3.0)));
		marioDist = rounded;
	}
	f32 lengthMax
	    = ((TBWParams*)getSaveParam())->mSLShakeLengthMax.value;
	f32 lengthMaxHP0
	    = ((TBWParams*)getSaveParam())->mSLShakeLengthMaxHP0.value;
	f32 ratio;
	MActor* actor = mMActor;

	if (actor->checkCurBckFromIndex(0)) {
		ratio = 1.0f;
	} else {
		ratio = (f32)mHitPoints
		        / (f32)((TBWParams*)getSaveParam())->mSLBWHitPointMax.get();
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

	if (sender->isActorType(0x4000005A)) {
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
	model->unk14 = mScaling;

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
	MActor* actor = self->mMActor;

	if (spine->getTime() == 0) {
		self->unk16C = 0;

		if (!actor->checkCurBckFromIndex(4)) {
			s32 waitIndex = self->mHitPoints == 0 ? 5 : 4;
			TBossWanwanMtxCalc* mtxCalc = self->mMtxCalc;
			J3DAnmTransform* waitAnm
			    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()
			          ->getUnk2C()
			          ->getAnmPtr(waitIndex);
			if (mtxCalc->unk54 != waitAnm) {
				mtxCalc->unk58 = mtxCalc->unk54;
				mtxCalc->unk54 = waitAnm;
				mtxCalc->unk50 = 1.0f;
			}

			actor->getAnmBck()->setFrameCtrl(waitIndex);
			J3DFrameCtrl* frameCtrl = actor->getFrameCtrl(0);
			self->unk178            = 10.0f / (f32)frameCtrl->getEnd();
			self->setAnmSound(bwanwan_bastable[waitIndex]);
			frameCtrl->setFrame(30.0f);
			frameCtrl->setRate(SMSGetAnmFrameRate());
		}

		actor->setBtpFromIndex(0);
		J3DFrameCtrl* btpCtrl = actor->getFrameCtrl(3);
		btpCtrl->setFrame(30.0f);
		btpCtrl->setRate(30.0f);
	}

	if (actor->curAnmEndsNext(0, nullptr) && self->mHitPoints == 0
	    && !actor->checkCurBckFromIndex(5)) {
		TBossWanwanMtxCalc* mtxCalc = self->mMtxCalc;
		J3DAnmTransform* waitAnm
		    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()
		          ->getUnk2C()
		          ->getAnmPtr(5);
		if (mtxCalc->unk54 != waitAnm) {
			mtxCalc->unk58 = mtxCalc->unk54;
			mtxCalc->unk54 = waitAnm;
			mtxCalc->unk50 = 1.0f;
		}

		actor->getAnmBck()->setFrameCtrl(5);
		J3DFrameCtrl* frameCtrl = actor->getFrameCtrl(0);
		self->unk178            = 10.0f / (f32)frameCtrl->getEnd();
		self->setAnmSound(bwanwan_bastable[5]);
		frameCtrl->setFrame(30.0f);
		frameCtrl->setRate(SMSGetAnmFrameRate());
	}

	if (self->unk17C == 0 && self->mPicket->isTaken()) {
		f32 pull = self->unk15C.x * self->unk15C.x
		           + self->unk15C.y * self->unk15C.y
		           + self->unk15C.z * self->unk15C.z;
		TBWParams* params = (TBWParams*)self->getSaveParam();
		if (pull >= params->mSLPullLimit.get()) {
			TGraphTracer* tracer = self->unk124;
			TGraphWeb* graph     = tracer->unk0;
			s32 prevIndex        = tracer->mPrevIdx;
			JGeometry::TVec3<f32> nodeDelta;
			graph->getGraphNode(prevIndex).getPoint(&nodeDelta);
			nodeDelta -= self->mPosition;

			if (PSVECMag(&nodeDelta) < 400.0f && prevIndex == graph->unk10) {
				spine->pushAfterCurrent(&TNerveBWJumpToBath::theNerve());
				return true;
			}

			if (PSVECMag(&nodeDelta) < 400.0f) {
				TGraphTracer* tracer2 = self->unk124;
				s32 oldPrev          = tracer2->mPrevIdx;
				s32 curr             = tracer2->mCurrIdx;
				JGeometry::TVec3<f32> toMario = *gpMarioPos;
				toMario -= self->mPosition;
				s32 next = tracer2->unk0->getAimToDirNextIndex(
				    oldPrev, curr, toMario, self->mPosition, -1);
				tracer2->mPrevIdx = next;
				tracer2->mCurrIdx = oldPrev;
				self->setGoalPathFromGraph();
				self->unk128 = 0;
				self->unk12C = 30.0f;
			}

			f32 ratio = (f32)self->mHitPoints
			            / (f32)params->mSLBWHitPointMax.get();
			self->slideToCurPathNode(400.0f * ratio * self->mMarchSpeed,
			                         self->mTurnSpeed);
			return false;
		}
	}

	if (self->isReachedToGoal()) {
		if (self->jumpToNextGraphNode() >= 0) {
			spine->pushAfterCurrent(&TNerveBWGraphWander::theNerve());
			spine->pushAfterCurrent(&TNerveBWJump::theNerve());
			return true;
		}

		spine->pushAfterCurrent(&TNerveBWGraphWander::theNerve());

		TGraphTracer* tracer = self->unk124;
		s32 prevIndex        = tracer->mPrevIdx;
		s32 currIndex        = tracer->mCurrIdx;
		TGraphWeb* graph     = tracer->unk0;

		if (prevIndex >= 0
		    && graph->getGraphNode(prevIndex).getRailNode()->mConnectionNum >= 2
		    && gpMarDirector->unk58 >= 0x3840) {
			if ((self->unk198 & 4) == 0)
				gpMarDirector->mConsole->startAppearBalloon(0xE001C, true);
			self->unk198 |= 4;
		}

		JGeometry::TVec3<f32> escapeDir(
		    JMASSin((s16)(self->mRotation.y * (65536.0f / 360.0f))), 30.0f,
		    JMASCos((s16)(self->mRotation.y * (65536.0f / 360.0f))));
		s32 next = graph->getEscapeDirLimited(currIndex, prevIndex, escapeDir,
		                                      self->mPosition, 1.1f, -1);
		tracer->moveTo(next);
		self->setGoalPathFromGraph();
		self->unk128 = 0;
		self->unk12C = 30.0f;
		return true;
	}

	self->unk16C = 0;

	TBWParams* params = (TBWParams*)self->getSaveParam();
	f32 hpRatio = (f32)self->mHitPoints / (f32)params->mSLBWHitPointMax.get();
	if (self->mPicket->isTaken()) {
		f32 marioAngle = SHORTANGLE2DEG(gpMarioOriginal->mIntendedYaw);
		JGeometry::TVec3<f32> intendedDir(JMASin(marioAngle), 30.0f,
		                                  JMACos(marioAngle));
		JGeometry::TVec3<f32> bossToMario = self->mPosition;
		bossToMario -= gpMarioOriginal->mPosition;
		PSVECNormalize(&bossToMario, &bossToMario);

		f32 speedScale = 1.0f
		                 - (0.03125f * gpMarioOriginal->mIntendedMag * 0.75f)
		                       * -(bossToMario.dot(intendedDir));
		if (speedScale < 0.0f)
			speedScale = 0.0f;
		else if (speedScale > 1.5f)
			speedScale = 1.5f;

		self->slideToCurPathNode(speedScale * hpRatio * self->mMarchSpeed,
		                         self->mTurnSpeed);
	} else {
		self->slideToCurPathNode(hpRatio * self->mMarchSpeed + 0.2f,
		                         self->mTurnSpeed);
	}
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
	if (spine->getTime() == 0) {
		TBossWanwanMtxCalc* mtxCalc = self->mMtxCalc;
		J3DAnmTransform* barkAnm
		    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()
		          ->getUnk2C()
		          ->getAnmPtr(0);
		if (mtxCalc->unk54 != barkAnm) {
			mtxCalc->unk58 = mtxCalc->unk54;
			mtxCalc->unk54 = barkAnm;
			mtxCalc->unk50 = 1.0f;
		}

		self->mMActor->getAnmBck()->setFrameCtrl(0);
		J3DFrameCtrl* frameCtrl = self->mMActor->getFrameCtrl(0);
		self->unk178 = (360.0f / 65536.0f) / (f32)frameCtrl->getEnd();
		self->setAnmSound(bwanwan_bastable[0]);
		self->unk16C = 0;
		self->unk168 = 0.0f;

		if (!self->unk194) {
			if (self->unk17C != 0) {
				JPABaseEmitter* emitter = gpMarioParticleManager->emit(
				    0xAE, &self->mPicket->mPosition, 0, nullptr);
				if (emitter != nullptr) {
					emitter->unk154.x = 0.03125f;
					emitter->unk154.y = 10000.0f;
					emitter->unk154.z = 0.03125f;
					emitter->unk174.x = 0.03125f;
					emitter->unk174.y = 10000.0f;
					emitter->unk174.z = 0.03125f;
				}
			}

			self->unk194 = 0;
			self->unk17C = 0;
			if (gpMSound->gateCheck(0x2966))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x2966, &self->mPicket->mPosition, 0, nullptr, 0, 4);
		}
	}

	if (spine->getTime() == 0x118)
		self->mHitPoints = ((TBWParams*)self->getSaveParam())
		                       ->mSLBWHitPointMax.get();

	if (self->mMActor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveBWFall::theNerve());
		if (!(self->unk198 & 2))
			gpMarDirector->mConsole->startAppearBalloon(0xE001B, true);
		self->unk198 |= 2;
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBWJump, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		const JGeometry::TVec3<f32>& target = self->getUnk104().getPoint();
		f32 speed = self->unk124->unkC;
		self->mVelocity
		    = self->calcVelocityToJumpToY(target, speed, self->getGravityY());
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->unk16C = 0;
	}

	if (self->isReachedToGoal()) {
		spine->pushAfterCurrent(&TNerveBWGraphWander::theNerve());
		return true;
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	return false;
}

DEFINE_NERVE(TNerveBWStun, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (self->mPicket->isTaken()) {
		BOOL pulled = false;
		if (self->unk15C.x * self->unk15C.x
		        + self->unk15C.y * self->unk15C.y
		        + self->unk15C.z * self->unk15C.z
		    >= ((TBWParams*)self->getSaveParam())->mSLPullLimit.get())
			pulled = true;

		if (pulled) {
			TGraphTracer* tracer = self->unk124;
			TGraphWeb* graph     = tracer->getGraph();
			int prevIndex        = tracer->mPrevIdx;
			JGeometry::TVec3<f32> nodeDelta;
			graph->getGraphNode(prevIndex).getPoint(&nodeDelta);
			nodeDelta.sub(self->mPosition);

			if (PSVECMag((Vec*)&nodeDelta) < 1.1f) {
				if (prevIndex == graph->unk10) {
					spine->pushAfterCurrent(
					    &TNerveBWGraphWander::theNerve());
					return true;
				}

				JGeometry::TVec3<f32> marioDelta = *gpMarioPos;
				marioDelta.sub(self->mPosition);
				int nextIndex = graph->getAimToDirNextIndex(
				    prevIndex, tracer->mCurrIdx, marioDelta, self->mPosition,
				    -1);
				tracer->mPrevIdx = nextIndex;
				tracer->mCurrIdx = prevIndex;
				self->setGoalPathFromGraph();
				self->unk128 = 0;
				self->unk12C = 0.0f;
			}

			return false;
		}
	}

	if (spine->getTime()
	    > ((TBWParams*)self->getSaveParam())->mSLStunTimer.get()) {
		self->mHitPoints
		    = ((TBWParams*)self->getSaveParam())->mSLBWHitPointMax.get();
		spine->pushAfterCurrent(&TNerveBWWakeup::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBWWakeup, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		TBossWanwanMtxCalc* mtxCalc = self->mMtxCalc;
		J3DAnmTransform* wakeAnm
		    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()
		          ->getUnk2C()
		          ->getAnmPtr(6);
		if (mtxCalc->unk54 != wakeAnm) {
			mtxCalc->unk58 = mtxCalc->unk54;
			mtxCalc->unk54 = wakeAnm;
			mtxCalc->unk50 = 1.0f;
		}

		self->mMActor->getAnmBck()->setFrameCtrl(6);
		J3DFrameCtrl* frameCtrl = self->mMActor->getFrameCtrl(0);
		self->unk178 = (360.0f / 65536.0f) / (f32)frameCtrl->getEnd();
		self->setAnmSound(bwanwan_bastable[6]);
		self->mMActor->setBtpFromIndex(2);
		self->unk16C = 0;
		self->unk168 = 0.0f;
	}

	if (self->mMActor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveBWGraphWander::theNerve());
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBWJumpToBath, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> velocity = self->calcVelocityToJumpToY(
		    BW_BATH_POS, 360.0f / 65536.0f, self->getGravityY());
		TPathNode bathNode(BW_BATH_POS);
		self->unkF4  = bathNode;
		self->unk104 = bathNode;
		self->unk114.clear();
		self->mVelocity = velocity;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->unk16C = 0;
	}

	if (spine->getTime() > 0x78 && (s8)self->unk195 == 0
	    && self->mPosition.y < BW_BATH_POS.y + (360.0f / 65536.0f)) {
		TEffectColumWater* water
		    = (TEffectColumWater*)gpConductor->makeOneEnemyAppear(
		        self->mPosition, "エフェクト水柱マネージャー", 1);
		if (water != nullptr) {
			JGeometry::TVec3<f32> scale;
			scale.x = 500.0f;
			scale.y = 500.0f;
			scale.z = 500.0f;
			JGeometry::TVec3<f32> position;
			position.x = self->mPosition.x;
			position.y = self->mPosition.y + 0.2f - 8.0f;
			position.z = self->mPosition.z;
			water->generate(position, scale);
			self->unk195 = 1;
		}

		if (gpMSound->gateCheck(0x2917))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2917, &self->mPosition, 0, nullptr, 0, 4);
	}

	JGeometry::TVec3<f32> bathDelta = BW_BATH_POS;
	bathDelta.sub(self->mPosition);
	if (bathDelta.squared() < 38.0f && self->mPosition.y <= BW_BATH_POS.y) {
		self->mPosition = BW_BATH_POS;
		spine->pushAfterCurrent(&TNerveBWRoll::theNerve());
		return true;
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	return false;
}

DEFINE_NERVE(TNerveBWDie, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	MActor* actor     = self->mMActor;
	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> velocity(30.0f, 30.0f, 30.0f);
		self->mVelocity       = velocity;
		self->mLinearVelocity = velocity;
		self->onLiveFlag(LIVE_FLAG_UNK10);
		self->offLiveFlag(LIVE_FLAG_AIRBORNE);
		gpMarioParticleManager->emit(0xB1, &self->mPosition, 0, nullptr);
	}

	if (self->mHitPoints != 0) {
		if (self->mHitPoints != 0)
			--self->mHitPoints;

		J3DFrameCtrl* frameCtrl = actor->getFrameCtrl(0);
		frameCtrl->setFrame(30.0f);
		frameCtrl->setRate(30.0f);
		spine->pushAfterCurrent(&TNerveBWDie::theNerve());
		return true;
	}

	if (spine->getTime() == 0) {
		JDrama::TFlagT<u16> flag(0);
		TMarDirector* director = gpMarDirector;
		director->fireStartDemoCamera(
		    "bwanwan_down_camera", nullptr, -1, 30.0f, true, nullptr, 0,
		    nullptr, flag);
		self->unk16C = 0;
		self->unk168 = 30.0f;
		self->unk18C = 1;
		self->mPosition = BW_BATH_POS;

		JGeometry::TVec3<f32> position = self->mPosition;
		position.y += 500.0f;
		JGeometry::TVec3<f32> scaling = self->mScaling;
		scaling.scale(1.1f);
		Mtx mtx;
		MsMtxSetTRS(mtx, position.x, position.y, position.z,
		            self->mRotation.x, self->mRotation.y, self->mRotation.z,
		            scaling.x, scaling.y, scaling.z);
		TMapCollisionBase* collision = self->mMapCollisionManager->unk8;
		collision->setMtx(mtx);
		collision->setUp();

		self->mHeadHit->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->mBodyHit->onHitFlag(HIT_FLAG_NO_COLLISION);
		for (int i = 0; i < self->mLeash->mRope->mNumPoints; ++i)
			self->mLeash->mNodes[i]->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->mPicket->onHitFlag(HIT_FLAG_NO_COLLISION);

		TBossWanwanMtxCalc* mtxCalc = self->mMtxCalc;
		J3DAnmTransform* dieAnm
		    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()
		          ->getUnk2C()
		          ->getAnmPtr(1);
		if (mtxCalc->unk54 != dieAnm) {
			mtxCalc->unk58 = mtxCalc->unk54;
			mtxCalc->unk54 = dieAnm;
			mtxCalc->unk50 = 1.0f;
		}

		self->mMActor->getAnmBck()->setFrameCtrl(1);
		J3DFrameCtrl* frameCtrl = self->mMActor->getFrameCtrl(0);
		self->unk178 = (10.0f) / (f32)frameCtrl->getEnd();
		self->setAnmSound(bwanwan_bastable[1]);
		actor->setBtpFromIndex(0);
		actor->setBrkFromIndex(1);
	}

	if (spine->getTime() > 0x3C && (s8)self->unk18D == 0
	    && gpMarDirector->unk124 != 3) {
		gpItemManager->makeShineAppearWithDemo(
		    "シャイン（ボス用）", "ボスシャイカメラ", self->mPosition.x,
		    self->mPosition.y, self->mPosition.z);
		self->unk18D = 1;
	}

	if (actor->curAnmEndsNext(5, nullptr)) {
		J3DFrameCtrl* frameCtrl = actor->getFrameCtrl(5);
		frameCtrl->setRate(30.0f);
	}

	return false;
}

DEFINE_NERVE(TNerveBWJumpAway, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		JGeometry::TVec3<f32> velocity
		    = self->calcVelocityToJumpToY(BW_HEAD_START, 1.5f,
		                                  self->getGravityY());
		TPathNode headNode(BW_HEAD_START);
		self->unkF4  = headNode;
		self->unk104 = headNode;
		self->unk114.clear();
		self->mVelocity = velocity;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->unk16C = 0;
	}

	if (self->unk188 != 0) {
		spine->pushAfterCurrent(&TNerveBWFall::theNerve());
		return true;
	}

	if (self->isReachedToGoal()) {
		self->mPosition = BW_HEAD_START;
		self->unk124->mPrevIdx = -1;
		self->unk124->mCurrIdx = -1;
		self->goToShortestNextGraphNode();
		spine->pushAfterCurrent(&TNerveBWGraphWander::theNerve());
		return true;
	}

	self->walkToCurPathNode(30.0f, self->mTurnSpeed, 30.0f);
	return false;
}

DEFINE_NERVE(TNerveBWShake, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		TBossWanwanMtxCalc* mtxCalc = self->mMtxCalc;
		J3DAnmTransform* shakeAnm
		    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()
		          ->getUnk2C()
		          ->getAnmPtr(2);
		if (mtxCalc->unk54 != shakeAnm) {
			mtxCalc->unk58 = mtxCalc->unk54;
			mtxCalc->unk54 = shakeAnm;
			mtxCalc->unk50 = 1.0f;
		}

		self->mMActor->getAnmBck()->setFrameCtrl(2);
		J3DFrameCtrl* frameCtrl = self->mMActor->getFrameCtrl(0);
		self->unk178 = (360.0f / 65536.0f) / (f32)frameCtrl->getEnd();
		self->setAnmSound(bwanwan_bastable[2]);
	}

	if (self->mMActor->curAnmEndsNext(0, nullptr)) {
		if (self->unk17C != 0) {
			JPABaseEmitter* emitter = gpMarioParticleManager->emit(
			    0xAE, &self->mPicket->mPosition, 0, nullptr);
			if (emitter != nullptr) {
				emitter->unk154.x = 0.03125f;
				emitter->unk154.y = 10000.0f;
				emitter->unk154.z = 0.03125f;
				emitter->unk174.x = 0.03125f;
				emitter->unk174.y = 10000.0f;
				emitter->unk174.z = 0.03125f;
			}
		}

		self->unk194 = 0;
		self->unk17C = 0;
		if (gpMSound->gateCheck(0x2967))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2967, &self->mPicket->mPosition, 0, nullptr, 0, 4);
		return true;
	}

	return false;
}

DEFINE_NERVE(TNerveBWFall, TLiveActor)
{
	TBossWanwan* self = (TBossWanwan*)spine->getBody();
	if (spine->getTime() == 0) {
		const JGeometry::TVec3<f32>& picketPos = self->mPicket->mPosition;
		JGeometry::TVec3<f32> velocity = self->calcVelocityToJumpToY(
		    picketPos, 500.0f, self->getGravityY());

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
		spine->pushAfterCurrent(&TNerveBWGraphWander::theNerve());
		return true;
	}

	return false;
}
