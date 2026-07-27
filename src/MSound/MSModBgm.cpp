#include <MSound/MSModBgm.hpp>
#include <MSound/MSSetSound.hpp>
#include <JSystem/JAudio/JAInterface/JAISound.hpp>
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>
#include <MSound/MSoundBGM.hpp>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

void MSBgmXFade::xFadeBgmForce(f32 timing)
{
	u8 idx = getTimingForce(timing);
	if (idx != 0xff) {
		MSBgm::setTrackVolume(0, scExp[0x11 - idx], 0, 0);
		MSBgm::setTrackVolume(1, scExp[idx], 0, 0);
		mLastTiming = timing;
	}
}

void MSBgmXFade::xFadeBgm(f32 timing)
{
	u8 idx = getTiming(timing);
	bool inRange = idx >= 1 && idx <= 0x10;
	if (inRange) {
		MSBgm::setTrackVolume(0, scExp[0x11 - idx], 2, 0);
		MSBgm::setTrackVolume(1, scExp[idx], 2, 0);
	}
	mLastTiming = timing;
}

f32 MSBgmXFade::scTiming[18] = {
	0.052632,   0.105263,   0.157895, 0.210526,   0.26315799, 0.315789,
	0.36842099, 0.42105299, 0.473684, 0.52631599, 0.578947,   0.63157898,
	0.684211,   0.73684198, 0.789474, 0.84210497, 0.894737,   0.947368,
};
f32 MSBgmXFade::scExp[18] = {
	0.0,      0.0,      0.030207,   0.063591, 0.100485, 0.14126,
	0.186324, 0.236126, 0.29116699, 0.351996, 0.419223, 0.49351999,
	0.57563,  0.666377, 0.766667,   0.877505, 1.0,      1.0,
};

void MSModBgm::changeTempo(u8 kind, u8 bgmId)
{
	JAISound* handle = MSBgm::getHandle(bgmId);
	if (handle) {
		f32 tempo = 1.0f;
		u32 frames = 5;
		switch (kind) {
		case 0:
			tempo = 1.07894f;
			break;
		case 1:
			tempo = 1.15789f;
			break;
		case 2:
			tempo  = 1.2f;
			frames = 0x14;
			break;
		}
		handle->setTempoProportion(tempo, frames);
	}
}

void MSModBgm::loop()
{
	switch (mState) {
	case 1:
		mCounter++;
		break;
	case 0:
	default:
		mCounter = 0;
		return;
	}
	mState = 0;
}

JAISound* MSModBgm::modBgm(u8 param1, u8 bgmId)
{
	switch (param1) {
	case 0:
	case 1:
		mState = 1;
		break;
	}
	JAISound* handle = MSBgm::getHandle(bgmId);
	if (!handle) {
		mState = 0;
	} else {
		switch (mCounter) {
		case 0:
			handle->setTempoProportion(1.3f, 0xA);
			handle->setPitch(1.3f, 0xA, 0);
			break;
		case 5:
			handle->setTempoProportion(0.3f, 0xB4);
			handle->setPitch(0.2f, 0xB4, 0);
			break;
		case 0xB4:
			handle->stop(1);
			handle = nullptr;
			mState = 0;
			break;
		}
	}
	return handle;
}
