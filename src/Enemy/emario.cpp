#include <Enemy/EMario.hpp>
#include <Enemy/EnemyMario.hpp>
#include <Enemy/Conductor.hpp>
#include <Camera/Camera.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjModel.hpp>
#include <System/MarDirector.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

bool SMS_isMultiPlayerMap();

static const char cDirtyFileName[] = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]  = "H_ma_rak_dummy";

TEMarioManager::TEMarioManager(const char* name)
    : TEnemyManager(name)
{
}

void TEMarioManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TSpineEnemyParams("/enemy/emario.prm");
	TEnemyManager::load(stream);
}

TSpineEnemy* TEMarioManager::createEnemyInstance()
{
	return new TEMario("マリオモドキ");
}

TEMario::TEMario(const char* name)
    : TSpineEnemy(name)
{
}

void TEMario::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (mLiveFlag & LIVE_FLAG_UNK40)
		return;

	u32 drawing = flags & 1;
	if (drawing) {
		u16 doing = *(u16*)((u8*)mEnemyMario + 0x4292);
		bool relevant;
		if (doing == 0xb || doing == 0xc || doing == 0x11)
			relevant = true;
		else
			relevant = false;
		if (relevant) {
			for (int i = 0; i < mColCount; i++) {
				THitActor* coll = mCollisions[i];
				u32 type        = coll->mActorType;
				switch (type) {
				case 0x80000001: {
					JGeometry::TVec3<f32> diff
					    = mPosition - coll->mPosition;
					f32 dist = JGeometry::TUtil<f32>::sqrt(
					    diff.x * diff.x + diff.y * diff.y
					    + diff.z * diff.z);
					f32 range = *(f32*)((u8*)mEnemyMario + 0x42b0);
					if (dist >= range)
						break;
					coll->receiveMessage(this, 0xe);
					break;
				}
				case 0x400000BC: {
					bool airborne;
					if (mEnemyMario->mAction & 0x10000)
						airborne = true;
					else
						airborne = false;
					if (airborne)
						break;
					f32 damageR = mEnemyMario->mDamageRadius;
					f32 attackR = coll->mAttackRadius;
					JGeometry::TVec3<f32> diff
					    = coll->mPosition - mPosition;
					f32 dist = JGeometry::TUtil<f32>::sqrt(
					    diff.x * diff.x + diff.y * diff.y
					    + diff.z * diff.z);
					if (dist >= attackR + damageR)
						break;
					mEnemyMario->changePlayerStatus(0x810446, 0, false);
					mEnemyMario->emitGetEffect();
					break;
				}
				}
			}
		}
	}

	mEnemyMario->perform(flags, gfx);

	if (drawing) {
		mPosition     = mEnemyMario->mPosition;
		mRotation     = mEnemyMario->mRotation;
		mScaling      = mEnemyMario->mScaling;
		mAttackRadius = mEnemyMario->mAttackRadius;
		calcEntryRadius();
		mAttackHeight = mEnemyMario->mAttackHeight;
		calcEntryRadius();
		mDamageRadius = mEnemyMario->mDamageRadius;
		calcEntryRadius();
		mDamageHeight = mEnemyMario->mDamageHeight;
		calcEntryRadius();
	}
}

void TEMario::forceDisappear()
{
	mEnemyMario->startDisappear(9);
}

void TEMario::startGateDrawing()
{
	mEnemyMario->startGateDrawing();
}

void TEMario::startMonteReplay(u32 nodeId)
{
	mEnemyMario->startMonteReplay(nodeId);
}

void TEMario::startRunAway()
{
	mEnemyMario->startRunAway();
}

bool TEMario::isDownWaitingToTalk() const
{
	if (*(u16*)((u8*)mEnemyMario + 0x4292) == 0xf)
		return TRUE;
	return FALSE;
}

bool TEMario::isReachedToGate() const
{
	if (*(u16*)((u8*)mEnemyMario + 0x4292) == 0x18)
		return TRUE;
	return FALSE;
}

BOOL TEMario::isGoal()
{
	if (*(u16*)((u8*)mEnemyMario + 0x4290) & 1)
		return TRUE;
	return FALSE;
}

void TEMario::kill()
{
	if (SMS_isMultiPlayerMap())
		gpCamera->removeMultiPlayer(&mPosition);
}

BOOL TEMario::receiveMessage(THitActor* sender, u32 message)
{
	TSpineEnemy::receiveMessage(sender, message);
	if (message == 0xf) {
		mEnemyMario->hitWater(sender);
		return TRUE;
	}
	if (message == 0) {
		return mEnemyMario->thinkTrample();
	}
	if (sender->mActorType == 0x40000246) {
		mEnemyMario->reachGoal();
	}
	return FALSE;
}

void TEMario::loadAfter()
{
	mEnemyMario->loadAfter();
	if (SMS_isMultiPlayerMap()) {
		gpCamera->addMultiPlayer(&mPosition, 60.0f, 150.0f);
	}
}

void TEMario::load(JSUMemoryInputStream& stream)
{
	TSpineEnemy::load(stream);
	stream.read(&unk154, 4);
	stream.read(&unk158, 4);
	stream.read(&unk15C, 4);
	stream.read(&unk160, 4);
	u32 stackD4;
	u32 stackD0;
	stream.read(&stackD4, 4);
	stream.read(&stackD0, 4);
	if (unk154 == 0xff)
		unk154 = 0;
	if (unk158 == 0xff)
		unk158 = 0;
	if (unk15C == 0xff)
		unk15C = 0;
	if (unk160 == 0xff)
		unk160 = 0;
	mEnemyMario = new TEnemyMario;
	*(TEMario**)((u8*)mEnemyMario + 0x42a0) = this;
	if (strcmp(mName, "マリオ２Ｐ") == 0) {
		mEnemyMario->setGamePad(gpMarDirector->unk18[1]);
		*(u8*)((u8*)mEnemyMario + 0x388) = 3;
	}
	if (strcmp(mName, "マリオ３Ｐ") == 0) {
		mEnemyMario->setGamePad(gpMarDirector->unk18[2]);
		*(u8*)((u8*)mEnemyMario + 0x388) = 4;
	}
	if (strcmp(mName, "マリオ４Ｐ") == 0) {
		mEnemyMario->setGamePad(gpMarDirector->unk18[3]);
		*(u8*)((u8*)mEnemyMario + 0x388) = 5;
	}

	{
		char buf[14] = "マリオ用カゴ";
		*(u32*)((u8*)mEnemyMario + 0x3c)
		    = (u32)JDrama::TNameRefGen::getInstance()
		          ->getRootNameRef()
		          ->searchF(JDrama::TNameRef::calcKeyCode(buf), buf);
	}

	mEnemyMario->initValues();
	mEnemyMario->mPosition = mPosition;
	mEnemyMario->mRotation = mRotation;
	mEnemyMario->mScaling  = mScaling;
	mEnemyMario->mFaceAngle.y
	    = (s16)(65536.0f * (mRotation.y * (1.0f / 360.0f)));
	mEnemyMario->initEnemyValues();
	mEnemyMario->mState |= 8;
}

void TEMario::init(TLiveManager* manager)
{
	if (manager == NULL) {
		TObjChara* chara = (TObjChara*)unk3C;
		if (chara) {
			mMActorKeeper = new TMActorKeeper((TLiveManager*)NULL,
			                                  (u16)1);
			mMActorKeeper->mModelLoaderFlags = 0x11300000;
			const char* folder = chara->mFolder;
			mMActor = mMActorKeeper->createMActorFromDefaultBmd(
			    folder, 0);
			for (int i = 0;
			     i < mMActor->getModel()->getModelData()->mMaterialNum;
			     i++) {
				SMS_InitPacket_Fog(mMActor->getModel(), (u16)i);
			}
			mMActor->setBtk("kagemario_scroll");
		}
		gpConductor->registerAloneActor(this);
	} else {
		mManager = manager;
		mMActorKeeper = new TMActorKeeper(mManager, (u16)1);
		mManager->manageActor(this);
		mMActor = mMActorKeeper->createMActorFromNthData(0, 0);
	}
	if (mMActor) {
		gpScreenTexture->replace(mMActor->getModel()->getModelData(),
		                         "H_kagemario_dummy");
	}
	onHitFlag(HIT_FLAG_NO_COLLISION);
	mLiveFlag &= ~0x400;
	if (mAnmSound == NULL)
		initAnmSound();
	initHitActor(0x08000002, 4, 0xE5000000, 70.0f, 45.0f, 60.0f, 40.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	mLiveFlag |= 0x10;
}
