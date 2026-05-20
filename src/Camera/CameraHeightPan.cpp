#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Player/MarioAccess.hpp>

static bool isHeightPanModeIndex(int idx)
{
	switch (idx) {
	case 0: case 5: case 7: case 10: case 11:
	case 35: case 41: case 43: case 54: case 57:
		return true;
	}
	return false;
}

void CPolarSubCamera::killHeightPan_()
{
	u16& flags = *(u16*)((u8*)this + 0x64);
	flags &= ~1;
	flags &= ~6;
	*(f32*)((u8*)this + 0x84)  = *(f32*)((u8*)this + 0x9C);
	*(f32*)((u8*)this + 0x24C) = 0.0f;
}

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

void CPolarSubCamera::execHeightPan_()
{
	bool touchesGround   = SMS_IsMarioTouchGround4cm();
	f32 trackY           = 0.0f;
	void* paramObj       = *(void**)((u8*)this + 0x68);
	if (touchesGround && paramObj) {
		trackY = *(f32*)((u8*)paramObj + 0x2C);
	}

	f32 chaseSpeed = paramObj ? *(f32*)((u8*)paramObj + 0x30) : 0.0f;
	CLBChaseGeneralConstantSpecifySpeed<f32>(
	    (f32*)((u8*)this + 0x24C), trackY, chaseSpeed);

	f32 panOff = *(f32*)((u8*)this + 0x24C);
	*(f32*)((u8*)this + 0x90) += panOff;

	f32 deltaY = *(f32*)((u8*)this + 0x9C) - *(f32*)((u8*)this + 0xD0);

	bool needsPan = false;
	if (!isNotHeightPanCamMode_() && !SMS_IsMarioTouchGround4cm()
	    && !gpCameraMario->isMarioGoDown() && !SMS_IsMarioOnWire()) {
		u32 status = SMS_GetMarioStatus();
		if (status - 0x20000000 != 0x345)
			needsPan = true;
	}

	u16& flags = *(u16*)((u8*)this + 0x64);
	if (needsPan) {
		flags |= 1;
		flags &= ~6;
		*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0xB8);

		f32 chaseRate = paramObj ? *(f32*)((u8*)paramObj + 0x34) : 0.0f;
		CLBChaseDecrease((f32*)((u8*)this + 0x84),
		                 *(f32*)((u8*)this + 0x9C), chaseRate, 0.0f);

		if (*(f32*)((u8*)this + 0xA8) != *(f32*)((u8*)this + 0xDC)) {
			*(f32*)((u8*)this + 0x84) += deltaY;
		}
	} else if ((flags & 3) != 0) {
		if (*(f32*)((u8*)this + 0xA8) != *(f32*)((u8*)this + 0xDC)) {
			*(f32*)((u8*)this + 0x84) += deltaY;
		}
		if ((flags & 1) != 0) {
			flags &= ~1;
			bool useLock = false;
			int mode     = mMode;
			if (mode == 5 || mode == 0x13) {
				void* cmd = *(void**)((u8*)this + 0x2D4);
				if (cmd) {
					s16 a = *(s16*)((u8*)this + 0x256);
					s16 b = *(s16*)((u8*)cmd + 0x54);
					if (a < b)
						useLock = true;
				}
			}
			if (useLock) {
				*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0x9C);
				*(f32*)((u8*)this + 0x14) = *(f32*)((u8*)this + 0x9C);
				flags &= ~6;
			} else {
				flags |= 2;
			}
		}

		if (*(f32*)((u8*)this + 0x84) != *(f32*)((u8*)this + 0x9C)) {
			void* cmd = *(void**)((u8*)this + 0x2D4);
			f32 a     = cmd ? *(f32*)((u8*)cmd + 0x2C) : 0.0f;
			f32 b     = cmd ? *(f32*)((u8*)cmd + 0x40) : 0.0f;
			s32 done  = CLBChaseSpecialDecrease(
                 (f32*)((u8*)this + 0x84), *(f32*)((u8*)this + 0x9C), a, b);
			if (done == 0 && !touchesGround) {
				flags &= ~6;
			}
		} else if (!touchesGround) {
			flags &= ~6;
		}
	} else {
		*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0x9C);
	}
}

void CPolarSubCamera::killHeightPanWhenChangeCamMode_()
{
	bool isOff = false;
	if (isLButtonCameraSpecifyMode(mMode) || isRailCameraSpecifyMode(mMode)) {
		isOff = true;
	} else {
		int idx = mMode - 8;
		if ((u32)idx <= 0x39 && isHeightPanModeIndex(idx)) {
			isOff = true;
		}
	}

	if (!isOff) {
		int prev = *(int*)((u8*)this + 0x54);
		if (prev == 0xF || prev == 0x3E) {
			isOff = true;
		}
	}

	if (isOff) {
		u16& flags = *(u16*)((u8*)this + 0x64);
		flags &= ~1;
		flags &= ~6;
		*(f32*)((u8*)this + 0x84)  = *(f32*)((u8*)this + 0x9C);
		*(f32*)((u8*)this + 0x24C) = 0.0f;
	}
}
