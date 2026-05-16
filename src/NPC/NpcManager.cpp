#include <NPC/NpcManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActorData.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/ResTIMG.hpp>
#include <Enemy/Conductor.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <NPC/NpcSave.hpp>
#include <NPC/NpcInitData.hpp>
#include <System/MarDirector.hpp>

class J3DMaterialTable;

const char* cDummyPollutionTexName    = "H_ma_rak_dummy";
const char* cRealPollutionTexName     = "/scene/map/pollution/H_ma_rak.bti";
const char* cMonteMDummyStrawTexName  = "I_mom_mino_dummyI4";
const char* cMonteWDummyStrawTexName  = "I_mow_mino_dummyI4";
const char* cMonteMRealStrawTexName   = "/scene/monteMCommon/I_mom_mino_rgba.bti";
const char* cMonteWRealStrawTexName   = "/scene/monteWCommon/I_mow_mino_rgba.bti";
const char* cMonteMCommonVolumeName   = "/scene/monteMCommon";
const char* cMonteWCommonVolumeName   = "/scene/monteWCommon";
const char* cMareMCommonVolumeName    = "/scene/mareM";
const char* cMareWCommonVolumeName    = "/scene/mareW";
const char* cMareCommonNormalBmtName  = "/scene/mareCommon/mare.bmt";
const char* cMareCommonPollutionBmtName = "/scene/mareCommon/mare_yogore.bmt";

TModelDataKeeper* TMonteMBaseManager::mStaticCommonKeeper;
TModelDataKeeper* TMonteWBaseManager::mStaticCommonKeeper;
J3DMaterialTable* TMareBaseManager::mStaticBmtNormal;
J3DMaterialTable* TMareBaseManager::mStaticBmtPollution;
TModelDataKeeper* TMareMBaseManager::mStaticCommonKeeper;
TModelDataKeeper* TMareWBaseManager::mStaticCommonKeeper;

TMareJellyFishManager* gpMareJellyFishManager;

// =====================================================================
// Pattern A: simple createModelDataArray only
// =====================================================================

void TSunflowerSManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "sunflower_s.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TSunflowerLManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "sunflower.bmd", 0x10020000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TRaccoonDogManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "tanuki.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TPeachManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "peach_model.bmd", 0x10010000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TKinojiiManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "kinoji_body.bmd", 0x10010000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TBoardNpcManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "boardNpc.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteMEManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momE_model.bmd", 0x10010000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

// =====================================================================
// Pattern B: createModelDataArray + pollution-only null-checked (Kinopio)
// =====================================================================

void TKinopioManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "kinopio_body.bmd", 0x10300000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (tex)
		SMS_ChangeTextureAll(data, cDummyPollutionTexName, *tex);
}

// =====================================================================
// Pattern C: createModelDataArray + straw only (no null check)
// =====================================================================

void TMonteMBManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momB_model.bmd", 0x10210000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteMDummyStrawTexName, *tex);
}

void TMonteMDManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momD_model.bmd", 0x10210000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteMDummyStrawTexName, *tex);
}

void TMonteWBManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mowB_model.bmd", 0x10210000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteWRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteWDummyStrawTexName, *tex);
}

// =====================================================================
// Pattern D: createModelDataArray + straw + pollution null-checked
// =====================================================================

void TMonteMManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mom_model.bmd", 0x10300000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* d1 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t1 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(d1, cMonteMDummyStrawTexName, *t1);
	J3DModelData* d2 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t2 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (t2)
		SMS_ChangeTextureAll(d2, cDummyPollutionTexName, *t2);
}

void TMonteMAManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momA_model.bmd", 0x10300000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* d1 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t1 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(d1, cMonteMDummyStrawTexName, *t1);
	J3DModelData* d2 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t2 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (t2)
		SMS_ChangeTextureAll(d2, cDummyPollutionTexName, *t2);
}

void TMonteMCManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momC_model.bmd", 0x10300000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* d1 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t1 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(d1, cMonteMDummyStrawTexName, *t1);
	J3DModelData* d2 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t2 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (t2)
		SMS_ChangeTextureAll(d2, cDummyPollutionTexName, *t2);
}

void TMonteWManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mow_model.bmd", 0x10300000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* d1 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t1 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteWRealStrawTexName);
	SMS_ChangeTextureAll(d1, cMonteWDummyStrawTexName, *t1);
	J3DModelData* d2 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t2 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (t2)
		SMS_ChangeTextureAll(d2, cDummyPollutionTexName, *t2);
}

void TMonteWAManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mowA_model.bmd", 0x10300000, 1 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
	J3DModelData* d1 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t1 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteWRealStrawTexName);
	SMS_ChangeTextureAll(d1, cMonteWDummyStrawTexName, *t1);
	J3DModelData* d2 = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* t2 = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (t2)
		SMS_ChangeTextureAll(d2, cDummyPollutionTexName, *t2);
}

// =====================================================================
// Pattern E: createModelDataArrayBase + straw (no null check)
// =====================================================================

void TMonteMHManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momA_model.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArrayBase(entry, "/scene/monteMA");
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteMDummyStrawTexName, *tex);
}

void TMonteMGManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momC_model.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArrayBase(entry, "/scene/monteMC");
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteMDummyStrawTexName, *tex);
}

void TMonteMFManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mom_model.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArrayBase(entry, "/scene/monteM");
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteMRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteMDummyStrawTexName, *tex);
}

void TMonteWCManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mow_model.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArrayBase(entry, "/scene/monteW");
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cMonteWRealStrawTexName);
	SMS_ChangeTextureAll(data, cMonteWDummyStrawTexName, *tex);
}

// =====================================================================
// Pattern F: createModelDataArrayBase + pollution null-checked
// =====================================================================

void TMareMBaseManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mareM.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArrayBase(entry, "/scene/mareM");
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (tex)
		SMS_ChangeTextureAll(data, cDummyPollutionTexName, *tex);
}

void TMareWBaseManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mareW.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArrayBase(entry, "/scene/mareW");
	J3DModelData* data = getModelDataKeeper()->getNthData(0)->unk0;
	const ResTIMG* tex = (const ResTIMG*)JKRFileLoader::getGlbResource(
	    cRealPollutionTexName);
	if (tex)
		SMS_ChangeTextureAll(data, cDummyPollutionTexName, *tex);
}

// =====================================================================
// TMareJellyFishManager - 6 model variants
// =====================================================================

void TMareJellyFishManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "jellyFish_A.bmd", 0x11240000, 0 },
		{ "jellyFish_B.bmd", 0x11240000, 0 },
		{ "jellyFish_C.bmd", 0x11240000, 0 },
		{ "jellyFish_D.bmd", 0x11240000, 0 },
		{ "jellyFish_E.bmd", 0x11240000, 0 },
		{ "jellyFish_F.bmd", 0x11240000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

// =====================================================================
// load functions
// =====================================================================

void TSunflowerLManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk3C = 500.0f;
}

void TPeachManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x17, 0x10010000, unk60);
}

void TKinopioManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x15, 0x10210000, unk60);
}

void TKinojiiManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x16, 0x10010000, unk60);
}

void TRaccoonDogManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x18, 0x10210000, unk60);
}

void TMonteMFManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x6, 0x10210000, unk60);
}

void TMonteMGManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x7, 0x10210000, unk60);
}

void TMonteMHManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x8, 0x10210000, unk60);
}

void TMonteWCManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0xc, 0x10210000, unk60);
}

void TMareMAManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0xe, 0x10210000, unk60);
}

void TMareMBManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0xf, 0x10210000, unk60);
}

void TMareMCManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x10, 0x10210000, unk60);
}

void TMareMDManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x11, 0x10210000, unk60);
}

void TMareWAManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x13, 0x10210000, unk60);
}

void TMareWBManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x14, 0x10210000, unk60);
}

// =====================================================================
// Misc small functions
// =====================================================================

void TMonteMSpecialManager::createAnmData()
{
	TObjManager::createAnmData();
}

void TMonteWSpecialManager::createAnmData()
{
	TObjManager::createAnmData();
}

void TMonteWBaseManager::createAnmData()
{
	MActorAnmData* p = new MActorAnmData();
	p->init(cMonteWCommonVolumeName, (const char**)NULL);
	unk20 = p;
}

void TMonteMBaseManager::createAnmData()
{
	MActorAnmData* p = new MActorAnmData();
	p->init(cMonteMCommonVolumeName, (const char**)NULL);
	unk20 = p;
}

// =====================================================================
// TNPCManager virtual methods
// =====================================================================

TNPCManager::TNPCManager(const char* name)
    : TEnemyManager(name)
{
	unk54 = 350.0f;
	unk58 = (const f32*)NULL;
	unk5C = (TModelDataKeeper*)NULL;
	unk60 = (TModelDataKeeper*)NULL;
	TNpcSaveStageFarClip* far = gpConductor->unkF4->unk0;
	switch (gpMarDirector->mMap) {
	case 0:  unk58 = &far->mSLFarAirport.value; break;
	case 1:  unk58 = &far->mSLFarDolpicTown.value; break;
	case 2:  unk58 = &far->mSLFarBiancoHills.value; break;
	case 3:  unk58 = &far->mSLFarRiccoHarbor.value; break;
	case 4:  unk58 = &far->mSLFarMammaBeach.value; break;
	case 5:  unk58 = &far->mSLFarPinnaBeach.value; break;
	case 6:  unk58 = &far->mSLFarSirenaBeach.value; break;
	case 7:  unk58 = &far->mSLFarHotelDelfino.value; break;
	case 8:  unk58 = &far->mSLFarMonteVillage.value; break;
	case 9:  unk58 = &far->mSLFarMareVillage.value; break;
	case 10: unk58 = &far->mSLFarCoronaMountain.value; break;
	case 13: unk58 = &far->mSLFarPinnaParco.value; break;
	default: unk58 = &far->mSLFarOthers.value; break;
	}
}

void TNPCManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 0x200) {
		for (int i = 0; i < mObjNum; i++) {
			((TLiveActor*)unk18[i])->mLiveFlag |= 0x01000000;
		}
	}
	TEnemyManager::perform(flags, gfx);
}

void TNPCManager::makePartsModelData_(u32 actorType, u32 flags,
                                      TModelDataKeeper* keeper)
{
	const TNpcInitInfo* info = SMSGetNpcInitData(actorType);

	for (int i = 0; i < 12; i++) {
		const TNpcModelData* model = info->unk4[i];
		if (model == NULL)
			continue;

		u32 localFlags = flags;
		if (model->unk2A) {
			localFlags &= ~0x00070000;
			localFlags |= 0x00100000;
		}

		for (int j = 0; j < 2; j++) {
			const char* name = model->unk8[j];
			if (name == NULL)
				continue;

			char fname[256];
			snprintf(fname, 256, "%s/%s", keeper->mFolder, name);

			void* res = JKRFileLoader::getGlbResource(fname);
			if (res == NULL)
				continue;

			SDLModelData* sdlModel
			    = keeper->createAndKeepData(model->unk8[j], localFlags);

			if (model->unk2B) {
				J3DMaterialTable* bmt = getBmt_(model->unk2A);
				if (bmt != NULL) {
					sdlModel->unk0->setMaterialTable(
					    bmt, (J3DMaterialCopyFlag)3);
				}
			}

			if (model->unk2A) {
				const ResTIMG* tex
				    = (const ResTIMG*)JKRFileLoader::getGlbResource(
				        cRealPollutionTexName);
				if (tex != NULL) {
					SMS_ChangeTextureAll(sdlModel->unk0,
					                     cDummyPollutionTexName, *tex);
				}
			}
		}
	}
}

// =====================================================================
// Constructors
// =====================================================================

TMonteWBaseManager::TMonteWBaseManager(const char* name)
    : TNPCManager(name)
{
	unk5C = mStaticCommonKeeper;
	if (unk5C == NULL) {
		unk5C = new TModelDataKeeper(cMonteWCommonVolumeName);
		mStaticCommonKeeper = unk5C;
		makePartsModelData_(9, 0x10210000, unk5C);
	}
}

TMonteMBaseManager::TMonteMBaseManager(const char* name)
    : TNPCManager(name)
{
	unk5C = mStaticCommonKeeper;
	if (unk5C == NULL) {
		unk5C = new TModelDataKeeper(cMonteMCommonVolumeName);
		mStaticCommonKeeper = unk5C;
		makePartsModelData_(0, 0x10210000, unk5C);
	}
}

TMareBaseManager::TMareBaseManager(const char* name)
    : TNPCManager(name)
{
	if (mStaticBmtNormal == NULL) {
		mStaticBmtNormal = J3DModelLoaderDataBase::loadMaterialTable(
		    JKRFileLoader::getGlbResource(cMareCommonNormalBmtName));
	}
	if (mStaticBmtPollution == NULL) {
		mStaticBmtPollution = J3DModelLoaderDataBase::loadMaterialTable(
		    JKRFileLoader::getGlbResource(cMareCommonPollutionBmtName));
	}
}

TMareMBaseManager::TMareMBaseManager(const char* name)
    : TMareBaseManager(name)
{
	unk5C = mStaticCommonKeeper;
	if (unk5C == NULL) {
		unk5C = new TModelDataKeeper(cMareMCommonVolumeName);
		mStaticCommonKeeper = unk5C;
		makePartsModelData_(0xd, 0x10210000, unk5C);
	}
}

TMareWBaseManager::TMareWBaseManager(const char* name)
    : TMareBaseManager(name)
{
	unk5C = mStaticCommonKeeper;
	if (unk5C == NULL) {
		unk5C = new TModelDataKeeper(cMareWCommonVolumeName);
		mStaticCommonKeeper = unk5C;
		makePartsModelData_(0x12, 0x10210000, unk5C);
	}
}

J3DMaterialTable* TMareBaseManager::getBmt_(bool isPollution)
{
	if (isPollution)
		return mStaticBmtPollution;
	return mStaticBmtNormal;
}

void TBoardNpcManager::clipActors(JDrama::TGraphics* gfx)
{
	clipActorsAux(gfx, *(f32*)((char*)gpConductor + 0x9C), 200.0f);
}
