#ifndef ENEMY_IGAIGA_HPP
#define ENEMY_IGAIGA_HPP

#include <Enemy/EnemyAttachment.hpp>
#include <Enemy/WalkerEnemy.hpp>
#include <JSystem/JGeometry/JGMatrix34.hpp>
#include <Strategic/Nerve.hpp>

class J3DNode;
class SDLModelData;
class TAreaCylinderManager;
class TGraphWeb;
class TLiveActor;
class TMapEventSink;
class TWaterEmitInfo;

DECLARE_NERVE(TNerveGorogoroDie, TLiveActor);
DECLARE_NERVE(TNerveGorogoroRollOnGraph, TLiveActor);
DECLARE_NERVE(TNerveIgaigaShootFromCannon, TLiveActor);
DECLARE_NERVE(TNerveIgaigaWaterHit, TLiveActor);
DECLARE_NERVE(TNerveIgaigaRollOnGraph, TLiveActor);

class TRollEnemySaveLoadParams : public TWalkerEnemyParams {
public:
	TRollEnemySaveLoadParams(const char* path)
	    : TWalkerEnemyParams(path)
	    , PARAM_INIT(mSLGenerateInterval, 300)
	    , PARAM_INIT(mSLExpandRate, 1.0f)
	    , PARAM_INIT(mSLExpandMax, 1.5f)
	    , PARAM_INIT(mSLBoundVYMax, 15.0f)
	    , PARAM_INIT(mSLGroundOffsetY, 150.0f)
	{
		load(mPrmPath);
	}

	/* 0x32C */ TParamRT<s32> mSLGenerateInterval;
	/* 0x340 */ TParamRT<f32> mSLExpandRate;
	/* 0x354 */ TParamRT<f32> mSLExpandMax;
	/* 0x368 */ TParamRT<f32> mSLBoundVYMax;
	/* 0x37C */ TParamRT<f32> mSLGroundOffsetY;
};

class TRollEnemy : public TWalkerEnemy {
public:
	TRollEnemy(const char*);

	virtual void reset();
	virtual void setBehavior();
	virtual void behaveToWater(THitActor*);
	virtual void attackToMario();
	virtual bool isCollidMove(THitActor*);
	virtual void setAfterDeadEffect() { }
	virtual void walkBehavior(int, f32);
	virtual void flagJump();
	virtual bool isReachedToGoalXZ();
	virtual void bound() { }
	virtual bool isRolling() { return false; }
	virtual void rollSE() { }
	virtual void boundSE() { }

	TRollEnemySaveLoadParams* getRollParams() const
	{
		return (TRollEnemySaveLoadParams*)getSaveParam();
	}

	static f32 mBoundVal;
	static f32 mTransYOffset;

public:
	/* 0x194 */ f32 unk194;
	/* 0x198 */ f32 unk198;
	/* 0x19C */ f32 unk19C;
	/* 0x1A0 */ f32 unk1A0;
	/* 0x1A4 */ void* unk1A4;
	/* 0x1A8 */ u8 unk1A8;
	/* 0x1A9 */ u8 unk1A9[3];
	/* 0x1AC */ f32 unk1AC;
	/* 0x1B0 */ f32 unk1B0;
};

class TGorogoro : public TRollEnemy {
public:
	TGorogoro(const char* name);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void kill();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setDeadAnm();
	virtual void setMeltAnm();
	virtual void forceKill();
	virtual void setMActorAndKeeper();
	virtual void walkBehavior(int, f32);
	virtual void flagJump();
	virtual void bound();
	virtual bool isRolling();
	virtual void rollSE();
	virtual void boundSE();

	void generateByGateKeeper(const JGeometry::TVec3<f32>&,
	                          const JGeometry::TVec3<f32>&);

public:
	/* 0x1B4 */ TPosition3f unk1B4;
	/* 0x1E4 */ bool unk1E4;
	/* 0x1E8 */ s32 unk1E8;
	/* 0x1EC */ bool unk1EC;
	/* 0x1ED */ u8 unk1ED[0x1F4 - 0x1ED];
};

class TIgaiga : public TRollEnemy {
public:
	TIgaiga(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual void kill();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setWalkAnm();
	virtual void setDeadAnm();
	virtual void setMeltAnm();
	virtual void setMActorAndKeeper();
	virtual bool isHitValid(u32);
	virtual void walkBehavior(int, f32);
	virtual bool isReachedToGoalXZ();
	virtual void bound();
	virtual bool isRolling();
	virtual void rollSE();
	virtual void boundSE();

	void shoot(JGeometry::TVec3<f32>&);

	static f32 mReachNodeDist;

public:
	/* 0x1B4 */ int unk1B4;
	/* 0x1B8 */ int unk1B8;
	/* 0x1BC */ bool unk1BC;
	/* 0x1BD */ u8 unk1BD[0x1C0 - 0x1BD];
	/* 0x1C0 */ JGeometry::TVec3<f32> unk1C0;
	/* 0x1CC */ f32 unk1CC;
	/* 0x1D0 */ void* unk1D0;
	/* 0x1D4 */ u8 unk1D4[0x1D8 - 0x1D4];
	/* 0x1D8 */ JGeometry::TVec3<f32> unk1D8;
	/* 0x1E4 */ f32 unk1E4;
	/* 0x1E8 */ int unk1E8;
};

class TGorogoroPolluteModel : public TEnemyPolluteModel {
public:
	TGorogoroPolluteModel(TLiveActor* owner, int index, SDLModelData* data,
	                      const char* name)
	    : TEnemyPolluteModel(owner, index, data, name)
	{
	}

	virtual void setAnm();
};

class TGorogoroPolluteModelManager : public TEnemyPolluteModelManager {
public:
	TGorogoroPolluteModelManager(const char* name)
	    : TEnemyPolluteModelManager(name)
	{
	}

	virtual void init(TLiveActor*);
};

class TIgaigaPolluteModel : public TEnemyPolluteModel {
public:
	TIgaigaPolluteModel(TLiveActor* owner, int index, SDLModelData* data,
	                    const char* name)
	    : TEnemyPolluteModel(owner, index, data, name)
	{
	}

	virtual void setAnm();
};

class TIgaigaPolluteModelManager : public TEnemyPolluteModelManager {
public:
	TIgaigaPolluteModelManager(const char* name)
	    : TEnemyPolluteModelManager(name)
	{
	}

	virtual void init(TLiveActor*);
};

class TGorogoroManager : public TSmallEnemyManager {
public:
	TGorogoroManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void initSetEnemies();

public:
	/* 0x60 */ s32 unk60;
	/* 0x64 */ TMapEventSink* unk64;
	/* 0x68 */ bool unk68;
	/* 0x6C */ TGorogoroPolluteModelManager* mPolluteModelManager;
	/* 0x70 */ TAreaCylinderManager* unk70;
};

class TIgaigaManager : public TSmallEnemyManager {
public:
	TIgaigaManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void initSetEnemies();

public:
	/* 0x60 */ TIgaigaPolluteModelManager* mPolluteModelManager;
	/* 0x64 */ void* unk64;
	/* 0x68 */ TWaterEmitInfo* mWaterEmitInfo;
};

int RollEnemyBodyCallback(J3DNode*, int);

#endif
