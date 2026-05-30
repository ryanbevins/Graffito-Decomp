#ifndef ENEMY_BOSS_HANACHAN_HPP
#define ENEMY_BOSS_HANACHAN_HPP

#include <Enemy/EnemyManager.hpp>
#include <Enemy/BossHanachanPartsBase.hpp>
#include <Strategic/Nerve.hpp>

class TBossHanachanCommonSaveParams;
class TBossHanachanChangeSaveParams;
class TBossHanachanPartsBody;
class TSphereLink;
class MActor;

DECLARE_NERVE(TNerveSBH_Fall, TLiveActor);
DECLARE_NERVE(TNerveSBH_SleepContinue, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanDead, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanSnort, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanDamage, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanGetUp, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanDown, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanTumble, TLiveActor);
DECLARE_NERVE(TNerveBossHanachanGraphWander, TLiveActor);

class TBossHanachan : public TSpineEnemy {
public:
	TBossHanachan(const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void init(TLiveManager*);
	virtual void bind();
	virtual void moveObject();
	virtual void kill();
	virtual BOOL hasMapCollision() const;

	void execDamage();
	void execSlip();
	void execWalk(bool);
	f32 getBodyMaxRotateZ() const;
	void removeAllMapCollision();
	void goToInitialRecoverGraphNode();
	bool checkFallDecideAndSetup();
	bool isTumbleCompletelyAllBody() const;
	void throwMario_(THitActor*);
	void setRandomWeakBodyIndex();
	void changeAnmRateAndFrameUpdate_();
	bool isAllBckAlreadyEnd(EnumBossHanachanAnmKind) const;
	bool isFinishedGetUp() const;
	void considerSetAnm(EnumBossHanachanNerveAnm);
	void setAnmTimerWhenDead();
	void setAnmTimerWhenDamage();
	void setAnmTimerWhenSnort();
	void setAnmTimerWhenGetUp();
	void setTumbleAnm(EnumBossHanachanStopMotionBlendOnOff);
	void setHeadAndBodyAnm(EnumBossHanachanAnmKind,
	                       EnumBossHanachanStopMotionBlendOnOff);
	void emitCamShake_();
	void emitOneTimeSandPillar_(TBossHanachanPartsBody*);
	void emitParticle_();
	static void staticLoadParticle();

public:
	/* 0x150 */ TBossHanachanPartsBase* mBody[8];
	/* 0x170 */ TBossHanachanPartsBase* mHead;
	/* 0x174 */ int unk174;
	/* 0x178 */ TSphereLink* mSphereLink;
	/* 0x17C */ JGeometry::TVec3<f32> unk17C;
	/* 0x188 */ JGeometry::TVec3<f32> unk188;
	/* 0x194 */ f32 unk194;
	/* 0x198 */ f32 unk198;
	/* 0x19C */ MActor* mEffectActor;
	/* 0x1A0 */ JGeometry::TVec3<f32> mEffectPos;
	/* 0x1AC */ JGeometry::TVec3<f32> unk1AC;
	/* 0x1B8 */ int unk1B8;
	/* 0x1BC */ TBossHanachanCommonSaveParams* mParams;
	/* 0x1C0 */ TBossHanachanChangeSaveParams* mChangeParams;
};

class TBossHanachanManager : public TEnemyManager {
public:
	TBossHanachanManager(const char*);

	virtual void loadAfter();
	virtual void createModelData();
	virtual BOOL hasMapCollision() const;
	virtual void clipEnemies(JDrama::TGraphics*);

public:
	/* 0x54 */ TBossHanachanCommonSaveParams* mCommonParams;
	/* 0x58 */ TBossHanachanChangeSaveParams* mChangeParams[3];
};

#endif
