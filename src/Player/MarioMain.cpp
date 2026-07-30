// This TU's caller passes full-width EFB coordinates; GXPeekARGB masks them.
#define GXPeekARGB GXPeekARGB_u16
#define GXSetDstAlpha GXSetDstAlpha_u8
#include <Player/MarioMain.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JMath.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioCap.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <System/Resolution.hpp>
#include <System/TimeRec.hpp>
#include <JSystem/JGeometry.hpp>
#include <dolphin/gx.h>
#undef GXPeekARGB
#undef GXSetDstAlpha

extern "C" void GXPeekARGB(u32, u32, u32*);
extern "C" void GXSetDstAlpha(GXBool, u32);

TMario* gpMarioOriginal;

static JGeometry::TVec3<f32> cDeformedTerrainCenter(0.0f, 5000.0f, 0.0f);

class TWaterGun {
public:
	virtual void perform(u32, JDrama::TGraphics*);
	void setBaseTRMtx(Mtx);
};

void TMario::drawSyncCallback(u16 token)
{
	(void)token;

	bool visible = (mSubState & 0x400) ? true : false;
	if (!visible)
		return;

	if (mMarioScreenPos.x < 0.0f || mMarioScreenPos.y < 0.0f
	    || mMarioScreenPos.x >= (f32)(u16)SMSGetGameRenderWidth()
	    || mMarioScreenPos.y >= (f32)(u16)SMSGetGameRenderHeight()) {
		mState &= ~1;
		return;
	}

	u32 color;
	GXPeekARGB((s32)mMarioScreenPos.x, (s32)mMarioScreenPos.y, &color);
	if ((color & 0xff000000) == 0x10000000)
		mState &= ~1;
	else
		mState |= 1;
}

void TMario::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (mSubState & 1)
		TTimeRec::startTimer(0xff, 0, 0, 0x80);

	if (checkFlag(MARIO_FLAG_IS_PERFORMING))
		return;

	u32 doMovement = flags & 1;
	if (doMovement) {
		if ((s16)unk14E > 0) {
			*(s16*)&unk14E = *(s16*)&unk14E - 1;
			if ((s16)unk14E <= 0)
				*(s16*)&unk150 = *(s16*)((u8*)this + 0x924);
		}

		if (*(s16*)&unk150 > 0)
			*(s16*)&unk150 = *(s16*)&unk150 - 1;

		if ((s16)unk14E <= 0) {
			playerControl(gfx);
			setPositions();

			if (mCap)
				mCap->perform(1, gfx);
			if (mWaterGun)
				mWaterGun->perform(1, gfx);
			if (mYoshi)
				mYoshi->movement();
			moveParticle();
		}

		if (isMario()) {
			JGeometry::TVec3<f32> pos = mPosition;
			pos.y += 75.0f;

			TCubeManagerArea* cubeArea = gpCubeArea;
			cubeArea->unk1C       = cubeArea->getInCubeNo(pos);
			TCubeManagerFast* fast = gpCubeFastA;
			fast->unk1C           = fast->getInCubeNo(pos);
			fast                 = gpCubeFastB;
			fast->unk1C          = fast->getInCubeNo(pos);
			fast                 = gpCubeFastC;
			fast->unk1C          = fast->getInCubeNo(pos);
		}

		soundMovement();
	}

	if (doMovement && (s16)unk14E <= 0) {
		if (checkFlag(MARIO_FLAG_HAS_SHIRT)) {
			J3DShape* shape
			    = mModel->unk8->mModelData->mShapeNodePointer[10];
			shape->offFlag(1);
		} else {
			J3DShape* shape
			    = mModel->unk8->mModelData->mShapeNodePointer[10];
			shape->onFlag(1);
		}

		calcAnim(2, gfx);
		animSound();

		if (mWaterGun) {
			MtxPtr mtx = mModel->unk8->getAnmMtx(mBoneIDs[0]);
			mWaterGun->setBaseTRMtx(mtx);
			mWaterGun->perform(2, gfx);
		}

		if (mYoshi)
			mYoshi->calcAnim();
	}

	if (flags & 4) {
		calcView(gfx);

		if (mWaterGun)
			mWaterGun->perform(4, gfx);
		if (mYoshi)
			mYoshi->viewCalc();

		if (this == gpMarioOriginal) {
			JGeometry::TVec3<f32> dir;
			dir.x = JMASSin(mFaceAngle.y);
			dir.y = 0.0f;
			dir.z = JMASCos(mFaceAngle.y);
			PSMTXMultVecSR(gfx->mViewMtx.mMtx, &dir, &unk4F0);
		}
	}

	if (flags & 0x200) {
		BOOL shouldEntry = TRUE;
		if ((mSubState & 2) == 0)
			shouldEntry = FALSE;
		if (checkFlag(4))
			shouldEntry = FALSE;

		if (shouldEntry == TRUE) {
			addDirty();
			addDamageFog(gfx);

			if (checkFlag(MARIO_FLAG_HAS_FLUDD))
				mWaterGun->perform(0x200, gfx);

			entryModels(gfx);
			mYoshi->entry();

			BOOL positiveHeight = (unk368 > 0.0f) ? TRUE : FALSE;
			if (positiveHeight == FALSE)
				((TMBindShadowBody*)unk390)->entryDrawShadow();
		} else {
			if (!onYoshi())
				mYoshi->entry();
		}
	}

	if ((flags & 0x4000000) && checkFlag(MARIO_FLAG_HAS_FLUDD))
		mWaterGun->perform(0x4000000, gfx);

	if (flags & 0x10000000) {
		unk394->frameInit();
		unk398->frameInit();
		unk39C = (u32)j3dSys.mDrawBuffer[0];
		unk3A0 = (u32)j3dSys.mDrawBuffer[1];
		j3dSys.mDrawBuffer[0] = unk394;
		j3dSys.mDrawBuffer[1] = unk398;
		mTrembleModelEffect->movement();
		mCap->perform(0x10000000, gfx);
	}

	if (flags & 0x8000000) {
		j3dSys.mDrawBuffer[0] = (J3DDrawBuffer*)unk39C;
		j3dSys.mDrawBuffer[1] = (J3DDrawBuffer*)unk3A0;
	}

	if (flags & 0x40000000) {
		bool visible = (mSubState & 0x10) ? true : false;
		if (visible) {
			j3dSys.unk4C = 3;
			unk394->draw();
			(*(J3DDrawBuffer**)((u8*)mYoshi + 0xA8))->draw();
		}
	}

	if (flags & 0x20000000) {
		bool visible = (mSubState & 0x10) ? true : false;
		if (visible) {
			j3dSys.unk4C = 4;
			unk398->draw();
			(*(J3DDrawBuffer**)((u8*)mYoshi + 0xAC))->draw();
		}
	}

	if (flags & 0x1000000)
		drawSpecial(gfx);

	if (flags & 0x2000000) {
		bool drawBox = (mSubState & 0x400) ? true : false;
		if (drawBox) {
			MtxPtr viewMtx = gfx->mViewMtx.mMtx;
			boxDrawPrepare(viewMtx);
			GXSetColorUpdate(GX_FALSE);
			GXSetAlphaUpdate(GX_TRUE);
			GXSetDstAlpha(GX_TRUE, 0x10);
			GXDrawCube();
			GXSetColorUpdate(GX_TRUE);
			GXSetAlphaUpdate(GX_FALSE);
			GXSetDstAlpha(GX_FALSE, 0);
		}
	}

	if (flags & 0x800000) {
		bool drawBox = (mSubState & 0x400) ? true : false;
		if (drawBox) {
			MtxPtr viewMtx = gfx->mViewMtx.mMtx;
			boxDrawPrepare(viewMtx);
			GXSetColorUpdate(GX_FALSE);
			GXSetAlphaUpdate(GX_TRUE);
			GXSetDstAlpha(GX_TRUE, 0);
			GXDrawCube();
			GXSetColorUpdate(GX_TRUE);
			GXSetAlphaUpdate(GX_FALSE);
			GXSetDstAlpha(GX_FALSE, 0);
		}
	}

	if ((flags & 0x80000000) && (mSubState & 2)) {
		j3dSys.mFlags |= 2;

		GXSetChanMatColor(
		    GX_COLOR0A0, (GXColor) { 0xff, 0xff, 0xff, 0xff });
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
		              GX_COLOR0A0);
		GXSetZCompLoc(GX_TRUE);
		GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
		GXSetColorUpdate(GX_FALSE);
		GXSetAlphaUpdate(GX_TRUE);
		GXSetDstAlpha(GX_TRUE, (s32)gpSilhouetteManager->unk48);

		j3dSys.unk4C = 3;
		unk394->draw();
		j3dSys.unk4C = 4;
		unk398->draw();

		MtxPtr viewMtx = gfx->mViewMtx.mMtx;
		boxDrawPrepare(viewMtx);

		GXColor color = gpSilhouetteManager->unk12;
		GXSetChanMatColor(GX_COLOR0A0, color);
		GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA,
		               GX_LO_NOOP);
		GXSetColorUpdate(GX_TRUE);
		GXSetAlphaUpdate(GX_TRUE);
		GXSetDstAlpha(GX_TRUE, 0);
		GXDrawCube();

		j3dSys.mFlags &= ~2;
	}

	if (mSubState & 1)
		TTimeRec::endTimer();
}

BOOL TMario::isMario()
{
	if (gpMarioOriginal == this)
		return TRUE;
	return FALSE;
}
