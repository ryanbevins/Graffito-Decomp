#include <Map/PollutionLayer.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <MSound/MSound.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/EmitterViewObj.hpp>

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

void TPollutionLayer::fire()
{
	if (getPollutedPosNear(mFireArea, &unk98[unk90])) {
		unk8C++;
		if ((int)unk8C > (int)mFireEffectWaitTime) {
			gpMSound->startSoundSet(0x3803, (Vec*)&unk98[unk90], 0, 0.0f, 0,
			                        0, 4);

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emit(0x1DC, &unk98[unk90], 2, this);
			if (emitter) {
				emitter->unk154.x = 1.5f;
				emitter->unk154.y = 1.5f;
				emitter->unk154.z = 1.5f;
				emitter->unk174.x = 1.5f;
				emitter->unk174.y = 1.5f;
				emitter->unk174.z = 1.5f;
			}

			emitter
			    = gpMarioParticleManager->emit(0x65, &unk98[unk90], 0, this);
			if (emitter) {
				emitter->unk154.x = 1.5f;
				emitter->unk154.y = 1.5f;
				emitter->unk154.z = 1.5f;
				emitter->unk174.x = 1.5f;
				emitter->unk174.y = 1.5f;
				emitter->unk174.z = 1.5f;
			}

			unk90++;
			if ((int)unk90 >= (int)unk94)
				unk90 = 0;

			unk8C = 0;
		}
	}
}

void TPollutionLayer::action() { }

bool TPollutionLayer::isInArea(f32 x, f32, f32 z) const
{
	if (x < unk38 || unk3C < x || z < unk40 || unk44 < z)
		return false;
	return true;
}
