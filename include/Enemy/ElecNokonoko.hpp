#ifndef ENEMY_ELEC_NOKONOKO_HPP
#define ENEMY_ELEC_NOKONOKO_HPP

#include <Enemy/WalkerEnemy.hpp>
#include <Enemy/SmallEnemy.hpp>
#include <Enemy/EnemyAttachment.hpp>
#include <Strategic/Nerve.hpp>
#include <Strategic/HitActor.hpp>

class TElecNokonoko;
class J3DMaterialTable;

class TElecNokonokoSaveLoadParams : public TWalkerEnemyParams {
public:
	TElecNokonokoSaveLoadParams(const char* path);

	/* 0x32C */ TParamRT<s32> mSLReadyTime;
	/* 0x340 */ TParamRT<f32> mSLCarapaceGravity;
	/* 0x354 */ TParamRT<f32> mSLCarapaceSpeed;
	/* 0x368 */ TParamRT<f32> mSLCarapaceTurnSpeed;
	/* 0x37C */ TParamRT<f32> mSLCarapaceSpinSpeed;
	/* 0x390 */ TParamRT<f32> mSLCarapaceShootRange;
	/* 0x3A4 */ TParamRT<f32> mSLCarapaceFlyDist;
};

// The detachable electric shell. Base TEnemyAttachment : TSpineEnemy.
class TElecCarapace : public TEnemyAttachment {
public:
	TElecCarapace(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void calcRootMatrix();
	virtual void bind();
	virtual void kill();
	virtual void loadInit(TSpineEnemy*, const char*);
	virtual void appear();
	virtual void rebirth();
	virtual void sendMessage();
	virtual void behaveToHitGround();
	virtual void behaveToHitWall(const TBGCheckData*);
	virtual void setBehavior();
	virtual void recoverScale();
	virtual f32 getNowGravity();
	virtual f32 getPhaseShift() const;
	virtual void shoot();

	void reflect(THitActor*);

public:
	/* 0x16C */ TSpineEnemy* unk16C;
	/* 0x170 */ int unk170;
	/* 0x174 */ u8 unk174;
	/* 0x175 */ u8 unk175;
	/* 0x176 */ u8 unk176;
	/* 0x178 */ f32 unk178;
	/* 0x17C */ f32 unk17C;
	/* 0x180 */ int unk180;
	/* 0x184 */ u8 unk184;
	/* 0x188 */ f32 unk188;
	/* 0x18C */ f32 unk18C;
	/* 0x190 */ f32 unk190;
	/* 0x194 */ f32 unk194;
	/* 0x198 */ f32 unk198;
};

class TElecNokonoko : public TWalkerEnemy {
public:
	TElecNokonoko(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual const char** getBasNameTable() const;
	virtual void genRandomItem();
	virtual void behaveToWater(THitActor*);
	virtual void setWalkAnm();
	virtual void setDeadAnm();
	virtual void setMeltAnm();
	virtual void setWaitAnm();
	virtual void setRunAnm();
	virtual void attackToMario();
	virtual void setMActorAndKeeper();
	virtual void sendAttackMsgToMario();
	virtual void behaveToFindMario();
	virtual bool isResignationAttack();
	virtual void rest();

	// fabricated accessor
	TElecNokonokoSaveLoadParams* getSaveParams2() const
	{
		return (TElecNokonokoSaveLoadParams*)getSaveParam();
	}

	static u8 mReflectSw;

public:
	/* 0x194 */ TElecCarapace* mCarapace;
	/* 0x198 */ int unk198;
	/* 0x19C */ int mReadyTimer;
	/* 0x1A0 */ TElecNokonokoSaveLoadParams* mSaveParams;
	/* 0x1A4 */ int unk1A4;
	/* 0x1A8 */ int unk1A8;
	/* 0x1AC */ int unk1AC;
	/* 0x1B0 */ int unk1B0;
};

class TLiveActor;

DECLARE_NERVE(TNerveElecCarapaceReturn, TLiveActor);
DECLARE_NERVE(TNerveElecCarapaceWait, TLiveActor);
DECLARE_NERVE(TNerveElecCarapaceMove, TLiveActor);
DECLARE_NERVE(TNerveElecNokonokoFreeze, TLiveActor);
DECLARE_NERVE(TNerveElecNokonokoCollect, TLiveActor);
DECLARE_NERVE(TNerveElecNokonokoAttack, TLiveActor);
DECLARE_NERVE(TNerveElecNokonokoRebirth, TLiveActor);
DECLARE_NERVE(TNerveElecNokonokoTurn, TLiveActor);
DECLARE_NERVE(TNerveElecNokonokoShoot, TLiveActor);

class TElecNokonokoManager : public TSmallEnemyManager {
public:
	TElecNokonokoManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void clipEnemies(JDrama::TGraphics*);
	virtual void initSetEnemies();

	J3DMaterialTable* getMaterialTable() const { return mMaterialTable; }

public:
	/* 0x60 */ J3DMaterialTable* mMaterialTable;
};

#endif
