#include <Camera/Camera.hpp>
#include <Camera/CameraMapTool.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/CubeMapTool.hpp>
#include <Player/MarioAccess.hpp>
#include <System/MarioGamePad.hpp>

bool CPolarSubCamera::controlByCameraCode_(int* outCode)
{
	bool result = true;
	*outCode    = -1;

	if (SMS_IsMarioOpeningDoor()) {
		if (mMode == 0x42 && gpCameraMario->mStatusTimer == 0x78) {
			changeCamModeSpecifyFrame_(0x14, 1);
			warpPosAndAt(unkA8, (s16)(*gpMarioAngleY + 0x9C4));
		}
		unk120->onNeutralMarioKey();
		result = false;
	} else {
		s32 cubeCount  = gpCubeCamera->unk10;
		Vec pos = *gpMarioPos;
		pos.y += 75.0f;

		for (s32 i = 0; i < cubeCount; i++) {
			if (gpCubeCamera->isInCube(pos, i)) {
				TCameraMapTool* mapTool
				    = (TCameraMapTool*)((TCubeCameraInfo*)
				                            gpCubeCamera->unk14->begin()[i])
				          ->unk38;
				if (mapTool != nullptr) {
					if (mMode != mapTool->unk24 || unk70 != mapTool) {
						changeCamModeSpecifyCamMapTool_(mapTool);
					}
					*outCode = mapTool->unk24;
				} else {
					*outCode = gpCubeCamera->getDataNo(i);
				}
				return true;
			}
		}
	}

	return result;
}
