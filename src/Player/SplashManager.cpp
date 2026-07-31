#include <Player/SplashManager.hpp>
#include <System/StageUtil.hpp>
#include <MarioUtil/DLUtil.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

extern f32 SMSGetAnmFrameRate();

TSplashManager* gpSplashManager;

void TSplashManager::load(JSUMemoryInputStream& stream)
{
	TViewObj::load(stream);

	mFlags = 3;

	ResTIMG* timg
	    = (ResTIMG*)JKRFileLoader::getGlbResource("/mario/timg/splash.bti");
	mTexture = new JUTTexture(timg);

	mUnk630 = 50.0f;
	mUnk634 = 100.0f;

	f32 rate = SMSGetAnmFrameRate();
	mGravity = -0.5f * SMSGetAnmFrameRate() * rate;

	mColor = (GXColor){ 0xA8, 0xCB, 0xE3, 0xFF };

	mActiveList.initiate();
	mFreeList.initiate();

	for (s32 i = 0; i < 0x40; i++) {
		mLinks[i]           = new JSULink<TWaterSplash>(&mSplashes[i]);
		mSplashes[i].mPos.z = 0.0f;
		mSplashes[i].mPos.y = 0.0f;
		mSplashes[i].mPos.x = 0.0f;
		mSplashes[i].mVelY  = 0.0f;
		mSplashes[i].mLife  = 0;
		mSplashes[i].mIndex = (u8)i;
		mFreeList.append(mLinks[i]);
	}

	mQuad = new TDLColorTexQuad();
	mQuad->createBuffer(0x40);

	mUnk644   = 3000.0f;
	mInitLife = 0x10;

	gpSplashManager = this;
}

void TSplashManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 2)
		move();
	if (mFlags & 1) {
		if (flags & 4)
			makeDL(gfx);
		if (flags & 8)
			draw();
	}
}

void TSplashManager::newSplash(JGeometry::TVec3<f32> pos, f32 size)
{
	if (mFreeList.getNumLinks() != 0 && !SMS_isDivingMap()) {
		JSULink<TWaterSplash>* link = mFreeList.getFirst();
		mFreeList.remove(link);
		mActiveList.append(link);

		TWaterSplash* splash = link->getObject();
		splash->mPos          = pos;
		splash->mVelY         = size;
		splash->mLife         = mInitLife;
	}
}

#pragma dont_inline on
void TSplashManager::move()
{
	if (mFlags & 2) {
		JSULink<TWaterSplash>* link = mActiveList.getFirst();
		while (link != NULL) {
			TWaterSplash* splash = link->getObject();
			splash->mVelY += mGravity;
			splash->mPos.y += splash->mVelY;
			if (splash->mLife != 0)
				splash->mLife -= 1;
			if (splash->mLife == 0) {
				JSULink<TWaterSplash>* dead = link;
				link = link->getNext();
				mActiveList.remove(dead);
				mFreeList.append(dead);
			} else {
				link = link->getNext();
			}
		}
	}
}

void TSplashManager::makeDL(JDrama::TGraphics* gfx) const
{
	MtxPtr mtx = (MtxPtr)((u8*)gfx + 0xb4);

	mQuad->reset();

	JSULink<TWaterSplash>* link = mActiveList.getFirst();
	while (link != NULL) {
		TWaterSplash* splash = link->getObject();

		JGeometry::TVec3<f32> out;
		PSMTXMultVec(mtx, (Vec*)&splash->mPos, (Vec*)&out);

		if (out.z < -mUnk644 || -250.0f < out.z) {
			splash->mLife = 0;
		} else {
			f32 ratio = ((f32)mInitLife - (f32)splash->mLife) / (f32)mInitLife;
			f32 size  = mUnk634 * ratio + mUnk630;

			GXColor col = { 0xFF, 0xFF, 0xFF, 0x00 };
			col.a       = (u8)(splash->mLife * 0xFF / mInitLife);

			f32 xl = out.x - size;
			f32 xr = out.x + size;
			f32 yt = out.y + size;
			f32 yb = out.y - size;

			JGeometry::TVec3<f32> quad[4];
			quad[0].set(xl, yt, out.z);
			quad[1].set(xr, yt, out.z);
			quad[2].set(xr, yb, out.z);
			quad[3].set(xl, yb, out.z);

			mQuad->requestCol(quad, col, splash->mIndex);
		}

		link = link->getNext();
	}

	mQuad->setEnd();
}

void TSplashManager::draw() const
{
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
	GXSetVtxDesc(GX_VA_CLR0, GX_INDEX8);
	GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);

	Mtx mtx;
	PSMTXIdentity(mtx);
	GXSetCurrentMtx(0);
	GXLoadPosMtxImm(mtx, 0);
	GXLoadNrmMtxImm(mtx, 0);

	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3c, 0, 0x7d);
	GXSetCullMode(GX_CULL_NONE);

	mTexture->load(GX_TEXMAP0);
	GXSetTevColor(GX_TEVREG0, mColor);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_C0, GX_CC_ONE, GX_CC_TEXC, GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE,
	                GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE,
	                GX_TEVPREV);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);

	GXSetClipMode(GX_CLIP_DISABLE);
	mQuad->draw();
	GXSetClipMode(GX_CLIP_ENABLE);
}
#pragma dont_inline off
