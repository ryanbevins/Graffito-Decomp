#include <Enemy/HanaSambo.hpp>
#include <Enemy/Conductor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DNode.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <Map/Map.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <MoveBG/Item.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <string.h>

static const char* sambo_bastable[] = {
	"/scene/sambo/bas/sambo_down.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/sambo/bas/sambo_Fhide.bas",
	nullptr,
	"/scene/sambo/bas/sambo_Fset.bas",
	"/scene/sambo/bas/sambo_Fwait.bas",
	"/scene/sambo/bas/sambo_hit.bas",
	nullptr,
	"/scene/sambo/bas/sambo_Ydown.bas",
};

static const char* sambohead_bastable[] = {
	"/scene/sambohead/bas/flower_shoot.bas",
	"/scene/sambohead/bas/samboHead_crash.bas",
	"/scene/sambohead/bas/samboHead_dance.bas",
	"/scene/sambohead/bas/samboHead_down.bas",
	"/scene/sambohead/bas/samboHead_Fhide.bas",
	"/scene/sambohead/bas/samboHead_hit.bas",
	"/scene/sambohead/bas/samboHead_hit_end.bas",
	"/scene/sambohead/bas/samboHead_jump_end.bas",
	"/scene/sambohead/bas/samboHead_jump_start.bas",
	nullptr,
	"/scene/sambohead/bas/samboHead_set.bas",
	"/scene/sambohead/bas/samboHead_turn.bas",
	nullptr,
};

u8 THanaSambo::mHeadJntIndex   = 3;
u8 THanaSambo::mPollenJntIndex = 6;
u8 TSamboHead::mBodyJntIndex;

static TSamboHead* gpCurSamboHead;

static int SamboHeadRollCallback(J3DNode* node, int timing)
{
	if (timing != 0)
		return 1;

	TSamboHead* head = gpCurSamboHead;
	if (!head)
		return 1;

	const TNerveBase<TLiveActor>* nerve = head->mSpine->getCurrentNerve();
	if (nerve != &TNerveSamboHeadAttack::theNerve()
	    && nerve != &TNerveSamboHeadHitWater::theNerve()
	    && nerve != &TNerveSamboHeadRecoverWater::theNerve())
		return 1;

	J3DJoint* joint = (J3DJoint*)node;
	MtxPtr jointMtx = head->getModel()->mNodeMatrices[joint->getJntNo()];

	JGeometry::TVec3<f32> velocity(head->mVelocity);
	if (velocity.x == 0.0f && velocity.z == 0.0f)
		velocity.x = 0.001f;

	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	JGeometry::TVec3<f32> cross;
	PSVECCrossProduct(&up, &velocity, &cross);

	f32 xLen = jointMtx[0][0] * jointMtx[0][0]
	           + jointMtx[1][0] * jointMtx[1][0]
	           + jointMtx[2][0] * jointMtx[2][0];
	f32 yLen = jointMtx[0][1] * jointMtx[0][1]
	           + jointMtx[1][1] * jointMtx[1][1]
	           + jointMtx[2][1] * jointMtx[2][1];
	f32 zLen = jointMtx[0][2] * jointMtx[0][2]
	           + jointMtx[1][2] * jointMtx[1][2]
	           + jointMtx[2][2] * jointMtx[2][2];

	JGeometry::TVec3<f32> axis(0.0f, 0.0f, 0.0f);
	if (xLen != 0.0f) {
		axis.x = (cross.x * jointMtx[0][0] + cross.y * jointMtx[1][0]
		          + cross.z * jointMtx[2][0])
		         / xLen;
	}
	if (yLen != 0.0f) {
		axis.y = (cross.x * jointMtx[0][1] + cross.y * jointMtx[1][1]
		          + cross.z * jointMtx[2][1])
		         / yLen;
	}
	if (zLen != 0.0f) {
		axis.z = (cross.x * jointMtx[0][2] + cross.y * jointMtx[1][2]
		          + cross.z * jointMtx[2][2])
		         / zLen;
	}

	Mtx roll;
	PSMTXRotAxisRad(roll, &axis, 0.017453292f * head->mRollAngle);
	PSMTXConcat(jointMtx, roll, jointMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, roll, J3DSys::mCurrentMtx);
	return 1;
}

static inline void initMarioGoal(TSpineEnemy* sambo)
{
	sambo->setGoalPathMario();
}

DEFINE_NERVE(TNerveSamboHeadHitWall, TLiveActor)
{
	TSamboHead* self = (TSamboHead*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(1);

		if (JPABaseEmitter* emitter = gpMarioParticleManager->emitWithRotate(
		        0xE2, &self->mPosition, 0,
		        self->mRotation.y * 182.04445f, 0, 0, nullptr)) {
			JGeometry::TVec3<f32> scale(1.5f, 1.5f, 1.5f);
			emitter->setScale(scale);
		}
		if (JPABaseEmitter* emitter = gpMarioParticleManager->emitWithRotate(
		        0xE3, &self->mPosition, 0,
		        self->mRotation.y * 182.04445f, 0, 0, nullptr)) {
			JGeometry::TVec3<f32> scale(1.5f, 1.5f, 1.5f);
			emitter->setScale(scale);
			SMSSetEmitterPolColor(emitter, 6);
		}
	}

	s32 wait = ((TSmallEnemyManager*)self->mManager)->unk5C;
	if (self->checkCurAnmEnd(0)
	    && spine->getTime()
	           > wait + self->mMActor->getFrameCtrl(0)->getEnd()) {
		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->onLiveFlag(LIVE_FLAG_UNK8);
		self->onLiveFlag(LIVE_FLAG_UNK20000);
		self->offLiveFlag(LIVE_FLAG_UNK10000);
		self->mHolder = nullptr;
		self->stopAnmSound();

		spine->reset();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		spine->pushAfterCurrent(&TNerveSmallEnemyDie::theNerve());
		self->genRandomItem();
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveSamboHeadRecoverWater, TLiveActor)
{
	TSamboHead* self = (TSamboHead*)spine->getBody();
	if (spine->getTime() == 0)
		self->setBckAnm(12);

	self->mRollAngle *= 0.99f;

	if (self->checkCurAnmEnd(0) && self->mRollAngle < 1.0f)
		return TRUE;

	return FALSE;
}

DEFINE_NERVE(TNerveSamboHeadHitWater, TLiveActor)
{
	TSamboHead* self = (TSamboHead*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(5);
		self->unk1A0 = self->mVelocity;
	}

	if (self->isHitWallInBound()) {
		self->mRollAngle = 0.0f;
		if (self->unk1B0)
			self->mRotation.y += 180.0f;

		spine->reset();
		spine->setNext(&TNerveSamboHeadHitWall::theNerve());
		spine->pushAfterCurrent(&TNerveSamboHeadHitWall::theNerve());
		return TRUE;
	}

	f32 maxRoll = self->mParams->mSLJumpAngY.get();
	if (self->isBckAnm(6)) {
		if (spine->getTime() < 100) {
			f32 roll = self->mRollAngle - 3.0f;
			if (roll > maxRoll)
				roll = maxRoll;
			else if (roll < -maxRoll)
				roll = -maxRoll;
			self->mRollAngle = roll;
		} else {
			f32 roll = self->mRollAngle + 3.0f;
			if (roll > 0.0f)
				roll = 0.0f;
			else if (roll < -maxRoll)
				roll = -maxRoll;
			self->mRollAngle = roll;
		}
	} else {
		f32 roll = self->mRollAngle + 3.0f;
		if (roll > maxRoll)
			roll = maxRoll;
		else if (roll < -maxRoll)
			roll = -maxRoll;
		self->mRollAngle = roll;
	}

	if (self->isBckAnm(5) && !self->isAirborne())
		self->setBckAnm(7);

	if (self->isBckAnm(6)) {
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->mPosition.y = self->mGroundHeight + 1.0f;
		self->mVelocity   = self->unk1A0;
	}

	if (!self->isAirborne())
		self->setBckAnm(6);

	if (self->checkCurAnmEnd(0) && self->isBckAnm(6)) {
		f32 rate = self->mParams->mSLHitJumpSpRateXZ.get();
		self->setBckAnm(6);
		self->unk1A0.x *= rate;
		self->unk1A0.z *= rate;
		self->unk1A0.y = 0.0f;
		self->mVelocity = self->unk1A0;
		self->mPosition.y = self->mGroundHeight;
		self->offLiveFlag(LIVE_FLAG_AIRBORNE);
		self->setBckAnm(12);
		spine->setNext(&TNerveSamboHeadAttack::theNerve());
		spine->pushAfterCurrent(&TNerveSamboHeadRecoverWater::theNerve());
		return TRUE;
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveSamboHeadHide, TLiveActor)
{
	TSamboHead* self = (TSamboHead*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(4);
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		gpMarioParticleManager->emit(0xB8, &self->mPosition, 0, nullptr);
		gpMarioParticleManager->emit(0xB9, &self->mPosition, 0, nullptr);
	} else if (self->checkCurAnmEnd(0)) {
		self->onLiveFlag(LIVE_FLAG_HIDDEN);
		self->setBckAnm(12);

		if (!self->unk198) {
			self->unk198 = gpConductor->makeOneEnemyAppear(
			    self->mPosition, "サンボフラワーマネージャー", 1);
			((TSpineEnemy*)self->unk198)->reset();
		}

		self->unk198->offHitFlag(HIT_FLAG_NO_COLLISION);
		self->unk198->onLiveFlag(LIVE_FLAG_UNK10);
		self->unk198->offLiveFlag(LIVE_FLAG_DEAD);
		self->unk198->mPosition.y = self->mGroundHeight;
		self->unk198->mPosition   = self->mPosition;
	} else if (self->isFindMario(1.0f)) {
		self->updateSquareToMario();
		f32 appearDist = self->mParams->mSLAppearDist.get();
		if (self->mDistToMarioSquared < appearDist * appearDist) {
			spine->pushAfterCurrent(&TNerveSamboHeadAppear::theNerve());
			return TRUE;
		}
	}

	self->walkToCurPathNode(0.0f, self->mTurnSpeed, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveSamboHeadAttack, TLiveActor)
{
	TSamboHead* self = (TSamboHead*)spine->getBody();
	if (!self->isAirborne()) {
		if (self->unk19C > self->mParams->mSLJumpPrepareTime.get()
		    && self->checkCurAnmEnd(0)) {
			self->unk19C = 0;
			self->updateSquareToMario();

			JGeometry::TVec3<f32> target = self->unk104.getPoint();
			target.x = gpMarioPos->x - self->mPosition.x;
			target.y = 0.0f;
			target.z = gpMarioPos->z - self->mPosition.z;
			if (target.x == 0.0f && target.y == 0.0f
			    && target.z == 0.0f)
				target.x += 1.0f;

			MsVECNormalize(&target, &target);
			f32 moveDist = self->mParams->mSLMoveDist.get();
			target.x     = self->mPosition.x + target.x * moveDist;
			target.z     = self->mPosition.z + target.z * moveDist;
			target.y     = self->mPosition.y;

			f32 jumpSp = self->mParams->mSLJumpSp.get();
			self->mVelocity = self->calcVelocityToJumpToY(
			    target, jumpSp, self->getGravityY());
			self->mPosition.y += 2.0f;
			self->onLiveFlag(LIVE_FLAG_AIRBORNE);
			self->setBckAnm(8);
		} else {
			++self->unk19C;
		}

		if (self->checkCurAnmEnd(0) && self->isBckAnm(7))
			self->setBckAnm(12);

		MActor* actor = self->mMActor;
		actor->setFrameRate(SMSGetAnmFrameRate(), 0);
	} else {
		JGeometry::TVec3<f32> velocity = self->mVelocity;
		if (velocity.y < 0.0f && self->isBckAnm(8)) {
			self->setBckAnm(7);
			self->mMActor->setFrameRate(0.0f, 0);
		}
	}

	if (self->mPosition.y > self->mGroundHeight + 30.0f) {
		f32 maxRoll = self->mParams->mSLJumpAngY.get();
		JGeometry::TVec3<f32> velocity = self->mVelocity;
		f32 roll = MsGetRotFromZaxis(velocity).x;
		if (roll > maxRoll)
			roll = maxRoll;
		else if (roll < -maxRoll)
			roll = -maxRoll;
		self->mRollAngle = roll;
	} else {
		self->mRollAngle *= 0.8f;
	}

	f32 turnSpeed = self->mTurnSpeed;
	if (self->isAirborne())
		turnSpeed = 5.0f;
	self->walkToCurPathNode(0.0f, turnSpeed, 0.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveSamboHeadAppear, TLiveActor)
{
	TSamboHead* self = (TSamboHead*)spine->getBody();
	if (spine->getTime() == 0) {
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->offHitFlag(HIT_FLAG_NO_COLLISION);

		TSamboFlower* flower = (TSamboFlower*)self->unk198;
		if (flower->unk150) {
			flower->unk150 = true;
			flower->unk154 = 0;
			gpMarioParticleManager->emit(0xB2, &flower->mPosition, 0,
			                             nullptr);
			flower->mMActor->setBck("flower_hit");
			if (flower->unk160 && flower->unk164) {
				--*flower->unk164;
				u32 soundID = *flower->unk164 + 0x89B9;
				if (gpMSound->gateCheck(soundID))
					MSoundSESystem::MSoundSE::startSoundActor(
					    soundID, &flower->mPosition, 0, nullptr, 0, 4);
			}
			self->setBckAnm(10);
		} else {
			self->setBckAnm(10);
		}

		gpMarioParticleManager->emit(0xB6, &self->mPosition, 0, nullptr);
		gpMarioParticleManager->emit(0xB7, &self->mPosition, 0, nullptr);

		flower = (TSamboFlower*)self->unk198;
		flower->onHitFlag(HIT_FLAG_NO_COLLISION);
		flower->mMActor->setBck("flower_fwait");
		flower->onLiveFlag(LIVE_FLAG_DEAD);
	}

	if (spine->getTime() == 20) {
		TSamboFlower* flower = (TSamboFlower*)self->unk198;
		((TSamboFlowerManager*)flower->mManager)
		    ->dropLeaf(self->mPosition, self->mScaling);
	}

	if (self->checkCurAnmEnd(0)) {
		self->setBckAnm(12);
		spine->pushAfterCurrent(&TNerveSamboHeadAttack::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveHanaSamboFreeze, TLiveActor)
{
	THanaSambo* self = (THanaSambo*)spine->getBody();
	if (spine->getTime() == 0)
		self->setBckAnm(5);

	if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(5)) {
			if (self->unsetUnk165())
				self->setBckAnm(8);
		} else {
			if (self->unsetUnk165())
				self->setBckAnm(8);
			else
				return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveHanaSamboDie, TLiveActor)
{
	THanaSambo* self = (THanaSambo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->setDeadAnm();
	} else if (self->checkCurAnmEnd(0) || spine->getTime() > 300) {
		static int jIndexTable[] = { 1, 3, 4, 5 };

		for (int i = 0; i < 4; ++i) {
			MtxPtr mtx
			    = self->mMActor->getModel()->mNodeMatrices[jIndexTable[i]];
			self->mPollenPositions[i].set(mtx[0][3], mtx[1][3], mtx[2][3]);

			JPABaseEmitter* emitter = gpMarioParticleManager->emit(
			    0xE4, &self->mPollenPositions[i], 0, nullptr);
			if (emitter)
				emitter->setScale(self->mScaling);

			emitter = gpMarioParticleManager->emit(
			    0xE6, &self->mPollenPositions[i], 0, nullptr);
			if (emitter)
				emitter->setScale(self->mScaling);
		}

		self->onLiveFlag(LIVE_FLAG_DEAD);
		self->onLiveFlag(LIVE_FLAG_UNK8);
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->offLiveFlag(LIVE_FLAG_UNK10000);
		self->mHolder = nullptr;
		self->stopAnmSound();
		self->onHitFlag(HIT_FLAG_NO_COLLISION);

		spine->reset();
		spine->setNext(&TNerveSmallEnemyDie::theNerve());
		spine->pushAfterCurrent(spine->getDefault());
		self->genRandomItem();
	}

	return FALSE;
}

DEFINE_NERVE(TNerveHanaSamboHide, TLiveActor)
{
	THanaSambo* self = (THanaSambo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(4);
		gpMarioParticleManager->emit(0xB8, &self->mPosition, 0, nullptr);
		gpMarioParticleManager->emit(0xB9, &self->mPosition, 0, nullptr);
	}

	if (spine->getTime() == 75) {
		if (!self->mFlower) {
			self->mFlower = (TSamboFlower*)gpConductor->makeOneEnemyAppear(
			    self->mPosition, "サンボフラワーマネージャー", 1);
			self->mFlower->reset();
		}

		self->mFlower->onLiveFlag(LIVE_FLAG_UNK10);
		self->mFlower->offLiveFlag(LIVE_FLAG_DEAD);
		self->mFlower->mPosition = self->mPosition;
		self->mFlower->mPosition.y = self->mGroundHeight;
	}

	if (self->checkCurAnmEnd(0)) {
		if (!self->mFlower) {
			self->mFlower = (TSamboFlower*)gpConductor->makeOneEnemyAppear(
			    self->mPosition, "サンボフラワーマネージャー", 1);
			self->mFlower->reset();
		}

		self->mFlower->onLiveFlag(LIVE_FLAG_UNK10);
		self->mFlower->offLiveFlag(LIVE_FLAG_DEAD);
		self->mFlower->mPosition = self->mPosition;
		self->mFlower->mPosition.y = self->mGroundHeight;
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->setBckAnm(6);
		self->mMActor->setFrameRate(0.0f, 0);
		self->onLiveFlag(LIVE_FLAG_HIDDEN);
	}

	self->updateSquareToMario();
	f32 appearDist = self->mParams->mSLAppearDist.get();
	if (self->mDistToMarioSquared < appearDist * appearDist) {
		spine->pushAfterCurrent(&TNerveHanaSamboAppear::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveHanaSamboAttack, TLiveActor)
{
	THanaSambo* self = (THanaSambo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->setBckAnm(3);
		self->mUseYDownAnim = true;
	} else if (self->checkCurAnmEnd(0)) {
		if (self->isBckAnm(3)) {
			self->createPollen();
			if (gpMSound->gateCheck(0x291B))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x291B, &self->mPosition, 0, nullptr, 0, 4);
			self->setBckAnm(1);
		} else if (self->isBckAnm(1)) {
			if (spine->getTime() > self->mParams->mSLAttackingTime.get()) {
				if (!self->unsetUnk165())
					self->setBckAnm(2);
				else
					self->setBckAnm(1);
			} else {
				self->setBckAnm(1);
			}
		} else {
			spine->pushAfterCurrent(&TNerveHanaSamboWait::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveHanaSamboWait, TLiveActor)
{
	THanaSambo* self = (THanaSambo*)spine->getBody();
	if (spine->getTime() == 0)
		self->setWaitAnm();

	if (!self->checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		if (self->mMActor->getFrameCtrl(0)->checkPass(0.001f))
			self->createPollen();
	}

	if (spine->getTime() > self->mParams->mSLAttackInterval.get()
	    && gpMarioPos->y < self->mPosition.y + 12.0f) {
		self->updateSquareToMario();
		f32 attackDist = self->mParams->mSLAttackDist.get();
		if (self->mDistToMarioSquared < attackDist * attackDist) {
			spine->pushAfterCurrent(&TNerveHanaSamboAttack::theNerve());
			return TRUE;
		}
	}

	self->updateSquareToMario();
	f32 hideDist = self->mParams->mSLHideDist.get();
	if (self->mDistToMarioSquared > hideDist * hideDist) {
		spine->pushAfterCurrent(&TNerveHanaSamboHide::theNerve());
		return TRUE;
	}

	self->walkToCurPathNode(16.0f, self->mTurnSpeed, 16.0f);
	return FALSE;
}

DEFINE_NERVE(TNerveHanaSamboAppear, TLiveActor)
{
	THanaSambo* self = (THanaSambo*)spine->getBody();
	if (spine->getTime() == 0) {
		self->offLiveFlag(LIVE_FLAG_HIDDEN);
		self->offHitFlag(HIT_FLAG_NO_COLLISION);
		self->setBckAnm(6);
		gpMarioParticleManager->emit(0xB6, &self->mPosition, 0, nullptr);
		gpMarioParticleManager->emit(0xB7, &self->mPosition, 0, nullptr);

		TSamboFlower* flower = self->mFlower;
		flower->onHitFlag(HIT_FLAG_NO_COLLISION);
		flower->mMActor->setBck("flower_fwait");
		flower->onLiveFlag(LIVE_FLAG_DEAD);
	}

	if (self->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveHanaSamboWait::theNerve());
		self->setWaitAnm();

		if (gpMarioPos->y < self->mPosition.y + 100.0f) {
			self->updateSquareToMario();
			f32 attackDist = self->mParams->mSLAttackDist.get();
			if (self->mDistToMarioSquared < attackDist * attackDist)
				spine->pushAfterCurrent(&TNerveHanaSamboAttack::theNerve());
		}

		return TRUE;
	}

	self->walkToCurPathNode(0.0f, 3.0f * self->mTurnSpeed, 0.0f);
	return FALSE;
}

TSamboFlowerSaveLoadParams::TSamboFlowerSaveLoadParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLLeafVelocityXZ, 4.0f)
    , PARAM_INIT(mSLLeafVelocityY, 6.0f)
    , PARAM_INIT(mSLLeafGravity, 0.2f)
    , PARAM_INIT(mSLBudDist, 2000.0f)
    , PARAM_INIT(mSLBloomTimer, 300)
    , PARAM_INIT(mSLCoinCircleR, 100.0f)
    , PARAM_INIT(mSLCoinVelocityXZ, 12.0f)
    , PARAM_INIT(mSLCoinVelocityY, 10.0f)
    , PARAM_INIT(mSLSeedShootRange, 500.0f)
    , PARAM_INIT(mSLSeedShootInterval, 200)
    , PARAM_INIT(mSLSeedGravity, 0.1f)
    , PARAM_INIT(mSLSeedSpeedXZ, 10.0f)
    , PARAM_INIT(mSLSeedSpeedY, 10.0f)
{
	TParams::load(mPrmPath);
}

void TSamboFlower::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("flower.bmd", 3);
	MActor* actor                = mMActor;
	TSamboFlowerManager* manager = (TSamboFlowerManager*)mManager;
	J3DModel* model              = actor->getModel();
	model->getModelData()->setMaterialTable(manager->mMaterialTable,
	                                        J3DMatCopyFlag_All);
	actor->initDL();
	actor->getModel()->lock();
}

void TSamboFlower::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);
	setMActorAndKeeper();

	unk130  = 1;
	mParams = (TSamboFlowerSaveLoadParams*)getSaveParam();
	if (mParams) {
		mBodyRadius       = mParams->mSLBodyRadius.get();
		mWallRadius       = mParams->mSLWallRadius.get();
		mHeadHeight       = mParams->mSLHeadHeight.get();
		mScaledBodyRadius = mBodyScale * mBodyRadius;
	}

	initHitActor(0x10000027, 1, 0x80000000, mBodyRadius, mHeadHeight,
	             mBodyRadius, mHeadHeight);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	offLiveFlag(LIVE_FLAG_UNK400);
	initAnmSound();
	mActorType = 0x10000027;
	unk150     = false;
	onLiveFlag(LIVE_FLAG_DEAD);
}

void TSamboFlower::loadAfter()
{
	JDrama::TNameRef::loadAfter();
	if (unk15C >= 0) {
		TMapObjBase* coin = TMapObjBaseManager::newAndRegisterObj(
		    "coin", JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
		    JGeometry::TVec3<f32>(0.0f, 0.0f, 0.0f),
		    JGeometry::TVec3<f32>(1.0f, 1.0f, 1.0f));
		if (coin)
			unk168 = coin;
	}
}

void TSamboFlower::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
	stream.read(&unk15C, sizeof(unk15C));
	stream.read(&unk158, sizeof(unk158));
	unk160 = true;
	reset();
	offLiveFlag(LIVE_FLAG_UNK800);
}

void TSamboFlower::drawObject(JDrama::TGraphics*)
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	TCircleShadowRequest request;
	request.unk0  = mPosition;
	request.unk10 = 120.0f;
	request.unkC  = 120.0f;
	gpBindShadowManager->request(request, getActorType() & 0xFFFF0000);

	mMActor->setLightData(mGroundPlane, mPosition);
	mMActor->entry();
}

void TSamboFlower::moveObject()
{
	if (unk150) {
		if (checkCurAnmEnd(0) && mMActor->checkCurAnm("flower_hit", 0))
			mMActor->setBck("flower_fwait");

		if (unk160) {
			++unk154;
			if (unk154 > mParams->mSLBloomTimer.get()) {
				if (mMActor->checkCurAnm("flower_fwait", 0)) {
					unk150 = false;
					if (unk164)
						++*unk164;

					mMActor->setBck("flower_hit");
					J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
					s16 end            = ctrl->getEnd();
					ctrl               = mMActor->getFrameCtrl(0);
					ctrl->setFrame(end);
					mMActor->setFrameRate(-SMSGetAnmFrameRate(), 0);
				} else {
					J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(0);
					if (ctrl->getFrame() < 1.0f) {
						unk154 = 0;
						mMActor->setBck("flower_wait");
					}
				}
			}
		}
	}

	if (!checkLiveFlag(LIVE_FLAG_UNK10)) {
		mPosition.y = gpMap->checkGround(mPosition.x, mPosition.y + 100.0f,
		                                 mPosition.z, &mGroundPlane);
	}
}

void TSamboFlower::reset()
{
	TSpineEnemy::reset();
	mMActor->setBck("flower_wait");
	offHitFlag(HIT_FLAG_NO_COLLISION);
	offLiveFlag(LIVE_FLAG_UNK800);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_UNK10);
}

BOOL TSamboFlower::receiveMessage(THitActor*, u32 message)
{
	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		if (!unk150) {
			unk150 = true;
			unk154 = 0;
			gpMarioParticleManager->emit(0xB2, &mPosition, 0, nullptr);
			mMActor->setBck("flower_hit");

			if (unk160 && unk164) {
				--*unk164;
				u32 soundID = *unk164 + 0x89B9;
				if (gpMSound->gateCheck(soundID)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    soundID, &mPosition, 0, nullptr, 0, 4);
				}
			}
		}

		return TRUE;
	}

	return FALSE;
}

void TSamboFlower::control() { }

void TSamboFlowerManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk38          = new TSamboFlowerSaveLoadParams("/enemy/samboflower.prm");
	mMaterialTable = J3DModelLoaderDataBase::loadMaterialTable(
	    JKRFileLoader::getGlbResource(
	        "/scene/samboflower/flower_orange.bmt"));
}

void TSamboFlowerManager::loadAfter()
{
	void* res = JKRFileLoader::getGlbResource("/scene/samboflower/leaf.bmd");
	SDLModelData* leafModelData
	    = new SDLModelData(J3DModelLoaderDataBase::load(res, 0x10210000));

	mLeaves = new TSamboLeaf*[18];
	for (int i = 0; i < 18; ++i) {
		TSamboLeaf* leaf = new TSamboLeaf("サンボリーフ", nullptr, this);
		leaf->mModel    = new SDLModel(leafModelData, 3, 1);
		mLeaves[i]      = leaf;
	}

	mCoinUnitCount = 0;
	for (int i = 0; i < gpItemManager->getObjNum(); ++i) {
		THitActor* item = gpItemManager->getObj(i);
		if (strstr(item->getName(), "コイン（フラワー用）"))
			++mCoinUnitCount;
	}

	mCoinUnits = new TSamboFlowerCoinUnit*[mCoinUnitCount];
	int* counts = new int[mCoinUnitCount];
	for (int i = 0; i < mCoinUnitCount; ++i)
		counts[i] = 0;

	for (int i = 0; i < getObjNum(); ++i) {
		TSamboFlower* flower = (TSamboFlower*)getObj(i);
		if (strstr(flower->getName(), "フラワー（コイン用）")
		    && flower->unk158 < mCoinUnitCount) {
			++counts[flower->unk158];
		}
	}

	for (int i = 0; i < mCoinUnitCount; ++i)
		mCoinUnits[i] = new TSamboFlowerCoinUnit(counts[i]);

	for (int i = 0; i < gpItemManager->getObjNum(); ++i) {
		TMapObjBase* coin = (TMapObjBase*)gpItemManager->getObj(i);
		if (!strstr(coin->getName(), "コイン（フラワー用）"))
			continue;

		int unitIndex = ((TFlowerCoin*)coin)->unk158;
		if (unitIndex >= mCoinUnitCount)
			continue;

		TSamboFlowerCoinUnit* unit = mCoinUnits[unitIndex];
		unit->mCoin                = coin;
		unit->mCenter              = coin->mPosition;
		coin->makeObjDead();
	}

	for (int i = 0; i < getObjNum(); ++i) {
		TSamboFlower* flower = (TSamboFlower*)getObj(i);
		if (!strstr(flower->getName(), "フラワー（コイン用）"))
			continue;

		int unitIndex = flower->unk158;
		if (unitIndex >= mCoinUnitCount)
			continue;

		TSamboFlowerCoinUnit* unit = mCoinUnits[unitIndex];
		if (unit->mFlowerCount >= unit->mCapacity)
			continue;

		unit->mFlowers[unit->mFlowerCount] = flower;
		flower->unk164                    = &unit->unk1C;
		++unit->mFlowerCount;
	}

	JDrama::TNameRef::loadAfter();
}

void TSamboFlowerManager::dropLeaf(JGeometry::TVec3<f32>& position,
                                   JGeometry::TVec3<f32>& scale)
{
	static const f32 angles[] = { 0.0f, 120.0f, 240.0f };
	int dropped               = 0;

	for (int i = 0; i < 18 && dropped < 3; ++i) {
		TSamboLeaf* leaf = mLeaves[i];
		if (leaf->mActive)
			continue;

		leaf->mPosition = position;
		leaf->mPosition.y += 10.0f;
		leaf->mActive = true;

		TSamboFlowerSaveLoadParams* params
		    = (TSamboFlowerSaveLoadParams*)getSaveParam();
		f32 minXZ = params->mSLLeafVelocityXZ.get();
		f32 minY  = params->mSLLeafVelocityY.get();
		TMsRange<f32> yRange(minY, minY * 1.2f);
		TMsRange<f32> xzRange(minXZ, minXZ * 1.2f);
		f32 randXZ = xzRange.rand();
		f32 randY  = yRange.rand();

		JGeometry::TVec3<f32> velocity(0.0f, randY, randXZ);
		Mtx rot;
		MsMtxSetRotRPH(rot, 0.0f, angles[dropped], 0.0f);
		PSMTXMultVec(rot, (Vec*)&velocity, (Vec*)&velocity);
		leaf->mVelocity = velocity;

		leaf->mRotation.set(0.0f, angles[dropped] - 90.0f, 0.0f);
		leaf->mScale = scale;
		++dropped;
	}
}

void TSamboFlowerManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mCoinUnitCount > 0 && (flags & 2)) {
		for (int i = 0; i < mCoinUnitCount; ++i)
			mCoinUnits[i]->checkGenCoin();
	}

	TEnemyManager::perform(flags, graphics);

	for (int i = 0; i < 18; ++i)
		mLeaves[i]->perform(flags, graphics);
}

void TSamboFlowerManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "flower.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TSamboFlowerManager::createEnemyInstance()
{
	return new TSamboFlower("サンボフラワー");
}

void TSamboFlowerCoinUnit::checkGenCoin()
{
	if (!mCoin)
		return;

	bool ready = true;
	for (int i = 0; i < mFlowerCount; ++i) {
		if (!mFlowers[i]->getMActor()->checkCurAnm("flower_fwait", 0))
			ready = false;
	}

	if (!ready)
		return;

	int coinFlowerCount = 0;
	for (int i = 0; i < mFlowerCount; ++i) {
		if (mFlowers[i]->unk168)
			++coinFlowerCount;
		mFlowers[i]->unk160 = false;
	}

	if (coinFlowerCount <= 0) {
		mCoin = nullptr;
		return;
	}

	int spawned = 0;
	for (int i = 0; i < mFlowerCount; ++i) {
		TSamboFlower* flower = mFlowers[i];
		if (!flower->unk168)
			continue;

		TSamboFlowerSaveLoadParams* params = flower->getSamboFlowerParams();
		f32 ratio = spawned / (f32)coinFlowerCount;
		JGeometry::TVec3<f32> offset(0.0f, 0.0f,
		                             params->mSLCoinCircleR.get());
		Mtx rot;
		MsMtxSetRotRPH(rot, 0.0f, 360.0f * ratio, 0.0f);
		PSMTXMultVec(rot, (Vec*)&offset, (Vec*)&offset);

		TMapObjBase* coin = flower->unk168;
		if (coin->getActorType() == 0x2000000E)
			coin = gpItemManager->makeObjAppear(0x2000000E);

		if (!coin)
			continue;

		coin->makeObjAppeared();
		JGeometry::TVec3<f32> coinPos = mCenter;
		coinPos.add(offset);
		coin->mPosition = coinPos;

		JGeometry::TVec3<f32> dir = offset;
		MsVECNormalize((Vec*)&dir, (Vec*)&dir);
		coin->mVelocity.x = dir.x * params->mSLCoinVelocityXZ.get();
		coin->mVelocity.y = params->mSLCoinVelocityY.get() + 8.0f * ratio;
		coin->mVelocity.z = dir.z * params->mSLCoinVelocityXZ.get();
		coin->offLiveFlag(LIVE_FLAG_UNK10);
		++spawned;
	}

	if (gpMSound->gateCheck(0x4813))
		MSoundSESystem::MSoundSE::startSoundSystemSE(0x4813, spawned,
		                                             nullptr, 0);

	mCoin = nullptr;
}

void TSamboLeaf::perform(u32 flags, JDrama::TGraphics*)
{
	if (!mActive)
		return;

	if (flags & 1) {
		mPosition.add(mVelocity);
		if (mVelocity.y > -20.0f)
			mVelocity.y
			    -= ((TSamboFlowerSaveLoadParams*)mManager->getSaveParam())
			           ->mSLLeafGravity.get();

		const TBGCheckData* ground;
		f32 groundY = gpMap->checkGround(mPosition.x, mPosition.y + 20.0f,
		                                 mPosition.z, &ground);
		if (mPosition.y < groundY)
			mActive = false;
	}

	if (flags & 2) {
		Mtx baseMtx;
		MtxPtr baseMtxPtr = baseMtx;
		MsMtxSetXYZRPH(baseMtxPtr, mPosition.x, mPosition.y, mPosition.z,
		               0.0f, mRotation.y, 0.0f);

		f32 rotX = MsGetRotFromZaxis(mVelocity).x;
		f32 sin  = JMASin(-rotX);
		f32 cos  = JMACos(-rotX);

		Mtx rollMtx;
		rollMtx[0][0] = cos;
		rollMtx[0][1] = -sin;
		rollMtx[0][2] = 0.0f;
		rollMtx[0][3] = 0.0f;
		rollMtx[1][0] = sin;
		rollMtx[1][1] = cos;
		rollMtx[1][2] = 0.0f;
		rollMtx[1][3] = 0.0f;
		rollMtx[2][0] = 0.0f;
		rollMtx[2][1] = 0.0f;
		rollMtx[2][2] = 1.0f;
		rollMtx[2][3] = 0.0f;

		PSMTXConcat(baseMtxPtr, rollMtx, baseMtxPtr);
		PSMTXCopy(baseMtxPtr, mModel->getBaseTRMtx());
		mModel->setBaseScale(mScale);
		mModel->calc();
	}

	if (flags & 4)
		mModel->viewCalc();

	if (flags & 0x200)
		mModel->entry();
}

TSamboHeadManager::TSamboHeadManager(const char* name)
    : TSmallEnemyManager(name)
{
	gpCurSamboHead = nullptr;
}

void TSamboHeadManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TSamboHeadSaveLoadParams("/enemy/sambohead.prm");
}

void TSamboHeadManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "samboHead.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* TSamboHeadManager::createEnemyInstance()
{
	return new TSamboHead("サンボヘッド");
}

TSamboHead::TSamboHead(const char* name)
    : TWalkerEnemy(name)
    , mParams(nullptr)
    , unk198(nullptr)
    , unk19C(0)
    , mRollAngle(0.0f)
    , unk1B0(0)
{
}

void TSamboHead::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
	initMarioGoal(this);
}

void TSamboHead::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x1000001B;
	unk150     = 17;
	mParams    = (TSamboHeadSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveSamboHeadHide::theNerve());
	initMarioGoal(this);
	mMActor->setJointCallback(mBodyJntIndex, SamboHeadRollCallback);
}

void TSamboHead::calcRootMatrix()
{
	gpCurSamboHead = this;
	TSpineEnemy::calcRootMatrix();
}

void TSamboHead::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("samboHead.bmd", 3);
}

const char** TSamboHead::getBasNameTable() const { return sambohead_bastable; }

void TSamboHead::genEventCoin()
{
	if (isBckAnm(1)) {
		for (int i = 0; i < 3; ++i) {
			Mtx rot;
			JGeometry::TVec3<f32> offset(0.0f, 0.0f, 100.0f);
			f32 angle = mRotation.y - 60.0f + 60.0f * i;
			s16 angleY = (s16)(angle * 182.04445f);
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

			TMapObjBase* coin;
			if (i == 1 && mCoin) {
				coin = mCoin;
				if (coin->isActorType(0x2000000E))
					coin = gpItemManager->makeObjAppear(0x2000000E);

				if (coin) {
					coin->makeObjAppeared();
					coin->mPosition = mPosition;
				}
			} else {
				coin = gpItemManager->makeObjAppear(
				    mPosition.x + offset.x, mPosition.y,
				    mPosition.z + offset.z, 0x2000000E, true);
			}

			if (coin) {
				coin->mPosition.y = mPosition.y;
				MsVECNormalize(&offset, &offset);
				TMsRange<f32> ySpeedRange(8.0f, 16.0f);
				coin->mVelocity.x = offset.x * 4.0f;
				coin->mVelocity.y = ySpeedRange.rand();
				coin->mVelocity.z = offset.z * 4.0f;
				coin->offLiveFlag(LIVE_FLAG_UNK10);
			}
		}
	} else {
		TSmallEnemy::genEventCoin();
	}
}

void TSamboHead::reset()
{
	gpCurSamboHead = this;
	TWalkerEnemy::reset();
	unk165     = false;
	unk1B0     = 0;
	unk19C     = 0;
	mRollAngle = 0.0f;
	offLiveFlag(LIVE_FLAG_UNK8);
}

void TSamboHead::kill()
{
	mHitPoints = 1;
	if (mSpine->getCurrentNerve() != &TNerveSmallEnemyDie::theNerve()) {
		mSpine->reset();
		mSpine->setNext(&TNerveSmallEnemyDie::theNerve());
		mSpine->pushAfterCurrent(&TNerveSmallEnemyDie::theNerve());
	}
	onLiveFlag(LIVE_FLAG_UNK40);
}

void TSamboHead::setDeadAnm() { setBckAnm(3); }

void TSamboHead::setAfterDeadEffect()
{
	if (JPABaseEmitter* emitter = gpMarioParticleManager->emit(
	        isBckAnm(1) ? 0xE5 : 0xE4, &mPosition, 0, nullptr)) {
		JGeometry::TVec3<f32> scale(1.5f, 1.5f, 1.5f);
		emitter->setScale(scale);
	}

	if (JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0xE6, &mPosition, 0, nullptr)) {
		JGeometry::TVec3<f32> scale(1.5f, 1.5f, 1.5f);
		emitter->setScale(scale);
	}

	if (gpMSound->gateCheck(0x295F))
		MSoundSESystem::MSoundSE::startSoundActor(0x295F, &mPosition, 0,
		                                          nullptr, 0, 4);
}

f32 TSamboHead::getGravityY() const
{
	f32 gravity = mGravity;
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadAttack::theNerve())
		gravity = mParams->mSLMoveGravity.get();
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadHitWater::theNerve())
		gravity = mParams->mSLHitJumpGravity.get();
	return gravity;
}

void TSamboHead::attackToMario()
{
	sendAttackMsgToMario();
	BOOL airborne;
	if (checkLiveFlag(LIVE_FLAG_AIRBORNE))
		airborne = TRUE;
	else
		airborne = FALSE;

	if (airborne) {
		JGeometry::TVec3<f32> velocity(mPosition.x - gpMarioPos->x, 10.0f,
		                               mPosition.z - gpMarioPos->z);
		MsVECNormalize(&velocity, &velocity);
		velocity.x *= 8.0f;
		velocity.y *= 8.0f;
		velocity.z *= 8.0f;
		mVelocity = velocity;
	} else if (mSpine->getCurrentNerve()
	           == &TNerveSamboHeadAttack::theNerve()) {
		mSpine->pushNerve(&TNerveSmallEnemyFreeze::theNerve());
	}
}

void TSamboHead::behaveToWater(THitActor*)
{
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadHide::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadAppear::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveSamboHeadHitWall::theNerve())
		return;
	if (mSpine->getCurrentNerve() == &TNerveSmallEnemyDie::theNerve())
		return;

	JGeometry::TVec3<f32> prevVelocity(mVelocity);
	prevVelocity.y = 0.0f;

	JGeometry::TVec3<f32> velocity(mPosition.x - gpMarioPos->x, 0.0f,
	                               mPosition.z - gpMarioPos->z);
	MsVECNormalize(&velocity, &velocity);
	f32 hitJumpSpXZ = mParams->mSLHitJumpSpXZ.get();
	velocity.x *= hitJumpSpXZ;
	velocity.y *= hitJumpSpXZ;
	velocity.z *= hitJumpSpXZ;
	velocity.y = mParams->mSLHitJumpSpY.get();

	if (mSpine->getCurrentNerve() != &TNerveSamboHeadHitWater::theNerve()) {
		mSpine->pushNerve(&TNerveSamboHeadHitWater::theNerve());
	} else {
		velocity.add(prevVelocity);
	}

	mVelocity = velocity;
	mPosition.y += 2.0f;
	onLiveFlag(LIVE_FLAG_AIRBORNE);
}

THanaSamboManager::THanaSamboManager(const char* name)
    : TSmallEnemyManager(name)
{
}

void THanaSamboManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new THanaSamboSaveLoadParams("/enemy/hanasambo.prm");
}

void THanaSamboManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "sambo.bmd", 0x10220000, 0 },
		{ "samboD.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSpineEnemy* THanaSamboManager::createEnemyInstance()
{
	return new THanaSambo("ハナサンボ");
}

THanaSambo::THanaSambo(const char* name)
    : TSmallEnemy(name)
    , mHead(nullptr)
    , mParams(nullptr)
    , mFlower(nullptr)
    , mBindShadow(nullptr)
{
}

void THanaSambo::load(JSUMemoryInputStream& stream)
{
	TSmallEnemy::load(stream);
	reset();
	initMarioGoal(this);
}

void THanaSambo::init(TLiveManager* manager)
{
	TSmallEnemy::init(manager);
	mActorType = 0x1000001A;
	unk150     = 17;
	mParams    = (THanaSamboSaveLoadParams*)getSaveParam();
	mSpine->initWith(&TNerveHanaSamboHide::theNerve());

	mBindShadow = new TMBindShadowBody(this, getModel(), 1.5f);
	initMarioGoal(this);

	mHead = new THanaSamboHead("ハナサンボ頭あたり");
	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	group->getChildren().push_back(mHead);
	mHead->initHitActor(0x1000001B, 2, 0x80000000,
	                    mParams->mSLHeadAttackRadius.get() * mBodyScale,
	                    mParams->mSLHeadAttackHeight.get() * mBodyScale,
	                    mParams->mSLHeadDamageRadius.get() * mBodyScale,
	                    mParams->mSLHeadDamageHeight.get() * mBodyScale);
	mHead->mOwner = this;
}

void THanaSambo::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("sambo.bmd", 3);
	mMActorKeeper->createMActor("samboD.bmd", 3);
}

void THanaSambo::perform(u32 flags, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(flags, graphics);
	if ((mLiveFlag & 7) == 0)
		mHead->THitActor::perform(flags, graphics);
}

const char** THanaSambo::getBasNameTable() const { return sambo_bastable; }

void THanaSambo::reset()
{
	TSmallEnemy::reset();
	unk165        = false;
	mRootPosition = mPosition;
}

void THanaSambo::drawObject(JDrama::TGraphics* graphics)
{
	TLiveActor::drawObject(graphics);
	if ((mLiveFlag & 0xB) == 0)
		mBindShadow->entryDrawShadow();
}

void THanaSambo::moveObject()
{
	TSmallEnemy::moveObject();
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		mHead->mPosition = mPosition;
	} else {
		u8 headJntIndex = mHeadJntIndex;
		J3DModel* model = getModel();
		MtxPtr mtx      = model->getAnmMtx(headJntIndex);
		mHead->mPosition.x = mtx[0][3];
		mHead->mPosition.y = mtx[1][3];
		mHead->mPosition.z = mtx[2][3];
	}

	int i                = 0;
	THanaSamboHead* head = mHead;
	for (; i < head->getColNum(); ++i) {
		THitActor* actor = head->getCollision(i);
		if (actor->isActorType(0x80000001))
			SMS_SendMessageToMario(head, HIT_MESSAGE_ATTACK);
	}

	mPosition.x = mRootPosition.x;
	mPosition.z = mRootPosition.z;
}

void THanaSambo::kill()
{
	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	mHitPoints = 1;
	if (isBckAnm(7))
		unk18C = 3;

	mSpine->setNext(&TNerveHanaSamboDie::theNerve());
	mHead->onHitFlag(HIT_FLAG_NO_COLLISION);
	onLiveFlag(LIVE_FLAG_UNK40);
}

void THanaSambo::setDeadAnm()
{
	mMActor = mMActorKeeper->getMActor("samboD.bmd");
	if (mUseYDownAnim)
		setBckAnm(10);
	else
		setBckAnm(0);

	mHead->onHitFlag(HIT_FLAG_NO_COLLISION);
	mUseYDownAnim = false;
	onLiveFlag(LIVE_FLAG_UNK8);
}

void THanaSambo::setWaitAnm()
{
	setBckAnm(7);
	mUseYDownAnim = false;
}

void THanaSambo::behaveToWater(THitActor*) { }

bool THanaSambo::isHitValid(u32 message)
{
	if (message == HIT_MESSAGE_UNKB)
		return true;

	return false;
}

bool THanaSambo::isCollidMove(THitActor*) { return false; }

void THanaSambo::createPollen()
{
	MtxPtr jointMtx = mMActor->getModel()->mNodeMatrices[mPollenJntIndex];
	JGeometry::TVec3<f32> pos;
	pos.x = jointMtx[0][3];
	MtxPtr pollenMtx = jointMtx;
	pos.y = jointMtx[1][3];
	pos.z = jointMtx[2][3];

	Mtx mtx;
	MsMtxSetRotRPH(mtx, 0.0f, 0.0f, -90.0f);
	PSMTXConcat(pollenMtx, mtx, mtx);

	JPABaseEmitter* emitter;
	if (mSpine->getCurrentNerve() == &TNerveHanaSamboWait::theNerve()) {
		emitter = gpMarioParticleManager->emit(0xB2, &pos, 0, nullptr);
		mtx[1][3] += 2000.0f;
	} else {
		emitter = gpMarioParticleManager->emit(0xB3, &pos, 0, nullptr);

		JGeometry::TVec3<f32> offset(0.0f, 0.0f, 2000.0f);
		Mtx rot;
		MsMtxSetRotRPH(rot, mRotation.x, mRotation.y, mRotation.z);
		PSMTXMultVec(rot, &offset, &offset);
		mtx[0][3] += offset.x;
		mtx[1][3] += offset.y;
		mtx[2][3] += offset.z;
	}

	if (emitter)
		emitter->setGlobalSRTMatrix(mtx);
}

BOOL THanaSamboHead::receiveMessage(THitActor*, u32 message)
{
	if (message <= HIT_MESSAGE_HIP_DROP) {
		mOwner->kill();
		return TRUE;
	}

	if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		THanaSambo* owner = mOwner;
		owner->unk165     = true;
		if (!owner->changeByJuice()) {
			if (owner->mSpine->getCurrentNerve()
			    == &TNerveHanaSamboWait::theNerve()) {
				owner->mSpine->pushNerve(&TNerveHanaSamboFreeze::theNerve());
			}
		}
		return TRUE;
	}

	return FALSE;
}
