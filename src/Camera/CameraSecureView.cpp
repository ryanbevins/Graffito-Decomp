#include <Camera/Camera.hpp>

void CPolarSubCamera::execSecureView_(s16 angle, Vec* out)
{
	(void)angle;
	(void)out;
	// TODO: secure-view target smoothing (trig-heavy, ~512 bytes)
}

void CPolarSubCamera::calcSecureViewTarget_(s16 angle, f32* outX, f32* outZ)
{
	(void)angle;
	if (outX != nullptr)
		*outX = 0.0f;
	if (outZ != nullptr)
		*outZ = 0.0f;
}
