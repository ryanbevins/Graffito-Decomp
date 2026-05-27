#include <NPC/NpcCoin.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <System/MarDirector.hpp>

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

void TNpcCoin::requestAppearCoin(const Vec& pos, f32 yawDeg, int count)
{
	unk4   = count;
	unk8.x = pos.x;
	unk8.y = pos.y;
	unk8.z = pos.z;

	s16 fixedPitch = 0x3552;
	unk14.x        = 0.0f;
	unk14.y        = JMASSin(fixedPitch);
	unk14.z        = JMASCos(fixedPitch);

	s16 yaw  = CLBRoundf<s16>(yawDeg * (65536.0f / 360.0f));
	f32 oldX = unk14.x;
	f32 negX = -oldX;
	f32 cosY = JMASCos(yaw);
	f32 sinY = JMASSin(yaw);
	unk14.x  = oldX * cosY + unk14.z * sinY;
	unk14.z  = negX * sinY + unk14.z * cosY;

	unk14.x *= 15.0f;
	unk14.y *= 15.0f;
	unk14.z *= 15.0f;

	if (unk4 != 0)
		return;

	bool talking = gpMarDirector->isTalkModeNow();
	bool blocked = talking || gpMarDirector->checkUnk124Thing2();
	if (blocked) {
		unk4 = 1;
		return;
	}

	if (unk0 != nullptr) {
		typedef void (*VFunc)(TMapObjBase*);
		void** vt    = *(void***)unk0;
		VFunc method = (VFunc)vt[63]; // 0xFC / 4
		method(unk0);

		TMapObjBase* obj         = unk0;
		*(u32*)((u8*)obj + 0x10) = *(u32*)&unk8.x;
		*(u32*)((u8*)obj + 0x14) = *(u32*)&unk8.y;
		*(u32*)((u8*)obj + 0x18) = *(u32*)&unk8.z;
		*(f32*)((u8*)obj + 0xAC) = unk14.x;
		*(f32*)((u8*)obj + 0xB0) = unk14.y;
		*(f32*)((u8*)obj + 0xB4) = unk14.z;
		u32* flagsPtr            = (u32*)((u8*)obj + 0xF0);
		*flagsPtr &= ~0x10u;
		unk0 = nullptr;
	} else {
		TMapObjBase* obj
		    = gpItemManager->makeObjAppear(unk8.x, unk8.y, unk8.z, 0x2000E, true);
		if (obj != nullptr) {
			*(f32*)((u8*)obj + 0xAC) = unk14.x;
			*(f32*)((u8*)obj + 0xB0) = unk14.y;
			*(f32*)((u8*)obj + 0xB4) = unk14.z;
			u32* flagsPtr            = (u32*)((u8*)obj + 0xF0);
			*flagsPtr &= ~0x10u;
		}
	}

	if (gpMSound->gateCheck(0x18807)) {
		MSoundSESystem::MSoundSE::startSoundNpcActor(
		    0x18807, (const Vec*)&unk8, 0, (JAISound**)NULL, 0, 4);
	}
}

void TNpcCoin::updateCoin()
{
	if (unk4 <= 0)
		return;

	bool talking = gpMarDirector->isTalkModeNow();
	bool blocked = talking || gpMarDirector->checkUnk124Thing2();
	if (blocked)
		return;

	unk4--;
	if (unk4 != 0)
		return;

	if (unk0 != nullptr) {
		typedef void (*VFunc)(TMapObjBase*);
		void** vt    = *(void***)unk0;
		VFunc method = (VFunc)vt[63]; // 0xFC / 4
		method(unk0);

		TMapObjBase* obj         = unk0;
		*(u32*)((u8*)obj + 0x10) = *(u32*)&unk8.x;
		*(u32*)((u8*)obj + 0x14) = *(u32*)&unk8.y;
		*(u32*)((u8*)obj + 0x18) = *(u32*)&unk8.z;
		*(f32*)((u8*)obj + 0xAC) = unk14.x;
		*(f32*)((u8*)obj + 0xB0) = unk14.y;
		*(f32*)((u8*)obj + 0xB4) = unk14.z;
		u32* flagsPtr            = (u32*)((u8*)obj + 0xF0);
		*flagsPtr &= ~0x10u;
		unk0 = nullptr;
	} else {
		TMapObjBase* obj
		    = gpItemManager->makeObjAppear(unk8.x, unk8.y, unk8.z, 0x2000E, true);
		if (obj != nullptr) {
			*(f32*)((u8*)obj + 0xAC) = unk14.x;
			*(f32*)((u8*)obj + 0xB0) = unk14.y;
			*(f32*)((u8*)obj + 0xB4) = unk14.z;
			u32* flagsPtr            = (u32*)((u8*)obj + 0xF0);
			*flagsPtr &= ~0x10u;
		}
	}

	if (gpMSound->gateCheck(0x18807)) {
		MSoundSESystem::MSoundSE::startSoundNpcActor(
		    0x18807, (const Vec*)&unk8, 0, (JAISound**)NULL, 0, 4);
	}
}
