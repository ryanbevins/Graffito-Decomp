#include <Map/PollutionLayer.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/MarioAccess.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

f32 TPollutionLayer::mAreaMinRate         = 0.7f;
f32 TPollutionLayer::mSpreadArea          = 2000.0f;
u32 TPollutionLayer::mSpreadFrequency     = 3;
f32 TPollutionLayer::mFireArea            = 1600.0f;
u32 TPollutionLayer::mFireEffectWaitTime  = 20;
f32 TPollutionLayer::mThunderArea         = 1000.0f;
u32 TPollutionLayer::mThunderScaleRate    = 0; // UNUSED
f32 TPollutionLayer::mGlassWallArea       = 1000.0f;
u32 TPollutionLayer::mGlassWallScaleRate  = 0; // UNUSED
u32 TPollutionLayer::mGlassWallEffectTime = 120;

int TPollutionLayer::getPlaneType() const { return 0; }

int TPollutionLayer::getTexPosS(f32 x) const
{
	return unk5C.worldToTexSize(x - unk38);
}

void TPollutionLayer::changeType(u16) { }

bool TPollutionLayer::getPollutedPosNear(f32 range, JGeometry::TVec3<f32>* pos)
{
	for (int i = 0; i < 5; ++i) {
		pos->x = gpMarioPos->x
		         + (MsRandF() - 0.5f) * (mAreaMinRate + MsRandF()) * range;
		pos->z = gpMarioPos->z
		         + (MsRandF() - 0.5f) * (mAreaMinRate + MsRandF()) * range;

		if (!isInArea(pos->x, 0.0f, pos->z))
			continue;

		int s = getTexPosS(pos->x);
		int t = getTexPosS(pos->z);
		if (s < 0 || unk5C.mWidth <= s || t < 0 || unk5C.mHeight <= t)
			continue;

		pos->y = unk5C.getDepthWorld(s, t);
		if (pos->y > gpMarioPos->y)
			return false;

		if (unk54[unk5C.index(s, t)] != 0)
			return true;
	}

	return false;
}

bool TPollutionLayer::getPollutedPos(f32 range, JGeometry::TVec3<f32>* pos)
{
	for (int i = 0; i < 5; ++i) {
		pos->x = gpMarioPos->x + range * (MsRandF() - 0.5f);
		pos->y = gpMarioPos->y;
		pos->z = gpMarioPos->z + range * (MsRandF() - 0.5f);

		if (isPolluted(pos->x, pos->y, pos->z))
			return true;
	}

	return false;
}

void TPollutionLayer::changeEffectScale(const JGeometry::TVec3<f32>&, f32) { }

void TPollutionLayer::spread() { }

void TPollutionLayer::electric() { }

void TPollutionLayer::glassWall() { }

void TPollutionLayer::fire() { }

void TPollutionLayer::action() { }

bool TPollutionLayer::isInArea(f32 x, f32, f32 z) const
{
	if (x < unk38 || unk3C < x || z < unk40 || unk44 < z)
		return false;
	return true;
}
