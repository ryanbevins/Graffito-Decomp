#include <Camera/Camera.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Player/MarioAccess.hpp>

template <> BOOL CLBChaseGeneralConstantSpecifySpeed<f32>(f32*, f32, f32);

void CPolarSubCamera::killHeightPan_()
{
	u16& flags = *(u16*)((u8*)this + 0x64);
	flags &= ~1;
	flags &= ~2;
	*(f32*)((u8*)this + 0x84)  = *(f32*)((u8*)this + 0x9C);
	*(f32*)((u8*)this + 0x24C) = 0.0f;
}

#pragma dont_inline on
bool CPolarSubCamera::isNotHeightPanCamMode_() const
{
	bool result = false;
	if (isLButtonCameraSpecifyMode(mMode) || isRailCameraSpecifyMode(mMode)) {
		result = true;
	} else {
		int idx = mMode - 8;
		switch (idx) {
		case 0: case 5: case 7: case 10: case 11:
		case 35: case 41: case 43: case 54: case 57:
			result = true;
		}
	}
	return result;
}
#pragma dont_inline off

void CPolarSubCamera::execHeightPan_()
{
	bool touchesGround   = SMS_IsMarioTouchGround4cm() == false;
	f32 trackY           = 0.0f;
	if (touchesGround) {
		trackY = unk68->unk2C;
	}

	f32 chaseSpeed = unk68->unk30;
	CLBChaseGeneralConstantSpecifySpeed<f32>(
	    (f32*)((u8*)this + 0x24C), trackY, chaseSpeed);

	f32 panOff = *(f32*)((u8*)this + 0x24C);
	*(f32*)((u8*)this + 0x90) += panOff;

	f32 deltaY = *(f32*)((u8*)this + 0x9C) - *(f32*)((u8*)this + 0xD0);

	bool needsPan = false;
	if (!isNotHeightPanCamMode_() && !SMS_IsMarioTouchGround4cm()
	    && !gpCameraMario->isMarioGoDown() && !SMS_IsMarioOnWire()) {
		u32 status = SMS_GetMarioStatus();
		if (status - 0x200000 != 0x345)
			needsPan = true;
	}

	if (needsPan) {
		*(u16*)((u8*)this + 0x64) |= 1;
		*(u16*)((u8*)this + 0x64) &= ~6;
		*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0xB8);

		f32 chaseRate = unk68->unk34;
		CLBChaseDecrease((f32*)((u8*)this + 0x84),
		                 *(f32*)((u8*)this + 0x9C), chaseRate, 0.0f);

		if (*(f32*)((u8*)this + 0xA8) != *(f32*)((u8*)this + 0xDC)) {
			*(f32*)((u8*)this + 0x84) += deltaY;
		}
	} else if ((*(u16*)((u8*)this + 0x64) & 3) != 0) {
		if (*(f32*)((u8*)this + 0xA8) != *(f32*)((u8*)this + 0xDC)) {
			*(f32*)((u8*)this + 0x84) += deltaY;
		}
		if ((*(u16*)((u8*)this + 0x64) & 1) != 0) {
			*(u16*)((u8*)this + 0x64) &= ~1;
			bool useLock = false;
			int mode     = mMode;
			switch (mode) {
			case 5:
			case 0x13: {
				void* cmd = *(void**)((u8*)this + 0x2D4);
				s16  a   = *(s16*)((u8*)this + 0x256);
				s16  b   = *(s16*)((u8*)cmd + 0x54);
				if (a < b)
					useLock = true;
				break;
			}
			}
			if (useLock) {
				*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0x9C);
				*(f32*)((u8*)this + 0x14) = *(f32*)((u8*)this + 0x9C);
				*(u16*)((u8*)this + 0x64) &= ~6;
			} else {
				*(u16*)((u8*)this + 0x64) |= 2;
			}
		}

		if (*(f32*)((u8*)this + 0x84) != *(f32*)((u8*)this + 0x9C)) {
			void* cmd = *(void**)((u8*)this + 0x2D4);
			f32  b   = *(f32*)((u8*)cmd + 0x40);
			f32  a   = *(f32*)((u8*)cmd + 0x2C);
			s32 done  = CLBChaseSpecialDecrease(
                (f32*)((u8*)this + 0x84), *(f32*)((u8*)this + 0x9C), a, b);
			if (done == 0 && !touchesGround) {
				*(u16*)((u8*)this + 0x64) &= ~6;
			}
		} else if (!touchesGround) {
			*(u16*)((u8*)this + 0x64) &= ~6;
		}
	} else {
		*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0x9C);
	}
}

void CPolarSubCamera::killHeightPanWhenChangeCamMode_()
{
	bool isModeOff = false;
	bool isOff     = false;
	if (isLButtonCameraSpecifyMode(mMode) || isRailCameraSpecifyMode(mMode)) {
		isModeOff = true;
	} else {
		int idx = mMode - 8;
		if ((u32)idx <= 0x39) {
			switch (idx) {
			case 0:
			case 5:
			case 7:
			case 10:
			case 11:
			case 35:
			case 41:
			case 43:
			case 54:
			case 57:
				isModeOff = true;
				break;
			}
		}
	}

	if (isModeOff)
		isOff = true;

	int prev = *(int*)((u8*)this + 0x54);
	switch (prev) {
	case 0xF:
	case 0x3E:
			isOff = true;
		break;
	}

	if (isOff) {
		u16& flags = *(u16*)((u8*)this + 0x64);
		flags &= ~1;
		flags &= ~6;
		*(f32*)((u8*)this + 0x84)  = *(f32*)((u8*)this + 0x9C);
		*(f32*)((u8*)this + 0x24C) = 0.0f;
	}
}
