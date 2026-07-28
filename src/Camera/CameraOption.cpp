#include <Camera/Camera.hpp>
#include <Camera/CameraMapTool.hpp>
#include <Camera/CameraOption.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Camera/CubeMapTool.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/NameRefAry.hpp>

template <> s16 CLBRoundf<s16>(f32);

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";
const char* cLoadCamName = "\x8d\xb6\x83\x54\x83\x43\x83\x68\x83\x4a\x83\x81\x83\x89"; // 左サイドカメラ
TCameraOption* gpCameraOption;

void TCameraOption::moveToDown()
{
	JGeometry::TVec3<f32>* pos = unk3C;
	pos->x                      = unk24.x;
	pos->y                      = unk24.y;
	pos->z                      = unk24.z;
	unk16   = unk14;
}

void TCameraOption::moveToUp()
{
	JGeometry::TVec3<f32>* pos = unk3C;
	pos->x                      = unk30.x;
	pos->y                      = unk30.y;
	pos->z                      = unk30.z;
	unk16   = unk14;
}

void TCameraOption::moveToLoadFromTitle()
{
	JGeometry::TVec3<f32>* pos = unk3C;
	pos->x                      = unk24.x;
	pos->y                      = unk24.y;
	pos->z                      = unk24.z;
	unkE     = unkC;
	unk0     = (unk0 & ~2);
}

TCameraOption::TCameraOption(JGeometry::TVec3<f32> center,
                             JGeometry::TVec3<f32>* outPos)
{
	unk0      = 2;
	mFovYunk4 = 40.0f;
	unk8      = 0x12C;
	unkA      = 0x12C;
	unkC      = 0x78;
	unkE      = 0;
	unk10     = 0x50;
	unk12     = 0;
	unk14     = 0x3C;
	unk16     = 0;
	unk18.x   = 0.0f;
	unk18.y   = 0.0f;
	unk18.z   = 0.0f;
	unk24.x   = 0.0f;
	unk24.y   = 0.0f;
	unk24.z   = 0.0f;
	unk30.x   = 0.0f;
	unk30.y   = 0.0f;
	unk30.z   = 0.0f;
	unk3C     = outPos;

	s16 pitch = CLBRoundf<s16>(-13289.245f);
	s16 yaw   = CLBRoundf<s16>(9830.4f);
	CLBPolarToCross(center, &unk18, 1000.0f, yaw, pitch);

	outPos->x = unk18.x;
	outPos->y = unk18.y;
	outPos->z = unk18.z;

	const char* name = cLoadCamName;
	TCameraMapTool* mapTool
	    = (TCameraMapTool*)gpCamMapToolTable->searchF(
	        JDrama::TNameRef::calcKeyCode(name), name);
	if (mapTool != nullptr) {
		JGeometry::TVec3<f32> tmp;
		mapTool->calcPosAndAt(&tmp, &unk24);
		f32 angle = mapTool->mPitchYaw.y;
		s16 ang  = CLBRoundf<s16>(182.04445f * angle);
		s16 pi  = CLBRoundf<s16>(10922.667f);
		CLBPolarToCross(tmp, &unk30, 1000.0f, pi, ang);
	}
}

static inline void chasePosAt(CPolarSubCamera* self, f32 frames)
{
	CLBChaseConstantSpecifyFrame<f32>((f32*)((u8*)self + 0x10),
	                                  *(f32*)((u8*)self + 0x80), frames);
	CLBChaseConstantSpecifyFrame<f32>((f32*)((u8*)self + 0x14),
	                                  *(f32*)((u8*)self + 0x84), frames);
	CLBChaseConstantSpecifyFrame<f32>((f32*)((u8*)self + 0x18),
	                                  *(f32*)((u8*)self + 0x88), frames);
	CLBChaseConstantSpecifyFrame<f32>((f32*)((u8*)self + 0x3C),
	                                  *(f32*)((u8*)self + 0x8C), frames);
	CLBChaseConstantSpecifyFrame<f32>((f32*)((u8*)self + 0x40),
	                                  *(f32*)((u8*)self + 0x90), frames);
	CLBChaseConstantSpecifyFrame<f32>((f32*)((u8*)self + 0x44),
	                                  *(f32*)((u8*)self + 0x94), frames);
}

void CPolarSubCamera::ctrlOptionCamera_()
{
	if (gpCameraOption->unkA > 0) {
		chasePosAt(this, (f32)gpCameraOption->unkA);
		gpCameraOption->unkA -= 1;
	} else if (gpCameraOption->unkE > 0) {
		chasePosAt(this, (f32)gpCameraOption->unkE);
		gpCameraOption->unkE -= 1;
	} else {
		bool skip = (gpCameraOption->unk0 & 2) != 0;
		if (!skip) {
			JGeometry::TVec3<f32> pos = *gpMarioPos;
			pos.y += 75.0f;
			s32 cubeNo = gpCubeCamera->getInCubeNo(pos);
			if (cubeNo >= 0) {
				TCameraMapTool* mt
				    = (TCameraMapTool*)((TCubeCameraInfo*)gpCubeCamera->unk14
				                            ->begin()[cubeNo])
				          ->unk38;
				if (mt != nullptr && mt != unk70) {
					gpCameraOption->unk0 ^= 1;
					unk70 = mt;
					unk70->calcPosAndAt((JGeometry::TVec3<f32>*)((u8*)this + 0x80),
					                    (JGeometry::TVec3<f32>*)((u8*)this + 0x8C));
					gpCameraOption->unk12 = gpCameraOption->unk10;
				}
			}

			if (gpCameraOption->unk12 > 0) {
				chasePosAt(this, (f32)gpCameraOption->unk12);
				gpCameraOption->unk12 -= 1;
			} else if (gpCameraOption->unk16 > 0) {
				chasePosAt(this, (f32)gpCameraOption->unk16);
				gpCameraOption->unk16 -= 1;
			}
		}
	}

	*(f32*)((u8*)this + 0x124) = *(f32*)((u8*)this + 0x10);
	*(f32*)((u8*)this + 0x128) = *(f32*)((u8*)this + 0x14);
	*(f32*)((u8*)this + 0x12C) = *(f32*)((u8*)this + 0x18);
	*(f32*)((u8*)this + 0x148) = *(f32*)((u8*)this + 0x3C);
	*(f32*)((u8*)this + 0x14C) = *(f32*)((u8*)this + 0x40);
	*(f32*)((u8*)this + 0x150) = *(f32*)((u8*)this + 0x44);
	*(f32*)((u8*)this + 0x48)  = gpCameraOption->mFovYunk4;
}
