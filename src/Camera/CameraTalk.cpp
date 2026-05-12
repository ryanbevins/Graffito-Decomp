#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <NPC/NpcBase.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>

void CPolarSubCamera::ctrlTalkCamera_()
{
	if (unk7C == 0) {
		unk8C.x = gpCameraMario->mPosX;
		unk8C.y = gpCameraMario->mPosY;
		unk8C.z = gpCameraMario->mPosZ;
	}
	calcPosAndAt_();
}

void CPolarSubCamera::makeMtxForPrevTalk()
{
	if (isTalkCameraSpecifyMode(mMode)) {
		unkA6        = unkAC;
		int oldMode  = unk58;
		s16 frame    = (s16)getCameraInbetweenFrame_(oldMode);
		changeCamModeSpecifyFrame_(oldMode, frame);
		unk120->onNeutralMarioKey();

		JGadget::TVector_pointer<TBaseNPC>& npcs = gpMarDirector->unk88;
		for (TBaseNPC** it = npcs.begin(); it != npcs.end(); ++it) {
			(*it)->npcTalkOut();
		}
	}
}

void CPolarSubCamera::makeMtxForTalk(const TBaseNPC* npc)
{
	killHeightPan_();
	unkAC      = unkA6;
	int mode   = 0xC;
	unkA6      = (s16)(*gpMarioAngleY - 0x8000);
	unk58      = mMode;

	int npcCode = npc->mActorType;
	if (npcCode == 0x400001A) {
		mode = 0x40;
	} else if (npcCode < 0x400001A) {
		if (npcCode == 0x4000007) {
			mode = 0xA;
		}
	} else if (npcCode < 0x400001C) {
		mode = 0x3F;
	}

	if (mode == 0xC) {
		if (npc->isSmallNpc()) {
			mode = 0x2D;
		}
	}

	s16 frame = (s16)getCameraInbetweenFrame_(mode);
	changeCamModeSpecifyFrame_(mode, frame);
}
