#ifndef CAMERA_CAMERA_HPP
#define CAMERA_CAMERA_HPP

#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/JDrama/JDRCamera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/CameraMode.hpp>
#include <Camera/cameralib.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <dolphin/mtx.h>

class TBaseNPC;
class TCameraMapTool;
class TCameraKindParam;
class TCameraInbetween;
class TCameraBck;
class TMarioGamePad;
class TCamSaveKindParam;
class TCamSaveNotice;
class TCamSaveEx;
class TLiveActor;

class TCameraJetCoaster;
class TCameraMultiPlayer;

class CPolarSubCamera : public JDrama::TLookAtCamera {
public:
	struct TCameraTargetState {
		/* 0x00 */ JGeometry::TVec3<f32> mPosition;
		/* 0x0C */ JGeometry::TVec3<f32> mTarget;
		/* 0x18 */ JGeometry::TVec3<f32> unk18;
		/* 0x24 */ s16 mPitch;
		/* 0x26 */ s16 mYaw;
		/* 0x28 */ f32 unk28;
		/* 0x2C */ s16 unk2C;
		/* 0x2E */ char unk2E[0x30 - 0x2E];
		/* 0x30 */ f32 unk30;
	};

	enum {
		CAMERA_FLAG_UNK1           = 0x1,
		CAMERA_FLAG_UNK2           = 0x2,
		CAMERA_FLAG_NOTICE_ACTIVE  = 0x20,
		CAMERA_FLAG_GATE_DEMO      = 0x200,
		CAMERA_FLAG_DEAD_DEMO      = 0x400,
		CAMERA_FLAG_HELL_DEAD_DEMO = 0x800,
	};

	enum EnumNoticeOnOffMode {
		NOTICE_MODE_UNK0 = 0,
		NOTICE_MODE_UNK1 = 1,
		NOTICE_MODE_UNK2 = 2,
	};

	void calcSecureViewTarget_(s16, f32*, f32*);
	void execSecureView_(s16, Vec*);
	bool isLButtonCameraSpecifyMode(int) const;
	bool isLButtonCameraInbetween() const;
	bool isJetCoaster1stCamera() const;
	bool isTalkCameraSpecifyMode(int) const;
	bool isTalkCameraInbetween() const;
	bool isNormalCameraSpecifyMode(int) const;
	bool isNormalCameraCompletely() const;
	bool isTowerCameraSpecifyMode(int) const;
	bool isFollowCameraSpecifyMode(int) const;
	bool isRailCameraSpecifyMode(int) const;
	bool isFixCameraSpecifyMode(int) const;
	bool isDefiniteCameraSpecifyMode(int) const;
	bool isOverHipAttackSpecifyMode(int) const;
	bool isSlopeCameraMode() const;
	void warpPosAndAt(const Vec&, const Vec&);
	void warpPosAndAt(f32, s16);
	void addMoveCameraAndMario(const Vec&);
	bool startReproduceDemoCamera_(const char*, const JGeometry::TVec3<f32>*);
	void restartReproduceDemoCamera_();
	void endReproduceDemoCamera_();
	void endSimpleDemoCamera_();
	void updateDemoCamera_(bool);
	void updateGateDemoCamera_();
	void startGateDemoCamera(const JDrama::TActor*);
	void startDemoCamera(const char*, const JGeometry::TVec3<f32>*, s32, f32,
	                     bool);
	void endDemoCamera();
	bool isSimpleDemoCamera() const;
	// fabricated inline -- actual definition unknown, always inlined.
	// The `? true : false` trick forces bool materialization when inlined
	// (otherwise MWCC inlines straight to cmpwi/beq).
	bool isOnGoingDemoCamera() const
	{
		return (mMode == 0x49) ? true : false;
	}
	bool isBckDemoCamera() const
	{
		return mMode == CAMERA_MODE_REPRODUCE_DEMO ? true : false;
	}
	int getTotalDemoFrames() const;
	int getRestDemoFrames() const;
	void ctrlNormalDeadDemo_();
	void execDeadDemoProc_();
	bool isHellDeadDemo() const;
	bool isNormalDeadDemo() const;
	void chaseOptionCamera_(f32);
	void ctrlOptionCamera_();
	void ctrlJetCoasterCamera_();
	inline void drawJetCoasterBalloonMessage_();
	void createMultiPlayer(u8);
	bool addMultiPlayer(const JGeometry::TVec3<f32>*, f32, f32);
	bool removeMultiPlayer(const JGeometry::TVec3<f32>*);
	void ctrlMultiPlayerCamera_();
	void makeMtxForTalk(const TBaseNPC*);
	void makeMtxForPrevTalk();
	void ctrlTalkCamera_();
	void calcTowerCenterPos_(Vec*);
	void ctrlNormalOrTowerCamera_();
	void setNoticeInfo();
	void* getNoticeActor_();
	void execNoticeOnOffProc_(CPolarSubCamera::EnumNoticeOnOffMode);
	void calcNoticeTargetYrot_(const Vec&);
	void getNozzleTopPos_(JGeometry::TVec3<f32>*) const;
	void ctrlLButtonCamera_();
	void killHeightPanWhenChangeCamMode_();
	void execHeightPan_();
	void killHeightPan_();

	bool isNotHeightPanCamMode_() const
	{
		bool result = false;
		if (isLButtonCameraSpecifyMode(mMode)
		    || isRailCameraSpecifyMode(mMode)) {
			result = true;
		} else {
			switch (mMode) {
			case CAMERA_MODE_MARE_UNDER_GROUND:
			case CAMERA_MODE_UNDER_GROUND:
			case CAMERA_MODE_HANG:
			case CAMERA_MODE_HOVERING:
			case CAMERA_MODE_JUMP_CODE:
			case CAMERA_MODE_DIVING:
			case CAMERA_MODE_SWIMMING:
			case CAMERA_MODE_LOOK_DOWN:
			case CAMERA_MODE_MONTE_HANG:
			case CAMERA_MODE_TOWER_E:
				result = true;
			}
		}
		return result;
	}

	bool fabricatedInline()
	{
		bool result = false;
		if (!isNotHeightPanCamMode_() && !SMS_IsMarioTouchGround4cm()
		    && !gpCameraMario->isMarioGoDown() && !SMS_IsMarioOnWire()
		    && SMS_GetMarioStatus() != 0x200345) {
			result = true;
		}
		return result;
	}

	void fabricatedInline2()
	{
		CLBCrossToPolar(mTarget, mPosition, &unk256, &unk258);

		unk25C.set(unk148.x - unk124.x, unk148.y - unk124.y,
		           unk148.z - unk124.z);
		unk25C.normalize();
		unk270 = MsClamp(CLBCalcRatio(mCurrentParams->mXAngleMin,
		                              mCurrentParams->mXAngleMax, unk256),
		                 0.0f, 1.0f);
	}

	CPolarSubCamera(const char* = "<CPolarCamera>");
	void startJetCoasterCam1();
	void loadAfter();
	bool isNowInbetween() const;
	MtxPtr getToroccoMtx_() const;
	void setMarioLookat_();
	JGeometry::TVec3<f32> getUsualLookat() const;
	s16 calcAngleXFromXRotRatio_() const;
	f32 calcDistFromXRotRatio_() const;
	void calcNowTargetFromPosAndAt_(const Vec&, const Vec&);
	void rotateX_ByStickY_(f32);
	void rotateY_ByStickX_(f32);
	void offMoveApproach_();
	void onMoveApproach_();
	bool isMarioReadyGun_() const;
	bool isMarioAimWithGun_() const;
	bool isMarioCrabWalk_() const;
	void execInvalidAutoChase_();
	bool isMomentDefinite_() const;
	void calcSlopeAngleX_(s16*);
	void calcPosAndAt_();
	void calcFinalPosAndAt_();
	void calcExternalData_();
	void ctrlGameCamera_();
	void perform(u32, JDrama::TGraphics*);
	s16 getOffsetAngleX() const;
	s16 getOffsetAngleY() const;
	s16 getFinalAngleZ() const;
	MtxPtr getUnk1EC() { return unk1EC; }
	MtxPtr getUnk16C() { return unk16C; }
	~CPolarSubCamera();
	bool controlByCameraCode_(int*);
	void getLButtonCameraModeByNozzle_();
	int getCameraInbetweenFrame_(int);
	void setUpToLButtonCamera_(int);
	void setUpFromLButtonCamera_();
	void changeCamModeSub_(int, int, bool);
	void changeCamModeSpecifyFrame_(int, int);
	void changeCamModeSpecifyCamMapTool_(const TCameraMapTool*);
	void changeCamModeSpecifyCamMapToolAndFrame_(const TCameraMapTool*, int);
	void execFrontRotate_();
	void doLButtonCameraOn_();
	void doLButtonCameraOff_(bool);
	bool isChangeToBossGesoCamera_() const;
	bool isChangeToCancanCamera_() const;
	bool isChangeToParallelCameraByMoveBG_() const;
	bool isChangeToParallelCameraCByMoveBG_() const;
	void execCameraModeChangeProc_(int);
	void calcInHouseNoSub_();
	void calcInHouseNo_(bool);
	bool isNeedGroundCheck_();
	bool isNeedRoofCheck_() const;
	bool isNeedWallCheck_() const;
	bool execWallCheck_(Vec*);
	bool execRoofCheck_(Vec);
	bool execGroundCheck_(Vec);

	s16 getUnk2C8() const { return unk2C8; }

public:
	/* 0x50 */ int mMode;
	/* 0x54 */ union {
		int unk54;
		int mPrevMode;
	};
	/* 0x58 */ u32 unk58;
	/* 0x5C */ int unk5C;
	/* 0x60 */ void* unk60;
	/* 0x64 */ u16 unk64;
	/* 0x66 */ char unk66[0x68 - 0x66];
	/* 0x68 */ union {
		TCameraKindParam* unk68;
		TCameraKindParam* mCurrentParams;
	};
	/* 0x6C */ TCameraInbetween* unk6C;
	/* 0x70 */ TCameraMapTool* unk70;
	/* 0x74 */ TCameraMapTool* unk74;
	/* 0x78 */ u32 unk78;
	/* 0x7C */ u32 unk7C;
	/* 0x80 */ TCameraTargetState mCurrentTarget;
	/* 0xB4 */ TCameraTargetState mPreviousTarget;
	/* 0xE8 */ JGeometry::TVec3<f32> unkE8;
	/* 0xF4 */ JGeometry::TVec3<f32> unkF4;
	/* 0x100 */ JGeometry::TVec3<f32> unk100;
	/* 0x10C */ s16 unk10C;
	/* 0x10E */ s16 unk10E;
	/* 0x110 */ f32 unk110;
	/* 0x114 */ s16 unk114;
	/* 0x116 */ char unk116[0x118 - 0x116];
	/* 0x118 */ f32 unk118;
	/* 0x11C */ void* unk11C;
	/* 0x120 */ TMarioGamePad* unk120;
	/* 0x124 */ JGeometry::TVec3<f32> unk124;
	/* 0x130 */ JGeometry::TVec3<f32> unk130;
	/* 0x13C */ JGeometry::TVec3<f32> unk13C;
	/* 0x148 */ JGeometry::TVec3<f32> unk148;
	/* 0x154 */ JGeometry::TVec3<f32> unk154;
	/* 0x160 */ JGeometry::TVec3<f32> unk160;
	/* 0x16C */ Mtx44 unk16C;
	/* 0x1AC */ Mtx44 unk1AC;
	/* 0x1EC */ Mtx unk1EC;
	/* 0x21C */ Mtx unk21C;
	/* 0x24C */ union {
		f32 unk24C;
		f32 mHeightPanOffset;
	};
	/* 0x250 */ f32 unk250;
	/* 0x254 */ s16 unk254;
	/* 0x256 */ s16 unk256;
	/* 0x258 */ s16 unk258;
	/* 0x25A */ char unk25A[0x25C - 0x25A];
	/* 0x25C */ JGeometry::TVec3<f32> unk25C;
	/* 0x268 */ f32 unk268;
	/* 0x26C */ f32 unk26C;
	/* 0x270 */ f32 unk270;
	/* 0x274 */ char unk274[0x278 - 0x274];
	/* 0x278 */ u16 unk278;
	/* 0x27A */ u16 unk27A;
	/* 0x27C */ u16 mDeadDemoCountdown;
	/* 0x27E */ u16 mDeadDemoCountdownToFovZoom;
	/* 0x280 */ u16 mDeadDemoFovZoomTimer;
	/* 0x282 */ u16 unk282;
	/* 0x284 */ s32 unk284;
	/* 0x288 */ f32 unk288;
	/* 0x28C */ s16 unk28C;
	/* 0x28E */ s16 unk28E;
	/* 0x290 */ f32 unk290;
	/* 0x294 */ f32 unk294;
	/* 0x298 */ f32 unk298;
	/* 0x29C */ s32 unk29C;
	/* 0x2A0 */ TLiveActor** unk2A0;
	/* 0x2A4 */ union {
		void* unk2A4;
		TLiveActor* mNoticeActor;
	};
	/* 0x2A8 */ TLiveActor* unk2A8;
	/* 0x2AC */ void* unk2AC;
	/* 0x2B0 */ TCameraBck* unk2B0;

	struct TCameraDemo {
		TCameraDemo()
		    : unk0(0)
		    , unk4(0.0f)
		    , unk8(0)
		    , unkC(0)
		    , mTotalFrames(0)
		    , mRemainingFrames(0)
		{
		}

		void setLengthFrames(int frames)
		{
			mTotalFrames     = frames;
			mRemainingFrames = frames;
		}

		/* 0x00 */ const JGeometry::TVec3<f32>* unk0;
		/* 0x04 */ f32 unk4;
		/* 0x08 */ TCameraMapTool* unk8;
		/* 0x0C */ u8 unkC;
		/* 0x10 */ int mTotalFrames;
		/* 0x14 */ int mRemainingFrames;
	};

	/* 0x2B4 */ union {
		void* unk2B4;
		TCameraDemo* mCameraDemo;
	};
	/* 0x2B8 */ TCameraJetCoaster* unk2B8;
	/* 0x2BC */ TCameraMultiPlayer* unk2BC;
	/* 0x2C0 */ f32 unk2C0;
	/* 0x2C4 */ f32 unk2C4;
	/* 0x2C8 */ s16 unk2C8;
	/* 0x2CA */ s16 unk2CA;
	/* 0x2CC */ u8 unk2CC;
	/* 0x2CD */ char unk2CD[0x2D0 - 0x2CD];
	/* 0x2D0 */ union {
		void* unk2D0;
		TCamSaveNotice* mSaveNotice;
	};
	/* 0x2D4 */ union {
		void* unk2D4;
		TCamSaveEx* mSaveEx;
	};
	/* 0x2D8 */ TCamSaveKindParam* unk2D8[0x49];

	static const char* mCamKindNameSaveFile[0x49];
};

extern CPolarSubCamera* gpCamera;

#endif
