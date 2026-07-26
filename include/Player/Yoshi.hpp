#ifndef YOSHI_HPP
#define YOSHI_HPP

#include <JSystem/JGeometry.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <M3DUtil/MActor.hpp>

class TMario;
class TEggYoshi;
class TYoshiTongue;
struct TRidingInfo;
class J3DDrawBuffer;
class MAnmSound;

class TYoshi {
public:
	enum Color { GREEN, ORANGE, PURPLE, PINK };
	enum State { EGG = 0, DROWNING = 3, DYING = 4, UNMOUNTED = 6, MOUNTED = 8 };

	bool appearFromEgg(const JGeometry::TVec3<f32>&, f32, TEggYoshi*);
	void calcAnim();
	void changeAnimation(int id);
	bool disappear();
	void doEat(u32 fruitID);
	void doSearch();
	void emitTongue();
	void entry();
	void getEmitPosDir(JGeometry::TVec3<f32>*, JGeometry::TVec3<f32>*) const;
	J3DFrameCtrl* getFrameCtrl() const;
	MtxPtr getMtxPtrFootL() const;
	MtxPtr getMtxPtrFootR() const;
	const JGeometry::TVec3<f32>& getTranslation() const { return mTranslation; }
	void getOff(bool knockedOff);
	void init(TMario*);
	void initInLoadAfter();
	BOOL isHatched() const { return mState == EGG ? FALSE : TRUE; }
	void kill();
	void movement();
	BOOL onYoshi();
	void ride();
	void setEggYoshiPtr(TEggYoshi*);
	void thinkAnimation();
	void thinkBtp(int);
	void thinkEat();
	void thinkHoldOut();
	void thinkUpper();
	void viewCalc();

	u8 mState;                          // 0x0000
	u16 mSubState;                      // 0x0002 ??
	u32 _01;                            // 0x0004
	s32 mMaxJuice;                      // 0x0008
	s32 mCurJuice;                      // 0x000C
	TMario* mMario;                     // 0x0010
	u32 _02[0xC / 4];                   // 0x0014
	JGeometry::TVec3<f32> mTranslation; // 0x0020
	u32 _03[0x8 / 4];                   // 0x002C
	MActor* mActor;                     // 0x0034
	TYoshiTongue* mTongue;              // 0x0038
	u16 mJoint;                         // 0x003c
	u16 _3e;                            // 0x003e
	u16 mJointFootR;                    // 0x0040
	u16 mJointCenter;                   // 0x0042
	J3DModel* mHandL;                   // 0x0044
	J3DModel* mHandR;                   // 0x0048
	u32 _04[0x24 / 4];                  // 0x004C
	s16 mEggRotSpeed;                   // 0x0070
	u16 _72;                            // 0x0072
	u32 _04b[0x10 / 4];                 // 0x0074
	f32 mRedComponent;                  // 0x0084
	f32 mGreenComponent;                // 0x0088
	f32 mBlueComponent;                 // 0x008C
	f32 mTongueSearchLength;            // 0x0090
	TRidingInfo* mRidingInfo;           // 0x0094
	f32 _98;                            // 0x0098
	f32 _9C;                            // 0x009C
	f32 _A0;                            // 0x00A0
	f32 _A4;                            // 0x00A4
	J3DDrawBuffer* mOpaDrawBuffer;      // 0x00A8
	J3DDrawBuffer* mXluDrawBuffer;      // 0x00AC
	u32 _B0;                            // 0x00B0
	u32 _B4;                            // 0x00B4
	u8 mFlutterState;                   // 0x00B8
	u8 _06;                             // 0x00B9
	u16 mFlutterTimer;                  // 0x00BA
	u16 mMaxFlutterTimer;               // 0x00BC
	u16 _07;                            // 0x00BE
	f32 mMaxVSpdStartFlutter;           // 0x00C0
	f32 mFlutterAcceleration;           // 0x00C4
	u32 _08[0x8 / 4];                   // 0x00C8
	u8 mType;                           // 0x00D0
	u8 _09;                             // 0x00D1
	u16 _10;                            // 0x00D2
	union {
		u32 _11[0x1C / 4];              // 0x00D4
		struct {
			u32 _D4;                    // 0x00D4
			u32 _D8;                    // 0x00D8
			u8 mState;                  // 0x00DC
			u8 _DD;                     // 0x00DD
			s16 mWait;                  // 0x00DE
			u16 mTargetAngle;           // 0x00E0
			u16 _E2;                    // 0x00E2
			f32 mTurnRate;              // 0x00E4
			s16 mWaitMin;               // 0x00E8
			s16 mWaitMax;               // 0x00EA
			f32 _EC;                    // 0x00EC
		} mSearch;
	};
	TEggYoshi* mEgg;                    // 0x00F0
	u16 mCurBtpIdx;                     // 0x00F4
	u16 mEmitJoint;                     // 0x00F6
	u16 mFootLJoint2;                   // 0x00F8
	u16 _FA;                            // 0x00FA
	JGeometry::TVec3<f32> mMtxTrans;    // 0x00FC
	JGeometry::TVec3<f32> mMtxTrans2;   // 0x0108
	f32 mSpineScale;                    // 0x0114
	MAnmSound* mBckPlayer;              // 0x0118
	void** mAnimFrameRates;             // 0x011C
	MAnmSound* mBckPlayer2;             // 0x0120
	u32 _124;
};

#endif

#if defined(PLAYER_YOSHI_DEFINE_ON_YOSHI) \
    && !defined(PLAYER_YOSHI_ON_YOSHI_DEFINED)
#define PLAYER_YOSHI_ON_YOSHI_DEFINED

BOOL TYoshi::onYoshi() { return (u8)mState == MOUNTED ? 1 : 0; }

#endif
