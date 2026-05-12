#ifndef CAMERA_SUN_MODEL_HPP
#define CAMERA_SUN_MODEL_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>

class TSunModel : public JDrama::TViewObj {
public:
	TSunModel(bool, const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

public:
	/* 0x10 */ u8 _10[0xAC - 0x10];
	/* 0xAC */ f32 unkAC;
	/* 0xB0 */ u8 _B0[0x1B0 - 0xB0];
};

extern TSunModel* gpSunModel;

#endif
