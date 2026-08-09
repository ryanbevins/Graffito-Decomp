#ifndef ANIMAL_BIRD_HPP
#define ANIMAL_BIRD_HPP

#include <Animal/AnimalBase.hpp>
#include <Animal/AnimalManager.hpp>
#include <Enemy/Enemy.hpp>
#include <Strategic/Nerve.hpp>
#include <System/ParamInst.hpp>
#include <System/Params.hpp>

class TAnimalBird;

class TAnimalBirdParams : public TSpineEnemyParams {
public:
	TAnimalBirdParams(const char* prm);

	/* 0xA8 */ TParamRT<f32> mMarchSpeed;
	/* 0xBC */ TParamRT<f32> mTurnSpeed;
	/* 0xD0 */ TParamRT<s32> mReturnTimer;
	/* 0xE4 */ TParamRT<f32> mSearchLength;
	/* 0xF8 */ TParamRT<f32> mSearchHeight;
	/* 0x10C */ TParamRT<f32> mSearchAware;
	/* 0x120 */ TParamRT<f32> mSearchAngle;
	/* 0x134 */ TParamRT<s32> mActionTimer;
	/* 0x148 */ TParamRT<s32> mWaterproofTimerMax;
	/* 0x15C */ TParamRT<s32> mFloatingTimerMax;
	/* 0x170 */ TParamRT<f32> mLandingGravityY;
	/* 0x184 */ TParamRT<f32> mLandingTorqueY;
	/* 0x198 */ TParamRT<f32> mWalkingTorqueY;
	/* 0x1AC */ TParamRT<f32> mWalkingSpeed;
	/* 0x1C0 */ TParamRT<s32> mWalkTimer;
	/* 0x1D4 */ TParamRT<f32> mLandingFric;
	/* 0x1E8 */ TParamRT<s32> mActionTimerAdd;
	/* 0x1FC */ TParamRT<f32> mWaterPowerY;
};

class TAnimalBirdManager : public TEnemyManager {
public:
	TAnimalBirdManager(const char* name);

	virtual void load(JSUMemoryInputStream& stream);
	virtual void loadAfter();
	virtual void createModelData();
};

class TAnimalBird : public TSpineEnemy {
public:
	TAnimalBird(const char* name);

	virtual void load(JSUMemoryInputStream& stream);
	virtual void loadAfter();
	virtual BOOL receiveMessage(THitActor* sender, u32 msg);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual void moveObject();
	virtual const char** getBasNameTable() const;

	void initParams();
	BOOL isFindMario() const;
	void doFlyToCurPathNode();
	bool doLanding(bool);

	/* 0x150 */ class TMapObjBase* unk150;
	/* 0x154 */ class TBinder* mBinder2;
	/* 0x158 */ JGeometry::TVec3<f32> unk158;
	/* 0x164 */ JGeometry::TVec3<f32> unk164;
	/* 0x170 */ f32 unk170;
	/* 0x174 */ f32 unk174;
	/* 0x178 */ int unk178;
	/* 0x17C */ int unk17C;
	/* 0x180 */ char unk180[0x210 - 0x180];
};

DECLARE_NERVE(TNerveAnimalBirdLanding, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdPreLanding, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdComeback, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdChangeToCoin, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdGraphWander, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdTakeoff, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdWalkOnGround, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdActionOnGround, TLiveActor);
DECLARE_NERVE(TNerveAnimalBirdWaitOnGround, TLiveActor);

#endif
