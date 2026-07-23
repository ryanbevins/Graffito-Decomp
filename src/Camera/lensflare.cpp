#define JGEOMETRY_EVENTWATCHER_TVEC3_SET_VEC_OUT_OF_LINE
#define JMATH_SELECTSHINE2_TRIG_OUT_OF_LINE
#include <Camera/LensFlare.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/SunMgr.hpp>
#include <Camera/SunModel.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/Components/J3DGXColorS10.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <stdio.h>
#undef JGEOMETRY_EVENTWATCHER_TVEC3_SET_VEC_OUT_OF_LINE
#undef JMATH_SELECTSHINE2_TRIG_OUT_OF_LINE

template <> f32 CLBTwoDegreeGeneralInbetween<f32>(f32, f32, f32, f32);
template <> s16 CLBRoundf<s16>(f32);

extern const char* cSunVolumeName;

TLensFlare::TLensFlare(const char* name)
    : JDrama::TViewObj(name)
{
	unk10 = nullptr;
	unk14 = nullptr;
	unk18 = 60.0f;
	unk1C = 60.0f;
	unk20 = 80.0f;
	unk24 = 0.0f;
	unk28 = 0.0f;
	unk2C = 0.04f;
	unk30 = 0.005f;
	unk34 = 0.04f;
	unk38 = 0.04f;
	unk3C = 30000.0f;
	unk40 = 3.0f;
	unk44 = 1.75f;
	unk48 = 75.0f;

	if ((gpSunMgr->unk15 & 2) != 0) {
		return;
	}

	char buf[0x100];
	snprintf(buf, 0x100, "%s/%s", cSunVolumeName, "sun_lensfx.bmd");
	void* res = JKRFileLoader::getGlbResource(buf);
	unk10     = (J3DModelData*)J3DModelLoaderDataBase::load(res, 0x10020000);
	unk14 = new J3DModel(unk10, 0, 1);
}

void TLensFlare::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)gfx;

	if ((gpSunMgr->unk15 & 2) != 0)
		return;

	u8 visible;
	if (gpCameraMario->isMarioIndoor()) {
		visible = 0;
	} else {
		visible = gpSunModel->isInBounds(unk40);
	}

	if (flags & 1) {
		bool active = gpSunModel->isInBounds(unk44);

		if (!active) {
			unk28 = 0.0f;
		} else {
			TSunModel* sun = gpSunModel;
			JGeometry::TVec2<s16>* coords = sun->mZBufCoords;
			u8* zBufVisible               = sun->mZBufVisible;
			int count = 0;
			for (int i = 0; i < 17; ++i, ++coords, ++zBufVisible) {
				if (coords->x != -1 && coords->y != -1 && !*zBufVisible) {
					++count;
				}
			}

			f32 occludedRatio = (1.0f / 17.0f) * (f32)count;
			f32 base          = unk48 * (1.0f - occludedRatio);
			unk28    = CLBEaseOutInbetween(base, 255.0f, sun->mUnk194);
		}

		f32 rate;
		if (unk24 < unk28) {
			if (gpSunModel->mUnk194 == 0.0f)
				rate = unk30;
			else
				rate = unk2C;
		} else {
			if (gpSunModel->mUnk194 == 0.0f)
				rate = unk38;
			else
				rate = unk34;
		}
		CLBChaseDecrease(&unk24, unk28, rate, 0.0f);
	}

	if (!visible)
		return;

	if (flags & 2) {
		TSunModel* sun = gpSunModel;
		Vec sunPos;
		sunPos = sun->mPos198;

		JGeometry::TVec3<f32> nearPos[9];
		S16Vec nearRot[9];

		f32 nearClip = gpCamera->mNear;
		f32 aspect   = gpCamera->mAspect;
		f32 fovy     = gpCamera->mFovy;
		s16 zAngle   = gpCamera->getFinalAngleZ();

		JGeometry::TVec3<f32> camPos;
		camPos.set(*(Vec*)((u8*)gpCamera + 0x148));
		JGeometry::TVec3<f32> camAt;
		camAt.set(*(Vec*)&gpCamera->unk124);

		s16 halfFov = CLBRoundf<s16>(182.04445f * (0.5f * fovy));
		f32 tanHalf = JMASSin(halfFov) * (1.0f / JMASCos(halfFov));
		JGeometry::TVec2<f32> size;
		size.y = 2.0f * nearClip * tanHalf;
		size.x = size.y * aspect;

		CLBCalcNearNinePos(nearPos, nearRot, camAt, camPos, zAngle, nearClip,
		                   size);

		const JGeometry::TVec3<f32>& center = nearPos[4];
		f32 sx = -sun->mFPos[0].x * unk3C;
		f32 sy = -sun->mFPos[0].y * unk3C;
		JGeometry::TVec3<f32> flarePos;
		flarePos.sub(nearPos[5], center);
		flarePos.scale(sx);
		JGeometry::TVec3<f32> vertical;
		vertical.sub(nearPos[1], center);
		vertical.scale(sy);
		flarePos.add(center);
		flarePos.add(vertical);

		JGeometry::TVec3<f32> copiedSunPos;
		copiedSunPos.set(sunPos);
		JGeometry::TVec3<f32> dir;
		dir.x = flarePos.x - copiedSunPos.x;
		dir.y = flarePos.y - copiedSunPos.y;
		dir.z = flarePos.z - copiedSunPos.z;

		JGeometry::TVec3<f32> rot = MsGetRotFromZaxis(dir);
		s16 rotX = CLBRoundf<s16>(182.04445f * rot.x);
		s16 rotY = CLBRoundf<s16>(182.04445f * rot.y);

		Mtx mtx;
		MsMtxSetTRS(mtx, sunPos.x, sunPos.y, sunPos.z,
		            (f32)rotX * (360.0f / 65536.0f),
		            (f32)rotY * (360.0f / 65536.0f), 0.0f, unk18, unk1C,
		            unk20);
		PSMTXCopy(mtx, unk14->unk20);
		unk14->calc();
	}

	if (flags & 0x200) {
		int num = unk10->getMaterialNum();
		for (int i = 0; (u16)i < num; ++i) {
			unk10->getMaterialNodePointer(i)->change();
			J3DGXColorS10 color;
			color.color =
			    unk10->getMaterialNodePointer(i)->getTevColor(0)->color;
			color.color.a = (s16)unk24;
			unk10->getMaterialNodePointer(i)->getTevBlock()->setTevColor(
			    0, &color);
		}
		unk14->entry();
	}

	if (flags & 4)
		unk14->viewCalc();
}

template f32 CLBEaseOutInbetween<f32>(f32, f32, f32);
