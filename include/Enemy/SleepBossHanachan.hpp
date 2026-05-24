#ifndef ENEMY_SLEEP_BOSS_HANACHAN_HPP
#define ENEMY_SLEEP_BOSS_HANACHAN_HPP

#include <Enemy/DemoBossHanachan.hpp>
#include <JSystem/JGeometry.hpp>

class TMirrorActor;

class TSleepBossHanachan : public TDemoBossHanachan {
public:
	TSleepBossHanachan(const char* name)
	    : TDemoBossHanachan(name)
	{
		mFallPos.set<f32>(0.0f, 0.0f, 0.0f);
		mMirrorActor = nullptr;
	}

	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual const char** getBasNameTable() const;

	void startFall(f32, f32, f32);

public:
	/* 0x150 */ JGeometry::TVec3<f32> mFallPos;
	/* 0x15C */ TMirrorActor* mMirrorActor;
};

class TSleepBossHanachanManager : public TDemoBossHanachanManager {
public:
	TSleepBossHanachanManager(const char* name)
	    : TDemoBossHanachanManager(name)
	{
		mSaveParams
		    = new TDemoBossHanachanSaveParams("/enemy/sleepBossHanachan.prm");
	}

	virtual void createModelData();
};

#endif
