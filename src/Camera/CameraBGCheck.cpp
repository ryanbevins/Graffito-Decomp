#include <Camera/Camera.hpp>

void CPolarSubCamera::execGroundCheck_(Vec p) { (void)p; }
void CPolarSubCamera::execRoofCheck_(Vec p) { (void)p; }
void CPolarSubCamera::execWallCheck_(Vec* p) { (void)p; }

bool CPolarSubCamera::isNeedWallCheck_() const { return false; }
bool CPolarSubCamera::isNeedRoofCheck_() const { return false; }
bool CPolarSubCamera::isNeedGroundCheck_() { return false; }

void CPolarSubCamera::calcInHouseNo_(bool flag) { (void)flag; }
