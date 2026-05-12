#include <Camera/Camera.hpp>

static const bool kNormalCameraTable[73] = {
	1, 1, 0, 1, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 0,
	1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1
};

static const bool kSlopeCameraTable[73] = {
	1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1
};

bool CPolarSubCamera::isSlopeCameraMode() const
{
	if ((u32)mMode > 0x48)
		return false;
	return kSlopeCameraTable[mMode];
}

bool CPolarSubCamera::isNormalCameraSpecifyMode(int mode) const
{
	if ((u32)mode > 0x48)
		return false;
	return kNormalCameraTable[mode];
}

bool CPolarSubCamera::isDefiniteCameraSpecifyMode(int mode) const
{
	if (mode == 9)
		return true;
	if (mode >= 0x1E && mode < 0x26)
		return true;
	if (mode == 0x3B)
		return true;
	return false;
}

bool CPolarSubCamera::isFixCameraSpecifyMode(int mode) const
{
	if (mode == 0x3A)
		return true;
	if (mode >= 0x16 && mode < 0x1E)
		return true;
	return false;
}

bool CPolarSubCamera::isRailCameraSpecifyMode(int) const { return false; }

bool CPolarSubCamera::isFollowCameraSpecifyMode(int mode) const
{
	if (mode == 0)
		return true;
	if (mode >= 0x35 && mode < 0x37)
		return true;
	if (mode >= 0x45 && mode < 0x47)
		return true;
	return false;
}

bool CPolarSubCamera::isTowerCameraSpecifyMode(int mode) const
{
	if (mode == 0x37)
		return true;
	if (mode == 0x41)
		return true;
	if (mode >= 0x27 && mode < 0x2A)
		return true;
	return false;
}

bool CPolarSubCamera::isTalkCameraSpecifyMode(int mode) const
{
	if (mode == 0x2D)
		return true;
	if (mode == 0x0C)
		return true;
	if (mode == 0x3F || mode == 0x40)
		return true;
	return false;
}

bool CPolarSubCamera::isLButtonCameraSpecifyMode(int mode) const
{
	return mode == 7;
}

bool CPolarSubCamera::isJetCoaster1stCamera() const
{
	if (mMode != 0x2E)
		return false;
	void* p = *(void**)((u8*)this + 0x2B8);
	if (p == nullptr)
		return false;
	u8 flag = *(u8*)((u8*)p + 0xC);
	return (flag & 1) != 0;
}

bool CPolarSubCamera::isOverHipAttackSpecifyMode(int mode) const
{
	if (isFixCameraSpecifyMode(mode))
		return true;
	if (isDefiniteCameraSpecifyMode(mode))
		return true;
	// Otherwise check explicit mode list
	switch (mode) {
	case 0x08: case 0x0B: case 0x0D: case 0x0E: case 0x11: case 0x14:
	case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x2F:
	case 0x33: case 0x37: case 0x38: case 0x39: case 0x41: case 0x42:
	case 0x43: case 0x47: case 0x48:
		return true;
	}
	return false;
}

bool CPolarSubCamera::isNormalCameraCompletely() const
{
	if (!isNormalCameraSpecifyMode(mMode))
		return false;
	if (!isNowInbetween())
		return true;
	u32 prevMode = *(u32*)((u8*)this + 0x54);
	if (prevMode > 0x48)
		return false;
	return kNormalCameraTable[prevMode];
}

bool CPolarSubCamera::isTalkCameraInbetween() const
{
	if (!isNowInbetween())
		return false;
	if (isTalkCameraSpecifyMode(mMode))
		return true;
	int prevMode = *(int*)((u8*)this + 0x54);
	return isTalkCameraSpecifyMode(prevMode);
}

bool CPolarSubCamera::isLButtonCameraInbetween() const
{
	if (!isNowInbetween())
		return false;
	if (mMode == 7)
		return true;
	int prevMode = *(int*)((u8*)this + 0x54);
	return prevMode == 7;
}
