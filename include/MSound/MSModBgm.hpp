#ifndef MSMODBGM_HPP
#define MSMODBGM_HPP

#include <dolphin/types.h>

class JAISound;

class MSModBgm {
public:
	JAISound* modBgm(u8, u8);
	void changeTempo(u8, u8);
	void loop();

	/* 0x0 */ u8 mState;
	/* 0x4 */ u32 mCounter;
};

class MSBgmXFade {
public:
	static f32 scTiming[];
	static f32 scExp[];

	void xFadeBgm(f32);
	void xFadeBgmForce(f32);
	f32 getTimingForce(f32);
	void getTiming(f32, u32*);

	/* 0x0 */ f32 mLastTiming;
};

#endif // MSMODBGM_HPP
