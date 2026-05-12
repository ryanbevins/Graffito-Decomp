#include <Camera/SunModel.hpp>

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
	gpSunModel = this;
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
