#include <Camera/Camera.hpp>
#include <JSystem/JDrama/JDRActor.hpp>

bool CPolarSubCamera::isNormalDeadDemo() const { return false; }
bool CPolarSubCamera::isHellDeadDemo() const { return false; }

void CPolarSubCamera::execDeadDemoProc_() { }
void CPolarSubCamera::ctrlNormalDeadDemo_() { }

int CPolarSubCamera::getRestDemoFrames() const { return 0; }
bool CPolarSubCamera::isSimpleDemoCamera() const { return false; }

void CPolarSubCamera::endDemoCamera() { }

void CPolarSubCamera::startDemoCamera(const char* name,
                                       const JGeometry::TVec3<f32>* offset,
                                       s32 mode, f32 strength, bool flag)
{
	(void)name;
	(void)offset;
	(void)mode;
	(void)strength;
	(void)flag;
}

void CPolarSubCamera::startGateDemoCamera(const JDrama::TActor* actor)
{
	(void)actor;
}

void CPolarSubCamera::updateGateDemoCamera_() { }

void CPolarSubCamera::updateDemoCamera_(bool flag) { (void)flag; }
