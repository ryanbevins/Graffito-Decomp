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
#include <JSystem/J2D/J2DPicture.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/JUtility/JUTRect.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <GC2D/MessageUtil.hpp>
#include <System/FlagManager.hpp>
#include <stdio.h>
#include <string.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

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

s8 TSelectMenu::getPrevIndex()
{
	u32 idx = mScenarioIndex;
	s8 result = -1;
	if (idx == 0)
		return -1;
	for (s32 i = idx - 1; i >= 0; i--) {
		u8 state = mStageStates[i];
		if (state == 2 || state == 3) {
			result = i;
			break;
		}
	}
	return result;
}

s8 TSelectMenu::getNextIndex()
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
	if (!(flags & 1))
		return;
	if (mState > 9)
		return;
	switch (mState) {
	case 0:
		break;
	case 1: {
		bool done = true;
		done &= m24ExPane->update();
		done &= m28ExPane->update();
		if (done || _138 > (s32)(20.0f * _14C)) {
			m30ExPane->mPane->mVisible = true;
			m30ExPane->setPaneOffset(
			    (s32)(20.0f * _14C), 0, 0,
			    0x259 - m30ExPane->mInitialBounds.x1, 0);
			mState = 2;
		}
		_138++;
		break;
	}
	case 2: {
		bool done = true;
		done &= m24ExPane->update();
		done &= m28ExPane->update();
		done &= m30ExPane->update();
		if (!done)
			break;

		JUTRect rect = ((TBoundPane*)m34ExPane)->unk0->mBounds;
		s32 frame    = (s32)(15.0f * _14C);

		((TBoundPane*)m34ExPane)
		    ->setPanePosition(frame, JUTPoint(0, 0), JUTPoint(0, -6),
		                      JUTPoint(0, -10));
		((TBoundPane*)m34ExPane)
		    ->setPaneSize(frame, JUTPoint(0, 0), JUTPoint(-110, 12),
		                  JUTPoint(-160, 20));
		((TBoundPane*)m38ExPane)
		    ->setPanePosition(frame, JUTPoint(0, 0), JUTPoint(0, -6),
		                      JUTPoint(0, -10));
		((TBoundPane*)m38ExPane)
		    ->setPaneSize(frame, JUTPoint(0, 0), JUTPoint(-110, 12),
		                  JUTPoint(-160, 20));
		mState = 3;
		break;
	}
	case 3: {
		bool done = true;
		done &= ((TBoundPane*)m34ExPane)->update();
		done &= ((TBoundPane*)m38ExPane)->update();
		if (!done)
			break;

		JUTRect rect = ((TBoundPane*)m34ExPane)->unk0->mBounds;
		s32 frame    = (s32)(20.0f * _14C);

		((TBoundPane*)m34ExPane)
		    ->setPanePosition(frame, JUTPoint(0, -10), JUTPoint(0, -6),
		                      JUTPoint(0, 4));
		((TBoundPane*)m34ExPane)
		    ->setPaneSize(frame, JUTPoint(-160, 20),
		                  JUTPoint(-110, 12), JUTPoint(40, -8));
		((TBoundPane*)m38ExPane)
		    ->setPanePosition(frame, JUTPoint(0, -10), JUTPoint(0, -6),
		                      JUTPoint(0, 4));
		((TBoundPane*)m38ExPane)
		    ->setPaneSize(frame, JUTPoint(-160, 20),
		                  JUTPoint(-110, 12), JUTPoint(40, -8));
		mState = 4;
		break;
	}
	case 4: {
		bool done = true;
		done &= ((TBoundPane*)m34ExPane)->update();
		done &= ((TBoundPane*)m38ExPane)->update();
		if (!done)
			break;

		JUTRect rect = ((TBoundPane*)m34ExPane)->unk0->mBounds;
		s32 frame    = (s32)(15.0f * _14C);

		((TBoundPane*)m34ExPane)
		    ->setPanePosition(frame, JUTPoint(0, 4), JUTPoint(0, 3),
		                      JUTPoint(0, 0));
		((TBoundPane*)m34ExPane)
		    ->setPaneSize(frame, JUTPoint(40, -8), JUTPoint(15, -6),
		                  JUTPoint(0, 0));
		((TBoundPane*)m38ExPane)
		    ->setPanePosition(frame, JUTPoint(0, 4), JUTPoint(0, 3),
		                      JUTPoint(0, 0));
		((TBoundPane*)m38ExPane)
		    ->setPaneSize(frame, JUTPoint(40, -8), JUTPoint(15, -6),
		                  JUTPoint(0, 0));

		m2CPane->mVisible          = true;
		m2CPane->mAlpha            = 0;
		m40ExPane->mPane->mVisible = true;
		m40ExPane->mPane->mAlpha   = 0;
		mA0Pane->mVisible          = true;
		mA0Pane->mAlpha            = 0;
		mA4Pane->mVisible          = true;
		mA4Pane->mAlpha            = 0;

		mState = 5;
		break;
	}
	case 5: {
		bool done = true;
		done &= ((TBoundPane*)m34ExPane)->update();
		done &= ((TBoundPane*)m38ExPane)->update();

		u32 a2c = (u32)m2CPane->mAlpha;
		s32 newAlpha
		    = (s32)((f32)a2c + 6.0f * SMSGetAnmFrameRate());
		if ((u16)newAlpha > 0xff)
			newAlpha = 0xff;
		m2CPane->mAlpha = (u8)newAlpha;

		s32 alpha2 = 0;
		if ((u16)newAlpha > 0x80) {
			s32 a40 = (s32)m40ExPane->mPane->mAlpha;
			alpha2  = (s32)(((f32)a40 + 6.0f * SMSGetAnmFrameRate()
			                  > 255.0f)
			                    ? 255.0f
			                    : (f32)a40
			                          + 6.0f * SMSGetAnmFrameRate());
			m40ExPane->mPane->mAlpha = (u8)alpha2;
			mA0Pane->mAlpha          = (u8)alpha2;
			mA4Pane->mAlpha          = (u8)alpha2;
		}

		if (done && alpha2 == 0xff) {
			mState = 6;
		}
		break;
	}
	case 9:
		if (_139 > _16C)
			mState = 0;
		_139++;
		break;
	case 6:
		break;
	case 7:
		break;
	case 8:
		break;
	}
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

	J2DPicture* scenarioPics[3];
	for (s32 i = 0; i < 3; i++) {
		scenarioPics[i] = (J2DPicture*)mScreen->search(0x73635F31 + i);
	}

	const u32 stagePrefixes[11] = {
		0,          0,          0x0062695F, 0x0072635F, 0x006D6D5F,
		0x0070695F, 0x0073725F, 0,          0x006D6F5F, 0x006D725F,
		0,
	};
	const u32 stageMsgOffsets[11] = {
		0, 0, 2, 3, 4, 5, 6, 0, 7, 8, 0,
	};
	const u8* stageShineTables[11] = {
		nullptr,
		nullptr,
		scShineTableBiancoEtc,
		scShineTableRiccoEtc,
		scShineTableMammaEtc,
		scShineTablePinnaEtc,
		scShineTableSirenaEtc,
		nullptr,
		scShineTableMonteEtc,
		scShineTableMareEtc,
		nullptr,
	};

	TFlagManager* flagMgr = TFlagManager::smInstance;
	u32 coins = flagMgr->getFlag(
	    ((u32)SMS_getShineStage(_13A) << 16) + 0x20005);
	if (coins > 999)
		coins = 999;
	if (coins < 0)
		coins = 0;

	J2DPane* p100 = mScreen->search('sc_s');
	p100->mVisible = false;
	if (coins < 100) {
		s32 d10 = coins / 10;
		((J2DPicture*)scenarioPics[1])
		    ->changeTexture((const ResTIMG*)((JUTTexture**)((u8*)this
		                                                    + 0xA8))[d10]
		                        ->mTexInfo,
		                    0);
		s32 d1 = coins - d10 * 10;
		((J2DPicture*)scenarioPics[2])
		    ->changeTexture((const ResTIMG*)((JUTTexture**)((u8*)this
		                                                    + 0xA8))[d1]
		                        ->mTexInfo,
		                    0);
		((J2DPicture*)scenarioPics[0])->mVisible = false;
	} else {
		s32 d100 = coins / 100;
		((J2DPicture*)scenarioPics[0])
		    ->changeTexture((const ResTIMG*)((JUTTexture**)((u8*)this
		                                                    + 0xA8))[d100]
		                        ->mTexInfo,
		                    0);
		s32 rem  = coins - d100 * 100;
		s32 d10  = rem / 10;
		((J2DPicture*)scenarioPics[1])
		    ->changeTexture((const ResTIMG*)((JUTTexture**)((u8*)this
		                                                    + 0xA8))[d10]
		                        ->mTexInfo,
		                    0);
		s32 d1 = rem - d10 * 10;
		((J2DPicture*)scenarioPics[2])
		    ->changeTexture((const ResTIMG*)((JUTTexture**)((u8*)this
		                                                    + 0xA8))[d1]
		                        ->mTexInfo,
		                    0);
		if (flagMgr->getShineFlag(stageShineTables[_13A][0])) {
			J2DPane* p = mScreen->search('sc_s');
			p->mVisible = true;
		}
	}

	s8 shines = 0;
	for (s32 i = 1; i < 3; i++) {
		if (flagMgr->getShineFlag(stageShineTables[_13A][i]))
			shines++;
	}
	if (shines == 0) {
		J2DPane* p = mScreen->search('sc_0');
		// vtable+0x10 second virtual: probably setVisible or similar
		(*(void (**)(J2DPane*, s32, s32))(*(u32*)p + 0x10))(p, 0x53, 0);
		J2DPane* pi = mScreen->search('r_i');
		pi->mVisible = false;
	} else if (shines == 1) {
		J2DPane* p = mScreen->search('r_s2');
		p->mVisible = false;
	}

	// 'sc_mark_1.bti' / 'sc_mark_0.bti'
	JUTTexture* tex1 = new JUTTexture(
	    (const ResTIMG*)JKRFileLoader::getGlbResource(
	        "/select/timg/sc_mark_1.bti"));
	*(JUTTexture**)((u8*)this + 0xD0) = tex1;
	JUTTexture* tex0 = new JUTTexture(
	    (const ResTIMG*)JKRFileLoader::getGlbResource(
	        "/select/timg/sc_mark_0.bti"));
	*(JUTTexture**)((u8*)this + 0xD4) = tex0;

	*(u8*)((u8*)this + 0x160) = 0xFF;
	*(u8*)((u8*)this + 0x161) = 0;
	*(u8*)((u8*)this + 0x162) = 0;
	*(u8*)((u8*)this + 0x163) = 0xFF;
	*(u8*)((u8*)this + 0x164) = 0xFF;
	*(u8*)((u8*)this + 0x165) = 0xFF;
	*(u8*)((u8*)this + 0x166) = 0;
	*(u8*)((u8*)this + 0x167) = 0xFF;
	*(s32*)((u8*)this + 0x14) = 2;
	*(s32*)((u8*)this + 0x18) = 0;
	*(s32*)((u8*)this + 0x1C) = 4;

	mScreen->search(0x62695F30)->mVisible = false;

	TExPane* exA = new TExPane(
	    mScreen, (stagePrefixes[_13A] << 8) + 0x30);
	m30ExPane                  = exA;
	m30ExPane->mPane->mVisible = true;

	TBoundPane* boundA = new TBoundPane(
	    mScreen, (stagePrefixes[_13A] << 8) + 0x61);
	m34ExPane                  = (TExPane*)boundA;
	m34ExPane->mPane->mVisible = true;

	TBoundPane* boundB = new TBoundPane(
	    mScreen, (stagePrefixes[_13A] << 8) + 0x62);
	m38ExPane                  = (TExPane*)boundB;
	m38ExPane->mPane->mVisible = true;

	_158 = (s32)JKRFileLoader::getGlbResource("/common/2d/stagename.bmg");
	const char* stageName
	    = SMSGetMessageData((void*)_158, (u16)stageMsgOffsets[_13A]);
	strncpy(((J2DTextBox*)m2CPane)->getStringPtr(), stageName, 0x11);
	((J2DTextBox*)m2CPane)->setFont((JUTFont*)gpSystemFont);

	mStageStates[0] = 2;
	mStageStates[1] = 2;
	mStageStates[2] = 2;
	mStageStates[3] = 2;
	mStageStates[4] = 2;
	mStageStates[5] = 2;
	mStageStates[6] = 2;
	mStageStates[7] = 2;
	{
		s8 i;
		for (i = 0; i < 8; i++) {
			s16 shineId = SMS_getShineID(SMS_getShineStage(_13A),
			                             (u32)i, false);
			bool flag;
			if (shineId == -1)
				flag = false;
			else
				flag = TFlagManager::smInstance->getShineFlag(
				    (u8)shineId);
			if (flag) {
				mStageStates[i] = 3;
				_13C            = (u8)(i + 2);
			}
		}
	}
	if (_13C > 8)
		_13C = 8;
	if (_13C == 0)
		_13C = 1;
	{
		s32 idx = (s32)_13C - 1;
		if (idx < 0)
			idx = 0;
		mScenarioIndex = (u8)idx;
	}

	{
		s32 i;
		for (i = _13C; i < 8; i++)
			mStageStates[i] = 0;
	}

	if (_13C > 1) {
		J2DPane* paneL = mScreen->search(0x615F6C30 + _13C);
		*(J2DPane**)((u8*)this + 0x104) = paneL;
		J2DPane* paneR = mScreen->search(0x615F7230 + _13C);
		*(J2DPane**)((u8*)this + 0x108) = paneR;
		*(u32*)((u8*)this + 0x110) = *(u32*)((u8*)paneL + 0x14);
		*(u32*)((u8*)this + 0x114) = *(u32*)((u8*)paneL + 0x18);
		*(u32*)((u8*)this + 0x118) = *(u32*)((u8*)paneL + 0x1C);
		*(u32*)((u8*)this + 0x11C) = *(u32*)((u8*)paneL + 0x20);
		*(u32*)((u8*)this + 0x120) = *(u32*)((u8*)paneR + 0x14);
		*(u32*)((u8*)this + 0x124) = *(u32*)((u8*)paneR + 0x18);
		*(u32*)((u8*)this + 0x128) = *(u32*)((u8*)paneR + 0x1C);
		*(u32*)((u8*)this + 0x12C) = *(u32*)((u8*)paneR + 0x20);
		if (mScenarioIndex != 0)
			paneL->mVisible = true;
		if ((s32)mScenarioIndex != (s32)_13C - 1)
			paneR->mVisible = true;
	}

	*(u32*)((u8*)this + 0xDC) = 0;
	*(u32*)((u8*)this + 0xE0) = 0;
	*(u32*)((u8*)this + 0xE4) = 0;
	*(u32*)((u8*)this + 0xE8) = 0;
	*(u32*)((u8*)this + 0xEC) = 0;
	*(u32*)((u8*)this + 0xF0) = 0;
	*(u32*)((u8*)this + 0xF4) = 0;
	*(u32*)((u8*)this + 0xF8) = 0;
	if (_13C & 1) {
		s32 start   = (7 - _13C) / 2;
		s32 end     = start + (_13C - 1);
		s32 paneIdx = 0;
		for (s32 i = start; i <= end; i++) {
			J2DPane* pane = mScreen->search(0x695F6F30 + i);
			*(J2DPane**)((u8*)this + 0xDC + paneIdx * 4) = pane;
			pane->mVisible = true;
			u8 state = mStageStates[paneIdx];
			if (state == 2 || state == 1) {
				((J2DPicture*)pane)
				    ->insert(*(JUTTexture**)((u8*)this + 0xD4), 0,
				             1.0f);
				((J2DPicture*)pane)->remove(1);
			} else if (state == 0) {
				pane->mVisible = false;
			}
			paneIdx++;
		}
	} else {
		s32 start   = (8 - _13C) / 2;
		s32 end     = start + (_13C - 1);
		s32 paneIdx = 0;
		for (s32 i = start; i <= end; i++) {
			J2DPane* pane = mScreen->search(0x695F6530 + i);
			*(J2DPane**)((u8*)this + 0xDC + paneIdx * 4) = pane;
			pane->mVisible = true;
			u8 state = mStageStates[paneIdx];
			if (state == 2 || state == 1) {
				((J2DPicture*)pane)
				    ->insert(*(JUTTexture**)((u8*)this + 0xD4), 0,
				             1.0f);
				((J2DPicture*)pane)->remove(1);
			} else if (state == 0) {
				pane->mVisible = false;
			}
			paneIdx++;
		}
	}

	_140 = *(s32*)((u8*)mScreen->search(0x695F6F30) + 0x13C);
	*(u8*)((u8*)this + 0x148) = 0xFF;
	_144 = *(s32*)((u8*)mScreen->search(0x695F6F32) + 0x13C);
	*(u8*)((u8*)this + 0x149)
	    = *(u8*)((u8*)mScreen->search(0x695F6F32) + 0xCC);
	*(s32*)((u8*)mScreen->search(0x695F6F30) + 0x13C) = _144;
	*(u8*)((u8*)mScreen->search(0x695F6F30) + 0xCC)
	    = *(u8*)((u8*)this + 0x149);

	*(s32*)((u8*)(*(J2DPane**)((u8*)this + 0xDC + mScenarioIndex * 4))
	        + 0x13C)
	    = _140;
	*(u8*)((u8*)(*(J2DPane**)((u8*)this + 0xDC + mScenarioIndex * 4))
	       + 0xCC)
	    = *(u8*)((u8*)this + 0x148);

	((J2DPicture*)*(J2DPane**)((u8*)this + 0x48))
	    ->insert(*(JUTTexture**)((u8*)this + 0x80 + mScenarioIndex * 4),
	             0, 1.0f);
	((J2DPicture*)*(J2DPane**)((u8*)this + 0x48))->remove(1);
	((J2DPicture*)*(J2DPane**)((u8*)this + 0x4C))
	    ->insert(*(JUTTexture**)((u8*)this + 0x80 + mScenarioIndex * 4),
	             0, 1.0f);
	((J2DPicture*)*(J2DPane**)((u8*)this + 0x4C))->remove(1);

	{
		char buf[256];
		snprintf(buf, 0xFE, "/common/2d/scenarioname.bmg");
		_15C = (s32)JKRFileLoader::getGlbResource(buf);
	}
	((J2DTextBox*)*(J2DPane**)((u8*)this + 0x44))
	    ->setFont((JUTFont*)gpSystemFont);
	((J2DTextBox*)*(J2DPane**)((u8*)this + 0x6C))
	    ->setFont((JUTFont*)gpSystemFont);

	{
		u8 scIdx = mScenarioIndex;
		s16 shineId
		    = SMS_getShineID(SMS_getShineStage(_13A), (u32)scIdx, false);
		const char* scName = SMSGetMessageData(
		    (void*)_15C, (u16)scScenarioNameTable[shineId]);
		strncpy(((J2DTextBox*)*(J2DPane**)((u8*)this + 0x44))
		            ->getStringPtr(),
		        scName, 0x7F);
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
