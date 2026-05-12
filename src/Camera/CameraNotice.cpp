#include <Camera/Camera.hpp>

void CPolarSubCamera::ctrlLButtonCamera_() { }

void CPolarSubCamera::getNozzleTopPos_(JGeometry::TVec3<f32>* out) const
{
	if (out != nullptr) {
		out->x = 0.0f;
		out->y = 0.0f;
		out->z = 0.0f;
	}
}

void CPolarSubCamera::calcNoticeTargetYrot_(const Vec& target)
{
	(void)target;
}

void CPolarSubCamera::execNoticeOnOffProc_(EnumNoticeOnOffMode mode)
{
	(void)mode;
}

void CPolarSubCamera::getNoticeActor_() { }

void CPolarSubCamera::setNoticeInfo() { }
