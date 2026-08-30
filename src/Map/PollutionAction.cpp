#include <Map/PollutionLayer.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/EmitterViewObj.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

class JAISound;
class MSound {
public:
	JAISound* startSoundSet(u32, const Vec*, u32, f32, u32, u32, u8);
};
extern MSound* gpMSound;

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

bool TPollutionLayer::getPollutedPosNear(f32 range, JGeometry::TVec3<f32>* pos)
{
	TPollutionPos* pollutionPos = &unk5C;

	for (int i = 0; i < 5; ++i) {
		f32 xOffset = (MsRandF() - 0.5f) * (mAreaMinRate + MsRandF());
		pos->x      = gpMarioPos->x + xOffset * range;
		f32 zOffset = (MsRandF() - 0.5f) * (mAreaMinRate + MsRandF());
		pos->z      = gpMarioPos->z + zOffset * range;

		if (!isInArea(pos->x, 0.0f, pos->z))
			continue;

		int s = getTexPosS(pos->x);
		int t = getTexPosS(pos->z);

		bool inBounds;
		if (s < 0 || pollutionPos->mWidth <= s || t < 0
		    || pollutionPos->mHeight <= t)
			inBounds = false;
		else
			inBounds = true;

		if (!inBounds)
			continue;

		pos->y = pollutionPos->getDepthWorld(s, t);
		if (pos->y > gpMarioPos->y)
			return false;

		if (unk54[unk5C.index(s, t)] != 0)
			return true;
	}

	return false;
}

#pragma dont_inline on

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

#pragma dont_inline off

void TPollutionLayer::fire()
{
	if (getPollutedPosNear(mFireArea, &unk98[unk90])) {
		unk8C++;
		if ((int)unk8C > (int)mFireEffectWaitTime) {
			gpMSound->startSoundSet(0x3803, (Vec*)&unk98[unk90], 0, 0.0f, 0,
			                        0, 4);
			JGeometry::TVec3<f32> scale(1.5f, 1.5f, 1.5f);

			JPABaseEmitter* emitter
			    = gpMarioParticleManager->emit(0x1DC, &unk98[unk90], 2, this);
			if (emitter)
				emitter->setScale(scale);

			emitter
			    = gpMarioParticleManager->emit(0x65, &unk98[unk90], 0, this);
			if (emitter)
				emitter->setScale(scale);

			unk90++;
			if ((int)unk90 >= (int)unk94)
				unk90 = 0;

			unk8C = 0;
		}
	}
}

void TPollutionLayer::action()
{
	if (getPlaneType() != 0)
		return;

	switch (unk30) {
	case 1:
		fire();
		break;
	case 4:
		if (getPollutedPosNear(mThunderArea, &unk98[unk90])) {
			unk8C++;
			if ((int)unk8C > 15) {
				gpMSound->startSoundSet(0x3805, (Vec*)&unk98[unk90], 0,
				                        0.0f, 0, 0, 4);
				gpMarioParticleManager->emit(0x6F, &unk98[unk90], 0, this);

				unk90++;
				if ((int)unk90 >= (int)unk94)
					unk90 = 0;

				unk8C = 0;
			}
		}
		break;
	case 3: {
		JGeometry::TVec3<f32> pos;
		if (getPollutedPos(mGlassWallArea, &pos)) {
			static int counter = 0;
			if ((int)counter < (int)mGlassWallEffectTime)
				counter++;
			else
				counter = 0;
		}
		break;
	}
	case 0:
	case 2:
	case 5:
	case 6:
	case 7:
	case 8:
	default:
		break;
	}

	bool found;
	int i = 0;
	JGeometry::TVec3<f32>* pos = &unk98[unk90];
	f32 spreadArea              = mSpreadArea;
	while (true) {
		pos->x = gpMarioPos->x + spreadArea * (MsRandF() - 0.5f);
		pos->y = gpMarioPos->y;
		pos->z = gpMarioPos->z + spreadArea * (MsRandF() - 0.5f);

		if (isPolluted(pos->x, pos->y, pos->z)) {
			found = true;
			break;
		}

		i++;
		if (i >= 5) {
			found = false;
			break;
		}
	}

	if (!found)
		return;

	if ((int)unk30 != 1 && (int)unk30 != 7) {
		if ((int)unk8C > 15) {
			gpMarioParticleManager->emitWithRotate(
			    0x1DA, &unk98[unk90], 0,
			    (s16)((f32)*gpMarioAngleY * 0.005493164f), 0, 2, nullptr);

			unk90++;
			if ((int)unk90 >= (int)unk94)
				unk90 = 0;

			unk8C = 0;
		}
		unk8C++;
	}

	if (unk32 & 1) {
		if ((int)unk4C < (int)mSpreadFrequency) {
			unk4C++;
		} else {
			unk4C = 0;
			JGeometry::TVec3<f32> spreadPos;
			if (getPollutedPosNear(mSpreadArea, &spreadPos)) {
				f32 z = spreadPos.z;
				f32 y = spreadPos.y;
				f32 x = spreadPos.x;
				gpPollution->stamp(1, x, y, z, 128.0f);
			}
		}
	}
}

bool TPollutionLayer::isInArea(f32 x, f32, f32 z) const
{
	if (x < unk38 || unk3C < x || z < unk40 || unk44 < z)
		return false;
	return true;
}
