#include <Player/Tongue.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>

void TYoshiTongue::entry()
{
	if ((int)mState != 0) {
		mModel->entry();
		mTipModel->entry();
	}
}

void TYoshiTongue::viewCalc()
{
	if ((int)mState != 0) {
		mModel->viewCalc();
		mTipModel->viewCalc();
	}
}
