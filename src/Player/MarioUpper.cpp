#include <Player/MarioMain.hpp>

#include <Player/Watergun.hpp>
#include <Player/MarioAnimeData.hpp>
#include <Camera/Camera.hpp>
#include <Strategic/LiveActor.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MSound/MSoundBGM.hpp>

// MarioUpper: -inline deferred, functions in REVERSE address order.

// checkPumping: 0x80141C98, size 0x118
void TMario::checkPumping()
{
	// Check pump trigger active (controller work offset 0x1C)
	f32 pumpFrame = *(f32*)((u8*)unk108 + 0x1C);
	if (pumpFrame > 0.0f && mPumpState != 0) {
		mPumpState = 0;
		unk37E = 0;
		return;
	}

	// Check if this is the active player and the camera is in L-button mode
	if (gpMarioOriginal == this
	    && gpCamera->isLButtonCameraSpecifyMode(gpCamera->mMode)
	    && checkPumpEnable()) {
		mPumpState = 1;
		unk37E = 0;
		return;
	}

	// Check action in range 0x800447
	u32 action = mAction;
	if ((action - 0x800000) == 0x447) {
		mPumpState = 1;
		unk37E = 0;
		return;
	}

	// Check action 0xC008220
	if ((action - 0xC000000) == 0x8220 && mPumpState == 5) {
		mPumpState = 1;
		unk37E = 0;
		return;
	}

	// Check FLUDD emitting flag (bit 14)
	if (checkFlag(MARIO_FLAG_FLUDD_EMITTING)) {
		mPumpState = 0;
		unk37E = 0;
	}
}

// checkPumpEnable: 0x80141ACC, size 0x1CC
BOOL TMario::checkPumpEnable()
{
	if (mWaterGun != NULL) {
		if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
			if (gMarioAnimeData[mAnimationId].isPumpOK()) {
				if (!onYoshi()) {
					f32 dirty = *(f32*)((u8*)this + 0x368);
					BOOL isDirty;
					if (dirty > 0.0f)
						isDirty = TRUE;
					else
						isDirty = FALSE;
					if (!isDirty
					    || !(dirty / (f32)mGraffitoParams.mSinkTime.get()
					         > mGraffitoParams.mSinkPumpLimit.get())) {
						u32 pumpState = mPumpState;
						if (pumpState != 4 && pumpState != 3
						    && pumpState != 2) {
							if (mAction != 0x88D
							    || *(u8*)((u8*)mWaterGun->getCurrentNozzle()
							              + 0x18)
							           != 1) {
								TWaterGun* wg2 = mWaterGun;
								TNozzleBase* nozzle2
								    = wg2->getCurrentNozzle();
								if (nozzle2->getNozzleKind() != 1
								    || ((TNozzleTrigger*)mWaterGun
								            ->getCurrentNozzle())
								               ->unk385
								           != TNozzleTrigger::DEAD) {
									TWaterGun* wg4 = mWaterGun;
									f32 wgVal      = wg4->unk1D00;
									bool belowZero;
									if (wgVal < 0.0f)
										belowZero = true;
									else
										belowZero = false;
									if (!belowZero) {
										bool aboveZero;
										if (wgVal > 0.0f)
											aboveZero = true;
										else
											aboveZero = false;
										if (!aboveZero) {
											if (!checkActionFlag(0x1000))
												return TRUE;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	mPumpState = 5;
	unk37E     = 0;
	return FALSE;
}

// stateMachineUpper: 0x80141854, size 0x278
void TMario::stateMachineUpper()
{
	switch (mPumpState) {
	case 0: {
		if (!checkPumpEnable()) {
			M3UModelMario* model = mModel;
			J3DModel* j3dModel = *(J3DModel**)((u8*)model + 0x0C);
			*(f32*)((u8*)j3dModel + 0x24) = 0.0f;
			mPumpState = 5;
		}

		f32 pumpFrame = *(f32*)((u8*)unk108 + 0x1C);
		if (pumpFrame == 0.0f) {
			mPumpState = 1;
			unk37E = mUpperBodyParams.mPumpWaitTime.value;
		}

		TWaterGun* wg;
		if (!checkFlag((E_MARIO_FLAG)(MARIO_FLAG_IN_SHALLOW_WATER
		                               | MARIO_FLAG_IN_WATER))) {
			if ((wg = mWaterGun) != NULL) {
				bool emitting;
				if (wg->mCurrentWater == 0) {
					emitting = false;
				} else {
					s32 kind = wg->getCurrentNozzle()->getNozzleKind();
					if (kind == 1) {
						TNozzleTrigger* nozzle
						    = (TNozzleTrigger*)wg->getCurrentNozzle();
						if (nozzle->unk385 == TNozzleTrigger::ACTIVE)
							emitting = true;
						else
							emitting = false;
					} else if (wg->getCurrentNozzle()->unk378 > 0.0f) {
						emitting = true;
					} else {
						emitting = false;
					}
				}
				if (emitting)
					emitSweatSometimes();
			}
		}
		break;
	}

	case 1: {
		if (!checkPumpEnable()) {
			M3UModelMario* model = mModel;
			J3DModel* j3dModel = *(J3DModel**)((u8*)model + 0x0C);
			*(f32*)((u8*)j3dModel + 0x24) = 0.0f;
			mPumpState = 5;
		}

		u16 timer = unk37E;
		if (timer != 0) {
			unk37E = timer - 1;
		} else {
			M3UModelMario* model = mModel;
			J3DModel* j3dModel = *(J3DModel**)((u8*)model + 0x0C);
			*(f32*)((u8*)j3dModel + 0x24) = 0.0f;
			mPumpState = 5;
		}

		checkPumping();
		break;
	}

	case 2: {
		if ((mAction - 0x80000000) == 0x387)
			mPumpState = 5;

		if (*(u32*)((u8*)this + 0x6C) == 0)
			mPumpState = 5;

		if ((mAction - 0x04000000) == 0x440) {
			if (mForwardVel > 20.0f)
				emitSweatSometimes();
		}
		break;
	}

	case 4: {
		M3UModelMario* model = mModel;
		J3DModel* j3dModel = *(J3DModel**)((u8*)model + 0x0C);
		u8 flags = *(u8*)((u8*)j3dModel + 0x19);
		BOOL isAnimPlaying;
		if (flags & 0x3)
			isAnimPlaying = TRUE;
		else
			isAnimPlaying = FALSE;
		if (isAnimPlaying) {
			mPumpState = 5;
		}
		break;
	}

	case 3:
	case 5:
	default:
		if (checkPumpEnable()) {
			checkPumping();
		}
		break;
	}
}
