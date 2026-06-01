#include <GC2D/BlendPane.hpp>
#include <JSystem/J2D/J2DPicture.hpp>

TBlendPane::TBlendPane(J2DScreen* screen, u32 id)
    : TBoundPane(screen, id)
{
	mActive  = false;
	mStep    = 0.0f;
	mCurrent = 0.0f;
}

bool TBlendPane::update()
{
	bool baseRet = TBoundPane::update();
	if (mActive) {
		if (mCurrent >= 1.0f) {
			mCurrent = 1.0f;
			mActive  = false;
		}
		f32 inv         = 1.0f - mCurrent;
		f32 cur         = mCurrent;
		J2DPicture* pic = (J2DPicture*)unk0;
		pic->setBlendKonstColor(cur, inv, 1.0f, 1.0f);
		pic->setBlendKonstAlpha(cur, inv, 1.0f, 1.0f);
		mCurrent += mStep;
	}
	return (baseRet && !mActive) ? true : false;
}
