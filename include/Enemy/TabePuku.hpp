#ifndef ENEMY_TABEPUKU_HPP
#define ENEMY_TABEPUKU_HPP

#include <Enemy/SmallEnemy.hpp>
#include <Strategic/Nerve.hpp>

class TTabePuku;
class TBGCheckData;

class TTPHitActor : public THitActor {
public:
	TTPHitActor(TTabePuku* owner, const char* name = "タベプク補助当たり")
	    : THitActor(name)
	    , mOwner(owner)
	{
	}

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init();

	void bind();
	void updateTerrainCollsion();

	/* 0x68 */ TTabePuku* mOwner;
	/* 0x6C */ JGeometry::TVec3<f32> mMove;
	/* 0x78 */ f32 mCheckHeight;
	/* 0x7C */ f32 mCheckRadius;
	/* 0x80 */ f32 mGroundHeight;
	/* 0x84 */ const TBGCheckData* mGroundPlane;
	/* 0x88 */ u8 mIsAirborne;
	/* 0x89 */ u8 mTouchedWall;
	/* 0x8A */ u8 unk8A[2];
};

class TTabePukuSaveLoadParams : public TSmallEnemyParams {
public:
	TTabePukuSaveLoadParams(const char* prm)
	    : TSmallEnemyParams(prm)
	    , PARAM_INIT(mMarchSpeed, 0.15f)
	    , PARAM_INIT(mAttackSpeed, 0.22f)
	    , PARAM_INIT(mDiveSpeed, 0.4f)
	    , PARAM_INIT(mWaterFric, 0.95f)
	    , PARAM_INIT(mTurnSlerpRate, 0.05f)
	    , PARAM_INIT(mApartHeight, 500.0f)
	    , PARAM_INIT(mCorrectY, -40.0f)
	    , PARAM_INIT(mCorrectZ, 150.0f)
	    , PARAM_INIT(mTerritoryRange, 1000.0f)
	    , PARAM_INIT(mDragLength, 2500.0f)
	{
		TParams::load(mPrmPath);
	}

	/* 0x2D4 */ TParamRT<f32> mMarchSpeed;
	/* 0x2E8 */ TParamRT<f32> mAttackSpeed;
	/* 0x2FC */ TParamRT<f32> mDiveSpeed;
	/* 0x310 */ TParamRT<f32> mWaterFric;
	/* 0x324 */ TParamRT<f32> mTurnSlerpRate;
	/* 0x338 */ TParamRT<f32> mApartHeight;
	/* 0x34C */ TParamRT<f32> mCorrectY;
	/* 0x360 */ TParamRT<f32> mCorrectZ;
	/* 0x374 */ TParamRT<f32> mTerritoryRange;
	/* 0x388 */ TParamRT<f32> mDragLength;
};

class TTabePuku : public TSmallEnemy {
public:
	TTabePuku(const char* name = "タベプク");

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual MtxPtr getTakingMtx();
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void control();
	virtual void bind();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void attackToMario();
	virtual void forceKill();
	virtual bool doKeepDistance();
	virtual bool isFindMario(float);

	void swimTo(const JGeometry::TVec3<f32>&);

	TTabePukuSaveLoadParams* getSaveParam2() const
	{
		return (TTabePukuSaveLoadParams*)getSaveParam();
	}

public:
	/* 0x194 */ TTPHitActor* mHitActor;
	/* 0x198 */ JGeometry::TQuat4<f32> mQuat;
	/* 0x1A8 */ Mtx mTakingMtx;
	/* 0x1D8 */ u32 mMouthJointIndex;
	/* 0x1DC */ u8 mTouchedWall;
	/* 0x1DD */ u8 unk1DD[3];
	/* 0x1E0 */ f32 mDiveStartY;
	/* 0x1E4 */ JGeometry::TVec3<f32> mDragDirection;
};

class TNerveTabePukuGraphWander : public TNerveBase<TLiveActor> {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveTabePukuGraphWander& theNerve()
	{
		static TNerveTabePukuGraphWander instance;
		return instance;
	}
};

class TNerveTabePukuFound : public TNerveBase<TLiveActor> {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveTabePukuFound& theNerve()
	{
		static TNerveTabePukuFound instance;
		return instance;
	}
};

DECLARE_NERVE(TNerveTabePukuRecoverGraph, TLiveActor);
class TNerveTabePukuAttack : public TNerveBase<TLiveActor> {
public:
	virtual BOOL execute(TSpineBase<TLiveActor>*) const;
	static const TNerveTabePukuAttack& theNerve()
	{
		static TNerveTabePukuAttack instance;
		return instance;
	}
};

DECLARE_NERVE(TNerveTabePukuBite, TLiveActor);
DECLARE_NERVE(TNerveTabePukuDive, TLiveActor);
DECLARE_NERVE(TNerveTabePukuDrag, TLiveActor);

class TTabePukuManager : public TSmallEnemyManager {
public:
	TTabePukuManager(const char* name = "タベプクマネージャー");

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
};

#endif
