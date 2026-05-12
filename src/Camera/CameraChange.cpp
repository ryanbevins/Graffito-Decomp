#include <Camera/Camera.hpp>
#include <Camera/CameraMapTool.hpp>

void CPolarSubCamera::execCameraModeChangeProc_(int mode) { (void)mode; }

bool CPolarSubCamera::isChangeToParallelCameraCByMoveBG_() const { return false; }
bool CPolarSubCamera::isChangeToParallelCameraByMoveBG_() const { return false; }
bool CPolarSubCamera::isChangeToCancanCamera_() const { return false; }
bool CPolarSubCamera::isChangeToBossGesoCamera_() const { return false; }

void CPolarSubCamera::doLButtonCameraOff_(bool flag) { (void)flag; }
void CPolarSubCamera::doLButtonCameraOn_() { }
void CPolarSubCamera::execFrontRotate_() { }

void CPolarSubCamera::changeCamModeSpecifyCamMapToolAndFrame_(
    const TCameraMapTool* tool, int frame)
{
	(void)tool;
	(void)frame;
}

void CPolarSubCamera::changeCamModeSpecifyCamMapTool_(const TCameraMapTool* tool)
{
	(void)tool;
}

void CPolarSubCamera::changeCamModeSpecifyFrame_(int mode, int frame)
{
	(void)mode;
	(void)frame;
}

void CPolarSubCamera::changeCamModeSub_(int newMode, int frame, bool flag)
{
	(void)newMode;
	(void)frame;
	(void)flag;
}

void CPolarSubCamera::setUpFromLButtonCamera_() { }
void CPolarSubCamera::setUpToLButtonCamera_(int mode) { (void)mode; }

int CPolarSubCamera::getCameraInbetweenFrame_(int mode)
{
	(void)mode;
	return 0;
}

void CPolarSubCamera::getLButtonCameraModeByNozzle_() { }
