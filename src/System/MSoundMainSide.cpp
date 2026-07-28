#define JGADGET_TVECTOR_BEGIN_OUT_OF_LINE
#define MSL_STDSQRTF_OUT_OF_LINE

#include <System/MSoundMainSide.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSHandle.hpp>
#include <MSound/MSModBgm.hpp>
#include <MSound/MSoundSE.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <math.h>

#undef JGADGET_TVECTOR_BEGIN_OUT_OF_LINE
#undef MSL_STDSQRTF_OUT_OF_LINE

static const u32 cMSBgmNone = 0xfffffff0;

u32 MSMainProc::MSStageInfo::msStg;
u32 MSMainProc::MSStageInfo::demoBgm;
u32 MSMainProc::MSStageInfo::stageBgm;
u32 MSMainProc::MSStageInfo::stageBgmSilent;
u8 MSMainProc::MSStageInfo::stageBgmSilentStartStatus;
u8 MSMainProc::MSStageInfo::flags;
u16 MSMainProc::MSStageInfo::volOffCategory;
u8 MSMainProc::MSStageInfo::fadeEvent;
u32 MSMainProc::MSStageInfo::switchBgm;
u32 MSMainProc::MSStageInfo::switchBgm2;
f32 MSMainProc::MSStageInfo::cubeFadeRatio;
bool MSMainProc::MSStageInfo::cubeFadeUsePan;
bool MSMainProc::MSStageInfo::bossLives;
bool MSMainProc::MSStageInfo::bossLives2;
bool MSMainProc::MSStageInfo::bossNotDamaged;
bool MSMainProc::MSStageInfo::distFadeStageToKage;
MSStage* MSStage::smMSStage;

void MSStage::setPosPtr(Vec*) { }

void MSSTageSimpleEnvironment::proc()
{
	if (mSoundID != 0xffffffff)
		MSoundSESystem::MSoundSE::startSoundSystemSE(mSoundID, 0, nullptr, 0);
}

inline void MSStageCubeSwitch::toStageBgm()
{
	if (!MSMainProc::MSStageInfo::bossLives)
		return;

	MSBgm::stopTrackBGM(1, 10);
	MSBgm::setTrackVolume(0, 1.0f, 15, 0);
}

inline void MSStageCubeSwitch::toBossBgm()
{
	if (!MSMainProc::MSStageInfo::bossLives)
		return;

	MSBgm::setTrackVolume(0, 0.0f, 15, 0);
	if (!MSMainProc::MSStageInfo::bossNotDamaged)
		MSBgm::startBGM(MSMainProc::MSStageInfo::switchBgm2);
	else
		MSBgm::startBGM(MSMainProc::MSStageInfo::switchBgm);
}

void MSStageCubeSwitch::proc()
{
	Vec pos = *gpMarioPos;
	pos.y += 75.0f;
	Vec cubePos = pos;
	mCurrentCube = gpCubeSoundChange->getInCubeNo(cubePos);

	switch (mCurrentCube) {
	case -1:
		if (mPreviousCube == 0) {
			unk10 = false;
			unk11 = false;
		} else {
			bool flag;
			if (!unk10)
				flag = !SMS_IsMarioStatusTypeJumping();
			else
				flag = true;
			unk10 = flag;

			if (unk10 == true && unk11 == false) {
				toStageBgm();
			}
			unk11 = unk10;
		}
		break;
	case 0:
		if (mPreviousCube == -1) {
			unk10 = false;
			unk11 = false;
		} else {
			bool flag;
			if (!unk10)
				flag = !SMS_IsMarioStatusTypeJumping();
			else
				flag = true;
			unk10 = flag;

			if (unk10 == true && unk11 == false) {
				toBossBgm();
			}
			unk11 = unk10;
		}
		break;
	}

	mPreviousCube = mCurrentCube;
}

void MSStageCubeFadeMonte::proc()
{
	JAISound* track1 = MSBgm::getHandle(1);
	MSBgm::getHandle(0);

	if (track1 == nullptr)
		return;

	TCubeGeneralInfo** cubes = gpCubeSoundChange->unk14->begin();
	Vec pos                 = *gpMarioPos;
	pos.y                   = 75.0f + cubes[0]->unkC.y;
	mCurrentCube            = gpCubeSoundChange->getInCubeNo(pos);

	unk10 = SMS_GetMonteVillageAreaInMario();

	f32 fade = 0.0f;
	if (mCurrentCube != -1) {
		f32 ratioX = 0.0f;
		f32 ratioY = 0.0f;
		f32 ratioZ = 0.0f;
		Vec ratioPos = *gpMarioPos;
		ratioPos.y   = 75.0f + cubes[mCurrentCube]->unkC.y;
		gpCubeSoundChange->calcPointInCubeRatio(
		    ratioPos, mCurrentCube, &ratioX, &ratioY, &ratioZ);

		f32 edgeX = __fabsf(ratioX - 0.5f);
		f32 edgeZ = __fabsf(ratioZ - 0.5f);
		f32 edge  = edgeX > edgeZ ? edgeX : edgeZ;
		if (edge < mFadeRatio)
			fade = 1.0f;
		else
			fade = (0.5f - edge) / (0.5f - mFadeRatio);
	}

	if (unk10 == 0) {
		MSoundSESystem::MSoundSE::startSoundSystemSE(
		    0x5023, 0, nullptr, 0);
		if (unk14 != 0) {
			MSBgm::setTrackVolume(1, 0.0f, 10, 0);
			MSBgm::setTrackVolume(0, 1.0f, 10, 0);
		}
		((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = fade;
	} else {
		if (unk14 == 0) {
			if (fade == 1.0f) {
				MSBgm::setTrackVolume(1, 1.0f, 10, 0);
				MSBgm::setTrackVolume(0, 0.0f, 10, 0);
				((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = fade;
			} else if (fade == 0.0f) {
				MSBgm::setTrackVolume(1, 0.0f, 10, 0);
				MSBgm::setTrackVolume(0, 1.0f, 10, 0);
				((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = fade;
			} else {
				((MSBgmXFade*)gpMSound->unk9C)->xFadeBgmForce(fade);
			}
		} else {
			if (mCurrentCube == -1) {
				if (mPreviousCube != -1) {
					MSBgm::setTrackVolume(1, 0.0f, 10, 0);
					MSBgm::setTrackVolume(0, 1.0f, 10, 0);
					((MSBgmXFade*)gpMSound->unk9C)->mLastTiming
					    = 0.0f;
				}
			} else {
				((MSBgmXFade*)gpMSound->unk9C)->xFadeBgm(fade);
			}
		}

		if (mCurrentCube != -1
		    && MSMainProc::MSStageInfo::cubeFadeUsePan) {
			TCubeGeneralInfo* info = cubes[mCurrentCube];
			Vec cubePos            = info->unkC;
			Vec marioPos           = *gpMarioPos;
			cubePos.y              = marioPos.y;

			f32 dx     = cubePos.x - marioPos.x;
			f32 dy     = cubePos.y - marioPos.y;
			f32 dz     = cubePos.z - marioPos.z;
			f32 distSq = dx * dx + dy * dy + dz * dz;
			f32 dist   = distSq;
			if (distSq > 0.0f) {
				f64 root = __frsqrte(distSq);
				root = 0.5 * root * (3.0 - distSq * (root * root));
				root = 0.5 * root * (3.0 - distSq * (root * root));
				root = 0.5 * root * (3.0 - distSq * (root * root));
				dist = (f32)(distSq * root);
			}

			Vec cameraPos;
			PSMTXMultVec(gpMSound->unk8->unk8, &cubePos, &cameraPos);
			f32 pan   = MSHandle::calcPan(cameraPos, dist, 10000.0f);
			f32 dolby = MSHandle::calcDolby(cameraPos, dist);
			MSBgm::setPan(1, pan, 1, 0);
			MSBgm::setDolby(1, dolby, 1, 0);
		}
	}

	mPreviousCube = mCurrentCube;
	unk14         = unk10;
}

#pragma dont_inline on
namespace JGadget {
template <>
TVector<void*, TAllocator<void*> >::iterator
TVector<void*, TAllocator<void*> >::begin()
{
	return pBegin_;
}
} // namespace JGadget
#pragma dont_inline off

void MSStageCubeFade::proc()
{
	JAISound* track1 = MSBgm::getHandle(1);
	MSBgm::getHandle(0);

	if (track1 == nullptr)
		return;

	TCubeGeneralInfo** cubes = gpCubeSoundChange->unk14->begin();
	Vec pos                 = *gpMarioPos;
	pos.y                   = 75.0f + cubes[0]->unkC.y;
	mCurrentCube = gpCubeSoundChange->getInCubeNo(pos);

	if (mCurrentCube == -1) {
		if (mPreviousCube != -1)
			((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = 0.0f;
		mPreviousCube = mCurrentCube;
		return;
	}

	f32 ratioX = 0.0f;
	f32 ratioY = 0.0f;
	f32 ratioZ = 0.0f;
	Vec ratioPos = *gpMarioPos;
	ratioPos.y   = 75.0f + cubes[mCurrentCube]->unkC.y;
	gpCubeSoundChange->calcPointInCubeRatio(
	    ratioPos, mCurrentCube, &ratioX, &ratioY, &ratioZ);

	f32 edgeX = __fabsf(ratioX - 0.5f);
	f32 edgeZ = __fabsf(ratioZ - 0.5f);
	f32 edge  = edgeX > edgeZ ? edgeX : edgeZ;
	f32 fade;
	if (edge < mFadeRatio)
		fade = 1.0f;
	else
		fade = (0.5f - edge) / (0.5f - mFadeRatio);

	((MSBgmXFade*)gpMSound->unk9C)->xFadeBgm(fade);

	if (MSMainProc::MSStageInfo::cubeFadeUsePan) {
		TCubeGeneralInfo* info = cubes[mCurrentCube];
		Vec cubePos            = info->unkC;
		Vec marioPos           = *gpMarioPos;
		cubePos.y              = marioPos.y;

		f32 dx     = cubePos.x - marioPos.x;
		f32 dy     = cubePos.y - marioPos.y;
		f32 dz     = cubePos.z - marioPos.z;
		f32 distSq = dx * dx + dy * dy + dz * dz;
		f32 dist   = distSq;
		if (distSq > 0.0f) {
			f64 root = __frsqrte(distSq);
			root = 0.5 * root * (3.0 - distSq * (root * root));
			root = 0.5 * root * (3.0 - distSq * (root * root));
			root = 0.5 * root * (3.0 - distSq * (root * root));
			dist = (f32)(distSq * root);
		}

		Vec cameraPos;
		PSMTXMultVec(gpMSound->unk8->unk8, &cubePos, &cameraPos);
		f32 pan = MSHandle::calcPan(cameraPos, dist, 10000.0f);
		f32 dolby = MSHandle::calcDolby(cameraPos, dist);
		MSBgm::setPan(1, pan, 1, 0);
		MSBgm::setDolby(1, dolby, 1, 0);
	}

	mPreviousCube = mCurrentCube;
}

static inline f32 vecDist(const Vec& a, const Vec& b)
{
	return std::sqrtf((a.x - b.x) * (a.x - b.x)
	                  + (a.y - b.y) * (a.y - b.y)
	                  + (a.z - b.z) * (a.z - b.z));
}

void MSStageDistFadeMonte::proc()
{
	JAISound* track1 = MSBgm::getHandle(1);
	JAISound* track0 = MSBgm::getHandle(0);

	if (track1 == nullptr || track0 == nullptr)
		return;

	Vec marioPos = *gpMarioPos;
	marioPos.y += 75.0f;
	Vec marioPos2 = marioPos;
	f32 dist      = vecDist(*unk10, marioPos2);

	f32 fade = 0.0f;
	if (dist < unkC) {
		fade = 1.0f;
	} else if (dist < unk8) {
		fade = (unk8 - dist) / (unk8 - unkC);
	}

	if (unk4 < unk14)
		fade = fade * unk4 / unk14;

	unk1C = SMS_GetMonteVillageAreaInMario();
	if (unk1C == 0) {
		MSoundSESystem::MSoundSE::startSoundSystemSE(
		    0x5023, 0, nullptr, 0);
		if (unk20 != 0) {
			MSBgm::setTrackVolume(1, 0.0f, 10, 0);
			MSBgm::setTrackVolume(0, 1.0f, 10, 0);
		}
		((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = fade;
	} else {
		if (unk20 == 0) {
			if (fade == 1.0f) {
				MSBgm::setTrackVolume(1, 1.0f, 10, 0);
				MSBgm::setTrackVolume(0, 0.0f, 10, 0);
				((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = fade;
			} else if (fade == 0.0f) {
				MSBgm::setTrackVolume(1, 0.0f, 10, 0);
				MSBgm::setTrackVolume(0, 1.0f, 10, 0);
				((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = fade;
			} else {
				((MSBgmXFade*)gpMSound->unk9C)->xFadeBgmForce(fade);
			}
		} else {
			((MSBgmXFade*)gpMSound->unk9C)->xFadeBgm(fade);
		}

		Vec pos = *unk10;
		Vec cameraPos;
		PSMTXMultVec(gpMSound->unk8->unk8, &pos, &cameraPos);
		f32 pan   = MSHandle::calcPan(cameraPos, dist, 10000.0f);
		f32 dolby = MSHandle::calcDolby(cameraPos, dist);

		if (unk4 < unk14) {
			pan   = 0.5f + ((pan - 0.5f) * unk4) / unk14;
			dolby = dolby * unk4 / unk14;
		}

		MSBgm::setPan(1, pan, 1, 0);
		MSBgm::setDolby(1, dolby, 1, 0);
	}

	unk20 = unk1C;
	unk4++;
}

#pragma dont_inline on
namespace std {
float sqrtf(float x)
{
	const double _half  = .5;
	const double _three = 3.0;
	volatile float y;
	if (x > 0.0f) {
		double guess = __frsqrte((double)x);
		guess        = _half * guess * (_three - guess * guess * x);
		guess        = _half * guess * (_three - guess * guess * x);
		guess        = _half * guess * (_three - guess * guess * x);
		y            = (float)(x * guess);
		return y;
	}
	return x;
}
} // namespace std
#pragma dont_inline off

void MSStageDistFade::proc()
{
	JAISound* track1 = MSBgm::getHandle(1);
	JAISound* track0 = MSBgm::getHandle(0);

	if (track1 == nullptr || track0 == nullptr)
		return;

	f32 dist = vecDist(*unk10, *gpMarioPos);

	f32 fade = 0.0f;
	if (dist < unkC) {
		fade = 1.0f;
	} else if (dist < unk8) {
		fade = (unk8 - dist) / (unk8 - unkC);
	}

	if (unk4 <= unk14) {
		f32 volume;
		if (unk18 == true)
			volume = fade * unk4 / unk14;
		else
			volume = 1.0f + ((fade - 1.0f) * unk4) / unk14;

		MSBgm::setTrackVolume(0, 1.0f - volume, 2, 0);
		MSBgm::setTrackVolume(1, volume, 2, 0);
		((MSBgmXFade*)gpMSound->unk9C)->mLastTiming = volume;
	} else {
		((MSBgmXFade*)gpMSound->unk9C)->xFadeBgm(fade);
	}

	u32 elapsed  = unk4;
	u32 duration = unk14;
	Vec pos      = *unk10;
	Vec cameraPos;
	PSMTXMultVec(gpMSound->unk8->unk8, &pos, &cameraPos);
	Vec cameraPos2 = cameraPos;
	f32 pan        = MSHandle::calcPan(cameraPos2, dist, 10000.0f);
	f32 dolby      = MSHandle::calcDolby(cameraPos2, dist);

	if (elapsed < duration) {
		pan   = 0.5f + ((pan - 0.5f) * elapsed) / duration;
		dolby = dolby * elapsed / duration;
	}

	MSBgm::setPan(1, pan, 1, 0);
	MSBgm::setDolby(1, dolby, 1, 0);
	unk4++;
}

MSStageDistFade::MSStageDistFade(const Vec* pos, float near_dist,
                                 float far_dist, u32 bgm, bool use_pan)
    : unk4(0)
    , unk8(near_dist == 0.0f ? 2000.0f : near_dist)
    , unkC(far_dist == 0.0f ? 500.0f : far_dist)
    , unk10(pos)
    , unk14(bgm)
    , unk18(use_pan)
{
}

void MSStage::stageLoop() { proc(); }

MSStage* MSStage::init(u8 map, u8)
{
	smMSStage = nullptr;
	*(f32*)gpMSound->unk9C = 0.0f;

	switch (MSMainProc::MSStageInfo::fadeEvent) {
	case 1: {
		if (map != 8) {
			u32 bgm      = 0x104;
			f32 nearDist = 6000.0f;
			f32 farDist  = 1600.0f;

			if (map == 7) {
				nearDist = 3000.0f;
				farDist  = 800.0f;
				bgm      = 0x104;
			} else if (map == 2) {
				bgm = 0x104;
			} else if (map == 4) {
				bgm = 0x154;
			}

			smMSStage = new MSStageDistFade(
			    *(const Vec**)((u8*)gpMSound + 0xB8), nearDist, farDist, bgm,
			    MSMainProc::MSStageInfo::distFadeStageToKage);
		} else {
			smMSStage = new MSStageDistFadeMonte(
			    *(const Vec**)((u8*)gpMSound + 0xB8), 6000.0f, 1600.0f,
			    0x5A, MSMainProc::MSStageInfo::distFadeStageToKage);
		}
		break;
	}
	case 2:
		if (gpCubeSoundChange->unk10 != 0) {
			if (map == 8)
				smMSStage = new MSStageCubeFadeMonte;
			else
				smMSStage = new MSStageCubeFade;
		}
		break;
	case 3:
		if (gpCubeSoundChange->unk10 == 1)
			smMSStage = new MSStageCubeSwitch(false);
		break;
	}

	switch (map) {
	case 0x3C:
	case 0x34:
		smMSStage = new MSSTageSimpleEnvironment(0x5012);
		break;
	}

	return smMSStage;
}

void MSMainProc::setBossNotDamagedFlag(bool flag)
{
	MSStageInfo::bossNotDamaged = flag;
}

void MSMainProc::setBossLivesFlagOnlyFlag(bool flag)
{
	MSStageInfo::bossLives = flag;
}

void MSMainProc::setBossLivesFlag2(bool flag)
{
	MSBgm::stopTrackBGM(1, 10);
	MSBgm::setTrackVolume(0, 1.0f, 18, 0);
	MSStageInfo::bossLives2 = flag;
}

void MSMainProc::setBossLivesFlag(bool flag)
{
	MSBgm::stopTrackBGM(1, 10);
	MSBgm::setTrackVolume(0, 1.0f, 18, 0);
	MSStageInfo::bossLives = flag;
}

void MSMainProc::startStageBGM(u8, u8)
{
	gpMSound->initSound();
	gpMSound->unkA0 = 0;
	gpMSound->unkC8[1] = 0;

	if (MSStageInfo::stageBgm != cMSBgmNone) {
		bool shouldStart = false;
		if (MSStageInfo::flags & 2)
			shouldStart = true;

		if (shouldStart) {
			if (MSStageInfo::flags & 1) {
				if (gpMSound->unkCF != 0) {
					JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgm);
					if (sound != nullptr)
						sound->setVolume(0.0f, 0, 0);
				}
			} else {
				MSBgm::startBGM(MSStageInfo::stageBgm);
			}
		}
	}

	if (MSStageInfo::stageBgmSilent != cMSBgmNone
	    && MSStageInfo::stageBgmSilentStartStatus == 2
	    && gpMSound->unkCF != 0) {
		JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgmSilent);
		if (sound != nullptr)
			sound->setVolume(0.0f, 0, 0);
	}
}

void MSMainProc::endStageEntranceDemo(u8, u8)
{
	if (MSStageInfo::demoBgm != cMSBgmNone) {
		bool stopDemo;
		if (MSStageInfo::flags & 8)
			stopDemo = true;
		else
			stopDemo = false;

		if (stopDemo)
			MSBgm::stopBGM(MSStageInfo::demoBgm, 20);
	}

	bool silentDemo;
	if (MSStageInfo::flags & 4)
		silentDemo = true;
	else
		silentDemo = false;

	if (silentDemo)
		MSBgm::setVolume(MSStageInfo::demoBgm, 0.0f, 20, 0);

	gpMSound->demoModeOut(false);
	gpMSound->unkC8[4] = 0;
}

void MSMainProc::entranceDemoLoop(u32) { }

void MSMainProc::startStageEntranceDemo(u8, u8)
{
	if (MSStageInfo::demoBgm != cMSBgmNone)
		MSBgm::startBGM(MSStageInfo::demoBgm);

	if (MSStageInfo::stageBgmSilent != cMSBgmNone
	    && MSStageInfo::stageBgmSilentStartStatus == 1
	    && gpMSound->unkCF != 0) {
		JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgmSilent);
		if (sound != nullptr)
			sound->setVolume(0.0f, 0, 0);
	}

	gpMSound->demoModeIn(MSStageInfo::volOffCategory, false);
	gpMSound->unkA4 = 0;
	gpMSound->unkC8[1] = 1;
}

void MSMainProc::setMSoundEnterStage(u8 map, u8 area)
{
	const u32 base = 0x80010000;
	bool waterCamera = false;

	MSStageInfo::msStg                     = MSBgm::getSceneNo(cMSBgmNone);
	MSStageInfo::stageBgm                  = cMSBgmNone;
	MSStageInfo::demoBgm                   = base + 0x17;
	MSStageInfo::flags                     = 10;
	MSStageInfo::stageBgmSilent            = cMSBgmNone;
	MSStageInfo::stageBgmSilentStartStatus = 2;
	MSStageInfo::fadeEvent                 = 0;
	MSStageInfo::switchBgm                 = cMSBgmNone;
	MSStageInfo::switchBgm2                = cMSBgmNone;
	MSStageInfo::cubeFadeRatio             = 0.15f;
	MSStageInfo::cubeFadeUsePan            = true;
	MSStageInfo::bossLives                 = true;
	MSStageInfo::bossLives2                = true;
	MSStageInfo::bossNotDamaged            = true;
	MSStageInfo::volOffCategory            = 0x1C7;
	MSStageInfo::distFadeStageToKage       = true;

	switch (map) {
	case 0:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x1A);
		MSStageInfo::stageBgm = base + 0x1A;
		break;
	case 1:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x01);
		MSStageInfo::stageBgm = base + 0x01;
		switch (area) {
		case 0:
			MSStageInfo::fadeEvent     = 2;
			MSStageInfo::switchBgm     = cMSBgmNone;
			MSStageInfo::switchBgm2    = cMSBgmNone;
			MSStageInfo::cubeFadeRatio = 0.22f;
			break;
		case 1:
			MSStageInfo::demoBgm = base + 0x16;
			MSStageInfo::flags   = 0;
			MSStageInfo::volOffCategory -= 0x183;
			break;
		case 5:
			MSStageInfo::fadeEvent     = 2;
			MSStageInfo::switchBgm     = cMSBgmNone;
			MSStageInfo::switchBgm2    = cMSBgmNone;
			MSStageInfo::cubeFadeRatio = 0.34f;
			break;
		case 8:
			if (TFlagManager::smInstance->getFlag(0x60003) > 0) {
				MSStageInfo::demoBgm = base + 0x16;
				MSStageInfo::flags   = 6;
				MSStageInfo::volOffCategory -= 0x83;
				MSStageInfo::fadeEvent = 1;
				MSStageInfo::switchBgm = cMSBgmNone;
				MSStageInfo::switchBgm2 = cMSBgmNone;
			}
			break;
		case 9:
			MSStageInfo::demoBgm = base + 0x16;
			MSStageInfo::flags   = 0;
			MSStageInfo::volOffCategory -= 0x82;
			break;
		}
		break;
	case 2:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x02);
		MSStageInfo::stageBgm = base + 0x02;
		if (area == 0) {
			MSStageInfo::demoBgm       = base + 0x27;
			MSStageInfo::flags         = 10;
			MSStageInfo::fadeEvent     = 2;
			MSStageInfo::switchBgm     = cMSBgmNone;
			MSStageInfo::switchBgm2    = cMSBgmNone;
			MSStageInfo::cubeFadeRatio = 0.28f;
		} else if (area == 6) {
			MSStageInfo::demoBgm = base + 0x16;
			MSStageInfo::flags   = 3;
			MSStageInfo::volOffCategory -= 0x83;
			MSStageInfo::fadeEvent           = 1;
			MSStageInfo::switchBgm           = cMSBgmNone;
			MSStageInfo::switchBgm2          = cMSBgmNone;
			MSStageInfo::distFadeStageToKage = false;
		}
		break;
	case 3:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x09);
		MSStageInfo::stageBgm = base + 0x09;
		if (area == 6) {
			MSStageInfo::demoBgm = base + 0x16;
			MSStageInfo::flags   = 3;
			MSStageInfo::volOffCategory -= 0x83;
			MSStageInfo::fadeEvent           = 1;
			MSStageInfo::switchBgm           = cMSBgmNone;
			MSStageInfo::switchBgm2          = cMSBgmNone;
			MSStageInfo::distFadeStageToKage = false;
		}
		if (area == 4) {
			MSStageInfo::switchBgm     = base + 0x0D;
			MSStageInfo::fadeEvent     = 3;
			MSStageInfo::switchBgm2    = base + 0x2A;
			MSStageInfo::cubeFadeRatio = 0.28f;
		}
		break;
	case 4:
		if (area == 2) {
			MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x0D);
			MSStageInfo::stageBgm = base + 0x0D;
		} else {
			MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x03);
			MSStageInfo::stageBgm = base + 0x03;
		}
		if (area == 6) {
			MSStageInfo::switchBgm = cMSBgmNone;
			MSStageInfo::demoBgm   = base + 0x16;
			MSStageInfo::flags     = 3;
			MSStageInfo::volOffCategory -= 0x83;
			MSStageInfo::fadeEvent           = 1;
			MSStageInfo::switchBgm2          = cMSBgmNone;
			MSStageInfo::distFadeStageToKage = false;
		} else if (area == 2) {
			MSStageInfo::demoBgm = base + 0x0D;
			MSStageInfo::flags   = 0;
		}
		break;
	case 5:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x04);
		MSStageInfo::stageBgm = base + 0x04;
		if (area == 0) {
			MSStageInfo::flags   = 10;
			MSStageInfo::demoBgm = base + 0x27;
		} else if (area == 1) {
			MSStageInfo::volOffCategory -= 4;
		}
		break;
	case 6:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x08);
		MSStageInfo::stageBgm = base + 0x08;
		break;
	case 7:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x13);
		MSStageInfo::stageBgm = base + 0x13;
		if (area == 3) {
			MSStageInfo::fadeEvent = 1;
			MSStageInfo::switchBgm = cMSBgmNone;
			MSStageInfo::switchBgm2 = cMSBgmNone;
			MSStageInfo::stageBgm = base + 0x16;
			MSStageInfo::stageBgmSilent = base + 0x13;
			MSStageInfo::stageBgmSilentStartStatus = 2;
			MSStageInfo::distFadeStageToKage       = false;
		}
		break;
	case 8:
		if (area == 5) {
			MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x22);
			MSStageInfo::stageBgm = base + 0x22;
		} else {
			MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x07);
			MSStageInfo::stageBgm = base + 0x07;
		}
		if (area == 6) {
			MSStageInfo::switchBgm = cMSBgmNone;
			MSStageInfo::demoBgm   = base + 0x16;
			MSStageInfo::flags     = 6;
			MSStageInfo::volOffCategory -= 0x83;
			MSStageInfo::fadeEvent  = 1;
			MSStageInfo::switchBgm2 = cMSBgmNone;
		} else {
			MSStageInfo::switchBgm  = cMSBgmNone;
			MSStageInfo::fadeEvent  = 2;
			MSStageInfo::switchBgm2 = cMSBgmNone;
			switch (area) {
			case 7:
				MSStageInfo::cubeFadeUsePan = false;
				MSStageInfo::cubeFadeRatio  = 0.28f;
				break;
			default:
				MSStageInfo::cubeFadeUsePan = true;
				MSStageInfo::cubeFadeRatio  = 0.15f;
				break;
			}

			if (area == 7) {
				MSStageInfo::stageBgmSilent = base + 0x2C;
				MSStageInfo::stageBgmSilentStartStatus = 2;
			} else {
				MSStageInfo::stageBgmSilent = base + 0x18;
				MSStageInfo::stageBgmSilentStartStatus = 0;
			}
		}
		break;
	case 9:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x14);
		MSStageInfo::stageBgm = base + 0x14;
		if (area == 6) {
			MSStageInfo::demoBgm = base + 0x16;
			MSStageInfo::flags   = 3;
			MSStageInfo::volOffCategory -= 0x83;
			MSStageInfo::fadeEvent           = 1;
			MSStageInfo::switchBgm           = cMSBgmNone;
			MSStageInfo::switchBgm2          = cMSBgmNone;
			MSStageInfo::distFadeStageToKage = false;
		} else if (area == 1) {
			MSStageInfo::switchBgm  = base + 0x0B;
			MSStageInfo::fadeEvent  = 3;
			MSStageInfo::switchBgm2 = base + 0x0B;
		}
		break;
	case 13:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x05);
		MSStageInfo::stageBgm = base + 0x05;
		if (area == 4) {
			MSStageInfo::fadeEvent = 1;
			MSStageInfo::switchBgm = cMSBgmNone;
			MSStageInfo::switchBgm2 = cMSBgmNone;
			MSStageInfo::stageBgm = base + 0x16;
			MSStageInfo::stageBgmSilent = base + 0x05;
			MSStageInfo::stageBgmSilentStartStatus = 2;
			MSStageInfo::distFadeStageToKage       = false;
		} else if (area == 0) {
			MSStageInfo::fadeEvent = 1;
			MSStageInfo::switchBgm = cMSBgmNone;
			MSStageInfo::switchBgm2 = cMSBgmNone;
			MSStageInfo::stageBgm = base + 0x16;
			MSStageInfo::stageBgmSilent = base + 0x05;
			MSStageInfo::stageBgmSilentStartStatus = 2;
			MSStageInfo::distFadeStageToKage       = false;
		} else {
			MSStageInfo::stageBgmSilent = base + 0x23;
			MSStageInfo::cubeFadeRatio  = 0.12f;
			MSStageInfo::stageBgmSilentStartStatus = 2;
			MSStageInfo::fadeEvent                 = 2;
			MSStageInfo::switchBgm                 = cMSBgmNone;
			MSStageInfo::switchBgm2                = cMSBgmNone;
			MSStageInfo::cubeFadeUsePan            = true;
		}
		if (area == 7)
			MSStageInfo::stageBgm = cMSBgmNone;
		break;
	case 14:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x1E);
		MSStageInfo::stageBgm = base + 0x1E;
		break;
	case 15:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x10);
		MSStageInfo::stageBgm = base + 0x10;
		gpMSound->loadArcSeqData(MSStageInfo::stageBgm, false);
		break;
	case 16:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x06);
		MSStageInfo::stageBgm = base + 0x06;
		waterCamera           = true;
		break;
	case 20:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x01);
		MSStageInfo::stageBgm = base + 0x01;
		MSStageInfo::demoBgm  = base + 0x01;
		MSStageInfo::flags    = 0;
		break;
	case 21:
	case 22:
	case 23:
	case 24:
	case 29:
	case 33:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x21);
		MSStageInfo::stageBgm = base + 0x21;
		MSStageInfo::demoBgm  = base + 0x21;
		MSStageInfo::flags    = 0;
		break;
	case 28:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x12);
		MSStageInfo::stageBgm = base + 0x12;
		MSStageInfo::demoBgm  = base + 0x12;
		MSStageInfo::flags    = 0;
		break;
	case 30:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x12);
		MSStageInfo::stageBgm = base + 0x12;
		break;
	case 31:
	case 32:
	case 34:
	case 35:
	case 40:
	case 41:
	case 42:
	case 43:
	case 45:
	case 46:
	case 47:
	case 48:
	case 49:
	case 50:
	case 51:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x12);
		MSStageInfo::stageBgm = base + 0x12;
		MSStageInfo::demoBgm  = base + 0x12;
		MSStageInfo::flags    = 0;
		break;
	case 44:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x21);
		waterCamera           = true;
		MSStageInfo::stageBgm = base + 0x21;
		MSStageInfo::demoBgm  = base + 0x21;
		MSStageInfo::flags    = 0;
		break;
	case 52:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x15);
		MSStageInfo::stageBgm = base + 0x15;
		MSStageInfo::demoBgm  = base + 0x15;
		MSStageInfo::flags    = 0;
		break;
	case 55:
	case 57:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x0D);
		MSStageInfo::stageBgm = base + 0x0D;
		break;
	case 58:
		if (area == 0) {
			MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x1F);
			MSStageInfo::stageBgm = base + 0x1F;
		} else if (area == 1) {
			MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x19);
			MSStageInfo::stageBgm = base + 0x19;
		}
		break;
	case 59:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x09);
		MSStageInfo::stageBgm = base + 0x09;
		MSBgm::startBGM(base + 0x0D);
		MSStageInfo::fadeEvent  = 2;
		MSStageInfo::flags      = 3;
		MSStageInfo::switchBgm  = cMSBgmNone;
		MSStageInfo::switchBgm2 = cMSBgmNone;
		break;
	case 60:
		MSStageInfo::msStg    = MSBgm::getSceneNo(base + 0x2E);
		MSStageInfo::stageBgm = base + 0x2E;
		MSStageInfo::demoBgm  = base + 0x2E;
		MSStageInfo::flags    = 0;
		MSStageInfo::volOffCategory -= 0x104;
		break;
	}

	gpMSound->initSound();
	if (MSStageInfo::msStg != (u32)-1)
		gpMSound->enterStage(
		    (MS_SCENE_WAVE)MSStageInfo::msStg, map, area);

	MSSeCallBack::setWaterCameraFir(waterCamera);

	if (MSStageInfo::stageBgmSilent != cMSBgmNone
	    && MSStageInfo::stageBgmSilentStartStatus == 0
	    && gpMSound->unkCF != 0) {
		JAISound* sound = MSBgm::startBGM(MSStageInfo::stageBgmSilent);
		if (sound != nullptr)
			sound->setVolume(0.0f, 0, 0);
	}

	gpMSound->unkC8[0] = 1;
}

void MSMainProc::fromTalkingCameraDemo(bool) { gpMSound->talkModeIn(false); }

void MSMainProc::toTalkingCameraDemo()
{
	u16 category = 0;
	switch (gpMarDirector->mMap) {
	case 9:
		category = 4;
		break;
	case 4:
		category = 4;
		break;
	case 1:
		category = 4;
		break;
	}
	gpMSound->setCategoryVOLs(category, 1.0f);
}

void MSMainProc::fromInnerCameraDemo() { gpMSound->unkC8[2] = 0; }
void MSMainProc::toInnerCameraDemo() { gpMSound->unkC8[2] = 1; }

u32 MSMainProc::getMonteVillageActorArea(const Vec& pos)
{
	u32 area = 4;
	if (MSGMSound->unkCD == 8) {
		Vec checkPos = pos;
		checkPos.y += 75.0f;
		Vec cubePos = checkPos;
		switch (gpCubeFastC->getInCubeNo(cubePos)) {
		case 0:
			area = 2;
			break;
		case 1:
			area = 0;
			break;
		default:
			area = 3;
			break;
		}
	}
	return area;
}
