#include <NPC/NpcManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/ResTIMG.hpp>

class J3DMaterialTable;

const char* cDummyPollutionTexName    = "H_ma_rak_dummy";
const char* cRealPollutionTexName     = "/scene/map/pollution/H_ma_rak.bti";
const char* cMonteMDummyStrawTexName  = "I_mom_mino_dummyI4";
const char* cMonteWDummyStrawTexName  = "I_mow_mino_dummyI4";
const char* cMonteMRealStrawTexName   = "/scene/monteMCommon/I_mom_mino_rgba.bti";
const char* cMonteWRealStrawTexName   = "/scene/monteWCommon/I_mow_mino_rgba.bti";

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
	makePartsModelData_(0x12, 0x10010000, unk60);
}

void TKinojiiManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x12, 0x10010000, unk60);
}

void TRaccoonDogManager::load(JSUMemoryInputStream& stream)
{
	TEnemyManager::load(stream);
	unk3C = 250.0f;
	unk60 = new TModelDataKeeper(unk1C->mFolder);
	makePartsModelData_(0x12, 0x10010000, unk60);
}
