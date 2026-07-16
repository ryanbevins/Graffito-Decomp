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
			warpPosAndAt(mCurrentTarget.unk28, *gpMarioAngleY + 0x9C4);
		}
		unk120->onNeutralMarioKey();
		result = false;
	} else {
		int cubeCount = gpCubeCamera->unk10;

		JGeometry::TVec3<f32> pos = SMS_GetMarioPos();
		pos.y += 75.0f;

		for (int i = 0; i < cubeCount; ++i) {
			if (gpCubeCamera->isInCube(pos, i)) {
				TCubeCameraInfo* info
				    = (TCubeCameraInfo*)&(*gpCubeCamera->unk14)[i];

				TCameraMapTool* mapTool = info->getCameraMapTool();
				if (mapTool) {
					if (mMode != mapTool->unk24 || mapTool != unk70)
						changeCamModeSpecifyCamMapTool_(mapTool);
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
