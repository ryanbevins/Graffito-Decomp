#include <Enemy/Kukku.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static char* tori_bastable[] = {
	(char*)"/scene/tori/bas/tori_back.bas",
	(char*)"/scene/tori/bas/tori_down.bas",
	(char*)0,
	(char*)"/scene/tori/bas/tori_hit.bas",
	(char*)"/scene/tori/bas/tori_wait.bas",
	(char*)"/scene/tori/bas/tori_fall_end.bas",
};

void TKukkuManager::createModelData()
{
	static const TModelDataLoadEntry entry[] = {
	    { "tori.bmd", 0x10210000, 0 },
	    { nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TKukkuManager::TKukkuManager(const char* name)
    : TSmallEnemyManager(name)
{
}

const char** TKukku::getBasNameTable() const
{
	return (const char**)tori_bastable;
}

void TKukku::setDeadAnm()
{
	getMActor()->setBck("tori_down");
}

void TKukku::perform(u32 action, JDrama::TGraphics* graphics)
{
	TSmallEnemy::perform(action, graphics);

	for (void** ball = &unk194[0]; ball != &unk1A0; ++ball) {
		THitActor* a = (THitActor*)*ball;
		a->perform(action, graphics);
	}
}

TKukku::TKukku(const char* name)
    : TSmallEnemy(name)
{
	unk1A0 = nullptr;
}
