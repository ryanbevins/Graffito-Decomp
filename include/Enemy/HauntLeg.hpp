#ifndef ENEMY_HAUNT_LEG_HPP
#define ENEMY_HAUNT_LEG_HPP

#include <Enemy/WalkerEnemy.hpp>
#include <Strategic/Nerve.hpp>

class THauntLeg;

class THauntedObject : public THitActor {
public:
	THauntedObject(const char* name)
	    : THitActor(name)
	{
		// mOwner left uninitialized — THauntLeg::init sets it explicitly
	}

	virtual BOOL receiveMessage(THitActor* sender, u32 message);

public:
	/* 0x68 */ THauntLeg* mOwner;
};

class THauntLegManager : public TSmallEnemyManager {
public:
	THauntLegManager(const char* = "ハントレッグマネージャー");

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
	virtual void initSetEnemies();
};

class THauntLeg : public TWalkerEnemy {
public:
	THauntLeg(const char* name = "ハントレッグ")
	    : TWalkerEnemy(name)
	    , mHauntedObject(nullptr)
	    , unk198(false)
	    , unk199(true)
	    , mTarget(nullptr)
	{
	}

	virtual MtxPtr getTakingMtx();
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void setGenerateAnm();
	virtual void setWaitAnm();
	virtual void setWalkAnm();
	virtual void setRunAnm();
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual void setMActorAndKeeper();
	virtual bool isCollidMove(THitActor*);

public:
	/* 0x194 */ THauntedObject* mHauntedObject;
	/* 0x198 */ bool unk198;
	/* 0x199 */ bool unk199;
	/* 0x19C */ THitActor* mTarget;
	/* 0x1A0 */ JGeometry::TVec3<f32> mJumpVelocity;
	/* 0x1AC */ f32 unk1AC;
};

DECLARE_NERVE(TNerveHauntLegHaunt, TLiveActor);

#endif
