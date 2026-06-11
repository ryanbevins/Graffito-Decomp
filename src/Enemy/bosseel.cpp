#include <Enemy/BossEel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DCluster.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <Map/Map.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/Particles.hpp>

// rogue includes needed for matching sinit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

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
static const char* MtxCalcTypeName[] = {
	MtxCalcTypeName0,
	MtxCalcTypeName1,
	MtxCalcTypeName2,
	MtxCalcTypeName3,
};
static const f32 dummy2933[3] = { 0.0f, 0.0f, 0.0f };
static const f32 dummy2935[3] = { 1.0f, 1.0f, 1.0f };

f32 TBossEel::mOpenRollSpeed = 0.3f;
u8 TBossEel::mUseObjCollision = TRUE;
f32 TBossEel::mForcePow       = 10.0f;
u8 TBossEel::mUseMapCollision;

static const char* bosseel_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	"/scene/bosseel/bas/meoto_in_loop.bas",
	nullptr,
	"/scene/bosseel/bas/meoto_mogu.bas",
	nullptr,
	nullptr,
	nullptr,
	"/scene/bosseel/bas/meoto_out_loop.bas",
	"/scene/bosseel/bas/meoto_paku.bas",
	nullptr,
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char* bossEelTears_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
	nullptr,
};

static const char cDummyTextureName[] = "M_dummy";

DEFINE_NERVE(TNerveBEelTearsMoveUp, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		tears->mMActor = tears->mMActorKeeper->getMActor("tears.bmd");
		tears->mMActor->setBckFromIndex(1);
	}

	tears->mPosition.y += tears->unk15C->mSLTearsUpSpeed.get();
	return FALSE;
}

DEFINE_NERVE(TNerveBEelTearsGenerate, TLiveActor)
{
	TBEelTears* tears = (TBEelTears*)spine->getBody();

	if (spine->getTime() == 0) {
		tears->mMActor = tears->mMActorKeeper->getMActor("tears.bmd");
		tears->mMActor->setBckFromIndex(2);
	}

	if (tears->checkCurAnmEnd(0)) {
		spine->pushAfterCurrent(&TNerveBEelTearsMoveUp::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveOilBallStay, TLiveActor)
{
	TLiveActor* body = spine->getBody();
	MActor* actor    = body->mMActor;
	TOilBall* oilBall = (TOilBall*)body;

	if (actor->checkCurBckFromIndex(3)
	    || (oilBall->checkCurAnmEnd(0)
	        && !oilBall->mMActor->checkCurBckFromIndex(1))) {
		oilBall->mMActor = oilBall->mMActorKeeper->getMActor("tears.bmd");
		oilBall->mMActor->setBckFromIndex(1);
	}

	return FALSE;
}

TBossEel::TBossEel(const char* name)
    : TSpineEnemy(name)
    , unk1A8(nullptr)
    , unk1AC(nullptr)
    , unk1B0(nullptr)
    , unk1BC(1.0f)
    , unk1C0(nullptr)
    , unk1C4(nullptr)
    , unk1CC(500.0f)
    , unk1D0(TRUE)
    , unk1D4(2350.0f)
    , unk1D8(0.75f)
    , unk1E8(nullptr)
    , unk1F0(FALSE)
    , unk1F4(0.0f)
    , unk1FC(FALSE)
    , unk1FD(FALSE)
    , unk1FE(FALSE)
    , unk200(nullptr)
    , unk210(nullptr)
    , unk214(nullptr)
    , unk218(nullptr)
    , unk21C(FALSE)
    , unk21D(TRUE)
{
	TBossEelUnk1EC* unk = new TBossEelUnk1EC;
	if (unk != nullptr) {
		unk->unk0 = 0;
		unk->unk4 = 1;
	}
	unk1EC = unk;
}

BOOL TBossEel::receiveMessage(THitActor*, u32) { return FALSE; }

BOOL TBossEel::hasMapCollision() const { return TRUE; }

const char** TBossEel::getBasNameTable() const { return bosseel_bastable; }

TBossEelSaveParams::TBossEelSaveParams()
    : TParams("/enemy/bosseel.prm")
    , PARAM_INIT(mSLInitTransYOffset, 0.0f)
    , PARAM_INIT(mSLAppearMoveDistY, 6000.0f)
    , PARAM_INIT(mSLBodyScale, 0.75f)
    , PARAM_INIT(mSLViewClipFar, 25000.0f)
    , PARAM_INIT(mSLViewClipRadius, 12000.0f)
    , PARAM_INIT(mSLBodyToHeadDistance, 6000.0f)
    , PARAM_INIT(mSLBodyAttackRadius, 2400.0f)
    , PARAM_INIT(mSLBodyAttackHeight, 6300.0f)
    , PARAM_INIT(mSLBodyDamageRadius, 2200.0f)
    , PARAM_INIT(mSLBodyDamageHeight, 6100.0f)
    , PARAM_INIT(mSLHeadAttackRadius, 3600.0f)
    , PARAM_INIT(mSLHeadAttackHeight, 5600.0f)
    , PARAM_INIT(mSLHeadDamageRadius, 3400.0f)
    , PARAM_INIT(mSLHeadDamageHeight, 5400.0f)
    , PARAM_INIT(mSLToothAttackRadius, 500.0f)
    , PARAM_INIT(mSLToothAttackHeight, 300.0f)
    , PARAM_INIT(mSLToothDamageRadius, 600.0f)
    , PARAM_INIT(mSLToothDamageHeight, 400.0f)
    , PARAM_INIT(mSLSpinAccel, 0.01f)
    , PARAM_INIT(mSLSpinMaxSpeed, 10.0f)
    , PARAM_INIT(mSLToothUpSpeed, 5.0f)
    , PARAM_INIT(mSLToothLiveHeight, 2000.0f)
    , PARAM_INIT(mSLToothMaxHitPoint, 50)
    , PARAM_INIT(mSLGenTearsTime, 100)
    , PARAM_INIT(mSLVortexAttackRadius, 500.0f)
    , PARAM_INIT(mSLVortexAttackHeight, 300.0f)
    , PARAM_INIT(mSLVortexDamageRadius, 600.0f)
    , PARAM_INIT(mSLVortexDamageHeight, 400.0f)
    , PARAM_INIT(mSLVortexLiveTimer, 300)
    , PARAM_INIT(mSLVortexScaleXZ, 1.0f)
    , PARAM_INIT(mSLVortexScaleY, 1.0f)
    , PARAM_INIT(mSLMouthOpenFrame, 500)
    , PARAM_INIT(mSLMouthOpenInterval, 2000)
    , PARAM_INIT(mSLCanEatFrame, 200)
    , PARAM_INIT(mSLBreathInPower, 10.0f)
{
	TParams::load(mPrmPath);
}

void TBossEelManager::clipEnemies(JDrama::TGraphics* graphics)
{
	clipActorsAux(graphics, mSaveParams.mSLViewClipFar.get(),
	              mSaveParams.mSLViewClipRadius.get());
}

void TBossEelManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "meoto_model.bmd", 0x10020000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

#define LOAD_BOSSEEL_PARTICLE(id, path)                                        \
	do {                                                                       \
		if (!gParticleFlagLoaded[id]) {                                        \
			gpResourceManager->load(path, id);                                 \
			gParticleFlagLoaded[id] = true;                                    \
		}                                                                      \
	} while (0)

void TBossEelManager::loadAfter()
{
	LOAD_BOSSEEL_PARTICLE(0xd3, "/scene/bosseel/jpa/ms_meo_awa_tooth.jpa");
	LOAD_BOSSEEL_PARTICLE(0xd4, "/scene/bosseel/jpa/ms_meo_awa_mouth.jpa");
	LOAD_BOSSEEL_PARTICLE(0x192, "/scene/bosseel/jpa/ms_meo_eyeblur.jpa");
	LOAD_BOSSEEL_PARTICLE(0x193, "/scene/bosseel/jpa/ms_meo_spin_smoke.jpa");
	LOAD_BOSSEEL_PARTICLE(0x194, "/scene/bosseel/jpa/ms_meo_spin_smoke_l.jpa");
	LOAD_BOSSEEL_PARTICLE(0x195, "/scene/bosseel/jpa/ms_meo_spin_awa.jpa");
	LOAD_BOSSEEL_PARTICLE(0x196, "/scene/bosseel/jpa/ms_meo_spin_awa_l.jpa");
	LOAD_BOSSEEL_PARTICLE(0x197, "/scene/bosseel/jpa/ms_meo_awa_body.jpa");
	LOAD_BOSSEEL_PARTICLE(0x198, "/scene/bosseel/jpa/ms_meo_awa_dead.jpa");
	LOAD_BOSSEEL_PARTICLE(0x199, "/scene/bosseel/jpa/ms_meo_suikomi.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19a, "/scene/bosseel/jpa/ms_meo_tooth_wash.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19b, "/scene/bosseel/jpa/ms_meo_tooth_kira.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19c, "/scene/bosseel/jpa/ms_meo_tooth_always.jpa");
}

TBEelTears::TBEelTears(const char* name)
    : TSpineEnemy(name)
    , unk15C(nullptr)
    , unk160(TRUE)
    , unk164(0)
    , unk168(nullptr)
    , unk16C(nullptr)
{
}

const char** TBEelTears::getBasNameTable() const
{
	return bossEelTears_bastable;
}

void TBEelTears::kill()
{
	onHitFlag(HIT_FLAG_NO_COLLISION);
	unk16C->onHitFlag(HIT_FLAG_NO_COLLISION);
	onLiveFlag(LIVE_FLAG_DEAD);
}

void TBEelTears::reset()
{
	mActorType = 0x8000003;
	TSpineEnemy::reset();
	mSpine->initWith(&TNerveBEelTearsGenerate::theNerve());
	onLiveFlag(LIVE_FLAG_UNK10);
	offLiveFlag(LIVE_FLAG_DEAD);
	offLiveFlag(LIVE_FLAG_CLIPPED_OUT);
	offLiveFlag(LIVE_FLAG_UNK200);
	offLiveFlag(LIVE_FLAG_HIDDEN);
	mGroundPlane = TMap::getIllegalCheckData();
	offHitFlag(HIT_FLAG_NO_COLLISION);
}

TBEelTearsManager::TBEelTearsManager(const char* name)
    : TEnemyManager(name)
{
}

void TBEelTearsManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBEelTearsSaveLoadParams("/enemy/bossEelTears.prm");
	TEnemyManager::load(stream);
}

void TBEelTearsManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		clipEnemies(graphics);

	for (int i = 0; i < getActiveObjNum(); ++i)
		getObj(i)->perform(flags, graphics);

	for (int i = 0; i < 30; ++i) {
		if (mDrops[i]->unk6C)
			mDrops[i]->perform(flags, graphics);
	}
}

TSpineEnemy* TBEelTearsManager::createEnemyInstance()
{
	return new TBEelTears("めおとウナギ涙");
}

void TBEelTearsManager::createEnemies(int count)
{
	TEnemyManager::createEnemies(count);

	void* resource
	    = JKRFileLoader::getGlbResource("/scene/bossEelTears/tears_drop.bmd");
	SDLModelData* modelData
	    = new SDLModelData(J3DModelLoaderDataBase::load(resource, 0x11240000));
	TBEelTears* tears = (TBEelTears*)unk18[0];

	for (int i = 0; i < 30; ++i)
		mDrops[i] = new TBEelTearsDrop(tears, 0, modelData, "涙粒");
}

void TBEelTearsManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
		{ "tears.bmd", 0x11240000, 0 },
		{ "tears_waterhit.bmd", 0x11240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBEelTearsManager::loadAfter()
{
	LOAD_BOSSEEL_PARTICLE(0xd5,
	                      "/scene/bossEelTears/ms_meo_tear_bomb.jpa");
	LOAD_BOSSEEL_PARTICLE(0xd6,
	                      "/scene/bossEelTears/ms_meo_tear_awaget.jpa");
	LOAD_BOSSEEL_PARTICLE(0x19d,
	                      "/scene/bossEelTears/ms_meo_tear_awa.jpa");
}

#undef LOAD_BOSSEEL_PARTICLE

TBEelTearsSaveLoadParams::TBEelTearsSaveLoadParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLTearsUpSpeed, 5.0f)
    , PARAM_INIT(mSLTearsDamageUpSpeed, 1.0f)
    , PARAM_INIT(mSLTearsLiveHeight, 2000.0f)
    , PARAM_INIT(mSLTearsSplitNum, 2)
    , PARAM_INIT(mSLTearsDamageRadius, 600)
    , PARAM_INIT(mSLTearsDamageHeight, 400)
    , PARAM_INIT(mSLTearsAttackRadius, 500)
    , PARAM_INIT(mSLTearsAttackHeight, 300)
    , PARAM_INIT(mSLTearsDropDamageRadius, 600)
    , PARAM_INIT(mSLTearsDropDamageHeight, 400)
    , PARAM_INIT(mSLTearsDropAttackRadius, 500)
    , PARAM_INIT(mSLTearsDropAttackHeight, 300)
    , PARAM_INIT(mSLHighPolyDistY, 5.0f)
    , PARAM_INIT(mSLHitAnmFrameRate, 1.0f)
    , PARAM_INIT(mSLBodyScaleLow, 1.0f)
    , PARAM_INIT(mSLBodyScaleHigh, 1.0f)
    , PARAM_INIT(mSLTearsDropScaleLow, 1.0f)
    , PARAM_INIT(mSLTearsDropScaleHigh, 1.0f)
    , mBodyScaleLow(0.0f)
    , mBodyScaleHigh(1.0f)
    , mTearsDropScaleLow(0.0f)
    , mTearsDropScaleHigh(1.0f)
{
	TParams::load(mPrmPath);
	mBodyScaleLow       = mSLBodyScaleLow.get();
	mBodyScaleHigh      = mSLBodyScaleHigh.get();
	mTearsDropScaleLow  = mSLTearsDropScaleLow.get();
	mTearsDropScaleHigh = mSLTearsDropScaleHigh.get();
}

void TOilBall::load(JSUMemoryInputStream& stream)
{
	unk160 = FALSE;
	TSpineEnemy::load(stream);
	unk150 = mPosition;
	reset();
}

void TOilBall::calcRootMatrix() { TSpineEnemy::calcRootMatrix(); }

void TBEelTearsDrop::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (flags & 1) {
		mPosition.y += unk70;
		if (mPosition.y > gpMarioPos->y + 2000.0f)
			unk6C = FALSE;
	}

	if (flags & 2) {
		Mtx mtx;
		MsMtxSetXYZRPH(mtx, mPosition.x, mPosition.y, mPosition.z,
		               mRotation.x, mRotation.y, mRotation.z);

		MActor* actor = unk68->getMActor();
		PSMTXCopy(mtx, actor->getModel()->getBaseTRMtx());

		f32 scale = unk74->unk15C->mSLTearsDropScaleLow.get();
		mScaling.x = scale;
		mScaling.y = scale;
		mScaling.z = scale;
		actor->getModel()->setBaseScale(mScaling);
	}

	unk68->getMActor()->perform(flags, graphics);
}

TBEelTearsDrop::TBEelTearsDrop(TBEelTears* tears, int index,
                               SDLModelData* model_data, const char* name)
    : THitActor(name)
    , unk68(nullptr)
    , unk74(tears)
{
	unk68 = new TSharedParts(tears, index, model_data, 0, "<TSharedParts>");

	TBEelTearsSaveLoadParams* params = tears->unk15C;
	initHitActor(0x2000002c, 3, 0x80000000,
	             params->mSLTearsDropAttackRadius.get(),
	             params->mSLTearsDropAttackHeight.get(),
	             params->mSLTearsDropDamageRadius.get(),
	             params->mSLTearsDropDamageHeight.get());

	TIdxGroupObj* group = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	JGadget::TList_pointer_void* list
	    = (JGadget::TList_pointer_void*)((u8*)group + 0x10);
	list->insert(list->end(), this);
	unk6C = FALSE;

	TScreenTexture* texture
	    = JDrama::TNameRefGen::search<TScreenTexture>("スクリーンテクスチャ");
	new J3DSkinDeform;

	MActor* actor = unk68->getMActor();
	SMS_ChangeTextureAll(actor->getModel()->getModelData(), cDummyTextureName,
	                     *texture->getTexture()->getTexInfo());
	actor->setBckFromIndex(0);
	actor->setLightType(3);
}
