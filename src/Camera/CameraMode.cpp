#include <Camera/Camera.hpp>

#pragma dont_inline on

bool CPolarSubCamera::isLButtonCameraSpecifyMode(int mode) const
{
	bool result = false;
	if (mode == 7)
		result = true;
	return result;
}

bool CPolarSubCamera::isLButtonCameraInbetween() const
{
	bool result = false;
	if (isNowInbetween()) {
		bool found = false;
		if (mMode == 7)
			found = true;

		if (!found) {
			int prevMode = *(int*)((u8*)this + 0x54);
			found = false;
			if (prevMode == 7)
				found = true;

		}
		if (found)
			result = true;
	}
	return result;
}

bool CPolarSubCamera::isJetCoaster1stCamera() const
{
	bool result = false;
	if (mMode == 0x2E) {
		void* p = *(void**)((u8*)this + 0x2B8);
		if (p != nullptr) {
			u8 flag = *(u8*)((u8*)p + 0xC);
			if (flag & 1)
				result = true;
		}
	}
	return result;
}

bool CPolarSubCamera::isTalkCameraSpecifyMode(int mode) const
{
	bool result = false;
	switch (mode) {
	case 0x0A:
	case 0x0C:
	case 0x2D:
	case 0x3F:
	case 0x40:
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isTalkCameraInbetween() const
{
	bool result = false;
	if (isNowInbetween()) {
		bool found = false;
		switch (mMode) {
		case 0x0A:
		case 0x0C:
		case 0x2D:
		case 0x3F:
		case 0x40:
			found = true;
		}

		if (!found) {
			int prevMode = *(int*)((u8*)this + 0x54);
			found = false;
			switch (prevMode) {
			case 0x0A:
			case 0x0C:
			case 0x2D:
			case 0x3F:
			case 0x40:
				found = true;
			}

		}
		if (found)
			result = true;
	}
	return result;
}

bool CPolarSubCamera::isNormalCameraSpecifyMode(int mode) const
{
	bool result = false;
	switch (mode) {
	case 0x00: case 0x01: case 0x03: case 0x04: case 0x05: case 0x06:
	case 0x08: case 0x0B: case 0x0D: case 0x0E: case 0x0F: case 0x10:
	case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x26:
	case 0x2A: case 0x2B: case 0x2C: case 0x2F: case 0x30: case 0x31:
	case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x38:
	case 0x39: case 0x3C: case 0x3D: case 0x3E: case 0x42: case 0x43:
	case 0x44: case 0x45: case 0x46: case 0x47: case 0x48:
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isNormalCameraCompletely() const
{
	bool result = false;
	if (isNormalCameraSpecifyMode(mMode)) {
		if (isNowInbetween()) {
			int prevMode = *(int*)((u8*)this + 0x54);
			bool found = false;
			switch (prevMode) {
			case 0x00: case 0x01: case 0x03: case 0x04: case 0x05: case 0x06:
			case 0x08: case 0x0B: case 0x0D: case 0x0E: case 0x0F: case 0x10:
			case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x26:
			case 0x2A: case 0x2B: case 0x2C: case 0x2F: case 0x30: case 0x31:
			case 0x32: case 0x33: case 0x34: case 0x35: case 0x36: case 0x38:
			case 0x39: case 0x3C: case 0x3D: case 0x3E: case 0x42: case 0x43:
			case 0x44: case 0x45: case 0x46: case 0x47: case 0x48:
				found = true;
			}
			if (found)
				result = true;
		} else {
			result = true;
		}
	}
	return result;
}

bool CPolarSubCamera::isTowerCameraSpecifyMode(int mode) const
{
	bool result = false;
	switch (mode) {
	case 0x27:
	case 0x28:
	case 0x29:
	case 0x37:
	case 0x41:
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isFollowCameraSpecifyMode(int mode) const
{
	bool result = false;
	switch (mode) {
	case 0x00:
	case 0x35:
	case 0x36:
	case 0x45:
	case 0x46:
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isRailCameraSpecifyMode(int) const { return false; }

bool CPolarSubCamera::isFixCameraSpecifyMode(int mode) const
{
	bool result = false;
	switch (mode) {
	case 0x16:
	case 0x17:
	case 0x18:
	case 0x19:
	case 0x1A:
	case 0x1B:
	case 0x1C:
	case 0x1D:
	case 0x3A:
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isDefiniteCameraSpecifyMode(int mode) const
{
	bool result = false;
	switch (mode) {
	case 0x09:
	case 0x1E:
	case 0x1F:
	case 0x20:
	case 0x21:
	case 0x22:
	case 0x23:
	case 0x24:
	case 0x25:
	case 0x3B:
		result = true;
	}
	return result;
}

bool CPolarSubCamera::isOverHipAttackSpecifyMode(int mode) const
{
	bool helper_match = true;
	bool result = false;
	if (!isFixCameraSpecifyMode(mode)) {
		if (!isDefiniteCameraSpecifyMode(mode))
			helper_match = false;
	}
	if (helper_match) {
		result = true;
	} else {
		switch (mode) {
		case 0x08: case 0x0B: case 0x0D: case 0x0E: case 0x11: case 0x14:
		case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x2F:
		case 0x33: case 0x37: case 0x38: case 0x39: case 0x41: case 0x42:
		case 0x43: case 0x47: case 0x48:
			result = true;
		}
	}
	return result;
}

bool CPolarSubCamera::isSlopeCameraMode() const
{
	bool result = false;
	switch (mMode) {
	case 0x00: case 0x01: case 0x04: case 0x0B: case 0x0E: case 0x14:
	case 0x26: case 0x27: case 0x28: case 0x29: case 0x2A: case 0x2C:
	case 0x2F: case 0x35: case 0x36: case 0x37: case 0x38: case 0x41:
	case 0x42: case 0x45: case 0x46: case 0x47: case 0x48:
		result = true;
	}
	return result;
}

const char* CPolarSubCamera::mCamKindNameSaveFile[0x49] = {
	"/Camera/kindFollow.prm",
	"/Camera/kindParallel.prm",
	"/Camera/kindMultiPlayer.prm",
	"/Camera/kindWallJump.prm",
	"/Camera/kindHipAttack.prm",
	"/Camera/kindRocketJump.prm",
	"/Camera/kindWire.prm",
	"/Camera/kindLNormal.prm",
	"/Camera/kindMareUnderGround.prm",
	"/Camera/kindDefiniteD2.prm",
	"/Camera/kindTalkE.prm",
	"/Camera/kindLeanMirror.prm",
	"/Camera/kindTalkA.prm",
	"/Camera/kindUnderGround.prm",
	"/Camera/kindIndoor.prm",
	"/Camera/kindHang.prm",
	"/Camera/kindWireHang.prm",
	"/Camera/kindSandBird.prm",
	"/Camera/kindHovering.prm",
	"/Camera/kindJumpCode.prm",
	"/Camera/kindDelfino.prm",
	"/Camera/kindClimb.prm",
	"/Camera/kindFixA.prm",
	"/Camera/kindFixB.prm",
	"/Camera/kindFixC.prm",
	"/Camera/kindFixD.prm",
	"/Camera/kindFixE.prm",
	"/Camera/kindFixF.prm",
	"/Camera/kindFixG.prm",
	"/Camera/kindFixH.prm",
	"/Camera/kindDefiniteA.prm",
	"/Camera/kindDefiniteB.prm",
	"/Camera/kindDefiniteC.prm",
	"/Camera/kindDefiniteD.prm",
	"/Camera/kindDefiniteE.prm",
	"/Camera/kindDefiniteF.prm",
	"/Camera/kindDefiniteG.prm",
	"/Camera/kindDefiniteH.prm",
	"/Camera/kindExMap0.prm",
	"/Camera/kindTowerA.prm",
	"/Camera/kindTowerB.prm",
	"/Camera/kindTowerC.prm",
	"/Camera/kindSlider.prm",
	"/Camera/kindDiving.prm",
	"/Camera/kindTurbo.prm",
	"/Camera/kindTalkB.prm",
	"/Camera/kindJetCoaster.prm",
	"/Camera/kindParallelB.prm",
	"/Camera/kindSurfing.prm",
	"/Camera/kindSwimming.prm",
	"/Camera/kindClimbJump.prm",
	"/Camera/kindLookDown.prm",
	"/Camera/kindRailFence.prm",
	"/Camera/kindFollowB.prm",
	"/Camera/kindFollowC.prm",
	"/Camera/kindTowerD.prm",
	"/Camera/kindDelfinoAttic.prm",
	"/Camera/kindBossGeso.prm",
	"/Camera/kindFixI.prm",
	"/Camera/kindDefiniteI.prm",
	"/Camera/kindFence.prm",
	"/Camera/kindMonteFence.prm",
	"/Camera/kindMonteHang.prm",
	"/Camera/kindTalkC.prm",
	"/Camera/kindTalkD.prm",
	"/Camera/kindTowerE.prm",
	"/Camera/kindDelfinoB.prm",
	"/Camera/kindCancan.prm",
	"/Camera/kindAquaticTurbo.prm",
	"/Camera/kindFollowD.prm",
	"/Camera/kindFollowE.prm",
	"/Camera/kindParallelC.prm",
	"/Camera/kindParallelD.prm",
};
