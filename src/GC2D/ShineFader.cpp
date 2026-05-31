#include <GC2D/ScrnFader.hpp>

void TShineFader::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
}

BOOL TShineFader::registFadeout(u16 duration, u16 delay)
{
	BOOL result = FALSE;

	if (duration == 0)
		duration = 1;

	switch (mFadeStatus) {
	case FADE_STATUS_FULLY_FADED_IN:
		mFadeStatus = FADE_STATUS_FADING_OUT;
		unk12 = 0;
		unk10 = duration;
		mFadeoutDelay = delay;
		result = TRUE;
		break;
	}
	return result;
}

void TShineFader::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x1)
		update();

	if (flags & 0x8) {
		TShineFader* fader = this;
		fader->draw(graphics->mViewportRect);
	}
}

void TShineFader::update()
{
	switch (mFadeStatus) {
	case FADE_STATUS_FULLY_FADED_IN:
		mFadeColor.a = 0;
		break;
	case FADE_STATUS_FULLY_FADED_OUT:
		mFadeColor.a = 0xff;
		break;
	case FADE_STATUS_FADING_IN:
		break;
	case FADE_STATUS_FADING_OUT: {
		unk12++;
		if (unk12 > mFadeoutDelay + unk10) {
			mFadeStatus = FADE_STATUS_FULLY_FADED_OUT;
			unk1C = false;
			return;
		}
		if (unk12 <= mFadeoutDelay)
			return;

		mFadeColor.a = ((u16)(unk12 - 1 - mFadeoutDelay) * 0xff) / unk10;
		break;
	}
	}
}
