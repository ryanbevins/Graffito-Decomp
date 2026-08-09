#ifndef ENEMY_DEBU_TELESA_HPP
#define ENEMY_DEBU_TELESA_HPP

#include <Enemy/SmallEnemy.hpp>
#include <Strategic/Nerve.hpp>

class TDebuTelesa : public TSmallEnemy {
public:
	TDebuTelesa(const char* name);

	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void kill();
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual void behaveToWater(THitActor*);
	virtual bool changeByJuice() { return false; }
	virtual void setDeadAnm();
	virtual void attackToMario();
	virtual bool isCollidMove(THitActor*);
	virtual bool doKeepDistance();

public:
	/* 0x194 */ char unk194[0x4];
	/* 0x198 */ s32 mJntNullYodare;
	/* 0x19C */ s32 mJntRhand;
	/* 0x1A0 */ JGeometry::TVec3<f32> mTipPos;
};

class TDebuTelesaParams : public TSmallEnemyParams {
public:
	TDebuTelesaParams(const char* path)
	    : TSmallEnemyParams(path)
	{
		TParams::load(mPrmPath);
	}
};

class TDebuTelesaManager : public TSmallEnemyManager {
public:
	TDebuTelesaManager(const char* name);

	virtual void load(JSUMemoryInputStream&);
	virtual void createModelData();
	virtual void clipEnemies(JDrama::TGraphics*);
};

DECLARE_NERVE(TNerveDebuTelesaWait, TLiveActor);

#endif
