#include <NPC/NpcCoin.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MoveBG/MapObjBase.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <System/MarDirector.hpp>

template <> s16 CLBRoundf<s16>(f32);

static inline bool isCoinAppearBlocked(const TMarDirector* director)
{
	bool blocked = true;
	bool isTalk  = director->isTalkModeNow();
	if (!isTalk) {
		bool isDemo = director->checkUnk124Thing2();
		if (!isDemo)
			blocked = false;
	}
	return blocked;
}

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
	unk0->kill();
}

void TNpcCoin::requestAppearCoin(const Vec& pos, f32 yawDeg, int count)
{
	unk4   = count;
	unk8.x = pos.x;
	unk8.y = pos.y;
	unk8.z = pos.z;

	s16 fixedPitch = 0x3552;
	f32 cosVal     = JMASCos(fixedPitch);
	f32 sinVal     = JMASSin(fixedPitch);
	unk14.x        = 0.0f;
	unk14.y        = sinVal;
	unk14.z        = cosVal;

	s16 yaw  = CLBRoundf<s16>(yawDeg * (65536.0f / 360.0f));
	f32 oldX = unk14.x;
	f32 negX = -oldX;
	unk14.x  = oldX * JMASCos(yaw) + unk14.z * JMASSin(yaw);
	unk14.z  = negX * JMASSin(yaw) + unk14.z * JMASCos(yaw);

	unk14.x *= 15.0f;
	unk14.y *= 15.0f;
	unk14.z *= 15.0f;

	if (unk4 != 0)
		return;

	if (isCoinAppearBlocked(gpMarDirector)) {
		unk4 = 1;
		return;
	}

	if (unk0 != nullptr) {
		(*(void (**)(TMapObjBase*))(*(u8**)unk0 + 0xFC))(unk0);

		TMapObjBase* obj                           = unk0;
		*(JGeometry::TVec3<f32>*)((u8*)obj + 0x10) = unk8;
		((JGeometry::TVec3<f32>*)((u8*)obj + 0xAC))->set(unk14.x, unk14.y, unk14.z);
		u32* flagsPtr = (u32*)((u8*)obj + 0xF0);
		*flagsPtr &= ~0x10u;
		unk0 = nullptr;
	} else {
		TMapObjBase* obj
		    = gpItemManager->makeObjAppear(unk8.x, unk8.y, unk8.z, 0x2000000E, true);
		if (obj != nullptr) {
			((JGeometry::TVec3<f32>*)((u8*)obj + 0xAC))->set(unk14.x, unk14.y, unk14.z);
			u32* flagsPtr = (u32*)((u8*)obj + 0xF0);
			*flagsPtr &= ~0x10u;
		}
	}

	if (gpMSound->gateCheck(0x8807)) {
		MSoundSESystem::MSoundSE::startSoundNpcActor(
		    0x8807, (const Vec*)&unk8, 0, (JAISound**)NULL, 0, 4);
	}
}

void TNpcCoin::updateCoin()
{
	if (unk4 <= 0)
		return;

	if (isCoinAppearBlocked(gpMarDirector))
		return;

	unk4--;
	if (unk4 != 0)
		return;

	if (unk0 != nullptr) {
		unk0->appear();

		TMapObjBase* obj                           = unk0;
		*(JGeometry::TVec3<f32>*)((u8*)obj + 0x10) = unk8;
		((JGeometry::TVec3<f32>*)((u8*)obj + 0xAC))->set(unk14.x, unk14.y, unk14.z);
		u32* flagsPtr = (u32*)((u8*)obj + 0xF0);
		*flagsPtr &= ~0x10u;
		unk0 = nullptr;
	} else {
		TMapObjBase* obj
		    = gpItemManager->makeObjAppear(unk8.x, unk8.y, unk8.z, 0x2000000E, true);
		if (obj != nullptr) {
			((JGeometry::TVec3<f32>*)((u8*)obj + 0xAC))->set(unk14.x, unk14.y, unk14.z);
			u32* flagsPtr = (u32*)((u8*)obj + 0xF0);
			*flagsPtr &= ~0x10u;
		}
	}

	if (gpMSound->gateCheck(0x8807)) {
		MSoundSESystem::MSoundSE::startSoundNpcActor(
		    0x8807, (const Vec*)&unk8, 0, (JAISound**)NULL, 0, 4);
	}
}
