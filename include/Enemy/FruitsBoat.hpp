#ifndef ENEMY_FRUITS_BOAT_HPP
#define ENEMY_FRUITS_BOAT_HPP

#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Strategic/Nerve.hpp>

class J3DAnmTransformKey;
class J3DFrameCtrl;

class TFruitsBoatParams : public TSpineEnemyParams {
public:
	TFruitsBoatParams(const char*);

	/* 0xA8 */ TParamRT<f32> mSLMoveSpeed;
	/* 0xBC */ TParamRT<f32> mSLRotSpeed;
	/* 0xD0 */ TParamRT<f32> mSLBckMoveSpeed;
};

class TFruitsBoatManager : public TEnemyManager {
public:
	TFruitsBoatManager(int, const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();

public:
	/* 0x54 */ int mBoatId;
};

class TFruitsBoat : public TSpineEnemy {
public:
	TFruitsBoat(const char* = "フルーツ運搬船");

	virtual BOOL receiveMessage(THitActor*, u32);
	virtual Mtx* getRootJointMtx() const;
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void setGroundCollision();
	virtual void moveObject();
	virtual void requestShadow();
	virtual void load(JSUMemoryInputStream&);

	int setBckTrack(const char*);

	// fabricated
	TFruitsBoatParams* getSaveParam2() const
	{
		return (TFruitsBoatParams*)getSaveParam();
	}

public:
	/* 0x150 */ u16 mAttrFlag;
	/* 0x154 */ f32 unk154;
	/* 0x158 */ f32 unk158;
	/* 0x15C */ J3DAnmTransformKey* mBckAnm;
	/* 0x160 */ J3DFrameCtrl* mBckFrameCtrl;
	/* 0x164 */ f32 mWaveNormalX;
	/* 0x168 */ f32 mWaveNormalY;
	/* 0x16C */ f32 mWaveNormalZ;
	/* 0x170 */ f32 mSwayAngle;
	/* 0x174 */ f32 mSwayVel;
};

DECLARE_NERVE(TNerveFruitsBoatGraphWander, TLiveActor);
DECLARE_NERVE(TNerveFruitsBoatBckTrace, TLiveActor);

#endif
