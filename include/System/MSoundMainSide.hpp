#ifndef M_SOUND_MAIN_SIDE_HPP
#define M_SOUND_MAIN_SIDE_HPP

#include <dolphin/mtx.h>

class MSStage {
public:
	virtual void setPosPtr(Vec*);
	virtual void stageLoop();
	virtual void proc() = 0;
	static MSStage* init(u8, u8);

	static MSStage* smMSStage;
};

class MSSTageSimpleEnvironmentMonte {
public:
	void proc();
};

class MSSTageSimpleEnvironment : public MSStage {
public:
	MSSTageSimpleEnvironment(u32 sound_id)
	    : mSoundID(sound_id)
	{
	}

	virtual void proc();

	/* 0x04 */ u32 mSoundID;
};

class MSStageCubeFade : public MSStage {
public:
	MSStageCubeFade();

	void calcParamRatioInCube(long);
	virtual void proc();

	/* 0x04 */ s32 mCurrentCube;
	/* 0x08 */ s32 mPreviousCube;
	/* 0x0C */ f32 mFadeRatio;
};

class MSStageCubeSwitch : public MSStageCubeFade {
public:
	MSStageCubeSwitch(u8 flag)
	    : MSStageCubeFade()
	    , unk10(flag)
	{
	}

	void toBossBgm();
	void toStageBgm();
	virtual void proc();

	/* 0x10 */ u8 unk10;
	/* 0x11 */ u8 unk11;
};

class MSStageCubeFadeMonte : public MSStageCubeFade {
public:
	MSStageCubeFadeMonte()
	    : MSStageCubeFade()
	    , unk10(3)
	    , unk14(3)
	{
	}

	virtual void proc();

	/* 0x10 */ s32 unk10;
	/* 0x14 */ s32 unk14;
};

class MSStageDistFade : public MSStage {
public:
	MSStageDistFade(const Vec*, float, float, u32, bool);
	virtual void proc();

	/* 0x04 */ u32 unk4;
	/* 0x08 */ f32 unk8;
	/* 0x0C */ f32 unkC;
	/* 0x10 */ const Vec* unk10;
	/* 0x14 */ u32 unk14;
	/* 0x18 */ bool unk18;
};

class MSStageDistFadeMonte : public MSStageDistFade {
public:
	MSStageDistFadeMonte(const Vec* pos, float near_dist, float far_dist,
	                     u32 bgm, bool use_pan)
	    : MSStageDistFade(pos, near_dist, far_dist, bgm, use_pan)
	    , unk1C(3)
	    , unk20(3)
	{
	}

	virtual void proc();

	/* 0x1C */ s32 unk1C;
	/* 0x20 */ s32 unk20;
};

class MSStageProc {
public:
	void setBgmPosition(const Vec&, float, bool, u32, u32);
};

class MSMainProc {
public:
	class MSStageInfo {
	public:
		static u32 msStg;
		static u32 demoBgm;
		static u32 stageBgm;
		static u32 stageBgmSilent;
		static u8 stageBgmSilentStartStatus;
		static u8 flags;
		static u16 volOffCategory;
		static u8 fadeEvent;
		static u32 switchBgm;
		static u32 switchBgm2;
		static f32 cubeFadeRatio;
		static bool cubeFadeUsePan;
		static bool bossLives;
		static bool bossLives2;
		static bool bossNotDamaged;
		static bool distFadeStageToKage;
	};

	static void startStageBGM(u8, u8);
	static void endStageEntranceDemo(u8, u8);
	static void entranceDemoLoop(u32);
	static void startStageEntranceDemo(u8, u8);
	static void setMSoundEnterStage(u8, u8);
	static void setBossNotDamagedFlag(bool);
	void getBossLivesFlag2();
	void getBossLivesFlag();
	static void setBossLivesFlag2(bool);
	static void setBossLivesFlagOnlyFlag(bool);
	static void setBossLivesFlag(bool);
	static void fromTalkingCameraDemo(bool);
	static void toTalkingCameraDemo();
	void fromTHPDemo();
	void toTHPDemo();
	static void fromInnerCameraDemo();
	static void toInnerCameraDemo();
	void entranceDemoWipeInEnd();
	static u32 getMonteVillageActorArea(const Vec&);
};

inline MSStageCubeFade::MSStageCubeFade()
    : mCurrentCube(-1)
    , mPreviousCube(-1)
    , mFadeRatio(MSMainProc::MSStageInfo::cubeFadeRatio)
{
}

#endif
