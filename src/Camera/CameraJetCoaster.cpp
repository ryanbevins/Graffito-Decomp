#include <Camera/Camera.hpp>
#include <Camera/CameraJetCoaster.hpp>
#include <Camera/cameralib.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/JGeometry.hpp>
#include <JSystem/JGeometry/JGRotation3.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MoveBG/ItemManager.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>

template <> s16 CLBRoundf<s16>(f32);

static inline void normalizeVec(JGeometry::TVec3<f32>* vec)
{
	vec->normalize();
}

static inline void setRotateVec(
    JGeometry::TRotation3<JGeometry::TMatrix33<JGeometry::SMatrix33C<f32> > >*
        rot,
    const JGeometry::TVec3<f32>& axis, f32 angle)
{
	rot->setRotate(axis, angle);
}

class TCameraJetCoaster {
public:
	TCameraJetCoaster();

	/* 0x00 */ TCamSaveJetCoaster* unk0;
	/* 0x04 */ s16 unk4;
	/* 0x06 */ s16 unk6;
	/* 0x08 */ s16 unk8;
	/* 0x0A */ s16 unkA;
	/* 0x0C */ u8 unkC;
	/* 0x10 */ f32 unk10;
	/* 0x14 */ f32 unk14;
	/* 0x18 */ f32 unk18;
	/* 0x1C */ f32 unk1C;
	/* 0x20 */ f32 unk20;
	/* 0x24 */ f32 unk24;
	/* 0x28 */ f32 unk28;
	/* 0x2C */ f32 unk2C;
	/* 0x30 */ f32 unk30;
	/* 0x34 */ f32 unk34;
	/* 0x38 */ u16 unk38;
};

TCameraJetCoaster::TCameraJetCoaster()
{
	unk4  = 0;
	unk6  = 0;
	unk8  = 0;
	unkA  = 0;
	unkC  = 1;
	unk10 = 0.0f;
	unk14 = 0.0f;
	unk18 = 500.0f;
	unk1C = 0.0f;
	unk20 = 0.0f;
	unk24 = 0.0f;
	unk28 = 0.0f;
	unk2C = 1.0f;
	unk30 = 0.0f;
	unk34 = 60.0f;
	unk38 = 0;
	unk0  = new TCamSaveJetCoaster();
}

void CPolarSubCamera::drawJetCoasterBalloonMessage_()
{
	static const Vec sFixCameraPos = { 3005.0f, 4020.0f, -9560.0f };
	gpMarioOriginal->loserExec();
	*(u32*)((u8*)this + 0x78) = 0xE10;
	*(u32*)((u8*)this + 0x7C) = 0;
	warpPosAndAt(sFixCameraPos, *(const Vec*)((u8*)this + 0x3C));
	unk2B8->unk38 = 1;
}

void CPolarSubCamera::ctrlJetCoasterCamera_()
{
	if (gpMarDirector->mMap == 0x3A && gpMarDirector->unk7D == 0) {
		s32 flagState = TFlagManager::smInstance->getFlag(0x60001);
		int balloonCount
		    = gpItemManager->getObjNumWithActorType(0x40000132);

		if (unk2B8->unk38 > 2) {
			--unk2B8->unk38;
			if (unk2B8->unk38 == 2) {
				unk2B8->unk38 = 1;
				gpMarDirector->setNextStage(0xE05, nullptr);
			}
		} else if (unk2B8->unk38 != 1) {
			s32 msgId = -1;
			if (flagState == balloonCount) {
				TFlagManager::smInstance->setBool(true, 0x30005);
				unk2B8->unk38 = 0x12C;
				msgId         = 0xE002D;
			} else {
				int scene = gpMarDirector->unk58;
				switch (scene) {
				case 0x3C:
					gpMarDirector->mConsole->startAppearJetBalloon(
					    0, balloonCount);
					break;
				case 0x1DB:
					msgId = 0xE0029;
					break;
				case 0x1D4C:
					msgId = 0xE002A;
					break;
				case 0x3A98:
					if ((u32)(balloonCount - flagState) >= 7)
						msgId = 0xE002B;
					else
						msgId = 0xE002C;
					break;
				case 0x57E4:
					drawJetCoasterBalloonMessage_();
					break;
				}
			}

			if (msgId != -1) {
				gpMarDirector->mConsole->startAppearBalloon(msgId, true);
			}
		}
	}

	bool isJet1stCamPressed = false;
	if (unk120->mEnabledFrameMeaning & 0x4000) {
		isJet1stCamPressed = true;
		s32 soundId        = 0x4825;
		unk2B8->unkC ^= 1;
		if (unk2B8->unkC & 1) {
			soundId = 0x4824;
		}
		if (gpMSound->gateCheck(soundId)) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(soundId, 0, nullptr,
			                                             0);
		}
	}

	JGeometry::TVec3<f32> at;

	if (unk2B8->unkC & 1) {
		// 1st-person camera mode
		if (isJet1stCamPressed) {
			setUpToLButtonCamera_(0x2E);
		}

		f32 stickInput = -*(f32*)((u8*)unk120 + 0xAC);

		if (*(u32*)((u8*)this + 0x7C) == 0) {
			getNozzleTopPos_(&unk8C);
		}

		if (*(u32*)((u8*)this + 0x78) == 0) {
			rotateX_ByStickY_(stickInput);
			unkA4 = calcAngleXFromXRotRatio_();
			unkA6 = *(s16*)((u8*)gpMarioOriginal + 0x410);
		}

		at = unk8C;

		s16 yawAngle   = unkA4 + getOffsetAngleX();
		s16 pitchAngle = unkA6 + getOffsetAngleY();

		MtxPtr mtx = getToroccoMtx_();

		JGeometry::TVec3<f32> col0(mtx[0][0], mtx[1][0], mtx[2][0]);
		JGeometry::TVec3<f32> col1(mtx[0][1], mtx[1][1], mtx[2][1]);

		*(f32*)((u8*)this + 0x30) = col1.x;
		*(f32*)((u8*)this + 0x34) = col1.y;
		*(f32*)((u8*)this + 0x38) = col1.z;

		f32 nd = -calcDistFromXRotRatio_();

		*(f32*)((u8*)this + 0x98) = at.x + nd * mtx[0][2];
		*(f32*)((u8*)this + 0x9C) = at.y + nd * mtx[1][2];
		*(f32*)((u8*)this + 0xA0) = at.z + nd * mtx[2][2];

		CLBRotatePosAndUp(yawAngle, pitchAngle, col0, col1, at,
		                  (JGeometry::TVec3<f32>*)((u8*)this + 0x98),
		                  (JGeometry::TVec3<f32>*)((u8*)this + 0x30));

		JGeometry::TVec3<f32> fwd;
		fwd.x = at.x - *(f32*)((u8*)this + 0x98);
		fwd.y = at.y - *(f32*)((u8*)this + 0x9C);
		fwd.z = at.z - *(f32*)((u8*)this + 0xA0);
		normalizeVec(&fwd);

		JGeometry::TVec3<f32> upAxis;
		upAxis.x = *(f32*)((u8*)this + 0x30);
		upAxis.y = *(f32*)((u8*)this + 0x34);
		upAxis.z = *(f32*)((u8*)this + 0x38);

		JGeometry::TVec3<f32> upOffset = upAxis;
		MsVECNormalize((Vec*)&upOffset, (Vec*)&upOffset);

		u8* p68 = *(u8**)((u8*)this + 0x68);
		f32 chainScale
		    = unkA8 * *(f32*)(p68 + 0x28) + *(f32*)(p68 + 0x24);
		upOffset.x *= chainScale;
		upOffset.y *= chainScale;
		upOffset.z *= chainScale;

		*(f32*)((u8*)this + 0x98) += upOffset.x;
		*(f32*)((u8*)this + 0x9C) += upOffset.y;
		*(f32*)((u8*)this + 0xA0) += upOffset.z;
		at.x += upOffset.x;
		at.y += upOffset.y;
		at.z += upOffset.z;

		JGeometry::TRotation3<
		    JGeometry::TMatrix33<JGeometry::SMatrix33C<f32> > >
		    rot;
		JGeometry::TVec3<f32> sideAxis
		    = *(JGeometry::TVec3<f32>*)((u8*)this + 0x30);
		rot.identity33();
		setRotateVec(&rot, fwd, -1.570796f);

		f32 sx = rot.at(0, 0) * sideAxis.x + rot.at(1, 0) * sideAxis.y
		         + rot.at(2, 0) * sideAxis.z;
		f32 sy = rot.at(0, 1) * sideAxis.x + rot.at(1, 1) * sideAxis.y
		         + rot.at(2, 1) * sideAxis.z;
		f32 sz = rot.at(0, 2) * sideAxis.x + rot.at(1, 2) * sideAxis.y
		         + rot.at(2, 2) * sideAxis.z;

		f32 sideScale = *(f32*)(p68 + 0x5C);
		sx *= sideScale;
		sy *= sideScale;
		sz *= sideScale;

		*(f32*)((u8*)this + 0x98) += sx;
		*(f32*)((u8*)this + 0x9C) += sy;
		*(f32*)((u8*)this + 0xA0) += sz;
		at.x += sx;
		at.y += sy;
		at.z += sz;

		f32 angleDeg;
		if (0.0f == col1.y) {
			if (col1.x >= 0.0f) {
				angleDeg = -90.0f;
			} else {
				angleDeg = 90.0f;
			}
		} else if (col1.y >= 0.0f) {
			angleDeg = -((f32)matan(col1.y, col1.x) * (360.0f / 65536.0f));
		} else {
			angleDeg
			    = 180.0f + (f32)matan(-col1.y, col1.x) * (360.0f / 65536.0f);
		}

		*(s16*)((u8*)this + 0x254)
		    = CLBRoundf<s16>(angleDeg * (65536.0f / 360.0f));

		*(f32*)((u8*)this + 0x48) = *(f32*)p68;

	} else {
		// 3rd-person camera mode
		if (isJet1stCamPressed) {
			setUpFromLButtonCamera_();
		}

		f32 stickX = *(f32*)((u8*)unk120 + 0xC0);
		f32 stickY = *(f32*)((u8*)unk120 + 0xC4);
		TCamSaveJetCoaster* save = unk2B8->unk0;

		unk2B8->unk8 = (s16)(s32)((f32)(s32)unk2B8->unk8
		                          - stickY
		                                * (f32)(s32)save
		                                      ->mSLOffsetAngleXManualSpeed
		                                      .value);
		unk2B8->unkA = (s16)(s32)((f32)(s32)unk2B8->unkA
		                          + stickX
		                                * (f32)(s32)save
		                                      ->mSLOffsetAngleYManualSpeed
		                                      .value);

		{
			s32 hi = save->mSLOffsetAngleXLimit.value;
			if ((s32)unk2B8->unk8 > hi)
				unk2B8->unk8 = (s16)hi;
			else if ((s32)unk2B8->unk8 < -hi)
				unk2B8->unk8 = (s16)-hi;
		}
		{
			s32 hi = save->mSLOffsetAngleYLimit.value;
			if ((s32)unk2B8->unkA > hi)
				unk2B8->unkA = (s16)hi;
			else if ((s32)unk2B8->unkA < -hi)
				unk2B8->unkA = (s16)-hi;
		}

		CLBChaseAngleDecrease(&unk2B8->unk4, unk2B8->unk8,
		                      save->mSLOffsetAngleXChase.value);
		CLBChaseAngleDecrease(&unk2B8->unk6, unk2B8->unkA,
		                      save->mSLOffsetAngleYChase.value);

		*(u32*)((u8*)this + 0x98) = *(u32*)((u8*)unk2B8 + 0x10);
		*(u32*)((u8*)this + 0x9C) = *(u32*)((u8*)unk2B8 + 0x14);
		*(u32*)((u8*)this + 0xA0) = *(u32*)((u8*)unk2B8 + 0x18);

		*(u32*)((u8*)this + 0x8C) = *(u32*)((u8*)unk2B8 + 0x1C);
		*(u32*)((u8*)this + 0x90) = *(u32*)((u8*)unk2B8 + 0x20);
		*(u32*)((u8*)this + 0x94) = *(u32*)((u8*)unk2B8 + 0x24);

		*(u32*)((u8*)this + 0x30) = *(u32*)((u8*)unk2B8 + 0x28);
		*(u32*)((u8*)this + 0x34) = *(u32*)((u8*)unk2B8 + 0x2C);
		*(u32*)((u8*)this + 0x38) = *(u32*)((u8*)unk2B8 + 0x30);

		*(f32*)((u8*)this + 0x48) = *(f32*)((u8*)unk2B8 + 0x34);

		at.x = *(f32*)((u8*)this + 0x8C);
		at.y = *(f32*)((u8*)this + 0x90);
		at.z = *(f32*)((u8*)this + 0x94);

		JGeometry::TVec3<f32> fwd;
		fwd.x = gpMarioPos->x - *(f32*)((u8*)this + 0x98);
		fwd.y = gpMarioPos->y - *(f32*)((u8*)this + 0x9C);
		fwd.z = gpMarioPos->z - *(f32*)((u8*)this + 0xA0);
		normalizeVec(&fwd);

		f32 ux = *(f32*)((u8*)this + 0x30);
		f32 uy = *(f32*)((u8*)this + 0x34);
		f32 uz = *(f32*)((u8*)this + 0x38);
		JGeometry::TVec3<f32> side;
		side.x = fwd.y * uz - fwd.z * uy;
		side.y = fwd.z * ux - fwd.x * uz;
		side.z = fwd.x * uy - fwd.y * ux;
		MsVECNormalize((Vec*)&side, (Vec*)&side);

		JGeometry::TVec3<f32> up;
		up.x = ux;
		up.y = uy;
		up.z = uz;

		CLBRotatePosAndUp(unk2B8->unk4, unk2B8->unk6, side, up, *gpMarioPos,
		                  (JGeometry::TVec3<f32>*)((u8*)this + 0x98),
		                  (JGeometry::TVec3<f32>*)((u8*)this + 0x30));

	}

	// Tail snap/chase
	*(f32*)((u8*)this + 0x80) = *(f32*)((u8*)this + 0x98);
	*(f32*)((u8*)this + 0x84) = *(f32*)((u8*)this + 0x9C);
	*(f32*)((u8*)this + 0x88) = *(f32*)((u8*)this + 0xA0);

	if (isJet1stCamPressed) {
		if (*(u32*)((u8*)this + 0x78) == 0) {
			*(f32*)((u8*)this + 0x10) = *(f32*)((u8*)this + 0x80);
			*(f32*)((u8*)this + 0x14) = *(f32*)((u8*)this + 0x84);
			*(f32*)((u8*)this + 0x18) = *(f32*)((u8*)this + 0x88);
		}
		if (*(u32*)((u8*)this + 0x7C) == 0) {
			*(f32*)((u8*)this + 0x3C) = at.x;
			*(f32*)((u8*)this + 0x40) = at.y;
			*(f32*)((u8*)this + 0x44) = at.z;
		}
	} else {
		u8* p68 = *(u8**)((u8*)this + 0x68);
		if (*(u32*)((u8*)this + 0x78) == 0) {
			CLBChaseDecrease((f32*)((u8*)this + 0x10),
			                 *(f32*)((u8*)this + 0x80),
			                 *(f32*)(p68 + 0x94), 0.0f);
			CLBChaseDecrease((f32*)((u8*)this + 0x14),
			                 *(f32*)((u8*)this + 0x84),
			                 *(f32*)(p68 + 0x9C), 0.0f);
			CLBChaseDecrease((f32*)((u8*)this + 0x18),
			                 *(f32*)((u8*)this + 0x88),
			                 *(f32*)(p68 + 0x94), 0.0f);
		}
		if (*(u32*)((u8*)this + 0x7C) == 0) {
			CLBChaseDecrease((f32*)((u8*)this + 0x3C), at.x,
			                 *(f32*)(p68 + 0xA4), 0.0f);
			CLBChaseDecrease((f32*)((u8*)this + 0x40), at.y,
			                 *(f32*)(p68 + 0xA8), 0.0f);
			CLBChaseDecrease((f32*)((u8*)this + 0x44), at.z,
			                 *(f32*)(p68 + 0xA4), 0.0f);
		}
	}
}
