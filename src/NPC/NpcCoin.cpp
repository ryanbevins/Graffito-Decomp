#include <NPC/NpcCoin.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjManager.hpp>

TNpcCoin::TNpcCoin(int eventID)
{
	unk0    = nullptr;
	unk4    = 0;
	unk8.x  = 0.0f;
	unk8.y  = 0.0f;
	unk8.z  = 0.0f;
	unk14.x = 0.0f;
	unk14.y = 0.0f;
	unk14.z = 0.0f;

	unk0 = TMapObjBaseManager::newAndRegisterObjByEventID((u32)eventID, nullptr);

	// Virtual call at vtable+0xE4 (call init / setup on the new TMapObjBase)
	typedef void (*VFunc)(TMapObjBase*);
	void** vt    = *(void***)unk0;
	VFunc method = (VFunc)vt[57]; // 0xE4 / 4
	method(unk0);
}

void TNpcCoin::execAppearCoin_() { }

void TNpcCoin::requestAppearCoin(const Vec& pos, f32 height, int count)
{
	(void)pos;
	(void)height;
	(void)count;
}

void TNpcCoin::updateCoin() { }
