#define JDRAMA_TFLAG_CTOR_DECL_ONLY
#include <Camera/SunModel.hpp>
#undef JDRAMA_TFLAG_CTOR_DECL_ONLY
#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Map/MapStaticObject.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <System/Resolution.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DAnmLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JDrama/JDRViewObjPtrList.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <stdio.h>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBEaseOutInbetween<f32>(f32, f32, f32);
template <> f32 CLBTwoDegreeGeneralInbetween<f32>(f32, f32, f32, f32);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<f32>(f32*, f32, f32);
template <> s16 CLBRoundf<s16>(f32);

extern const char* cSunVolumeName;
extern const char* cSunsetVolumeName;
extern f32 SMSGetAnmFrameRate();

static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char SMS_NO_MEMORY_MESSAGE[]
    = "\x83\x81\x83\x82\x83\x8A\x82\xAA\x91\xAB\x82\xE8\x82\xDC\x82\xB9"
      "\x82\xF1\n";
static const char cSunSceneName[]    = "/scene/sun";
static const char cSunsetSceneName[] = "/scene/sunset";

TSunModel* gpSunModel;

TSunModel::TSunModel(bool sunset, const char* name)
    : JDrama::TActor(name)
    , mModelData(0)
    , mModel(0)
    , mAnmTexSRT(0)
    , mFrameCtrl(0)
    , mMapStaticObj(0)
    , mUnk68(0xFF)
    , mUnk6C(0.1f)
    , mUnk70(0.01f)
    , mUnk74(50)
    , mUnk78(0.1f)
    , mUnk7C(0.01f)
    , mUnk80(100)
    , mUnk84(3.0f)
    , mUnk88(2.0f)
    , mUnk9C(0.0f)
    , mUnkA0(0.0f)
    , mUnkA4(0.0f)
    , mUnkA8(0.0f)
    , unkAC(0.0f)
    , mUnkB0(0.0f)
    , mVisibleCount(0)
    , mUnk194(0.0f)
    , mUnk1A4(0.014f)
    , mUnk1A8(3.0f)
    , mFlags(0)
{
	gpSunModel = this;
	if (sunset) {
		mFlags |= 4;
		mUnk80 = 0x30;
	}

	JGeometry::TVec2<s16>* zbufCoords = mZBufCoords;
	JGeometry::TVec2<f32>* fpos       = mFPos;
	u8* zbufVisible                   = mZBufVisible;
	for (s32 i = 17; i > 0; --i) {
		zbufCoords->y = -1;
		zbufCoords->x = -1;
		++zbufCoords;
		fpos->y = 10000.0f;
		fpos->x = 10000.0f;
		++fpos;
		*zbufVisible = 0;
		++zbufVisible;
	}
}

void TSunModel::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);
	mScaling.x *= 0.4f;
	mScaling.y *= 0.4f;
	mScaling.z *= 0.4f;

	const char* volName  = cSunVolumeName;
	u32         loadFlags = 0x10020000;
	if ((mFlags & 4) != 0) {
		loadFlags |= 0x01000000;
		volName    = cSunsetVolumeName;
	}

	char path[0x100];
	snprintf(path, 0x100, "%s/%s", volName, "model.bmd");
	void* bmd = JKRFileLoader::getGlbResource(path);
	mModelData = (J3DModelData*)J3DModelLoaderDataBase::load(bmd, loadFlags);

	mModel = new J3DModel(mModelData, 0, 1);

	snprintf(path, 0x100, "%s/%s", volName, "model.btk");
	void* btk    = JKRFileLoader::getGlbResource(path);
	mAnmTexSRT   = (J3DAnmTextureSRTKey*)J3DAnmLoaderDataBase::load(btk);
	mAnmTexSRT->searchUpdateMaterialID(mModelData);

	int num = mModelData->getMaterialNum();
	for (int i = 0; (u16)i < num; i++) {
		J3DMaterialAnm* anm = new J3DMaterialAnm;
		mModelData->getMaterialNodePointer(i)->change();
		mModelData->getMaterialNodePointer(i)->setMaterialAnm(anm);
	}
	mModelData->entryTexMtxAnimator(mAnmTexSRT);

	// Two indirect virtual calls copy 8 bytes each into mUnk8C..0x98
	// (likely material-related render data); store as int pairs.
	{
		J3DMaterial* mat0 = mModelData->getMaterialNodePointer(0);
		void* sub = *(void**)((u8*)mat0 + 0x28);
		typedef u32* (*F)(void*, u32);
		u32* p0 = ((F)(*(void***)sub)[13])(sub, 0);
		u32 word0 = p0[0];
		u32 word1 = p0[1];
		*(u32*)((u8*)this + 0x8C) = word0;
		*(u32*)((u8*)this + 0x90) = word1;
	}
	{
		J3DMaterial* mat1 = mModelData->getMaterialNodePointer(1);
		void* sub = *(void**)((u8*)mat1 + 0x28);
		typedef u32* (*F)(void*, u32);
		u32* p1 = ((F)(*(void***)sub)[13])(sub, 0);
		u32 word0 = p1[0];
		u32 word1 = p1[1];
		*(u32*)((u8*)this + 0x94) = word0;
		*(u32*)((u8*)this + 0x98) = word1;
	}

	f32 color0 = (f32)(u8)mUnk68;
	mUnkA4    = color0;
	mUnk9C    = color0;
	f32 color1 = (f32)(u8)mUnk74;
	mUnkA8    = color1;
	mUnkA0    = color1;

	mFrameCtrl.init(*(s16*)((u8*)mAnmTexSRT + 2));
	mFrameCtrl.setRate(SMSGetAnmFrameRate());
	mFrameCtrl.setAttribute(J3DFrameCtrl::ATTR_LOOP);

	mPos198 = *(const Vec*)&mPosition;

	mMapStaticObj = new TMapStaticObj("\x91\xBE\x97\x7A"
	                                  "in"
	                                  "\x8B\xBE");
	mMapStaticObj->init("sun_mirror");

	*(Vec*)((u8*)mMapStaticObj + 0x10) = *(const Vec*)&mPosition;
	*(Vec*)((u8*)mMapStaticObj + 0x30) = *(const Vec*)&mRotation;
	*(Vec*)((u8*)mMapStaticObj + 0x24) = *(const Vec*)&mScaling;

	const char* sceneName  = "\x8B\xBE\x83\x56\x81\x5B\x83\x93"; // JIS string
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* scene
	    = JDrama::TNameRefGen::search<
	        JDrama::TViewObjPtrListT<JDrama::TViewObj> >(sceneName);
	scene->getChildren().push_back(mMapStaticObj);
}

void TSunModel::calcOtherFPosFromCenterAndRadius_(
    JGeometry::TVec2<f32>* out, const JGeometry::TVec2<f32>& center, f32 radius)
{
	f32 d = 0.707f * radius;
	out[0].x = center.x;
	out[0].y = center.y + radius;
	out[1].x = center.x - d;
	out[1].y = center.y + d;
	out[2].x = center.x - radius;
	out[2].y = center.y;
	out[3].x = center.x - d;
	out[3].y = center.y - d;
	out[4].x = center.x;
	out[4].y = center.y - radius;
	out[5].x = center.x + d;
	out[5].y = center.y - d;
	out[6].x = center.x + radius;
	out[6].y = center.y;
	out[7].x = center.x + d;
	out[7].y = center.y + d;
}

#pragma dont_inline on
void CLBScreenFPosToSPos(JGeometry::TVec2<s16>* dst,
                         const JGeometry::TVec2<f32>& src)
{
	f32 x = src.x;
	if (x < -1.0f || 1.0f < x) {
		dst->x = -1;
	} else {
		u16 width = SMSGetGameRenderWidth();
		dst->x = CLBRoundf<s16>(
		    (1.0f + x) * (0.5f * (f32)(width - 1)));
	}
	f32 y = src.y;
	if (y < -1.0f || 1.0f < y) {
		dst->y = -1;
	} else {
		u16 height = SMSGetGameRenderHeight();
		dst->y = CLBRoundf<s16>(
		    (y - 1.0f) * (-0.5f * (f32)(height - 1)));
	}
}
#pragma dont_inline off

void TSunModel::calcDispRatioAndScreenPos_()
{
	mVisibleCount = 0;
	u8* visible = mZBufVisible;
	for (s32 i = 0; i < 17; i++) {
		if (*visible != 0) {
			mVisibleCount += 1;
		}
		++visible;
	}
	mUnk194 = 0.05882353f * (f32)(u8)mVisibleCount;

	CLBCalc2DFPos(&mFPos[0], gpCamera->getUnk16C(),
	              gpCamera->getUnk1EC(), mPos198, 0, false);

	f32 inner = mUnk1A4 * mScaling.y;
	calcOtherFPosFromCenterAndRadius_(&mFPos[1], mFPos[0], inner);
	inner *= 0.5f;
	calcOtherFPosFromCenterAndRadius_(&mFPos[9], mFPos[0], inner);

	JGeometry::TVec2<s16>* coords = mZBufCoords;
	JGeometry::TVec2<f32>* fpos   = mFPos;
	for (s32 i = 0; i < 17; i++) {
		CLBScreenFPosToSPos(coords, *fpos);
		++coords;
		++fpos;
	}
}

void TSunModel::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)gfx;
	bool inMode = false;
	if (gpCameraMario->isMarioIndoor()) {
		inMode = false;
	} else {
		bool a = false;
		bool b = false;
		bool c = false;
		if (-mUnk1A8 <= mFPos[0].x && mFPos[0].x <= mUnk1A8) {
			a = true;
		}
		if (a && -mUnk1A8 <= mFPos[0].y) {
			b = true;
		}
		if (b && mFPos[0].y <= mUnk1A8) {
			c = true;
		}
		inMode = (c ? true : false);
	}

	if ((flags & 1) != 0) {
		mUnkA4 = CLBLinearInbetween<f32>((f32)mUnk68, 255.0f, mUnk194);
		mUnkA8 = CLBEaseOutInbetween<f32>((f32)mUnk74, 255.0f, mUnk194);

		f32 speed;
		if (mUnk9C < mUnkA4) {
			speed = mUnk6C;
		} else {
			speed = mUnk70;
		}
		CLBChaseDecrease(&mUnk9C, mUnkA4, speed, 0.5f);
		mUnk92 = mUnk9C;

		if (mUnkA0 < mUnkA8) {
			speed = mUnk78;
		} else {
			speed = mUnk7C;
		}
		CLBChaseDecrease(&mUnkA0, mUnkA8, speed, 0.5f);
		mUnk9A = mUnkA0;

		if (gpCameraMario->isMarioIndoor()) {
			mUnkB0 = 0.0f;
		} else {
			f32 d2 = mFPos[0].x * mFPos[0].x + mFPos[0].y * mFPos[0].y;
			if (d2 > 2.0f) {
				mUnkB0 = 0.0f;
			} else {
				f32 ratio = 0.5f * (2.0f - d2) * mUnk194;
				mUnkB0 = CLBLinearInbetween<f32>(0.0f, (f32)mUnk80, ratio);
			}
		}

		f32 chase_speed;
		if (unkAC < mUnkB0) {
			chase_speed = mUnk84;
		} else {
			chase_speed = mUnk88;
		}
		CLBChaseGeneralConstantSpecifySpeed<f32>(&unkAC, mUnkB0, chase_speed);

		f32* gpcam = (f32*)((u8*)gpCamera + 0x124);
		JGeometry::TVec3<f32> axis(mPosition.x - gpcam[0],
		                           mPosition.y - gpcam[1],
		                           mPosition.z - gpcam[2]);
		MsVECNormalize(axis, axis);

		JGeometry::TVec3<f32> cam_pos;
		cam_pos.set(*(const Vec*)((u8*)gpCamera + 0x124));

		mPos198.x = axis.x * 250000.0f + cam_pos.x;
		mPos198.y = axis.y * 250000.0f + cam_pos.y;
		mPos198.z = axis.z * 250000.0f + cam_pos.z;

		if (mMapStaticObj != NULL) {
			*(Vec*)((u8*)mMapStaticObj + 0x10) = mPos198;
		}

		calcDispRatioAndScreenPos_();
	}

	if ((flags & 2) != 0) {
		mFrameCtrl.update();
		if (inMode) {
			Mtx mtx;
			MsMtxSetTRS(mtx, mPos198.x, mPos198.y, mPos198.z,
			            mRotation.x, mRotation.y, mRotation.z,
			            mScaling.x, mScaling.y, mScaling.z);
			PSMTXCopy(mtx, (MtxPtr)((u8*)mModel + 0x20));
			mModel->calc();
		}
	}

	if ((flags & 0x200) != 0) {
		if (inMode) {
			*(f32*)((u8*)mAnmTexSRT + 4) = mFrameCtrl.getFrame();
			{
				void* sub = *(void**)((u8*)mModelData->getMaterialNodePointer(0) + 0x28);
				typedef void (*F)(void*, u32, void*);
				((F)(*(void***)sub)[11])(sub, 0, (u8*)this + 0x8C);
			}
			{
				void* sub = *(void**)((u8*)mModelData->getMaterialNodePointer(1) + 0x28);
				typedef void (*F)(void*, u32, void*);
				((F)(*(void***)sub)[11])(sub, 0, (u8*)this + 0x94);
			}
			mModel->entry();
		}
	}

	if ((flags & 4) != 0) {
		if (inMode) {
			mModel->viewCalc();
		}
	}
}

void TSunModel::getZBufValue()
{
	bool indoor = gpCameraMario->isMarioIndoor();
	JGeometry::TVec2<s16>* coords = mZBufCoords;
	u8* visible                    = mZBufVisible;
	for (int i = 0; i < 17; i++) {
		*visible = 0;
		if (!indoor && coords->x != -1 && coords->y != -1) {
			u32 zVal;
			GXPeekZ(coords->x, coords->y, &zVal);
			if ((zVal - 0xFF0000) == 0xFFFF) {
				*visible = 1;
			}
		}
		++coords;
		++visible;
	}
}
