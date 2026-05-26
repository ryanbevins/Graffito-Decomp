#include <Camera/SunModel.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <System/Resolution.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/gx.h>

template <class S, class F>
void CLBScreenFPosToSPos(JGeometry::TVec2<S>* dst, const JGeometry::TVec2<F>& src)
{
	if (src.x < -1.0f || src.x > 1.0f) {
		dst->x = -1;
	} else {
		s32 w  = (s32)SMSGetGameRenderWidth();
		dst->x = (S)CLBRoundf<S>(0.5f * (1.0f + src.x) * (f32)(w - 1));
	}
	if (src.y < -1.0f || src.y > 1.0f) {
		dst->y = -1;
	} else {
		s32 h  = (s32)SMSGetGameRenderHeight();
		dst->y = (S)CLBRoundf<S>(-0.5f * (src.y - 1.0f) * (f32)(h - 1));
	}
}

template void CLBScreenFPosToSPos<s16, f32>(JGeometry::TVec2<s16>*,
                                            const JGeometry::TVec2<f32>&);


TSunModel* gpSunModel;

TSunModel::TSunModel(bool sunset, const char* name)
    : JDrama::TViewObj(name)
{
	(void)sunset;
	for (u32 i = 0; i < sizeof(_10); i++) {
		_10[i] = 0;
	}
	unkAC = 0.0f;
	for (u32 i = 0; i < sizeof(_B0); i++) {
		_B0[i] = 0;
	}
	for (int i = 0; i < 17; i++) {
		mZBufCoords[i][0] = -1;
		mZBufCoords[i][1] = -1;
		mZBufVisible[i]   = 0;
	}
	unkF8 = 0.0f;
	unkFC = 0.0f;
	for (u32 i = 0; i < sizeof(_100); i++) {
		_100[i] = 0;
	}
	for (u32 i = 0; i < sizeof(_191); i++) {
		_191[i] = 0;
	}
	gpSunModel = this;
}

void TSunModel::getZBufValue()
{
	bool indoor = gpCameraMario->isMarioIndoor();
	for (int i = 0; i < 17; i++) {
		mZBufVisible[i] = 0;
		if (indoor)
			continue;
		s16 x = mZBufCoords[i][0];
		if (x == -1)
			continue;
		s16 y = mZBufCoords[i][1];
		if (y == -1)
			continue;
		u32 zVal;
		GXPeekZ(x, y, &zVal);
		if ((zVal - 0xFF000000) == 0xFFFF) {
			mZBufVisible[i] = 1;
		}
	}
}

void TSunModel::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
	// TODO: load sun model params
}

void TSunModel::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
	// TODO: draw sun
}
