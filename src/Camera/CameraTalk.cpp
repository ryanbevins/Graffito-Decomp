#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <NPC/NpcBase.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>

void CPolarSubCamera::makeMtxForTalk(const TBaseNPC* npc)
{
	killHeightPan_();
	mCurrentTarget.unk2C      = mCurrentTarget.mYaw;
	int mode   = 0xC;
	mCurrentTarget.mYaw      = (s16)(*gpMarioAngleY - 0x8000);
	unk58      = mMode;

	int npcCode = npc->mActorType;
	switch (npcCode) {
	case 0x400001B:
		mode = 0x3F;
		break;
	case 0x400001A:
		mode = 0x40;
		break;
	case 0x4000007:
		mode = 0xA;
		break;
	default:
		if (npc->isSmallNpc()) {
			mode = 0x2D;
		}
		break;
	}

	s16 frame = (s16)getCameraInbetweenFrame_(mode);
	changeCamModeSpecifyFrame_(mode, frame);
}

void CPolarSubCamera::makeMtxForPrevTalk()
{
	if (isTalkCameraSpecifyMode(mMode)) {
		mCurrentTarget.mYaw        = mCurrentTarget.unk2C;
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

void CPolarSubCamera::ctrlTalkCamera_()
{
	if (unk7C == 0) {
		TCameraMarioData* mario = gpCameraMario;
		mCurrentTarget.mTarget.x                 = mario->mPosX;
		mCurrentTarget.mTarget.y                 = mario->mPosY;
		mCurrentTarget.mTarget.z                 = mario->mPosZ;
	}
	calcPosAndAt_();
}
