#ifndef ENEMY_CANNON_HPP
#define ENEMY_CANNON_HPP

#include <Enemy/SmallEnemy.hpp>
#include <MSound/MAnmSound.hpp>
#include <Strategic/SharedParts.hpp>
#include <JSystem/JGeometry.hpp>

class SDLModelData;
class TMapCollisionMove;

class TCannonSaveLoadParams : public TSmallEnemyParams {
public:
	TCannonSaveLoadParams(const char*);

	/* 0x2D4 */ TParamRT<f32> mSLHideDist;
	/* 0x2E8 */ TParamRT<f32> mSLBombDist;
	/* 0x2FC */ TParamRT<f32> mSLKillerDist;
	/* 0x310 */ TParamRT<s32> mSLBombInterval;
	/* 0x324 */ TParamRT<s32> mSLKillerInterval;
	/* 0x338 */ TParamRT<s32> mSLShootInterval;
	/* 0x34C */ TParamRT<f32> mSLChorobeiAttackRadius;
	/* 0x360 */ TParamRT<f32> mSLChorobeiAttackHeight;
	/* 0x374 */ TParamRT<f32> mSLChorobeiDamageRadius;
	/* 0x388 */ TParamRT<f32> mSLChorobeiDamageHeight;
	/* 0x39C */ TParamRT<f32> mSLKillerTransYOffset;
	/* 0x3B0 */ TParamRT<f32> mSLBombHeiGenerateRate;
	/* 0x3C4 */ TParamRT<f32> mSLThrowXZSpeed;
};

class TCannon;
class TMareGate;

class TCannonDom : public TSharedParts {
public:
	TCannonDom(TLiveActor*, int, SDLModelData*, u32, const char*);

	virtual void perform(u32, JDrama::TGraphics*);

public:
	/* 0x1C */ MAnmSound* unk1C;
	/* 0x20 */ const char* unk20;
	/* 0x24 */ u8 unk24;
	/* 0x25 */ u8 pad25[3];
	/* 0x28 */ f32 unk28;
	/* 0x2C */ f32 unk2C;
	/* 0x30 */ f32 unk30;
};

class TChorobei : public THitActor {
public:
	TChorobei(TCannon*, int, const char*);

	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);

	void checkHit();

public:
	/* 0x68 */ TCannon* unk68;
	/* 0x6C */ TSharedParts* unk6C;
	/* 0x70 */ f32 unk70;
	/* 0x74 */ MAnmSound* unk74;
	/* 0x78 */ const char* unk78;
	/* 0x7C */ f32 unk7C;
};

class TCannon : public TSmallEnemy {
public:
	TCannon(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual MtxPtr getTakingMtx();
	virtual void init(TLiveManager*);
	virtual void calcRootMatrix();
	virtual void moveObject();
	virtual const char** getBasNameTable() const;
	virtual void reset();
	virtual bool isCollidMove(THitActor*) { return false; }
	virtual BOOL isInhibitedForceMove() { return TRUE; }

	void startChorobeiShout();
	bool isObject();
	void setKillerGoalPoint();
	void killerShoot();
	void bombShoot();
	void bombSet();
	virtual bool isHitVallid(u32);

	TCannonSaveLoadParams* getCannonParams() const
	{
		return (TCannonSaveLoadParams*)getSaveParam();
	}

	static u8 mChorobeiJntIdx;
	static u8 mChorobeiHandJntIdx;
	static f32 mVelocityRate;
	static f32 mSearchRate;

public:
	/* 0x194 */ JGeometry::TVec3<f32> unk194;
	/* 0x1A0 */ TSmallEnemy* unk1A0;
	/* 0x1A4 */ TSmallEnemy* unk1A4;
	/* 0x1A8 */ TChorobei* unk1A8;
	/* 0x1AC */ TCannonDom* unk1AC[3];
	/* 0x1B8 */ TCannonDom* unk1B8;
	/* 0x1BC */ TSharedParts* unk1BC;
	/* 0x1C0 */ TMapCollisionMove* unk1C0[3];
	/* 0x1CC */ u8 pad1CC[0x14];
	/* 0x1E0 */ MtxPtr unk1E0;
	/* 0x1E4 */ TPosition3f unk1E4;
	/* 0x214 */ int unk214;
	/* 0x218 */ void* unk218;
	/* 0x21C */ bool unk21C;
	/* 0x220 */ f32 unk220;
	/* 0x224 */ f32 unk224;
	/* 0x228 */ f32 unk228;
	/* 0x22C */ f32 unk22C;
	/* 0x230 */ u8 unk230;
	/* 0x231 */ u8 pad231[3];
	/* 0x234 */ f32 unk234;
	/* 0x238 */ bool unk238;
	/* 0x239 */ bool unk239;
	/* 0x23A */ u8 pad23A[2];
	/* 0x23C */ JGeometry::TVec3<f32> unk23C;
	/* 0x248 */ JGeometry::TVec3<f32> unk248;
	/* 0x254 */ TMareGate* unk254;
	/* 0x258 */ TMapCollisionMove* unk258;
	/* 0x25C */ JGeometry::TVec3<f32> unk25C[4];
	/* 0x28C */ TCannonSaveLoadParams* unk28C;
	/* 0x290 */ bool unk290;
	/* 0x291 */ u8 pad291[3];
	/* 0x294 */ JGeometry::TVec3<f32> unk294;
	/* 0x2A0 */ JGeometry::TVec3<f32> unk2A0;
	/* 0x2AC */ f32 unk2AC;
	/* 0x2B0 */ TMapCollisionMove* unk2B0;
};

class TCannonManager : public TSmallEnemyManager {
public:
	TCannonManager(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual TSmallEnemy* createEnemyInstance();
	virtual void clipEnemies(JDrama::TGraphics*) { }
};

DECLARE_NERVE(TNerveCannonObject, TLiveActor);
DECLARE_NERVE(TNerveCannonDamageDemo, TLiveActor);
DECLARE_NERVE(TNerveCannonDamage, TLiveActor);
DECLARE_NERVE(TNerveCannonClose, TLiveActor);
DECLARE_NERVE(TNerveCannonForceBombShoot, TLiveActor);
DECLARE_NERVE(TNerveCannonShoot, TLiveActor);
DECLARE_NERVE(TNerveCannonSearch, TLiveActor);
DECLARE_NERVE(TNerveCannonOpen, TLiveActor);

#endif
