#include <Camera/CameraShake.hpp>

class TCamSaveShake;
extern TCamSaveShake* mCamShakeNameSave__12TCameraShake[];

TCameraShake::TCameraShake()
{
	// Allocate save data for each camera shake mode (41 entries)
	// Initialize TCamShakeInfo array (~8 entries)
	// TODO match exactly (~250 bytes)
}

void TCameraShake::getUseShakeData_()
{
	// TODO: returns the active shake data based on mode
}

void TCameraShake::startShake(EnumCamShakeMode mode, f32 strength)
{
	(void)mode;
	(void)strength;
	// TODO
}

void TCameraShake::keepShake(EnumCamShakeMode mode, f32 strength)
{
	(void)mode;
	(void)strength;
	// TODO
}

void TCameraShake::execShake(const JGeometry::TVec3<f32>& center,
                             JGeometry::TVec3<f32>* outPos,
                             JGeometry::TVec3<f32>* outAt)
{
	(void)center;
	(void)outPos;
	(void)outAt;
	// TODO: apply current shake transform
}
