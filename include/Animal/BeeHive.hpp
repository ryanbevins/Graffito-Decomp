#ifndef ANIMAL_BEE_HIVE_HPP
#define ANIMAL_BEE_HIVE_HPP

#include <Animal/Realoid.hpp>
#include <Enemy/EnemyManager.hpp>
#include <JSystem/JGeometry/JGQuat4.hpp>
#include <Strategic/Nerve.hpp>
#include <System/ParamInst.hpp>
#include <System/Params.hpp>

class MActor;
class TMapObjBase;
class TBee;

class TBeeHiveParams : public TSpineEnemyParams {
public:
	TBeeHiveParams(const char*);

	/* 0x0A8 */ TParamRT<s32> mGiveupTimer;
	/* 0x0BC */ TParamRT<f32> mGiveupRange;
	/* 0x0D0 */ TParamRT<s32> mDecrimentTimer;
	/* 0x0E4 */ TParamRT<f32> mRebound;
	/* 0x0F8 */ TParamRT<f32> mDecay;
	/* 0x10C */ TParamRT<f32> mAngleMaxAdd;
	/* 0x120 */ TParamRT<f32> mShakePower;
	/* 0x134 */ TParamRT<f32> mFallAngularVel;
	/* 0x148 */ TParamRT<f32> mSearchRange;
};

class TBeeHiveManager : public TEnemyManager {
public:
	TBeeHiveManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
};

class TBeeHive : public TRealoid {
public:
	TBeeHive(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void control();
	virtual void bind();
	virtual void reset();
	virtual TRealoidActor* createRealoidActor(MActor*);

	JGeometry::TVec3<f32> getCenterOfGravity() const;
	void appearBee(int);
	BOOL doWait();
	void controlSound();
	void controlCollision();
	void receiveMessageFromChild(TBee*);

	TBeeHiveParams* getBeeParams() const { return (TBeeHiveParams*)getSaveParam(); }

	/* 0x158 */ JGeometry::TQuat4<f32> mInitialQuat;
	/* 0x168 */ JGeometry::TQuat4<f32> mCurrentQuat;
	/* 0x178 */ JGeometry::TQuat4<f32> mCenterQuat;
	/* 0x188 */ JGeometry::TVec3<f32> mAngularVelocity;
	/* 0x194 */ JGeometry::TVec3<f32> mInitialPosition;
	/* 0x1A0 */ JGeometry::TVec3<f32> mBeeSoundPos;
	/* 0x1AC */ TMapObjBase* mBreakObj;
	/* 0x1B0 */ s32 mBreakTimer;
	/* 0x1B4 */ s32 mWaitTimer;
	/* 0x1B8 */ TMapObjBase** mCoinObjs;
	/* 0x1BC */ s32 mCoinIndex;
};

class TBee : public TRealoidActor {
public:
	TBee(MActor* actor, TBeeHive* owner)
	    : TRealoidActor(actor)
	    , mOwner(owner)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init();

	/* 0xA8 */ TBeeHive* mOwner;
};

DECLARE_NERVE(TNerveBeeHiveWait, TLiveActor);
DECLARE_NERVE(TNerveBeeHiveFall, TLiveActor);
DECLARE_NERVE(TNerveBeeHiveBreak, TLiveActor);
DECLARE_NERVE(TNerveBeeHiveAttack, TLiveActor);
DECLARE_NERVE(TNerveBeeHiveMarioWaterIn, TLiveActor);
DECLARE_NERVE(TNerveBeeHiveReset, TLiveActor);

#endif
