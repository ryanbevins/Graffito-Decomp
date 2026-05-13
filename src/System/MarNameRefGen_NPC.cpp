#include <System/MarNameRefGen.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcManager.hpp>
#include <Strategic/LiveManager.hpp>

JDrama::TNameRef* TMarNameRefGen::getNameRef_NPC(const char* name) const
{
	if (strcmp(name, "NPCMonteM") == 0)
		return new TBaseNPC(0x04000001, "?");
	if (strcmp(name, "NPCMonteMA") == 0)
		return new TBaseNPC(0x04000002, "?");
	if (strcmp(name, "NPCMonteMB") == 0)
		return new TBaseNPC(0x04000003, "?");
	if (strcmp(name, "NPCMonteMC") == 0)
		return new TBaseNPC(0x04000004, "?");
	if (strcmp(name, "NPCMonteMD") == 0)
		return new TBaseNPC(0x04000005, "?");
	if (strcmp(name, "NPCMonteME") == 0)
		return new TBaseNPC(0x04000006, "?");
	if (strcmp(name, "NPCMonteMF") == 0)
		return new TBaseNPC(0x04000007, "?");
	if (strcmp(name, "NPCMonteMG") == 0)
		return new TBaseNPC(0x04000008, "?");
	if (strcmp(name, "NPCMonteMH") == 0)
		return new TBaseNPC(0x04000009, "?");
	if (strcmp(name, "NPCMonteW") == 0)
		return new TBaseNPC(0x0400000A, "?");
	if (strcmp(name, "NPCMonteWA") == 0)
		return new TBaseNPC(0x0400000B, "?");
	if (strcmp(name, "NPCMonteWB") == 0)
		return new TBaseNPC(0x0400000C, "?");
	if (strcmp(name, "NPCMonteWC") == 0)
		return new TBaseNPC(0x0400000D, "?");
	if (strcmp(name, "NPCMareM") == 0)
		return new TBaseNPC(0x0400000E, "?");
	if (strcmp(name, "NPCMareMA") == 0)
		return new TBaseNPC(0x0400000F, "?");
	if (strcmp(name, "NPCMareMB") == 0)
		return new TBaseNPC(0x04000010, "?");
	if (strcmp(name, "NPCMareMC") == 0)
		return new TBaseNPC(0x04000011, "?");
	if (strcmp(name, "NPCMareMD") == 0)
		return new TBaseNPC(0x04000012, "?");
	if (strcmp(name, "NPCMareW") == 0)
		return new TBaseNPC(0x04000013, "?");
	if (strcmp(name, "NPCMareWA") == 0)
		return new TBaseNPC(0x04000014, "?");
	if (strcmp(name, "NPCMareWB") == 0)
		return new TBaseNPC(0x04000015, "?");
	if (strcmp(name, "NPCKinopio") == 0)
		return new TBaseNPC(0x04000016, "?");
	if (strcmp(name, "NPCKinojii") == 0)
		return new TBaseNPC(0x04000017, "?");
	if (strcmp(name, "NPCPeach") == 0)
		return new TBaseNPC(0x04000018, "?");
	if (strcmp(name, "NPCRaccoonDog") == 0)
		return new TBaseNPC(0x04000019, "?");
	if (strcmp(name, "NPCSunflowerL") == 0)
		return new TBaseNPC(0x0400001A, "?");
	if (strcmp(name, "NPCSunflowerS") == 0)
		return new TBaseNPC(0x0400001B, "?");
	if (strcmp(name, "NPCDummy") == 0)
		return new TBaseNPC(0x0400001C, "?");
	if (strcmp(name, "NPCBoard") == 0)
		return new TBaseNPC(0x0400001D, "?");

	if (strcmp(name, "MonteMManager") == 0)
		return new TMonteMBaseManager("MonteMManager");
	if (strcmp(name, "MonteMAManager") == 0)
		return new TMonteMBaseManager("MonteMAManager");
	if (strcmp(name, "MonteMBManager") == 0)
		return new TMonteMBaseManager("MonteMBManager");
	if (strcmp(name, "MonteMCManager") == 0)
		return new TMonteMBaseManager("MonteMCManager");
	if (strcmp(name, "MonteMDManager") == 0)
		return new TMonteMBaseManager("MonteMDManager");
	if (strcmp(name, "MonteMEManager") == 0)
		return new TMonteMBaseManager("MonteMEManager");
	if (strcmp(name, "MonteMFManager") == 0)
		return new TMonteMBaseManager("MonteMFManager");
	if (strcmp(name, "MonteMGManager") == 0)
		return new TMonteMBaseManager("MonteMGManager");
	if (strcmp(name, "MonteMHManager") == 0)
		return new TMonteMBaseManager("MonteMHManager");
	if (strcmp(name, "MonteWManager") == 0)
		return new TMonteWBaseManager("MonteWManager");
	if (strcmp(name, "MonteWAManager") == 0)
		return new TMonteWBaseManager("MonteWAManager");
	if (strcmp(name, "MonteWBManager") == 0)
		return new TMonteWBaseManager("MonteWBManager");
	if (strcmp(name, "MonteWCManager") == 0)
		return new TMonteWBaseManager("MonteWCManager");
	if (strcmp(name, "MareMManager") == 0)
		return new TMareMBaseManager("MareMManager");
	if (strcmp(name, "MareMAManager") == 0)
		return new TMareMBaseManager("MareMAManager");
	if (strcmp(name, "MareMBManager") == 0)
		return new TMareMBaseManager("MareMBManager");
	if (strcmp(name, "MareMCManager") == 0)
		return new TMareMBaseManager("MareMCManager");
	if (strcmp(name, "MareMDManager") == 0)
		return new TMareMBaseManager("MareMDManager");
	if (strcmp(name, "MareWManager") == 0)
		return new TMareWBaseManager("MareWManager");
	if (strcmp(name, "MareWAManager") == 0)
		return new TMareWBaseManager("MareWAManager");
	if (strcmp(name, "MareWBManager") == 0)
		return new TMareWBaseManager("MareWBManager");
	if (strcmp(name, "KinopioManager") == 0)
		return new TNPCManager("KinopioManager");
	if (strcmp(name, "KinojiiManager") == 0)
		return new TNPCManager("KinojiiManager");
	if (strcmp(name, "PeachManager") == 0)
		return new TNPCManager("PeachManager");
	if (strcmp(name, "RaccoonDogManager") == 0)
		return new TNPCManager("RaccoonDogManager");
	if (strcmp(name, "SunflowerLManager") == 0)
		return new TNPCManager("SunflowerLManager");
	if (strcmp(name, "SunflowerSManager") == 0)
		return new TNPCManager("SunflowerSManager");
	if (strcmp(name, "MareJellyFish") == 0)
		return new TMareJellyFishManager("MareJellyFish");
	if (strcmp(name, "BoardNpcManager") == 0)
		return new TLiveManager("BoardNpcManager");
	return nullptr;
}
