#ifndef ENEMY_DIRECTION_CALC_HPP
#define ENEMY_DIRECTION_CALC_HPP

#include <JSystem/JGeometry/JGVec3.hpp>
#include <dolphin/types.h>

// Defined in koopajr.cpp (currently a stub). Forward signatures only -
// limitkoopajr embeds two TDirectionCalc instances and references the
// member functions but does not implement them.
class TDirectionCalc {
public:
	TDirectionCalc();
	TDirectionCalc(f32);
	TDirectionCalc(JGeometry::TVec3<f32>);

	static f32 r2d(f32);
	static f32 d2r(f32);

	f32 absDirection(f32) const;
	JGeometry::TVec3<f32> calcDirectionVector() const;
	void makeDirection(JGeometry::TVec3<f32>);
	f32 calcTurnDirection(f32, f32);
	f32 calcNearerDirection(f32) const;
	f32 sub(f32) const;

public:
	/* 0x0 */ f32 mDirection;
	/* 0x4 */ f32 mLength;
};

#endif
