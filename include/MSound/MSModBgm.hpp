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
	u8 getTimingForce(f32 timing)
	{
		for (u8 i = 0; i < 0x11; ++i) {
			if (timing >= scTiming[i] && timing < scTiming[i + 1]) {
				return i;
			}
		}
		return 0xff;
	}

	u8 getTiming(f32 timing)
	{
		f32 last = mLastTiming;
		for (u8 i = 0; i < 0x12; ++i) {
			f32 threshold = scTiming[i];
			if (timing > threshold && last <= threshold) {
				return i;
			}
			if (timing < threshold && last >= threshold) {
				return i;
			}
		}
		return 0xff;
	}

	/* 0x0 */ f32 mLastTiming;
};

#endif // MSMODBGM_HPP
