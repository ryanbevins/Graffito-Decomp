#ifndef ENEMY_AMI_NOKO_HPP
#define ENEMY_AMI_NOKO_HPP

#include <Enemy/WalkerEnemy.hpp>
#include <Enemy/SmallEnemy.hpp>
#include <Strategic/Nerve.hpp>
#include <Strategic/HitActor.hpp>

class TAmiNoko;

class TAmiNokoSaveLoadParams : public TWalkerEnemyParams {
public:
	TAmiNokoSaveLoadParams(const char* path);

	/* 0x338 */ TParamRT<f32> mSLMtxRotSpeed;
};

class TAmiHit : public THitActor {
public:
	TAmiHit(const char* name) : THitActor(name) { }
	virtual ~TAmiHit();

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor* sender, u32 message);

public:
	/* 0x?? */ TAmiNoko* mOwner;
};

class TAmiNoko : public TWalkerEnemy {
public:
	TAmiNoko(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void bind();
	virtual void calcRootMatrix();
	virtual f32 getGravityY() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual void setMActorAndKeeper();
	virtual bool isHitValid(u32);
	virtual bool isCollidMove(THitActor*);
	virtual void attackToMario();
	virtual void setWalkAnm();
	virtual const char** getBasNameTable() const;

	void creepToCurPathNode(f32);

public:
	/* 0x194 */ const TBGCheckData* unk194;
	/* 0x198 */ int unk198;
	/* 0x19C */ JGeometry::TVec3<f32> unk19C;
	/* 0x1A8 */ JGeometry::TVec3<f32> unk1A8;
	/* 0x1B4 */ JGeometry::TVec3<f32> unk1B4;
	/* 0x1C0 */ JGeometry::TVec3<f32> unk1C0;
	/* 0x1CC */ Mtx unk1CC;
	/* 0x1FC */ JGeometry::TVec3<f32> unk1FC;
	/* 0x208 */ TAmiHit* mAmiHit;
	/* 0x20C */ TAmiNokoSaveLoadParams* mSaveParams;
	/* 0x210 */ u8 unk210;
};

class TLiveActor;

DECLARE_NERVE(TNerveAmiNokoFreeze, TLiveActor);
DECLARE_NERVE(TNerveAmiNokoDie, TLiveActor);
DECLARE_NERVE(TNerveAmiNokoTurn, TLiveActor);
DECLARE_NERVE(TNerveAmiNokoWalkOnFence, TLiveActor);

class TAmiNokoManager : public TSmallEnemyManager {
public:
	TAmiNokoManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual TSpineEnemy* createEnemyInstance();
};

#endif
