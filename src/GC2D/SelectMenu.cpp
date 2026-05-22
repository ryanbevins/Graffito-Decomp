#include <GC2D/SelectMenu.hpp>
#include <GC2D/SelectShine2.hpp>
#include <GC2D/ExPane.hpp>
#include <GC2D/BoundPane.hpp>
#include <System/SelectDir.hpp>
#include <System/StageUtil.hpp>
#include <System/Application.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <JSystem/J2D/J2DPane.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/JUtility/JUTRect.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <GC2D/MessageUtil.hpp>
#include <stdio.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";

static const u32 scNormalStageTable[] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0xD, 0x6, 0x8, 0x9, 0xA,
};

void TSelectMenu::startOpenWindow()
{
	if (_14A != 0)
		return;

	mState = 1;

	m2CPane->mVisible         = false;
	m30ExPane->mPane->mVisible = false;
	m40ExPane->mPane->mVisible = false;
	mA0Pane->mVisible         = false;
	mA4Pane->mVisible         = false;

	s32 frame = (s32)(30.0f * _14C);

	JUTRect rect = m24ExPane->mPane->mBounds;
	s32 w1       = rect.x2 - rect.x1;
	s32 h1       = rect.y2 - rect.y1;
	m24ExPane->setPaneSize(frame, w1, h1, w1, 0);

	rect       = m28ExPane->mPane->mBounds;
	s32 w2     = rect.x2 - rect.x1;
	s32 h2     = rect.y2 - rect.y1;
	m28ExPane->setPaneSize(frame, w2, h2, w2, 0);

	s32 h3 = rect.y2 - rect.y1;
	m28ExPane->setPaneOffset(frame, 0, 0, 0, h3);

	MSBgm::startBGM(0x80010024);
	_138 = 0;
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
	JPAEmitterManager* em = mDir->mEmitterMgr1;
	mShineManager->initData(&mStageStates[0], _13C, mScenarioIndex, em);
	mShineManager->mShines[mScenarioIndex]->unk24 = 1;
}

void TSelectMenu::initData(u8 cup, JKRArchive* archive,
                           TSelectShineManager* shineMgr, TSelectDir* dir)
{
	_13A          = cup;
	mShineManager = shineMgr;
	mDir          = dir;

	_14C = 1.0f / SMSGetAnmFrameRate();

	switch (_13A) {
	case 0:
	case 1:
	case 10:
		_14A           = 1;
		mScenarioIndex = 0xFF;
		return;
	}

	// TODO: full implementation of main path pending — large (~3500B).
	mScreen   = new J2DSetScreen("scenario_select_1.blo", archive);
	m24ExPane = new TExPane(mScreen, 'msk1');
	m28ExPane = new TExPane(mScreen, 'msk2');
	m2CPane   = mScreen->search('map');
	m40ExPane = new TExPane(mScreen, 's_0');
	m68ExPane = new TExPane(mScreen, '0_0');
	m68ExPane->mPane->mVisible = false;
	*(JUTRect*)_58             = m40ExPane->mPane->mBounds;

	*(J2DPane**)((u8*)this + 0x48) = mScreen->search('s_2a');
	*(J2DPane**)((u8*)this + 0x4C) = mScreen->search('s_2b');
	*(J2DPane**)((u8*)this + 0x70) = mScreen->search('0_2a');
	*(J2DPane**)((u8*)this + 0x74) = mScreen->search('0_2b');
	*(J2DPane**)((u8*)this + 0x50) = mScreen->search('s_2b');
	*(J2DPane**)((u8*)this + 0x78) = mScreen->search('0_2b');

	for (s32 i = 0; i < 8; i++) {
		char buf[256];
		snprintf(buf, 0xfe, "/select/timg/sc_number_%d.bti", i + 1);
		JUTTexture* tex = new JUTTexture(
		    (const ResTIMG*)JKRFileLoader::getGlbResource(buf));
		*(JUTTexture**)((u8*)this + 0x80 + i * 4) = tex;
	}

	*(J2DTextBox**)((u8*)this + 0x44) = (J2DTextBox*)mScreen->search('sttx');
	SMSMakeTextBuffer(*(J2DTextBox**)((u8*)this + 0x44), 0x80);
	*(J2DTextBox**)((u8*)this + 0x6C) = (J2DTextBox*)mScreen->search('0ttx');
	SMSMakeTextBuffer(*(J2DTextBox**)((u8*)this + 0x6C), 0x80);

	*(s16*)((u8*)this + 0x7C)
	    = (s16)(m68ExPane->mPane->mBounds.x1 - m40ExPane->mPane->mBounds.x1);

	mA0Pane = mScreen->search('i_0');
	mA4Pane = mScreen->search('sc_0');

	for (s32 i = 0; i < 10; i++) {
		char buf[256];
		snprintf(buf, 0x100, "/select/timg/coin_number_%d.bti", i);
		JUTTexture* tex = new JUTTexture(
		    (const ResTIMG*)JKRFileLoader::getGlbResource(buf));
		*(JUTTexture**)((u8*)this + 0xA8 + i * 4) = tex;
	}
}

TSelectMenu::TSelectMenu(const char* name)
    : JDrama::TViewObj(name)
{
	mState                    = 0;
	mScreen                   = nullptr;
	m24ExPane                 = nullptr;
	m28ExPane                 = nullptr;
	m2CPane                   = nullptr;
	m30ExPane                 = nullptr;
	m38ExPane                 = nullptr;
	*(u32*)((u8*)this + 0x3C) = 0;
	m40ExPane                 = nullptr;
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
	mA0Pane                   = nullptr;
	mA4Pane                   = nullptr;
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
		s32 i;
		bool changed = false;
		for (i = 0; i < 3; i++) {
			u8* col2_byte;
			if (i == 0)
				col2_byte = &mColor2.r;
			else if (i == 1)
				col2_byte = &mColor2.g;
			else
				col2_byte = &mColor2.b;

			switch ((&_10)[i]) {
			case 0: {
				s32 v = (s16)((u8)*col2_byte + 2);
				if (v > 0xff) {
					v       = 0xff;
					changed = true;
				}
				*col2_byte = (u8)v;
				break;
			}
			case 3: {
				s16 v = (s16)(*col2_byte - 2);
				if (v < 0) {
					v       = 0;
					changed = true;
				}
				*col2_byte = (u8)v;
				break;
			}
			}

			s32 mode2 = (&_10)[i] - 1;
			if (mode2 < 0)
				mode2 = 5;

			u8* col1_byte;
			if (i == 0)
				col1_byte = &mColor1.r;
			else if (i == 1)
				col1_byte = &mColor1.g;
			else
				col1_byte = &mColor1.b;

			switch (mode2) {
			case 0: {
				s32 v = (s16)((u8)*col1_byte + 2);
				if (v > 0xff)
					v = 0xff;
				*col1_byte = (u8)v;
				break;
			}
			case 3: {
				s16 v = (s16)(*col1_byte - 2);
				if (v < 0)
					v = 0;
				*col1_byte = (u8)v;
				break;
			}
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
		GXSetCullMode(GX_CULL_BACK);
		GXSetNumTexGens(0);
		GXSetNumTevStages(1);
		GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GXSetNumChans(1);
		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);

		static const GXColor cAmbColor = { 0xFF, 0xFF, 0xFF, 0xFF };
		GXColor ambColor = cAmbColor;
		GXSetChanAmbColor(GX_COLOR0A0, ambColor);

		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGB, GX_RGB8, 0);
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);

		u8 bmid = (u8)((mColor2.b + mColor1.b) >> 1);
		u8 gmid = (u8)((mColor2.g + mColor1.g) >> 1);
		u8 rmid = (u8)((mColor2.r + mColor1.r) >> 1);

		GXBegin(GX_QUADS, GX_VTXFMT0, 4);
		GXPosition3f32(0.0f, 16.0f, -100.0f);
		GXColor3u8(mColor1.r, mColor1.g, mColor1.b);
		GXPosition3f32(600.0f, 16.0f, -100.0f);
		GXColor3u8(rmid, gmid, bmid);
		GXPosition3f32(600.0f, 464.0f, -100.0f);
		GXColor3u8(mColor2.r, mColor2.g, mColor2.b);
		GXPosition3f32(0.0f, 464.0f, -100.0f);
		GXColor3u8(rmid, gmid, bmid);
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
