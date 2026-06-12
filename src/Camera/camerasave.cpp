#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraJetCoaster.hpp>
#include <Camera/CameraShake.hpp>
#include <System/ParamInst.hpp>
#include <System/Params.hpp>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

class TCamSaveKindParam : public TParams {
public:
	TCamSaveKindParam(const char* path);

	/* 0x008 */ TParamRT<f32> mSLFovy;
	/* 0x01C */ TParamRT<f32> mSLNearClip;
	/* 0x030 */ TParamRT<f32> mSLDistMin;
	/* 0x044 */ TParamRT<f32> mSLDistMax;
	/* 0x058 */ TParamRT<f32> mSLCushionMin;
	/* 0x06C */ TParamRT<f32> mSLCushionMax;
	/* 0x080 */ TParamRT<s16> mSLXAngleMin;
	/* 0x094 */ TParamRT<s16> mSLXAngleMax;
	/* 0x0A8 */ TParamRT<f32> mSLXRotRatioManualSpeed;
	/* 0x0BC */ TParamRT<s16> mSLYAngleManualSpeedXMin;
	/* 0x0D0 */ TParamRT<s16> mSLYAngleManualSpeedXMax;
	/* 0x0E4 */ TParamRT<f32> mSLAtOffsetY;
	/* 0x0F8 */ TParamRT<f32> mSLXRotRatioAtOffsetY;
	/* 0x10C */ TParamRT<f32> mSLTargetAtJumpOffsetY;
	/* 0x120 */ TParamRT<f32> mSLAtJumpOffsetSpeed;
	/* 0x134 */ TParamRT<f32> mSLHeightPanChaseRateY;
	/* 0x148 */ TParamRT<f32> mSLSecureViewChase;
	/* 0x15C */ TParamRT<f32> mSLSecureViewDistXMin;
	/* 0x170 */ TParamRT<f32> mSLSecureViewDistZMin;
	/* 0x184 */ TParamRT<f32> mSLSecureViewDistXMax;
	/* 0x198 */ TParamRT<f32> mSLSecureViewDistZMax;
	/* 0x1AC */ TParamRT<f32> mSLHoldAddDistXZMin;
	/* 0x1C0 */ TParamRT<f32> mSLHoldAddDistXZMax;
	/* 0x1D4 */ TParamRT<s16> mSLHoldOffsetAngleXMin;
	/* 0x1E8 */ TParamRT<s16> mSLHoldOffsetAngleXMax;
	/* 0x1FC */ TParamRT<s16> mSLOffsetAngleX;
	/* 0x210 */ TParamRT<s16> mSLOffsetAngleY;
	/* 0x224 */ TParamRT<f32> mSLOffsetLookatXZ;
	/* 0x238 */ TParamRT<s16> mSLMaxAddAngleY;
	/* 0x24C */ TParamRT<s32> mSLAutoChaseStartFrame;
	/* 0x260 */ TParamRT<s32> mSLAutoChaseCompleteFrame;
	/* 0x274 */ TParamRT<f32> mSLFollowSpeedXmin;
	/* 0x288 */ TParamRT<f32> mSLFollowSpeedXmax;
	/* 0x29C */ TParamRT<f32> mSLJumpFollowSpeedXmin;
	/* 0x2B0 */ TParamRT<f32> mSLJumpFollowSpeedXmax;
	/* 0x2C4 */ TParamRT<f32> mSLObstructMaginfXmin;
	/* 0x2D8 */ TParamRT<f32> mSLObstructMaginfXmax;
	/* 0x2EC */ TParamRT<f32> mSLInHouseMaginfXmin;
	/* 0x300 */ TParamRT<f32> mSLInHouseMaginfXmax;
	/* 0x314 */ TParamRT<f32> mSLLFollowMaginfXmin;
	/* 0x328 */ TParamRT<f32> mSLLFollowMaginfXmax;
	/* 0x33C */ TParamRT<f32> mSLPosChaseRateXZ;
	/* 0x350 */ TParamRT<f32> mSLPosChaseRateXZ_C;
	/* 0x364 */ TParamRT<f32> mSLPosChaseRateY;
	/* 0x378 */ TParamRT<f32> mSLPosChaseRateY_C;
	/* 0x38C */ TParamRT<f32> mSLAtChaseRateXZ;
	/* 0x3A0 */ TParamRT<f32> mSLAtChaseRateY;
	/* 0x3B4 */ TParamRT<s16> mSLInbetFollow;
	/* 0x3C8 */ TParamRT<s16> mSLInbetParallel;
	/* 0x3DC */ TParamRT<s16> mSLInbetMultiPlayer;
	/* 0x3F0 */ TParamRT<s16> mSLInbetWallJump;
	/* 0x404 */ TParamRT<s16> mSLInbetHipAttack;
	/* 0x418 */ TParamRT<s16> mSLInbetRocketJump;
	/* 0x42C */ TParamRT<s16> mSLInbetWire;
	/* 0x440 */ TParamRT<s16> mSLInbetLNormal;
	/* 0x454 */ TParamRT<s16> mSLInbetMareUnderGround;
	/* 0x468 */ TParamRT<s16> mSLInbetDefiniteD2;
	/* 0x47C */ TParamRT<s16> mSLInbetTalkE;
	/* 0x490 */ TParamRT<s16> mSLInbetLeanMirror;
	/* 0x4A4 */ TParamRT<s16> mSLInbetTalkA;
	/* 0x4B8 */ TParamRT<s16> mSLInbetUnderGround;
	/* 0x4CC */ TParamRT<s16> mSLInbetIndoor;
	/* 0x4E0 */ TParamRT<s16> mSLInbetHang;
	/* 0x4F4 */ TParamRT<s16> mSLInbetWireHang;
	/* 0x508 */ TParamRT<s16> mSLInbetSandBird;
	/* 0x51C */ TParamRT<s16> mSLInbetHovering;
	/* 0x530 */ TParamRT<s16> mSLInbetJumpCode;
	/* 0x544 */ TParamRT<s16> mSLInbetDelfino;
	/* 0x558 */ TParamRT<s16> mSLInbetClimb;
	/* 0x56C */ TParamRT<s16> mSLInbetFixA;
	/* 0x580 */ TParamRT<s16> mSLInbetFixB;
	/* 0x594 */ TParamRT<s16> mSLInbetFixC;
	/* 0x5A8 */ TParamRT<s16> mSLInbetFixD;
	/* 0x5BC */ TParamRT<s16> mSLInbetFixE;
	/* 0x5D0 */ TParamRT<s16> mSLInbetFixF;
	/* 0x5E4 */ TParamRT<s16> mSLInbetFixG;
	/* 0x5F8 */ TParamRT<s16> mSLInbetFixH;
	/* 0x60C */ TParamRT<s16> mSLInbetDefiniteA;
	/* 0x620 */ TParamRT<s16> mSLInbetDefiniteB;
	/* 0x634 */ TParamRT<s16> mSLInbetDefiniteC;
	/* 0x648 */ TParamRT<s16> mSLInbetDefiniteD;
	/* 0x65C */ TParamRT<s16> mSLInbetDefiniteE;
	/* 0x670 */ TParamRT<s16> mSLInbetDefiniteF;
	/* 0x684 */ TParamRT<s16> mSLInbetDefiniteG;
	/* 0x698 */ TParamRT<s16> mSLInbetDefiniteH;
	/* 0x6AC */ TParamRT<s16> mSLInbetExMap0;
	/* 0x6C0 */ TParamRT<s16> mSLInbetTowerA;
	/* 0x6D4 */ TParamRT<s16> mSLInbetTowerB;
	/* 0x6E8 */ TParamRT<s16> mSLInbetTowerC;
	/* 0x6FC */ TParamRT<s16> mSLInbetSlider;
	/* 0x710 */ TParamRT<s16> mSLInbetDiving;
	/* 0x724 */ TParamRT<s16> mSLInbetTurbo;
	/* 0x738 */ TParamRT<s16> mSLInbetTalkB;
	/* 0x74C */ TParamRT<s16> mSLInbetJetCoaster;
	/* 0x760 */ TParamRT<s16> mSLInbetParallelB;
	/* 0x774 */ TParamRT<s16> mSLInbetSurfing;
	/* 0x788 */ TParamRT<s16> mSLInbetSwimming;
	/* 0x79C */ TParamRT<s16> mSLInbetClimbJump;
	/* 0x7B0 */ TParamRT<s16> mSLInbetLookDown;
	/* 0x7C4 */ TParamRT<s16> mSLInbetRailFence;
	/* 0x7D8 */ TParamRT<s16> mSLInbetFollowB;
	/* 0x7EC */ TParamRT<s16> mSLInbetFollowC;
	/* 0x800 */ TParamRT<s16> mSLInbetTowerD;
	/* 0x814 */ TParamRT<s16> mSLInbetDelfinoAttic;
	/* 0x828 */ TParamRT<s16> mSLInbetBossGeso;
	/* 0x83C */ TParamRT<s16> mSLInbetFixI;
	/* 0x850 */ TParamRT<s16> mSLInbetDefiniteI;
	/* 0x864 */ TParamRT<s16> mSLInbetFence;
	/* 0x878 */ TParamRT<s16> mSLInbetMonteFence;
	/* 0x88C */ TParamRT<s16> mSLInbetMonteHang;
	/* 0x8A0 */ TParamRT<s16> mSLInbetTalkC;
	/* 0x8B4 */ TParamRT<s16> mSLInbetTalkD;
	/* 0x8C8 */ TParamRT<s16> mSLInbetTowerE;
	/* 0x8DC */ TParamRT<s16> mSLInbetDelfinoB;
	/* 0x8F0 */ TParamRT<s16> mSLInbetCancan;
	/* 0x904 */ TParamRT<s16> mSLInbetAquaticTurbo;
	/* 0x918 */ TParamRT<s16> mSLInbetFollowD;
	/* 0x92C */ TParamRT<s16> mSLInbetFollowE;
	/* 0x940 */ TParamRT<s16> mSLInbetParallelC;
	/* 0x954 */ TParamRT<s16> mSLInbetParallelD;
};

class TCamSaveEx : public TParams {
public:
	TCamSaveEx();

	/* 0x008 */ TParamRT<f32> mXRotStart;
	/* 0x01C */ TParamRT<f32> mPanAfterMagnif;
	/* 0x030 */ TParamRT<f32> mPanAfterMinHeight;
	/* 0x044 */ TParamRT<s16> mPanWarpAngleX;
	/* 0x058 */ TParamRT<f32> mSLMinCushionXZ;
	/* 0x06C */ TParamRT<f32> mSLWallCheckRadius;
	/* 0x080 */ TParamRT<f32> mSLWallRevisionRatio;
	/* 0x094 */ TParamRT<f32> mGroundChangeY;
	/* 0x0A8 */ TParamRT<f32> mSLGroundHeightNormal;
	/* 0x0BC */ TParamRT<f32> mSLGroundHeightReadyGun;
	/* 0x0D0 */ TParamRT<f32> mSLRoofChangeY;
	/* 0x0E4 */ TParamRT<f32> mSLRoofHeight;
	/* 0x0F8 */ TParamRT<u8>  mInHouseMinFrame;
	/* 0x10C */ TParamRT<s16> mYButtonRotateChase;
	/* 0x120 */ TParamRT<s16> mLButtonRotateChase;
	/* 0x134 */ TParamRT<s16> mSLAddAngleYSpeed;
	/* 0x148 */ TParamRT<f32> mSLReproduceDemoNearClip;
	/* 0x15C */ TParamRT<s16> mSLHoldAngleXChase;
	/* 0x170 */ TParamRT<f32> mSLHoldDistChase;
	/* 0x184 */ TParamRT<s16> mSLAimAngleYChaseMin;
	/* 0x198 */ TParamRT<s16> mSLAimAngleYChaseMax;
	/* 0x1AC */ TParamRT<s16> mSLLimitMinAngleX;
	/* 0x1C0 */ TParamRT<s16> mSLLimitMaxAngleX;
	/* 0x1D4 */ TParamRT<s16> mSLSlopeMaxAngleX;
	/* 0x1E8 */ TParamRT<s16> mSLSlopeSpeedAngleX;
	/* 0x1FC */ TParamRT<f32> mSLSlopeForwardDistXZ;
};

// TCamSaveShake is declared in <Camera/CameraShake.hpp> so camerashake.cpp can
// use `new TCamSaveShake(name)`.

class TCamSaveNotice : public TParams {
public:
	TCamSaveNotice();

	/* 0x08 */ TParamRT<f32> mOnDist;
	/* 0x1C */ TParamRT<f32> mOffDist;
	/* 0x30 */ TParamRT<f32> mOnClipRatio;
	/* 0x44 */ TParamRT<f32> mOffClipRatio;
	/* 0x58 */ TParamRT<f32> mOnDegree;
	/* 0x6C */ TParamRT<s16> mRotateYSpeed;
	/* 0x80 */ TParamRT<f32> mRotateMinDistXZ;
	/* 0x94 */ TParamRT<f32> mRotateFastMinDistXZ;
	/* 0xA8 */ TParamRT<f32> mRotateMagnifXmax;
};

TCamSaveKindParam::TCamSaveKindParam(const char* path)
    : TParams(path)
    , PARAM_INIT(mSLFovy, 50.0f)
    , PARAM_INIT(mSLNearClip, 10.0f)
    , PARAM_INIT(mSLDistMin, 450.0f)
    , PARAM_INIT(mSLDistMax, 2000.0f)
    , PARAM_INIT(mSLCushionMin, 350.0f)
    , PARAM_INIT(mSLCushionMax, 600.0f)
    , PARAM_INIT(mSLXAngleMin, 894)
    , PARAM_INIT(mSLXAngleMax, 10000)
    , PARAM_INIT(mSLXRotRatioManualSpeed, 0.01f)
    , PARAM_INIT(mSLYAngleManualSpeedXMin, 250)
    , PARAM_INIT(mSLYAngleManualSpeedXMax, 250)
    , PARAM_INIT(mSLAtOffsetY, 133.398f)
    , PARAM_INIT(mSLXRotRatioAtOffsetY, 0.0f)
    , PARAM_INIT(mSLTargetAtJumpOffsetY, -100.0f)
    , PARAM_INIT(mSLAtJumpOffsetSpeed, 20.0f)
    , PARAM_INIT(mSLHeightPanChaseRateY, 0.0f)
    , PARAM_INIT(mSLSecureViewChase, 0.01f)
    , PARAM_INIT(mSLSecureViewDistXMin, 0.0f)
    , PARAM_INIT(mSLSecureViewDistZMin, 0.0f)
    , PARAM_INIT(mSLSecureViewDistXMax, 0.0f)
    , PARAM_INIT(mSLSecureViewDistZMax, 0.0f)
    , PARAM_INIT(mSLHoldAddDistXZMin, 350.0f)
    , PARAM_INIT(mSLHoldAddDistXZMax, 0.0f)
    , PARAM_INIT(mSLHoldOffsetAngleXMin, 0)
    , PARAM_INIT(mSLHoldOffsetAngleXMax, 0)
    , PARAM_INIT(mSLOffsetAngleX, 0)
    , PARAM_INIT(mSLOffsetAngleY, 0)
    , PARAM_INIT(mSLOffsetLookatXZ, 0.0f)
    , PARAM_INIT(mSLMaxAddAngleY, 0)
    , PARAM_INIT(mSLAutoChaseStartFrame, 0)
    , PARAM_INIT(mSLAutoChaseCompleteFrame, 0)
    , PARAM_INIT(mSLFollowSpeedXmin, 3.5f)
    , PARAM_INIT(mSLFollowSpeedXmax, 1.0f)
    , PARAM_INIT(mSLJumpFollowSpeedXmin, 2.0f)
    , PARAM_INIT(mSLJumpFollowSpeedXmax, 0.0f)
    , PARAM_INIT(mSLObstructMaginfXmin, 1.5f)
    , PARAM_INIT(mSLObstructMaginfXmax, 1.2f)
    , PARAM_INIT(mSLInHouseMaginfXmin, 1.7f)
    , PARAM_INIT(mSLInHouseMaginfXmax, 1.3f)
    , PARAM_INIT(mSLLFollowMaginfXmin, 4.0f)
    , PARAM_INIT(mSLLFollowMaginfXmax, 1.5f)
    , PARAM_INIT(mSLPosChaseRateXZ, 0.04f)
    , PARAM_INIT(mSLPosChaseRateXZ_C, 0.3f)
    , PARAM_INIT(mSLPosChaseRateY, 0.08f)
    , PARAM_INIT(mSLPosChaseRateY_C, 0.3f)
    , PARAM_INIT(mSLAtChaseRateXZ, 0.1f)
    , PARAM_INIT(mSLAtChaseRateY, 0.03f)
    , PARAM_INIT(mSLInbetFollow, 16)
    , PARAM_INIT(mSLInbetParallel, 16)
    , PARAM_INIT(mSLInbetMultiPlayer, 16)
    , PARAM_INIT(mSLInbetWallJump, 16)
    , PARAM_INIT(mSLInbetHipAttack, 16)
    , PARAM_INIT(mSLInbetRocketJump, 16)
    , PARAM_INIT(mSLInbetWire, 16)
    , PARAM_INIT(mSLInbetLNormal, 16)
    , PARAM_INIT(mSLInbetMareUnderGround, 16)
    , PARAM_INIT(mSLInbetDefiniteD2, 16)
    , PARAM_INIT(mSLInbetTalkE, 16)
    , PARAM_INIT(mSLInbetLeanMirror, 16)
    , PARAM_INIT(mSLInbetTalkA, 16)
    , PARAM_INIT(mSLInbetUnderGround, 16)
    , PARAM_INIT(mSLInbetIndoor, 16)
    , PARAM_INIT(mSLInbetHang, 16)
    , PARAM_INIT(mSLInbetWireHang, 16)
    , PARAM_INIT(mSLInbetSandBird, 16)
    , PARAM_INIT(mSLInbetHovering, 16)
    , PARAM_INIT(mSLInbetJumpCode, 16)
    , PARAM_INIT(mSLInbetDelfino, 16)
    , PARAM_INIT(mSLInbetClimb, 16)
    , PARAM_INIT(mSLInbetFixA, 16)
    , PARAM_INIT(mSLInbetFixB, 16)
    , PARAM_INIT(mSLInbetFixC, 1)
    , PARAM_INIT(mSLInbetFixD, 1)
    , PARAM_INIT(mSLInbetFixE, 1)
    , PARAM_INIT(mSLInbetFixF, 1)
    , PARAM_INIT(mSLInbetFixG, 1)
    , PARAM_INIT(mSLInbetFixH, 1)
    , PARAM_INIT(mSLInbetDefiniteA, 16)
    , PARAM_INIT(mSLInbetDefiniteB, 16)
    , PARAM_INIT(mSLInbetDefiniteC, 1)
    , PARAM_INIT(mSLInbetDefiniteD, 1)
    , PARAM_INIT(mSLInbetDefiniteE, 1)
    , PARAM_INIT(mSLInbetDefiniteF, 1)
    , PARAM_INIT(mSLInbetDefiniteG, 1)
    , PARAM_INIT(mSLInbetDefiniteH, 1)
    , PARAM_INIT(mSLInbetExMap0, 16)
    , PARAM_INIT(mSLInbetTowerA, 16)
    , PARAM_INIT(mSLInbetTowerB, 16)
    , PARAM_INIT(mSLInbetTowerC, 16)
    , PARAM_INIT(mSLInbetSlider, 16)
    , PARAM_INIT(mSLInbetDiving, 16)
    , PARAM_INIT(mSLInbetTurbo, 16)
    , PARAM_INIT(mSLInbetTalkB, 16)
    , PARAM_INIT(mSLInbetJetCoaster, 16)
    , PARAM_INIT(mSLInbetParallelB, 16)
    , PARAM_INIT(mSLInbetSurfing, 16)
    , PARAM_INIT(mSLInbetSwimming, 16)
    , PARAM_INIT(mSLInbetClimbJump, 16)
    , PARAM_INIT(mSLInbetLookDown, 16)
    , PARAM_INIT(mSLInbetRailFence, 16)
    , PARAM_INIT(mSLInbetFollowB, 16)
    , PARAM_INIT(mSLInbetFollowC, 16)
    , PARAM_INIT(mSLInbetTowerD, 16)
    , PARAM_INIT(mSLInbetDelfinoAttic, 16)
    , PARAM_INIT(mSLInbetBossGeso, 16)
    , PARAM_INIT(mSLInbetFixI, 16)
    , PARAM_INIT(mSLInbetDefiniteI, 16)
    , PARAM_INIT(mSLInbetFence, 16)
    , PARAM_INIT(mSLInbetMonteFence, 16)
    , PARAM_INIT(mSLInbetMonteHang, 16)
    , PARAM_INIT(mSLInbetTalkC, 16)
    , PARAM_INIT(mSLInbetTalkD, 16)
    , PARAM_INIT(mSLInbetTowerE, 16)
    , PARAM_INIT(mSLInbetDelfinoB, 16)
    , PARAM_INIT(mSLInbetCancan, 16)
    , PARAM_INIT(mSLInbetAquaticTurbo, 16)
    , PARAM_INIT(mSLInbetFollowD, 16)
    , PARAM_INIT(mSLInbetFollowE, 16)
    , PARAM_INIT(mSLInbetParallelC, 16)
    , PARAM_INIT(mSLInbetParallelD, 16)
{
	TParams::load(mPrmPath);
}

TCamSaveEx::TCamSaveEx()
    : TParams("/Camera/camera_normal.prm")
    , PARAM_INIT(mXRotStart, 0.2f)
    , PARAM_INIT(mPanAfterMagnif, 0.01f)
    , PARAM_INIT(mPanAfterMinHeight, 1.5f)
    , PARAM_INIT(mPanWarpAngleX, -10000)
    , PARAM_INIT(mSLMinCushionXZ, 300.0f)
    , PARAM_INIT(mSLWallCheckRadius, 80.0f)
    , PARAM_INIT(mSLWallRevisionRatio, 1.0f)
    , PARAM_INIT(mGroundChangeY, 20.0f)
    , PARAM_INIT(mSLGroundHeightNormal, 144.0f)
    , PARAM_INIT(mSLGroundHeightReadyGun, 30.0f)
    , PARAM_INIT(mSLRoofChangeY, 20.0f)
    , PARAM_INIT(mSLRoofHeight, 60.0f)
    , PARAM_INIT(mInHouseMinFrame, 24)
    , PARAM_INIT(mYButtonRotateChase, 2)
    , PARAM_INIT(mLButtonRotateChase, 10)
    , PARAM_INIT(mSLAddAngleYSpeed, 50)
    , PARAM_INIT(mSLReproduceDemoNearClip, 30.0f)
    , PARAM_INIT(mSLHoldAngleXChase, 50)
    , PARAM_INIT(mSLHoldDistChase, 0.02f)
    , PARAM_INIT(mSLAimAngleYChaseMin, 60)
    , PARAM_INIT(mSLAimAngleYChaseMax, 32767)
    , PARAM_INIT(mSLLimitMinAngleX, -15872)
    , PARAM_INIT(mSLLimitMaxAngleX, 15872)
    , PARAM_INIT(mSLSlopeMaxAngleX, 8192)
    , PARAM_INIT(mSLSlopeSpeedAngleX, 50)
    , PARAM_INIT(mSLSlopeForwardDistXZ, 50.0f)
{
	TParams::load(mPrmPath);
}

TCamSaveJetCoaster::TCamSaveJetCoaster()
    : TParams("/Camera/camera_jetCoaster.prm")
    , PARAM_INIT(mSLOffsetAngleXLimit, 10000)
    , PARAM_INIT(mSLOffsetAngleYLimit, 10000)
    , PARAM_INIT(mSLOffsetAngleXManualSpeed, 250)
    , PARAM_INIT(mSLOffsetAngleYManualSpeed, 250)
    , PARAM_INIT(mSLOffsetAngleXChase, 25)
    , PARAM_INIT(mSLOffsetAngleYChase, 25)
{
	TParams::load(mPrmPath);
}

TCamSaveShake::TCamSaveShake(const char* path)
    : TParams(path)
    , PARAM_INIT(mShakeTime, 40)
    , PARAM_INIT(mShakeAmpX, 500.0f)
    , PARAM_INIT(mShakeVelX, 5000)
    , PARAM_INIT(mShakeAmpY, 0.0f)
    , PARAM_INIT(mShakeVelY, 0)
    , PARAM_INIT(mShakeAmpZ, 0.0f)
    , PARAM_INIT(mShakeVelZ, 0)
{
	TParams::load(mPrmPath);
}

TCamSaveNotice::TCamSaveNotice()
    : TParams("/Camera/camera_bossA.prm")
    , PARAM_INIT(mOnDist, 2500.0f)
    , PARAM_INIT(mOffDist, 4000.0f)
    , PARAM_INIT(mOnClipRatio, 1.0f)
    , PARAM_INIT(mOffClipRatio, 3.0f)
    , PARAM_INIT(mOnDegree, 150.0f)
    , PARAM_INIT(mRotateYSpeed, 300)
    , PARAM_INIT(mRotateMinDistXZ, 20.0f)
    , PARAM_INIT(mRotateFastMinDistXZ, 500.0f)
    , PARAM_INIT(mRotateMagnifXmax, 1.0f)
{
	TParams::load(mPrmPath);
}
