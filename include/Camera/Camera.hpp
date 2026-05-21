#ifndef CAMERA_CAMERA_HPP
#define CAMERA_CAMERA_HPP

#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/JDrama/JDRCamera.hpp>
#include <dolphin/mtx.h>

class TBaseNPC;
class TCameraMapTool;
class TMarioGamePad;

class TCameraJetCoaster;

class CPolarSubCamera : public JDrama::TLookAtCamera {
public:
	enum EnumNoticeOnOffMode { };

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
	void startReproduceDemoCamera_(const char*, const JGeometry::TVec3<f32>*);
	void restartReproduceDemoCamera_();
	void endReproduceDemoCamera_();
	void updateDemoCamera_(bool);
	void updateGateDemoCamera_();
	void startGateDemoCamera(const JDrama::TActor*);
	void startDemoCamera(const char*, const JGeometry::TVec3<f32>*, s32, f32,
	                     bool);
	void endDemoCamera();
	bool isSimpleDemoCamera() const;
	// fabricated inline — actual definition unknown, always inlined.
	// The `? true : false` trick forces bool materialization when inlined
	// (otherwise MWCC inlines straight to cmpwi/beq).
	bool isOnGoingDemoCamera() const
	{
		return (mMode == 0x49) ? true : false;
	}
	void getTotalDemoFrames() const;
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
	void removeMultiPlayer(const JGeometry::TVec3<f32>*);
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
	bool isNotHeightPanCamMode_() const;
	void execHeightPan_();
	void killHeightPan_();

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
	void getFinalAngleZ() const;
	~CPolarSubCamera();
	int controlByCameraCode_(int*);
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
	/* 0x54 */ char unk54[0x58 - 0x54];
	/* 0x58 */ u32 unk58;
	/* 0x5C */ char unk5C[0x70 - 0x5C];
	/* 0x70 */ TCameraMapTool* unk70;
	/* 0x74 */ char unk74[0x7C - 0x74];
	/* 0x7C */ u32 unk7C;
	/* 0x80 */ char unk80[0x8C - 0x80];
	/* 0x8C */ JGeometry::TVec3<f32> unk8C;
	/* 0x98 */ char unk98[0xA4 - 0x98];
	/* 0xA4 */ s16 unkA4;
	/* 0xA6 */ s16 unkA6;
	/* 0xA8 */ f32 unkA8;
	/* 0xAC */ s16 unkAC;
	/* 0xAE */ char unkAE[0x120 - 0xAE];
	/* 0x120 */ TMarioGamePad* unk120;
	/* 0x124 */ JGeometry::TVec3<f32> unk124;
	/* 0x130 */ char unk130[0xC];
	/* 0x13C */ JGeometry::TVec3<f32> unk13C;
	/* 0x148 */ char unk148[0x1EC - 0x148];
	/* 0x1EC */ Mtx unk1EC;
	/* 0x21C */ char unk21C[0x2A4 - 0x21C];
	/* 0x2A4 */ void* unk2A4;
	/* 0x2A8 */ char unk2A8[0x2B8 - 0x2A8];
	/* 0x2B8 */ TCameraJetCoaster* unk2B8;
	/* 0x2BC */ char unk2BC[0x2C8 - 0x2BC];
	/* 0x2C8 */ s16 unk2C8;
};

extern CPolarSubCamera* gpCamera;

#endif
