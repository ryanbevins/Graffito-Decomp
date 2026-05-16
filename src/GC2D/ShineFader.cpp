#include <GC2D/ScrnFader.hpp>

void TShineFader::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
}

BOOL TShineFader::registFadeout(u16 duration, u16 delay)
{
	switch (mFadeStatus) {
	case FADE_STATUS_FULLY_FADED_IN:
		mFadeStatus = FADE_STATUS_FADING_OUT;
		unk12 = 0;
		unk10 = duration;
		mFadeoutDelay = delay;
		return TRUE;
	default:
		return FALSE;
	}
}

void TShineFader::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x1)
		update();

	if (flags & 0x8)
		draw(graphics->mViewportRect);
}

void TShineFader::update()
{
	switch (mFadeStatus) {
	case FADE_STATUS_FULLY_FADED_IN:
		if (mFadeoutDelay != 0) {
			mFadeoutDelay--;
			if (mFadeoutDelay == 0) {
				mFadeStatus = FADE_STATUS_FADING_OUT;
				unk12 = 0;
			}
		}
		break;
	case FADE_STATUS_FULLY_FADED_OUT:
		break;
	case FADE_STATUS_FADING_IN:
		unk12++;
		if (unk12 > unk10)
			mFadeStatus = FADE_STATUS_FULLY_FADED_IN;
		break;
	case FADE_STATUS_FADING_OUT:
		unk12++;
		if (unk12 > unk10)
			mFadeStatus = FADE_STATUS_FULLY_FADED_OUT;
		break;
	}
}
