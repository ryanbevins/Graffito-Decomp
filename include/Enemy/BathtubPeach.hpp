#ifndef ENEMY_BATHTUB_PEACH_HPP
#define ENEMY_BATHTUB_PEACH_HPP

#include <Enemy/BathtubBinder.hpp>
#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>

class TBathtubPeachParams : public TSpineEnemyParams {
public:
	TBathtubPeachParams(const char* prm = "/enemy/bathtubpeach.prm")
	    : TSpineEnemyParams(prm)
	    , PARAM_INIT(mTurnSpeed, 8.0f)
	    , PARAM_INIT(mTurnSpeed2, 1.0f)
	    , PARAM_INIT(mSpeed, 16.0f)
	    , PARAM_INIT(mAngle, 72.0f)
	    , PARAM_INIT(mRange, 100.0f)
	    , PARAM_INIT(mRadius, 2200.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x0A8 */ TParamRT<f32> mTurnSpeed;
	/* 0x0BC */ TParamRT<f32> mTurnSpeed2;
	/* 0x0D0 */ TParamRT<f32> mSpeed;
	/* 0x0E4 */ TParamRT<f32> mAngle;
	/* 0x0F8 */ TParamRT<f32> mRange;
	/* 0x10C */ TParamRT<f32> mRadius;
};

class TBathtubPeach : public TSpineEnemy {
public:
	TBathtubPeach(const char* name = "バスタブピーチ");

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual Mtx* getRootJointMtx() const;
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual const char** getBasNameTable() const;
	virtual void reset();

public:
	/* 0x150 */ TBathtubBinder mBinder;
};

class TBathtubPeachManager : public TEnemyManager {
public:
	TBathtubPeachManager(const char* name = "バスタブピーチマネージャー");

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

DECLARE_NERVE(TNervePeachEscape, TLiveActor);
DECLARE_NERVE(TNervePeachStagger, TLiveActor);

#endif
