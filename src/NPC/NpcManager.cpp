#include <NPC/NpcManager.hpp>
#include <Strategic/ObjModel.hpp>

class J3DMaterialTable;

TModelDataKeeper* TMonteMBaseManager::mStaticCommonKeeper;
TModelDataKeeper* TMonteWBaseManager::mStaticCommonKeeper;
J3DMaterialTable* TMareBaseManager::mStaticBmtNormal;
J3DMaterialTable* TMareBaseManager::mStaticBmtPollution;
TModelDataKeeper* TMareMBaseManager::mStaticCommonKeeper;
TModelDataKeeper* TMareWBaseManager::mStaticCommonKeeper;

TMareJellyFishManager* gpMareJellyFishManager;

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

void TKinopioManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "kinopio_body.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteMManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mom_model.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteMAManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momA_model.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteMBManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momB_model.bmd", 0x10210000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteMCManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momC_model.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteMDManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "momD_model.bmd", 0x10210000, 0 },
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

void TMonteWManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mow_model.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteWAManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mowA_model.bmd", 0x10300000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void TMonteWBManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "mowB_model.bmd", 0x10210000, 0 },
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
