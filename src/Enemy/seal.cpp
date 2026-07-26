#include <Enemy/Seal.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/ObjManager.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionEntryInline.hpp>
#include <Player/ModelWaterManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <System/EmitterViewObj.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

DEFINE_NERVE(TNerveSealDie, TLiveActor)
{
	TSeal* self = (TSeal*)spine->getBody();
	if (spine->getTime() == 0) {
		self->getMActor()->setBckFromIndex(0);
		MtxPtr mtx = self->getMActor()->getModel()->getBaseTRMtx();
		JPABaseEmitter* e1
		    = gpMarioParticleManager->emitAndBindToMtxPtr(0xD1, mtx, 0, self);
		if (e1) {
			e1->unk154.x = self->mScaling.x;
			e1->unk154.y = self->mScaling.y;
			e1->unk154.z = self->mScaling.z;
			e1->unk174.x = self->mScaling.x;
			e1->unk174.y = self->mScaling.y;
			e1->unk174.z = self->mScaling.z;
		}
		JPABaseEmitter* e2 = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0xD2, mtx, 0, (const void*)((const u8*)self + 1));
		if (e2) {
			e2->unk154.x = self->mScaling.x;
			e2->unk154.y = self->mScaling.y;
			e2->unk154.z = self->mScaling.z;
			e2->unk174.x = self->mScaling.x;
			e2->unk174.y = self->mScaling.y;
			e2->unk174.z = self->mScaling.z;
		}
	}
	if (gpMSound->gateCheck(0x6010))
		MSoundSESystem::MSoundSE::startSoundActor(0x6010, &self->mPosition, 0,
		                                          nullptr, 0, 4);
	if (self->getMActor()->curAnmEndsNext(0, nullptr)) {
		self->onHitFlag(HIT_FLAG_NO_COLLISION);
		self->kill();
		spine->pushAfterCurrent(&TNerveSealSleep::theNerve());
		return true;
	}
	return false;
}

DEFINE_NERVE(TNerveSealWait, TLiveActor)
{
	TSeal* self = (TSeal*)spine->getBody();
	if (spine->getTime() == 0)
		self->getMActor()->setBckFromIndex(3);
	if (self->getMActor()->curAnmEndsNext(0, nullptr)) {
		if (self->getMActor()->checkCurBckFromIndex(3))
			self->getMActor()->setBckFromIndex(2);
	}
	if (self->mDistToMarioSquared > 2250000.0f) {
		if (self->getMActor()->curAnmEndsNext(0, nullptr)) {
			self->getMActor()->setBckFromIndex(1);
			spine->pushAfterCurrent(&TNerveSealSleep::theNerve());
			return true;
		}
	}
	return false;
}

DEFINE_NERVE(TNerveSealSleep, TLiveActor)
{
	TSeal* self = (TSeal*)spine->getBody();
	if (spine->getTime() == 0) {
		if (!self->getMActor()->checkCurBckFromIndex(1))
			self->getMActor()->setBckFromIndex(-1);
	}
	if (self->mDistToMarioSquared < 1000000.0f) {
		spine->pushAfterCurrent(&TNerveSealWait::theNerve());
		return true;
	}
	if (self->getMActor()->curAnmEndsNext(0, nullptr)) {
		if (self->getMActor()->checkCurBckFromIndex(1))
			self->getMActor()->setBckFromIndex(-1);
	}
	return false;
}

void TSealManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
}

void TSealManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "gene_orange_model1.bmd", 0x11210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TSealManager::TSealManager(const char* name)
    : TEnemyManager(name)
{
}

void TSeal::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (!(mLiveFlag & (LIVE_FLAG_DEAD | LIVE_FLAG_HIDDEN)) && (flags & 1)) {
		for (int i = 0; i < mColCount; ++i) {
			THitActor* col = mCollisions[i];
			if (col->isActorTypeOf(ACTOR_TYPE_PLAYER))
				col->receiveMessage(this, HIT_MESSAGE_ATTACK);
		}
	}
	TSpineEnemy::perform(flags, gfx);
	if (flags & 1) {
		updateSquareToMario();
		mDamageCount = 0;
	}
	if ((flags & 2) && !(mLiveFlag & (LIVE_FLAG_DEAD | LIVE_FLAG_HIDDEN))
	    && mDistToMarioSquared < 2250000.0f) {
		if (gpMSound->gateCheck(0x2154))
			MSoundSESystem::MSoundSE::startSoundActor(0x2154, &mPosition, 0,
			                                          nullptr, 0, 4);
	}
}

void TSeal::calcRootMatrix()
{
	J3DModel* model = getModel();
	MsMtxSetXYZRPH(model->getBaseTRMtx(), mPosition.x, mPosition.y, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);
	model->setBaseScale(mScaling);
}

BOOL TSeal::receiveMessage(THitActor* sender, u32 msg)
{
	if ((sender->mActorType - 0x01000000) == 1
	    && msg == HIT_MESSAGE_SPRAYED_BY_WATER) {
		gpMarioParticleManager->emit(0xE7, &sender->mPosition, 0, nullptr);
		gpMSound->startSoundSet(0x6802, &sender->mPosition, 0, 0.0f, 0, 0, 4);

		if (gpModelWaterManager->unk5D5F) {
			gpMSound->startSoundSet(0x6809, &sender->mPosition, 0, 0.0f, 0, 0,
			                        4);

			if (&TNerveSealDie::theNerve() != mSpine->getLatestNerve()) {
				if (mMapCollisionManager->getUnk8())
					mMapCollisionManager->getUnk8()->remove();
				mSpine->pushNerve(&TNerveSealDie::theNerve());
			}
			mDamageCount += 1;
		}
		return true;
	}
	return false;
}

void TSeal::init(TLiveManager* manager)
{
	mManager = manager;
	mManager->manageActor(this);
	mMActorKeeper = new TMActorKeeper(mManager, 2);
	mMActor       = mMActorKeeper->createMActor("gene_orange_model1.bmd", 0);
	mMActor->offMakeDL();

	f32 r = 100.0f * mScaling.x;
	initHitActor(0x10000024, 1, -0x7f000000, r, r, r, r);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(this);

	mRotation.x = MsWrap(mRotation.x + 270.0f, 0.0f, 360.0f);

	mMapCollisionManager = new TMapCollisionManager(1, "/scene/seal", this);
	mMapCollisionManager->init("gene_orange_col1.col", 2, nullptr);

	Mtx mtx;
	MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z, mRotation.x,
	            mRotation.y, mRotation.z, mScaling.x, mScaling.y, mScaling.z);
	TMapCollisionBase* col = mMapCollisionManager->getUnk8();
	PSMTXCopy(mtx, col->unk20);
	col->setUp();

	mHitPoints = getSaveParam() ? getSaveParam()->mSLHitPointMax.get() : 1;

	mSpine->initWith(&TNerveSealSleep::theNerve());
}

TSeal::TSeal(const char* name)
    : TSpineEnemy(name)
{
	mDamageCount = 0;
	mLiveFlag |= LIVE_FLAG_UNK10;
}
