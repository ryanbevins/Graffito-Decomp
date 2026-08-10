#define JDRAMA_TFLAG_COPY_CTOR_DECL_ONLY
#include <System/MarDirector.hpp>
#include <System/TalkCursor.hpp>
#include <System/MarioGamePad.hpp>
#include <System/FlagManager.hpp>
#include <System/Application.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MoveBG/Item.hpp>
#include <NPC/NpcBase.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

const char* cCameraBckNameShineGetInside
    = "/common/camera/camera_demo_shine_get_inside";
const char* cCameraBckNameShineGetOutside
    = "/common/camera/camera_demo_shine_get_outside";
const char* cCameraBckNameGate = "/common/camera/camera_demo_gate_in";
static const char* cNicoMamaName = "ニコママ";

void TMarDirector::entryNPC(TBaseNPC* npc) { unk88.push_back(npc); }

TBaseNPC* TMarDirector::findNearestTalkNPC()
{
	TBaseNPC* result = nullptr;
	if (gpMarioOriginal->mAction == 0xC400201) {
		f32 bestDist                   = 5000000.0f;
		JGeometry::TVec3<f32> marioPos = *gpMarioPos;
		JGadget::TVector_pointer<TBaseNPC>::iterator it;

		for (it = unk88.begin(); it != unk88.end(); ++it) {
			TBaseNPC* npc = *it;
			if (npc->checkLiveFlag(LIVE_FLAG_UNK100000)
			    || !npc->checkLiveFlag(LIVE_FLAG_UNK20000))
				continue;

			f32 dist = (npc->mPosition.x - marioPos.x)
			               * (npc->mPosition.x - marioPos.x)
			           + (npc->mPosition.y - marioPos.y)
			                 * (npc->mPosition.y - marioPos.y)
			           + (npc->mPosition.z - marioPos.z)
			                 * (npc->mPosition.z - marioPos.z);
			if (dist < bestDist) {
				bestDist = dist;
				result   = npc;
			}
		}
	}
	return result;
}

void TMarDirector::movement_game()
{
	unk84->associateNPC(nullptr);
	if ((int)unk124 != 0)
		return;

	unk18[0]->offFlag(0x4);
	if (gpMarioOriginal->isHolding())
		return;

	if (gpCamera->isLButtonCameraSpecifyMode(gpCamera->mMode))
		return;

	bool bVar1 = true;
	if (!gpCamera->isSimpleDemoCamera() && gpCamera->mMode != 0x49) {
		bVar1 = false;
	}

	if (!bVar1) {
		TBaseNPC* takeNpc = nullptr;
		JGadget::TVector_pointer<TBaseNPC>::iterator it;
		for (it = unk88.begin(); it != unk88.end(); ++it) {
			TBaseNPC* npc = *it;
			if (npc->isNowCanTaken() && gpMarioOriginal->isTakeSituation(npc))
				takeNpc = npc;
		}

		if (takeNpc != nullptr) {
			unk84->associateNPC(takeNpc);
		} else {
			TBaseNPC* talkNpc = findNearestTalkNPC();
			if (talkNpc != nullptr) {
				unkA0 = talkNpc;
				unk84->associateNPC(talkNpc);
				unk18[0]->onFlag(4);
				unk128 |= 0x1;
				if ((unk128 & 2) && (unk18[0]->mEnabledFrameMeaning & 0x800))
					unk126 = 1;
			}
		}
	}
}

void TMarDirector::fireGetBlueCoin(TCoin* coin)
{
	if (!coin)
		return;

	TFlagManager::smInstance->setBlueCoinFlag(gpApplication.mCurrArea.unk0,
	                                          coin->unk134);
	unk4C |= 0x200;
	unk261 = 1;
	if (gpMSound->gateCheck(0x4845))
		MSoundSESystem::MSoundSE::startSoundActor(0x4845, coin->mPosition, 0,
		                                          nullptr, 0, 4);
}

void TMarDirector::fireGetNozzle(TItemNozzle* nozzle)
{
	if (!nozzle)
		return;

	u8 area = gpApplication.mCurrArea.unk0;

	if (nozzle->isActorType(0x20000022)
	    && !TFlagManager::smInstance->getNozzleRight(area, 0)) {
		TFlagManager::smInstance->setNozzleRight(area, 0);
		unk4C |= 0x200;
		unk261 = 3;
	} else if (nozzle->isActorType(0x2000002A)
	           && !TFlagManager::smInstance->getNozzleRight(area, 1)) {
		TFlagManager::smInstance->setNozzleRight(area, 1);
		unk4C |= 0x200;
		unk261 = 4;
	}
}

void TMarDirector::fireGetStar(TShine* shine)
{
	unk25C = shine;
	unk4C |= 1;
	JGeometry::TVec3<f32>& v = shine->mInitialRotation;
	fireStartDemoCamera(!shine->unk190 ? cCameraBckNameShineGetOutside
	                                   : cCameraBckNameShineGetInside,
	                    &gpMarioOriginal->mPosition, -1, v.y, false, nullptr, 0,
	                    nullptr, JDrama::TFlagT<u16>(0));
}

void TMarDirector::fireRideYoshi(TYoshi* yoshi)
{
	if (!yoshi)
		return;

	if (gpApplication.mCurrArea.unk0 != 1)
		return;

	if (TFlagManager::smInstance->getBool(0x1038F))
		return;

	TFlagManager::smInstance->setBool(true, 0x1038F);
	unk4C |= 0x200;
	unk261 = 5;
}

void TMarDirector::movement()
{
	switch (mState) {
	case STATE_UNK4:
		movement_game();
		break;
	}
}

#pragma dont_inline on
void TMarDirector::setNextStage(u16 param_1, JDrama::TActor* param_2)
{
	if (unk4C & 0x2)
		return;

	TGameSequence next;

	if (param_1 >= 0x100) {
		next.unk0 = (param_1 >> 8) - 1;
		next.unk1 = param_1;
	} else {
		next.unk0 = param_1;
		next.unk1 = 0xff;
	}

	gpApplication.mNextArea = next;

	if (param_2) {
		unk4C |= 0x4;
		unk250 = param_2;
	} else if (gpApplication.mCurrArea.unk0 == 1
	           && (next.unk0 == 5 || next.unk0 == 6 || next.unk0 == 8)) {
		unk4C |= 0x8;
	} else {
		unk4C |= 0x2;
	}

	if (next.unk0 == 0x37) {
		unk4C |= 0x100;
		gpApplication.mMovie = 6;
	}
}
#pragma dont_inline off

#pragma dont_inline on
void TMarDirector::fireStartDemoCamera(const char* param_1,
                                       const JGeometry::TVec3<f32>* param_2,
                                       s32 param_3, f32 param_4, bool param_5,
                                       s32 (*param_6)(u32, u32), u32 param_7,
                                       JDrama::TActor* param_8,
                                       JDrama::TFlagT<u16> param_9)
{
	if (((unk24C - unk24D) & 7) >= 7)
		return;

	unk4C |= 0x40;
	unk12C[unk24C].unk0  = param_1;
	unk12C[unk24C].unk4  = param_2;
	unk12C[unk24C].unk8  = param_3;
	unk12C[unk24C].unkC  = param_4;
	unk12C[unk24C].unk10 = param_5;
	unk12C[unk24C].unk14 = param_6;
	unk12C[unk24C].unk18 = param_7;
	unk12C[unk24C].unk1C = param_8;
	unk12C[unk24C].unk20 = param_9;

	unk24C += 1;
	unk24C &= 7;
}
#pragma dont_inline off

void TMarDirector::fireEndDemoCamera() { unk4C |= 0x80; }

void TMarDirector::fireStreamingMovie(u8 param_1)
{
	switch (param_1) {
	case 0:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0x1, nullptr);
			TFlagManager::smInstance->setBool(true, 0x10389);
			TFlagManager::smInstance->setBool(true, 0x30004);
			gpApplication.mMovie = param_1;
		}
		break;

	case 10:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0x3B, nullptr);
			gpApplication.mMovie = param_1;
		}
		break;

	case 7:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0xE06, nullptr);
			gpApplication.mMovie = param_1;
		}
		break;

	case 8:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0xE07, nullptr);
			gpApplication.mMovie = param_1;
		}
		break;

	case 11:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0x3C, nullptr);
			gpApplication.mMovie = param_1;
		}
		break;

	case 2:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0x101, nullptr);
			gpApplication.mMovie = param_1;
		}
		break;

	case 12:
	default:
		if (!(unk4C & 0x100)) {
			unk4C |= 0x100;
			setNextStage(0xF, nullptr);
			gpApplication.mMovie = (u8)param_1;
		}
		break;
	}
}
