#include <Camera/Camera.hpp>

void CPolarSubCamera::ctrlNormalOrTowerCamera_()
{
	// TODO: complex normal/tower camera control (~1200 bytes)
}

void CPolarSubCamera::calcTowerCenterPos_(Vec* out)
{
	if (out != nullptr) {
		out->x = 0.0f;
		out->y = 0.0f;
		out->z = 0.0f;
	}
}
