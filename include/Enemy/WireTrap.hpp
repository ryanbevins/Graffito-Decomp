#ifndef ENEMY_WIRE_TRAP_HPP
#define ENEMY_WIRE_TRAP_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/WireBinder.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/Nerve.hpp>

class TWireTrap;

class TWireTrapParams : public TSpineEnemyParams {
public:
	TWireTrapParams(const char*);

	/* 0xA8 */ TParamRT<f32> mInWaterPowerRate;
	/* 0xBC */ TParamRT<s32> mScaleTimerMax;
	/* 0xD0 */ TParamRT<s32> mGoTimerMax;
};

class TWireTrapManager : public TEnemyManager {
public:
	TWireTrapManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
};

class TWireTrap : public TSpineEnemy {
public:
	TWireTrap(const char* = "\x83\x8F\x83\x43\x83\x84\x82\xCD\x82\xDC\x82\xE8");

	virtual void load(JSUMemoryInputStream&);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void kill();

	TWireBinder* getWireBinderDirect() const { return (TWireBinder*)mBinder; }
#ifdef WIRETRAP_GETWIREBINDER_OUT_OF_LINE
	TWireBinder* getWireBinder() const;
#else
	TWireBinder* getWireBinder() const { return getWireBinderDirect(); }
#endif
	const JGeometry::TVec3<f32>& getWireDir() const;
	static const TNerveBase<TLiveActor>* getNerveFromMode(int);
	void checkHitActors();

	// fabricated
	TWireTrapParams* getSaveParam2() const
	{
		return (TWireTrapParams*)getSaveParam();
	}

public:
	/* 0x150 */ u8 unk150[0x10];
	/* 0x160 */ s32 unk160;
	/* 0x164 */ s32 mShakeTimer;
	/* 0x168 */ f32 mShakeWidth;
	/* 0x16C */ s32 mBiriTimer;
	/* 0x170 */ f32 mWireDir;
	/* 0x174 */ s32 mWaitTime;
	/* 0x178 */ f32 mScaleRate;
	/* 0x17C */ f32 mScaleSpeed;
	/* 0x180 */ s16 mColorType;
};

DECLARE_NERVE(TNerveWireTrapGoWait, TLiveActor);
DECLARE_NERVE(TNerveWireTrapWait, TLiveActor);
DECLARE_NERVE(TNerveWireTrapSearch, TLiveActor);
DECLARE_NERVE(TNerveWireTrapOnewayMoveEnd, TLiveActor);
DECLARE_NERVE(TNerveWireTrapOnewayMove, TLiveActor);
DECLARE_NERVE(TNerveWireTrapOnewayMoveStart, TLiveActor);
DECLARE_NERVE(TNerveWireTrapReturnMove, TLiveActor);

#endif
