#ifndef CAMERA_SUN_MODEL_HPP
#define CAMERA_SUN_MODEL_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>

class TSunModel : public JDrama::TViewObj {
public:
	TSunModel(bool, const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

	void getZBufValue();

public:
	/* 0x10 */ u8 _10[0xAC - 0x10];
	/* 0xAC */ f32 unkAC;
	/* 0xB0 */ u8 _B0[0xB4 - 0xB0];
	/* 0xB4 */ s16 mZBufCoords[17][2];
	/* 0xF8 */ u8 _F8[0x180 - 0xF8];
	/* 0x180 */ u8 mZBufVisible[17];
	/* 0x191 */ u8 _191[0x1B0 - 0x191];
};

extern TSunModel* gpSunModel;

#endif
