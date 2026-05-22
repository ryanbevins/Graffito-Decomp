#include <GC2D/SelectMenu.hpp>
#include <GC2D/SelectShine2.hpp>
#include <System/SelectDir.hpp>
#include <System/StageUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <JSystem/JUtility/JUTRect.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";

static const u32 scNormalStageTable[] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0xD, 0x6, 0x8, 0x9, 0xA,
};

void TSelectMenu::startOpenWindow()
{
	// TODO: full implementation pending — large fn (0x24C bytes), defer.
	if (mState != 0)
		return;
	mState = 1;
}

int TSelectMenu::getPrevIndex()
{
	u32 idx = mScenarioIndex;
	s8 result = -1;
	if (idx == 0)
		return -1;
	idx--;
	for (s32 i = idx + 1; i > 0; i--) {
		u8 state = mStageStates[idx];
		if (state == 2 || state == 3) {
			result = idx;
			break;
		}
		idx--;
	}
	return result;
}

int TSelectMenu::getNextIndex()
{
	u32 idx = mScenarioIndex;
	s8 result = -1;
	if (idx >= 8)
		return -1;
	idx++;
	for (s32 i = idx; i < 8; i++) {
		u8 state = mStageStates[idx];
		if (state == 2 || state == 3) {
			result = idx;
			break;
		}
		idx++;
	}
	return result;
}

void TSelectMenu::perform(u32 flags, JDrama::TGraphics* gfx)
{
	// TODO: full implementation pending — very large fn (0x180C bytes).
}

void TSelectMenu::startMove()
{
	mShineManager->initData(&mStageStates[0], _13C, mScenarioIndex,
	                        mDir->mEmitterMgr1);
	mShineManager->mShines[mScenarioIndex]->unk24 = 1;
}

void TSelectMenu::initData(u8 cup, JKRArchive* archive,
                           TSelectShineManager* shineMgr, TSelectDir* dir)
{
	// TODO: full implementation pending — very large fn (0xFE4 bytes).
	mShineManager = shineMgr;
	mDir          = dir;
}

TSelectMenu::TSelectMenu(const char* name)
    : JDrama::TViewObj(name)
{
	mState = 0;
	*(u32*)((u8*)this + 0x20) = 0;
	*(u32*)((u8*)this + 0x24) = 0;
	*(u32*)((u8*)this + 0x28) = 0;
	*(u32*)((u8*)this + 0x2C) = 0;
	*(u32*)((u8*)this + 0x30) = 0;
	*(u32*)((u8*)this + 0x38) = 0;
	*(u32*)((u8*)this + 0x3C) = 0;
	*(u32*)((u8*)this + 0x40) = 0;
	*(u32*)((u8*)this + 0x44) = 0;
	*(u32*)((u8*)this + 0x48) = 0;
	*(u32*)((u8*)this + 0x4C) = 0;
	*(u32*)((u8*)this + 0x50) = 0;
	*(u8*)((u8*)this + 0x54)  = 0;
	((JUTRect*)_58)->set(0, 0, 0, 0);

	*(u32*)((u8*)this + 0x68) = 0;
	*(u32*)((u8*)this + 0x6C) = 0;
	*(u32*)((u8*)this + 0x70) = 0;
	*(u32*)((u8*)this + 0x74) = 0;
	*(u32*)((u8*)this + 0x78) = 0;
	*(u16*)((u8*)this + 0x7C) = 0;
	*(u32*)((u8*)this + 0xA0) = 0;
	*(u32*)((u8*)this + 0xA4) = 0;
	*(u32*)((u8*)this + 0xD0) = 0;
	*(u32*)((u8*)this + 0xD4) = 0;
	*(u8*)((u8*)this + 0xD8)  = 1;
	*(u32*)((u8*)this + 0x104) = 0;
	*(u32*)((u8*)this + 0x108) = 0;
	*(u8*)((u8*)this + 0x10C)  = 1;
	*(u8*)((u8*)this + 0x10D)  = 0;
	((JUTRect*)_110)->set(0, 0, 0, 0);

	((JUTRect*)_120)->set(0, 0, 0, 0);
	_138 = 0;
	_139 = 0;
	_13A = 0;
	mScenarioIndex = 0;
	_13C = 0;
	_140 = -1;
	_144 = -1;
	_148 = 0;
	_149 = 0;
	_14A = 0;
	_14B = 0;
	_14C = 0.0f;
	_158 = 0;
	_15C = 0;
	_160 = -1;
	_164 = -1;
	_168 = 200;
	_16A = 200;
	_16C = 10;
}

void TSelectGrad::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 2) {
		bool changed = false;
		for (s32 i = 0; i < 3; i++) {
			u8* col2_byte = (&mColor2.r) + i;
			s32 mode = (&_10)[i];

			if (mode == 0) {
				s32 v = (s16)((u8)*col2_byte + 2);
				if (v > 0xff) {
					v        = 0xff;
					changed  = true;
				}
				*col2_byte = (u8)v;
			} else if (mode == 3) {
				s32 v = (s16)(s32)(*col2_byte - 2);
				v     = (s16)v;
				if (v < 0) {
					v        = 0;
					changed  = true;
				}
				*col2_byte = (u8)v;
			}

			s32 mode2 = (&_10)[i];
			if (mode2 - 1 < 0)
				mode2 = 5;
			u8* col1_byte = (&mColor1.r) + i;
			if (mode2 == 0) {
				s32 v = (s16)((u8)*col1_byte + 2);
				if (v > 0xff)
					v = 0xff;
				*col1_byte = (u8)v;
			} else if (mode2 == 3) {
				s32 v = (s16)(s32)(*col1_byte - 2);
				v     = (s16)v;
				if (v < 0)
					v = 0;
				*col1_byte = (u8)v;
			}
		}

		if (changed) {
			_10++;
			if (_10 >= 6)
				_10 = 0;
			_14++;
			if (_14 >= 6)
				_14 = 0;
			_18++;
			if (_18 >= 6)
				_18 = 0;
		}
	}

	if (flags & 8) {
		GXSetDither(GX_TRUE);
		Mtx mtx;
		PSMTXIdentity(mtx);
		GXLoadPosMtxImm(mtx, 0);
		GXSetCullMode(GX_CULL_NONE);
		GXSetNumTexGens(0);
		GXSetNumTevStages(1);
		GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GXSetNumChans(1);
		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0,
		              GX_DF_NONE, GX_AF_SPEC);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_SPEC);

		GXColor ambColor;
		*(u32*)&ambColor = 0xFFFFFFFFu;
		GXSetChanAmbColor(GX_COLOR0A0, ambColor);

		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);

		u8 rmid = (u8)((mColor2.r + mColor1.r) >> 1);
		u8 gmid = (u8)((mColor2.g + mColor1.g) >> 1);
		u8 bmid = (u8)((mColor2.b + mColor1.b) >> 1);

		GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GXPosition3f32(0.0f, 0.0f, 0.0f);
		GXColor4u8(mColor1.r, mColor1.g, mColor1.b, 0xFF);
		GXPosition3f32(600.0f, 0.0f, 0.0f);
		GXColor4u8(rmid, gmid, bmid, 0xFF);
		GXPosition3f32(0.0f, 480.0f, 0.0f);
		GXColor4u8(rmid, gmid, bmid, 0xFF);
		GXPosition3f32(600.0f, 480.0f, 0.0f);
		GXColor4u8(mColor2.r, mColor2.g, mColor2.b, 0xFF);
	}
}

void TSelectGrad::setStageColor(u8 cup)
{
	switch (cup) {
	case 2:
		_10 = 3;
		_14 = 1;
		_18 = 5;
		mColor1.set(0xFF, 0xFF, 0x00, 0xFF);
		mColor2.set(0x00, 0xFF, 0x00, 0xFF);
		break;
	case 3:
		_10 = 4;
		_14 = 2;
		_18 = 0;
		mColor1.set(0x00, 0xFF, 0x00, 0xFF);
		mColor2.set(0x00, 0xFF, 0xFF, 0xFF);
		break;
	case 4:
		_10 = 2;
		_14 = 0;
		_18 = 4;
		mColor1.set(0xFF, 0x00, 0x00, 0xFF);
		mColor2.set(0xFF, 0xFF, 0x00, 0xFF);
		break;
	case 13:
		_10 = 0;
		_14 = 4;
		_18 = 2;
		mColor1.set(0x00, 0x00, 0xFF, 0xFF);
		mColor2.set(0xFF, 0x00, 0xFF, 0xFF);
		break;
	default:
		break;
	}
}

TSelectGrad::TSelectGrad(const char* name)
    : JDrama::TViewObj(name)
{
	_10 = 2;
	_14 = 0;
	_18 = 4;
	mColor1.set(0xFF, 0x00, 0x00, 0xFF);
	mColor2.set(0xFF, 0xFF, 0x00, 0xFF);
}
