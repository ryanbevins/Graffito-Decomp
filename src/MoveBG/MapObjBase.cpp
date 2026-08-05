#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjGeneral.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Strategy.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <System/MarDirector.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

void TMapCollisionBase::setMtx(MtxPtr mtx) { PSMTXCopy(mtx, unk20); }

void TMapObjBase::changeObjMtx(MtxPtr mtx)
{
	mPosition.x = mtx[0][3];
	mPosition.y = mtx[1][3] + mYOffset;
	mPosition.z = mtx[2][3];
	if (mMActor) {
		if (checkMapObjFlag(0x100)) {
			setModelMtx(mtx);
		} else {
			calcRootMatrix();
			getModel()->calc();
		}
	}
	removeMapCollision();
	setUpCurrentMapCollision();
}

void TMapObjBase::changeObjSRT(const JGeometry::TVec3<f32>& param_1,
                               const JGeometry::TVec3<f32>& param_2,
                               const JGeometry::TVec3<f32>& param_3)
{
	Mtx mtx;
	MsMtxSetTRS(mtx, param_1.x, param_1.y, param_1.z, param_2.x, param_2.y,
	            param_2.z, param_3.x, param_3.y, param_3.z);
	changeObjMtx(mtx);
}

u32 TMapObjBase::getSDLModelFlag() const { return 3; }

void TMapObjBase::awake()
{
	offLiveFlag(LIVE_FLAG_UNK4000);
	unk64 &= ~1;
	setUpCurrentMapCollision();
}

void TMapObjBase::sleep()
{
	onLiveFlag(LIVE_FLAG_UNK4000);
	unk64 |= 1;
	TMapCollisionManager* mgr = mMapCollisionManager;
	if (!mgr)
		return;
	TMapCollisionBase* col = mgr->unk8;
	if (!col)
		return;
	if (!col->getUnk8())
		return;
	if (col)
		mgr->unk8->remove();
}

void TMapObjBase::setObjHitData(u16 param_1)
{
	if (!mMapObjData->mHit)
		return;

	if (mMapObjData->mHit->unk0 <= param_1)
		return;

	const TMapObjHitDataTable* table = &mMapObjData->mHit->unkC[param_1];

	if (table->unk0 >= 0.0f) {
		f32 fVar2 = mScaling.x > mScaling.z ? mScaling.x : mScaling.z;
		setHitParams(table->unk0 * fVar2, table->unk4 * mScaling.y,
		             table->unk8 * fVar2, table->unkC * mScaling.y);
	}
}

void TMapObjBase::removeMapCollision()
{
	TMapCollisionManager* mgr = mMapCollisionManager;
	if (!mgr)
		return;
	TMapCollisionBase* col = mgr->unk8;
	if (!col)
		return;
	if (!col->getUnk8())
		return;
	if (col)
		mgr->unk8->remove();
}

void TMapObjBase::setUpCurrentMapCollision()
{
	TMapCollisionManager* colman = mMapCollisionManager;
	if (!colman)
		return;

	if (checkMapObjFlag(8)) {
		J3DModel* model        = getModel();
		TMapCollisionBase* col = mMapCollisionManager->getUnk8();
		MTXCopy(model->getAnmMtx(0), col->unk20);
		col->setUp();
	} else {
		Mtx mtx;
		JGeometry::TVec3<f32> pos(mPosition.x, mPosition.y - mYOffset,
		                          mPosition.z);
		MsMtxSetTRS(mtx, pos.x, pos.y, pos.z, mRotation.x, mRotation.y,
		            mRotation.z, mScaling.x, mScaling.y, mScaling.z);
		TMapCollisionBase* col = colman->getUnk8();
		MTXCopy(mtx, col->unk20);
		col->setUp();
	}
}

void TMapObjBase::setUpMapCollision(u16 param_1)
{
	if (!mMapObjData->mCollision
	    || !mMapObjData->mCollision->unk4[param_1].unk0)
		return;

	JGeometry::TVec3<f32> pos(mPosition.x, mPosition.y - mYOffset, mPosition.z);

	mMapCollisionManager->changeCollision(param_1);

	if (checkMapObjFlag(8)) {
		J3DModel* model        = getModel();
		TMapCollisionBase* col = mMapCollisionManager->getUnk8();
		MTXCopy(model->getAnmMtx(0), col->unk20);
		col->setUp();
	} else {
		Mtx mtx;
		TMapCollisionManager* colman = mMapCollisionManager;
		MsMtxSetTRS(mtx, pos.x, pos.y, pos.z, mRotation.x, mRotation.y,
		            mRotation.z, mScaling.x, mScaling.y, mScaling.z);
		TMapCollisionBase* col = colman->getUnk8();
		MTXCopy(mtx, col->unk20);
		col->setUp();
	}
}

void TMapObjBase::soundBas(u32 param_1, f32 param_2, f32 param_3)
{
	f32 currFrame = mMActor->getFrameCtrl(0)->getFrame();
	if (currFrame <= param_2 && param_2 < currFrame + param_3) {
		if (gpMSound->gateCheck(param_1))
			MSoundSESystem::MSoundSE::startSoundActor(param_1, &mPosition, 0,
			                                          nullptr, 0, 4);
	}
}

void TMapObjBase::startSound(u16 param_1)
{
	if (unk100 != param_1)
		unk100 = param_1;

	if (!mMapObjData->mSound) {
		u32 uVar3 = TMapObjGeneral::mDefaultSound.unk0[unk100];
		if (uVar3 != 0xffffffff && gpMSound->gateCheck(uVar3))
			MSoundSESystem::MSoundSE::startSoundActor(uVar3, &mPosition, 0,
			                                          nullptr, 0, 4);
	} else {
		u32 uVar3 = mMapObjData->mSound->unk4->unk0[unk100];
		if (uVar3 != 0xffffffff && gpMSound->gateCheck(uVar3))
			MSoundSESystem::MSoundSE::startSoundActor(uVar3, &mPosition, 0,
			                                          nullptr, 0, 4);
	}
}

bool TMapObjBase::hasModelOrAnimData(u16 param_1) const
{
	if (!mMapObjData->mAnim || mMapObjData->mAnim->unk0 <= param_1
	    || (!mMapObjData->mAnim->unk4[param_1].unk4
	        && !mMapObjData->mAnim->unk4[param_1].unk0)) {
		return false;
	}

	return true;
}

bool TMapObjBase::hasAnim(u16 param_1) const
{
	if (!mMapObjData->mAnim || mMapObjData->mAnim->unk0 <= param_1
	    || !mMapObjData->mAnim->unk4[param_1].unk4) {
		return false;
	}

	return true;
}

bool TMapObjBase::animIsFinished() const
{
	if (!mMapObjData->mAnim || mMapObjData->mAnim->unk0 == 0
	    || !mMapObjData->mAnim->unk4[unkFE].unk4)
		return true;

	if (mMActor->curAnmEndsNext(0, nullptr))
		return true;
	else
		return false;
}

void TMapObjBase::stopAnim() { }

void TMapObjBase::startControlAnim(u16 param_1)
{
	startAnim(param_1);
	if (mMapObjData->mAnim && param_1 < mMapObjData->mAnim->unk0)
		mMActor->getFrameCtrl(mMapObjData->mAnim->unk4[param_1].unk8)
		    ->setRate(0);
}

void TMapObjBase::startBck(const char* param_1)
{
	unkF8 &= ~0x100;
	mMActor->setBck(param_1);
}

void TMapObjBase::startAnim(u16 param_1)
{
	if (mAnmSound)
		setAnmSound(nullptr);
	if (!mMActor) {
		if (mMapObjData->mAnim->unk0 != 0) {
			mMActor = mMActorKeeper->getMActor(
			    mMapObjData->mAnim->unk4->unk0);
		}
	}
	if (!mMapObjData->mAnim)
		return;
	if (mMapObjData->mAnim->unk0 <= param_1)
		return;
	const TMapObjAnimData* anim = &mMapObjData->mAnim->unk4[param_1];
	if (anim->unk8 == 0) {
		if (unkFE != 0xffff && mMapObjData->mAnim
		    && mMapObjData->mAnim->unk0 != 0) {
			const TMapObjAnimData* prev
			    = &mMapObjData->mAnim->unk4[unkFE];
			if (prev->unk4) {
				int track = prev->unk8;
				mMActor->getFrameCtrl(track)->setRate(0.0f);
				mMActor->getFrameCtrl(track)->setFrame(0.0f);
				*((u32*)mMActor->getUnk28(track)) = 0xffffffff;
				unkFE = 0xffff;
			}
		}
		stopAnmSound();
		if (unkFE != param_1) {
			unkFE = param_1;
			if (anim->unk0) {
				mMActor = mMActorKeeper->getMActor(anim->unk0);
			}
		}
	}
	if (anim->unk4 != 0) {
		unkF8 &= ~0x100;
		mMActor->setAnimation(anim->unk4, anim->unk8);
		if (checkMapObjFlag(0x200))
			offMapObjFlag(0x100);
		if (anim->unk10) {
			setAnmSound(anim->unk10);
		}
	} else {
		MActor* m = mMActor;
		J3DModel* mdl = m->getModel();
		mdl->getModelData()->getJointNodePointer(0)->setMtxCalc(m->unk8);
	}
}

void TMapObjBase::makeObjDefault()
{
	mPosition.set(mInitialPosition.x, mInitialPosition.y + mYOffset,
	              mInitialPosition.z);

	mRotation = mInitialRotation;
	mScaling  = mInitialScaling;

	mVelocity.x = mVelocity.y = mVelocity.z = 0.0f;
	onLiveFlag(LIVE_FLAG_UNK10);
	if (mMActor) {
		calcRootMatrix();
		getModel()->calc();
	}
	mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);
}

void TMapObjBase::makeObjDead()
{
	mVelocity.x = mVelocity.y = mVelocity.z = 0.0f;
	mLiveFlag |= 0x10;

	if (unkFE != 0xffff && mMapObjData->mAnim && mMapObjData->mAnim->unk0 > 0
	    && mMapObjData->mAnim->unk4[unkFE].unk4) {
		u32 uVar6 = mMapObjData->mAnim->unk4[unkFE].unk8;
		mMActor->getFrameCtrl(uVar6)->setRate(0.0f);
		mMActor->getFrameCtrl(uVar6)->setFrame(0.0f);
		mMActor->getUnk28(uVar6)->unk0 = 0xffffffff;
		unkFE                          = 0xffff;
	}

	unk100 = 0xffff;
	unk64 |= 1;
	removeMapCollision();
	mLifeTimer = 0;
	if (mHeldObject) {
		mHeldObject->receiveMessage(this, HIT_MESSAGE_UNK8);
		mHeldObject = nullptr;
	}

	if (mHolder) {
		mHolder->receiveMessage(this, HIT_MESSAGE_UNK8);
		mHolder = nullptr;
	}

	onLiveFlag(0xD9);
	mState = 0;
	if (mMActor)
		SMS_HideAllShapePacket(getModel());
}

void TMapObjBase::makeObjAppeared()
{
	mLiveFlag &= ~0x9;
	mVelocity.z = 0.0f;
	mVelocity.y = 0.0f;
	mVelocity.x = 0.0f;
	mLiveFlag |= 0x10;
	mLifeTimer = 0;
	unk64 &= ~1;
	setObjHitData(0);
	if (unk100 != 0)
		unk100 = 0;
	if (mMapObjData->mSound == nullptr) {
		u32 snd = TMapObjGeneral::mDefaultSound.unk0[unk100];
		if (snd != 0xffffffff) {
			if (gpMSound->gateCheck(snd))
				MSoundSESystem::MSoundSE::startSoundActor(
				    snd, mPosition, 0, nullptr, 0, 4);
		}
	} else {
		u32 snd = mMapObjData->mSound->unk4->unk0[unk100];
		if (snd != 0xffffffff) {
			if (gpMSound->gateCheck(snd))
				MSoundSESystem::MSoundSE::startSoundActor(
				    snd, mPosition, 0, nullptr, 0, 4);
		}
	}
	mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);
	if (mLiveFlag & 0x10)
		mLiveFlag |= 0x80;
	if (mMapObjData->mMove != nullptr) {
		J3DFrameCtrl* fc = mMapObjData->mMove->unk8;
		fc->setFrame((f32)fc->getStart());
		fc->setRate(1.0f);
		mMapObjData->mMove->unk8->setRate(SMSGetAnmFrameRate());
	}
	startAnim(0);
	if (mMActor)
		SMS_ShowAllShapePacket(getModel());
	mPosition.y -= mYOffset;
	if (mMapObjData->mCollision
	    && mMapObjData->mCollision->unk4[0].unk0) {
		f32 px = mPosition.x;
		f32 py = mPosition.y - mYOffset;
		f32 pz = mPosition.z;
		mMapCollisionManager->changeCollision(0);
		if (unkF8 & 0x8) {
			J3DModel* model = getModel();
			TMapCollisionBase* col = mMapCollisionManager->unk8;
			PSMTXCopy(model->getAnmMtx(0), col->unk20);
			col->setUp();
		} else {
			Mtx mtx;
			MsMtxSetTRS(mtx, px, py, pz, mRotation.x, mRotation.y,
			            mRotation.z, mScaling.x, mScaling.y, mScaling.z);
			TMapCollisionBase* col = mMapCollisionManager->unk8;
			col->setMtx(mtx);
			col->setUp();
		}
	}
	mPosition.y += mYOffset;
	mState = 1;
}

void TMapObjBase::moveByBck() { }

void TMapObjBase::touchBoss(THitActor* boss)
{
	boss->receiveMessage(this, HIT_MESSAGE_ATTACK);
}

void TMapObjBase::touchEnemy(THitActor* enemy)
{
	enemy->receiveMessage(this, HIT_MESSAGE_ATTACK);
}

void TMapObjBase::touchPlayer(THitActor* player)
{
	player->receiveMessage(this, HIT_MESSAGE_ATTACK);
}

void TMapObjBase::touchActor(THitActor* actor)
{
	if (actor->checkActorType(ACTOR_TYPE_PLAYER))
		touchPlayer(actor);
	else if (actor->checkActorType(ACTOR_TYPE_ENEMY))
		touchEnemy(actor);
	else if (actor->checkActorType(ACTOR_TYPE_BOSS))
		touchBoss(actor);
}

void TMapObjBase::ensureTakeSituation()
{
	if (mHeldObject && mHeldObject->mHolder != this)
		mHeldObject = nullptr;

	if (mHolder && mHolder->mHeldObject != this) {
		if (mPosition.y != mGroundHeight)
			mLiveFlag &= ~0x10;

		mHolder = nullptr;
	}
}

void TMapObjBase::control()
{
	for (int i = 0; i < mColCount; ++i)
		touchActor(getCollision(i));

	if (mMapObjData->mMove) {
		TMapObjMoveData* move = mMapObjData->mMove;

		move->unk4->setFrame(move->unk8->getFrame());
		move->unk8->update();
		J3DTransformInfo info;
		move->unk4->getTransform(1, &info);
		mLinearVelocity.x
		    = info.mTranslate.x + mInitialPosition.x - mPosition.x;
		mLinearVelocity.y
		    = info.mTranslate.y + mInitialPosition.y - mPosition.y;
		mLinearVelocity.z
		    = info.mTranslate.z + mInitialPosition.z - mPosition.z;
		mRotation.x
		    = info.mRotation.x * (360.0f / 65536.0f) + mInitialRotation.x;
		mRotation.y
		    = info.mRotation.y * (360.0f / 65536.0f) + mInitialRotation.y;
		mRotation.z
		    = info.mRotation.z * (360.0f / 65536.0f) + mInitialRotation.z;
	}
}

void TMapObjBase::setGroundCollision()
{
	TMapCollisionManager* colman = mMapCollisionManager;
	if (!colman)
		return;
	TMapCollisionBase* col = colman->unk8;
	if (col->getUnk8() != 1)
		return;
	if (unkF8 & 0x2) {
		if (mColCount == 0 && unk102 == 0)
			return;
		unk102 -= 1;
		if (mColCount != 0)
			unk102 = 4;
	}
	if (unkF8 & 0x8) {
		MtxPtr anmMtx = getModel()->getAnmMtx(0);
		TMapCollisionBase* col2 = mMapCollisionManager->unk8;
		if (!col2)
			return;
		col2->moveMtx(anmMtx);
		return;
	}
	JGeometry::TVec3<f32> pos;
	pos.x = mPosition.x;
	pos.y = mPosition.y - mYOffset;
	pos.z = mPosition.z;
	if (unkF8 & 0x4) {
		TMapCollisionBase* col2 = mMapCollisionManager->unk8;
		col2->unk5C &= ~0x8000;
		TMapCollisionBase* col3 = mMapCollisionManager->unk8;
		col3->unk5C &= ~0x4000;
		TMapCollisionBase* col4 = mMapCollisionManager->unk8;
		if (!col4)
			return;
		col4->moveSRT(pos, mRotation, mScaling);
	} else {
		TMapCollisionBase* col2 = mMapCollisionManager->unk8;
		col2->unk5C &= ~0x4000;
		TMapCollisionBase* col3 = mMapCollisionManager->unk8;
		if (!col3)
			return;
		col3->moveTrans(pos);
	}
}

void TMapObjBase::perform(u32 param_1, JDrama::TGraphics* gfx)
{
	u8 state1;
	if (gpMarDirector->unk124 == 1 || gpMarDirector->unk124 == 2)
		state1 = 1;
	else
		state1 = 0;
	if (state1) {
		u8 state2;
		if (gpMarDirector->unk124 == 3 || gpMarDirector->unk124 == 4)
			state2 = 1;
		else
			state2 = 0;

		if (!state2) {
			if (mLiveFlag & 1)
				return;
			u8 isType3B;
			if ((mActorType - 0x40000000) == 0x3B)
				isType3B = 1;
			else
				isType3B = 0;
			if (isType3B)
				return;
			if (param_1 & 1) {
				setGroundCollision();
				param_1 &= ~1;
			}
			if ((param_1 & 2) && mMActor) {
				if (mLiveFlag & 0x204) {
					if (getModel()->mShapePackets->unk30 != 0)
						SMS_HideAllShapePacket(getModel());
				} else {
					if (getModel()->mShapePackets->unk30 == 0)
						SMS_ShowAllShapePacket(getModel());
				}
			}
			if (mLiveFlag & 0x200)
				return;
			if (hasMapCollision())
				param_1 &= ~2;
		}
	}
	if (param_1 & 1) {
		if (mLifeTimer > 0)
			mLifeTimer -= 1;
		if (unk100 == 0) {
			u32 sound;
			if (mMapObjData->mSound == nullptr) {
				sound = TMapObjGeneral::mDefaultSound.unk0[unk100];
			} else {
				sound = mMapObjData->mSound->unk4->unk0[unk100];
			}
			if (sound != 0xffffffff && gpMSound->gateCheck(sound))
				MSoundSESystem::MSoundSE::startSoundActor(
				    sound, mPosition, 0, nullptr, 0, 4);
		}
		if (mLiveFlag & 1)
			dead();
	}
	if (mLiveFlag & 1)
		return;
	if (param_1 & 8)
		draw();
	if (mLiveFlag & 0x4000) {
		if (param_1 & 2)
			param_1 &= ~2;
		if (param_1 & 0x200)
			param_1 &= ~0x200;
		if (param_1 & 4)
			param_1 &= ~4;
	}
	if (param_1 & 2) {
		calc();
		if (mMActor) {
			if (mLiveFlag & 0x204) {
				if (getModel()->mShapePackets->unk30 != 0)
					SMS_HideAllShapePacket(getModel());
			} else {
				if (getModel()->mShapePackets->unk30 == 0)
					SMS_ShowAllShapePacket(getModel());
			}
		}
		if (unkF8 & 0x100) {
			param_1 &= ~2;
		} else if ((unkF8 & 0x200) && mMActor
		           && mMActor->curAnmEndsNext(0, nullptr)) {
			unkF8 |= 0x100;
		}
	}
	if (param_1 & 0x200) {
		if (unkF8 & 0x1000)
			param_1 &= ~0x200;
		if (unkF8 & 0x800)
			param_1 &= ~0x200;
	}
	if ((param_1 & 4) && mMActor && (unkF8 & 0x400)) {
		J3DModel* model = getModel();
		((SDLModel*)model)->viewCalcSimple();
		requestShadow();
		param_1 &= ~4;
	}
	TLiveActor::perform(param_1, gfx);
}

u32 TMapObjBase::getShadowType()
{
	if (isActorType(0x40000034) || isActorType(0x40000035)
	    || isActorType(0x40000036) || isActorType(0x40000037)
	    || isActorType(0x40000039)) {
		return 2;
	} else if (checkMapObjFlag(0x2000)) {
		return 1;
	} else {
		return 0;
	}
}

Mtx* TMapObjBase::getRootJointMtx() const
{
	if (checkMapObjFlag(0x8) || mYOffset != 0.0f)
		return (Mtx*)mMActor->getModel()->getAnmMtx(0);
	return nullptr;
}

void TMapObjBase::calcRootMatrix()
{
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(model->getBaseTRMtx(), mPosition.x, mPosition.y - mYOffset,
	               mPosition.z, mRotation.x, mRotation.y, mRotation.z);
	model->setBaseScale(mScaling);
}

BOOL TMapObjBase::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_UNK5 && checkMapObjFlag(0x40)) {
		mHeldObject = (TTakeActor*)sender;
		return 1;
	} else if (message == HIT_MESSAGE_SPRAYED_BY_WATER) {
		return touchWater(sender);
	} else {
		return false;
	}
}

void TMapObjBase::initAndRegister(const char* param_1)
{
	unkF4 = param_1;
	initMapObj();
	if (mMapObjData->unkC) {
		TIdxGroupObj* group
		    = JDrama::TNameRefGen::search<TIdxGroupObj>(mMapObjData->unkC);
		group->getChildren().push_back(this);
	}
}

void TMapObjBase::loadAfter()
{
	TLiveActor::loadAfter();
	mGroundHeight = gpMap->checkGround(mPosition, &mGroundPlane);
}

void TMapObjBase::load(JSUMemoryInputStream& stream)
{
	TActor::load(stream);

	unkF4 = stream.readString();
	loadBeforeInit(stream);
	initMapObj();
	unkF8 &= ~0x40000;
	makeObjAppeared();
	if (checkMapObjFlag(0x80)) {
		f32 value;
		stream.read(&value, 4);
		mDamageHeight = value;
		calcEntryRadius();
		mAttackHeight = value;
		calcEntryRadius();
		unk64 &= ~4;
		unk64 &= ~2;
	}
}

TMapObjBase::TMapObjBase(const char* name)
    : TLiveActor(name)
    , unkF4(nullptr)
    , unkF8(0)
    , mState(1)
    , unkFE(0xffff)
    , unk100(0xffff)
    , unk102(0)
    , mLifeTimer(0)
    , mYOffset(0.0f)
    , mMapObjData(nullptr)
    , unk134(0)
{
	mInitialPosition.zero();
	mInitialRotation.zero();
	mInitialScaling.set(1.0f, 1.0f, 1.0f);
}
