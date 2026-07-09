#ifndef MSOUND_HPP
#define MSOUND_HPP

#include <dolphin/types.h>
#include <dolphin/mtx.h>

#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JAudio/JASystem/JASTrackMgr.hpp>
#include <JSystem/JAudio/JASystem/JASWaveArcLoader.hpp>
#include <JSystem/JAudio/JAInterface/JAISound.hpp>
#include <JSystem/JAudio/JAInterface/JAIBasic.hpp>
#include <MSound/MSoundSE.hpp>

class JAIActor;
class JAICamera;
class JAIAnimeFrameSoundData;
class MSBgmXFade;
class MSModBgm;

enum MS_SCENE_WAVE {
	MS_WAVE_UNK0         = 0,
	MS_WAVE_UNK128       = 128,
	MS_WAVE_DEFAULT      = 256,
	MS_WAVE_DOLPIC       = 513,
	MS_WAVE_BIANCO       = 514,
	MS_WAVE_MANMA        = 515,
	MS_WAVE_PINNAPACO_S  = 516,
	MS_WAVE_PINNAPACO    = 516,
	MS_WAVE_MARE_SEA     = 517,
	MS_WAVE_MONTEVILLAGE = 518,
	MS_WAVE_SHILENA      = 519,
	MS_WAVE_RICO         = 520,
	MS_WAVE_CLEAR        = 521,
	MS_WAVE_UNK528       = 528,
};

class MSSeCallBack {
public:
	static u16 setParameterSeqSync(JASystem::TTrack*, u16);
	static void setWaterFilter(u16);
	static void setWaterCameraFir(bool);
	static u16 smTrackCategory[32];
	static u8 smPolifonic[16];
	static u16 smWaterFilter;
};

class MSLoadWave {
public:
	static bool loadWaveBackword(int, int);
	static bool loadWaveBackword(JASystem::WaveArcLoader::TObject*);
};

class MSMarioPosVolume {
public:
	static f32 getDistFromMario(const Vec&);
};

class MSound : public JAIBasic {
public:
	MSound(JKRHeap*, JKRHeap*, u32, u8*, u8*, u32);
	~MSound() { }

	virtual JAISound* makeSound(u32);
	virtual u32 getMapInfoGround(u32) { return 0; }
	virtual void setSeExtParameter(JAISound*);
	virtual void setRegisterTrackCallback();
	virtual void loadGroupWave(s32, s32);

	static MSound* getMSound();

	void initSound();
	void mainLoop();
	void exitStage();
	void enterStage(MS_SCENE_WAVE, u8, u8);
	void loadWave(MS_SCENE_WAVE);
	void cleanUpAramWave(u8);
	bool checkWaveOnAram(MS_SCENE_WAVE);
	bool checkSeqOnMemory(u32);

	void stopAllSound();
	void fadeOutAllSound(u32);
	void stopAllSeInCategory(u8, u32);
	void setCategoryAllVolume(u8, f32, u32, u8);
	void setCategoryVOLs(u16, f32);
	void setCategoryVOLsDefault(u16);

	void pauseOn(bool);
	void pauseOff(u8);
	void talkModeIn(bool);
	void talkModeOut();
	void demoModeIn(u16, bool);
	void demoModeOut(bool);

	void setPlayerInfo(Vec*, Vec*, MtxPtr, bool);
	void setCameraInfo(Vec*, Vec*, MtxPtr, u32);
	f32 getDistFromCamera(Vec*);
	bool cameraLooksAtMario();

	void startSoundSet(u32, const Vec*, u32, f32, u32, u32, u8);
	void startSoundSetGrp(u32, const Vec*, u32, f32, u32, u32, u8);
	void startSoundActorSpecial(u32, const Vec*, f32, f32, u32, JAISound**, u32,
	                            u8);
	void startBeeSe(Vec*, u32);

	u32 startMarioVoice(u32, s16, u8);
	void stopMarioVoice(u32, u8);
	u32 getMarioVoiceID(u8);
	void* checkMarioVoicePlaying(u8);

	void playTimer(u32);
	void requestShineAppearFanfare();

	u32 getWallSound(u32, f32);
	u32 getBstPitch(u32);
	static u32 getBstSwitch(u32);
	static u32 getSwitch(u32, u32, u32);
	bool gateCheck(u32);

	bool resetAudioAll(u16);

	/* 0x7C */ JAISound* unk7C;
	/* 0x80 */ JAISound* unk80;
	/* 0x84 */ u32 unk84;
	/* 0x88 */ u8 unk88;
	/* 0x8C */ JAISound* unk8C[2];
	/* 0x94 */ u16 unk94;
	/* 0x98 */ MSModBgm* unk98;
	/* 0x9C */ MSBgmXFade* unk9C;
	/* 0xA0 */ u32 unkA0;
	/* 0xA4 */ u32 unkA4;
	/* 0xA8 */ u8 unkA8;
	/* 0xAC */ JAICamera unkAC[2];
	/* 0xC4 */ JAISound* unkC4;
	/* 0xC8 */ u8 unkC8[5];
	/* 0xCD */ u8 unkCD;
	/* 0xCE */ u8 unkCE;
	/* 0xCF */ u8 unkCF;
	/* 0xD0 */ u8 unkD0;
	/* 0xD1 */ u8 unkD1;
	/* 0xD2 */ u8 unkD2[0xD4 - 0xD2];

	// real
	void startSoundSystemSE(u32 param_1, u32 param_2, JAISound** param_3,
	                        u32 param_4)
	{
		if (gateCheck(param_1))
			MSoundSESystem::MSoundSE::startSoundSystemSE(param_1, param_2,
			                                             param_3, param_4);
	}

	void startSoundActor(u32 param_1, const Vec* param_2, u32 param_3,
	                     JAISound** param_4, u32 param_5, u8 param_6)
	{
		if (gateCheck(param_1))
			MSoundSESystem::MSoundSE::startSoundActor(
			    param_1, param_2, param_3, param_4, param_5, param_6);
	}

	void startSoundActorWithInfo(u32 param_1, const Vec* param_2, Vec* param_3,
	                             f32 param_4, u32 param_5, u32 param_6,
	                             JAISound** param_7, u32 param_8, u8 param_9)
	{
		if (gateCheck(param_1))
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    param_1, param_2, param_3, param_4, param_5, param_6, param_7,
			    param_8, param_9);
	}

	void startForceJumpSound(Vec*, u32, f32, u32);
};

#ifdef MSOUND_EMIT_START_FORCE_JUMP_SOUND
void MSound::startForceJumpSound(Vec* pos, u32 groundType, f32 height,
                                 u32 dist)
{
	u32 soundID;
	u8 type = (u8)groundType;

	switch (type) {
	case 21:
	case 23:
	case 29:
		soundID = 0x180A;
		break;
	case 30:
	default:
		if (dist < 6000) {
			soundID = 0x1810;
		} else if (dist < 12000) {
			soundID = 0x1811;
		} else {
			soundID = 0x1812;
		}
		break;
	}

	if (gateCheck(soundID)) {
		if (gateCheck(soundID)) {
			MSoundSESystem::MSoundSE::startSoundActor(soundID, pos, 0,
			                                          (JAISound**)NULL, 0, 4);
		}
	}
}
#endif

extern MSound* MSGMSound;
extern JAIBasic* MSGBasic;
extern MSound* gpMSound;

// real
inline MSound* SMSGetMSound() { return gpMSound; }

#endif // MSOUND_HPP
