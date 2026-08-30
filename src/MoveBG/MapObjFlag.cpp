#define JGEOMETRY_GEKKO_PS_COPY12_OUT_OF_LINE
#include <MoveBG/MapObjFlag.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <System/MarDirector.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JMath.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>
#include <MSound/MSoundBGM.hpp>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

f32 TMapObjFlag::mFlutterSpeed = 4.0f;

TMapObjFlagManager* gpMapObjFlagManager;

TMapObjFlagManager::TMapObjFlagInfo::TMapObjFlagInfo()
{
	mCount   = 0;
	mTexture = nullptr;
}

TMapObjFlagManager::TMapObjFlagManager(const char* name)
    : JDrama::TViewObj(name)
{
	gpMapObjFlagManager = this;
}

TMapObjFlagManager::~TMapObjFlagManager() { }

void TMapObjFlagManager::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);

	char buf[8];
	stream.readString(buf, 8);

	switch (gpMarDirector->mMap) {
	case 2:
		TMapObjFlag::mFlutterSpeed = 16.0f;
		break;
	case 0:
		TMapObjFlag::mFlutterSpeed = 16.0f;
		break;
	case 4:
		TMapObjFlag::mFlutterSpeed = 12.0f;
		break;
	default:
		TMapObjFlag::mFlutterSpeed = 8.0f;
		break;
	}
}

#define REGISTER_FLAG(N, NAME)                                                 \
	if (strcmp(name, NAME) == 0) {                                             \
		if (mInfo[N].mTexture == nullptr) {                                    \
			char buf[64];                                                      \
			snprintf(buf, 64, "/scene/mapObj/%s.bti", name);                   \
			mInfo[N].mTexture                                                  \
			    = (ResTIMG*)JKRFileLoader::getGlbResource(buf);                \
		}                                                                      \
		mInfo[N].mFlags[mInfo[N].mCount] = flag;                               \
		mInfo[N].mCount++;                                                     \
		return;                                                                \
	}

void TMapObjFlagManager::registerObj(TMapObjFlag* flag, const char* name)
{
	REGISTER_FLAG(0, "flagSun")
	REGISTER_FLAG(1, "flagWhite")
	REGISTER_FLAG(2, "flagRedsun")
	REGISTER_FLAG(3, "flagMonte")
	REGISTER_FLAG(4, "flagBird")
	REGISTER_FLAG(5, "flagHigekuri")
	REGISTER_FLAG(6, "flagBenvenuto")
	REGISTER_FLAG(7, "flagDolpicDolphin")
	REGISTER_FLAG(8, "flagDolSun")
	REGISTER_FLAG(9, "flagDolSunWelcome")
	REGISTER_FLAG(10, "flagBianco")
	REGISTER_FLAG(11, "flagRiccoBuoy")
	REGISTER_FLAG(12, "flagSailMonte")
	REGISTER_FLAG(13, "MammaYacht00")
	REGISTER_FLAG(14, "flagMare")
}

void TMapObjFlagManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 2) {
		for (int i = 0; i < 15; i++) {
			for (int j = 0; j < mInfo[i].mCount; j++) {
				TMapObjFlag* flag = mInfo[i].mFlags[j];

				MsMtxSetXYZRPH(flag->mLocalMtx, flag->mPosition.x,
				               flag->mPosition.y, flag->mPosition.z,
				               flag->mRotation.x, flag->mRotation.y,
				               flag->mRotation.z);

				flag->updateVertex();

				flag->mPhase += TMapObjFlag::mFlutterSpeed;
				if (flag->mPhase > 360.0f) {
					flag->mPhase -= 360.0f;
				}

				if (flag->mScaling.y > 3.0f && flag->mScaling.z > 3.0f) {
					if (gpMSound->gateCheck(0x302f)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x302f, &flag->mPosition, 0, nullptr, 0, 4);
					}
				}
			}
		}
	}

	if (flags & 8) {
		initDraw();
		for (int i = 0; i < 15; i++) {
			if (mInfo[i].mCount != 0) {
				JUTTexture texture(mInfo[i].mTexture);
				texture.load(GX_TEXMAP0);
				for (int j = 0; j < mInfo[i].mCount; j++) {
					mInfo[i].mFlags[j]->draw();
				}
			}
		}
	}
}

void TMapObjFlagManager::initDraw()
{
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GXSetCurrentMtx(0);

	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);

	GXSetChanMatColor(GX_COLOR0A0,
	                  (GXColor) { 0xff, 0xff, 0xff, 0xff });

	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);

	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);

	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetZCompLoc(0);
	GXSetCullMode(GX_CULL_NONE);
}

TMapObjFlag::TMapObjFlag(const char* name)
    : THitActor(name)
{
	mScaledHeight = 0.0f;
	mScaledWidth  = 0.0f;
	mNumRows      = 0;
	mNumCols      = 0;
	mVertexGrid   = nullptr;
	mFlagHeight   = 125.0f;
	mFlagWidth    = 130.0f;
	mSegmentSize  = 20.0f;
	mStepSize     = 1;
	mPhase        = 360.0f * ((f32)rand() * 0.000030517578f);

	mLocalMtx[0][3] = mLocalMtx[1][3] = mLocalMtx[2][3] = 0.0f;
	mLocalMtx[0][2] = mLocalMtx[1][2] = 0.0f;
	mLocalMtx[0][1] = mLocalMtx[2][1] = 0.0f;
	mLocalMtx[1][0] = mLocalMtx[2][0] = 0.0f;
	mLocalMtx[0][0] = mLocalMtx[1][1] = mLocalMtx[2][2] = 1.0f;
}

TMapObjFlag::~TMapObjFlag() { }

void TMapObjFlag::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	char buf[64];
	stream.readString(buf, 64);
	init(buf);
}

void TMapObjFlag::init(const char* name)
{
	mScaledHeight = 100.0f * mScaling.z;
	mScaledWidth  = 100.0f * mScaling.y;
	mFlagHeight /= mScaling.z;
	mFlagWidth /= mScaling.y;
	mSegmentSize *= mScaling.z;

	mNumRows = (int)(mScaledHeight / 50.0f);
	mNumCols = (int)(mScaledWidth / 100.0f);

	if (mNumRows < 2)
		mNumRows = 3;
	if (mNumCols < 2)
		mNumCols = 3;

	MsMtxSetXYZRPH(mLocalMtx, mPosition.x, mPosition.y, mPosition.z,
	               mRotation.x, mRotation.y, mRotation.z);

	f32 stepHeight = mScaledHeight / (f32)mNumRows;
	f32 stepWidth  = mScaledWidth / (f32)mNumCols;

	JKRHeap::sCurrentHeap->getTotalFreeSize();
	mVertexGrid = (Vec**)new u8[mNumCols * sizeof(Vec*)];

	for (int col = 0; col < mNumCols; col++) {
		JGeometry::TVec3<f32>* rowArr
		    = new JGeometry::TVec3<f32>[mNumRows];
		mVertexGrid[col] = rowArr;
		f32 colCoord     = (f32)col * stepWidth;
		for (int row = 0; row < mNumRows; row++) {
			rowArr[row].x = 0.0f;
			rowArr[row].y = colCoord;
			rowArr[row].z = (f32)row * stepHeight;
		}
	}

	static u32 total_use_size = 0;
	JKRHeap::sCurrentHeap->getTotalFreeSize();

	gpMapObjFlagManager->registerObj(this, name);

	initHitActor(0x4000000D, 1, 0, 0.0f, 0.0f, 0.0f, 0.0f);
}

void TMapObjFlag::updateVertex()
{
	for (int col = 0; col < mNumCols; col += mStepSize) {
		f32 colTerm = (f32)col * mFlagWidth;
		for (int row = 0; row < mNumRows; row += mStepSize) {
			f32 rowRatio = (f32)row / (f32)mNumRows;
			f32 phase    = mPhase + (f32)(-row) * mFlagHeight + colTerm;

			while (phase >= 180.0f)
				phase -= 360.0f;
			while (phase < -180.0f)
				phase += 360.0f;

			f32 scaledPhase         = phase * 182.04445f;
			s16 angle               = (s16)scaledPhase;
			f32 sinVal              = JMASSin(angle);
			mVertexGrid[col][row].x = mSegmentSize * rowRatio * sinVal;
		}
	}
}

void TMapObjFlag::draw()
{
	Mtx mtx;
	JGeometry::gekko_ps_copy12(mtx, j3dSys.mViewMtx);
	PSMTXConcat(mtx, mLocalMtx, mtx);
	GXLoadPosMtxImm(mtx, 0);

	int vertsPerStrip
	    = (((mNumRows - 2 * mStepSize) / mStepSize + 2) << 1) & 0xfffe;

	f32 invRows = 1.0f / (f32)(mNumRows - 1);
	f32 invCols = 1.0f / (f32)(mNumCols - 1);

	for (int col = 0; col < mNumCols - mStepSize; col += mStepSize) {
		f32 u0 = (f32)(mNumCols - 1 - col) * invCols;
		f32 u1 = (f32)(mNumCols - 1 - (col + 1)) * invCols;

		GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, (u16)vertsPerStrip);

		Vec& v0 = mVertexGrid[col][0];
		GXPosition3f32(v0.x, v0.y, v0.z);
		GXTexCoord2f32(0.0f, u0);

		Vec& v1 = mVertexGrid[col + 1][0];
		GXPosition3f32(v1.x, v1.y, v1.z);
		GXTexCoord2f32(0.0f, u1);

		for (int row = 1; row < mNumRows - mStepSize;
		     row += mStepSize) {
			f32 vCoord = (f32)row * invRows;

			Vec& va = mVertexGrid[col][row];
			GXPosition3f32(va.x, va.y, va.z);
			GXTexCoord2f32(vCoord, u0);

			Vec& vb = mVertexGrid[col + 1][row];
			GXPosition3f32(vb.x, vb.y, vb.z);
			GXTexCoord2f32(vCoord, u1);
		}

		Vec& vLastA = mVertexGrid[col][mNumRows - 1];
		GXPosition3f32(vLastA.x, vLastA.y, vLastA.z);
		GXTexCoord2f32(1.0f, u0);

		Vec& vLastB = mVertexGrid[col + 1][mNumRows - 1];
		GXPosition3f32(vLastB.x, vLastB.y, vLastB.z);
		GXTexCoord2f32(1.0f, u1);
	}
}
