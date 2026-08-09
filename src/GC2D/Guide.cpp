#define EXPANE_SET_HELPERS_OUT_OF_LINE
#include <GC2D/Guide.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/ExPane.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>
#include <System/StageUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/MessageUtil.hpp>
#include <GC2D/ScrnFader.hpp>
#include <JSystem/J2D/J2DOrthoGraph.hpp>
#include <JSystem/J2D/J2DPane.hpp>
#include <JSystem/J2D/J2DPicture.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JUtility/JUTPoint.hpp>
#include <JSystem/JUtility/JUTRect.hpp>
#include <JSystem/JUtility/JUTResFont.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <stdio.h>
#include <string.h>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";

static u8 setup_wait;

static const u32 scNormalStageTable[] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0xD, 0x6, 0x8, 0x9, 0xA,
};

// Wrapper to defeat MWCC's auto-inlining of SMS_getShineID in resetObjects.
// MWCC inlines this wrapper at the call site, but does NOT recursively inline
// through it, so the contained SMS_getShineID call survives as a bl.
static inline s16 getShineID(u32 stage, u32 scenario, bool is_etc)
{
	return SMS_getShineID(stage, scenario, is_etc);
}

void TGuide::perform(unsigned long flags, JDrama::TGraphics* gfx)
{
	if (setup_wait != 0) {
		setup_wait--;
		if (setup_wait == 0) {
			SMSSwitch2DArchive("game_6", gArBkGuide);
			unkC4 = 0;
			s16 stage = SMS_getShineStage(gpMarDirector->mMap);
			_42C     = stage;
			resetObjects();
			changeBotStatus((int)stage);
			for (int i = 0; i < 10; i++) {
				if (i == (int)stage) {
					_3D0[i]->mVisible = true;
				} else {
					_3D0[i]->mVisible = false;
				}
			}
			unk164 = 0;
		} else {
			return;
		}
	}

	if (flags & 0x8) {
		if (mState != 9 && mState != 8) {
			J2DOrthoGraph graph(gfx->mViewportRect);
			graph.setup2D();
			unkBC->draw(0, 0, &graph);
		}
	}

	if (flags & 0x1) {
		bool ok = true;
		if (mState > 0xB)
			return;
		switch (mState) {
		case 9: {
			if (unkC5 != 0 && gpApplication.mFader->mFadeStatus == 0) {
				gpApplication.mFader->startWipe(5, 1.0f, 0.0f);
				mState = 10;
			}
			u8 stage = SMS_getShineStage(gpMarDirector->mMap);
			JUTRect rect(_168[stage]->unk14.x1, _168[stage]->unk14.y1,
			             _168[stage]->unk14.x2, _168[stage]->unk14.y2);
			_128->mPane->move(rect.x1 + 6, rect.y1 - 1);
			_12C->mPane->move(rect.x1 + 6, rect.y1 - 1);
			break;
		}
		case 10: {
			if (gpApplication.mFader->mFadeStatus != 1)
				break;
			mState              = 0;
			_428                = nullptr;
			_424                = nullptr;
			_128->mPane->mAlpha = 0xFF;
			_12C->mPane->mAlpha = 0x50;
			break;
		}
		case 0:
			linkSelect();
			break;
		case 1: {
			ok &= _424->update();
			for (int i = 0; i < 2; i++) {
				ok &= (&_128)[i]->update();
			}
			if (!ok)
				break;
			if (!_428->update())
				break;
			mState = 2;
			break;
		}
		case 2: {
			if (unkC0->mEnabledFrameMeaning & 0x60) {
				s16 idx = _42C;
				_428->setPaneAlpha(20, 0, 0xFF);
				JUTRect rect1(_218[idx]);
				JUTRect rect2(_168[idx]->unk14.x1, _168[idx]->unk14.y1,
				              _168[idx]->unk14.x2, _168[idx]->unk14.y2);
				gpMSound->startSoundSystemSE(0x4805, 0, nullptr, 0);
				int widthR1  = rect1.x2 - rect1.x1;
				int heightR1 = rect1.y2 - rect1.y1;
				_424->setCenteredSize(20, 0, 0, widthR1, heightR1);
				_424->setPaneOffset(20, rect2.x1 - rect1.x1,
				                    rect2.y1 - rect1.x1 - 40, 0, 0);
				_128->setPaneAlpha(20, 0xFF, 0);
				_12C->setPaneAlpha(20, 0x50, 0);
				mState = 3;
			} else if (unkC0->mButton.mTrigger & 0x10) {
				mState = 7;
			}
			break;
		}
		case 3: {
			if (!_428->update())
				break;
			ok &= _424->update();
			for (int i = 0; i < 2; i++) {
				ok &= (&_128)[i]->update();
			}
			if (!ok)
				break;
			_424->mPane->mVisible = false;
			_428->mPane->mVisible = false;
			mState                = 0;
			_F0                   = 0;
			break;
		}
		case 7: {
			gpApplication.mFader->startWipe(6, 1.0f, 0.0f);
			unkC0->mFlags &= ~0x80;
			gpMSound->startSoundSystemSE(0x4818, 0, nullptr, 0);
			mState = 11;
			break;
		}
		case 11: {
			if (gpApplication.mFader->mFadeStatus != 0)
				break;
			gpApplication.mFader->startWipe(5, 1.0f, 0.0f);
			if (_424 != nullptr && _424->mPane->mVisible) {
				_424->mPane->mVisible = false;
			}
			if (_428 != nullptr && _428->mPane->mVisible) {
				_428->mPane->mVisible = false;
			}
			unkC4  = 1;
			mState = 8;
			break;
		}
		}
	}
}

void TExPane::setPaneSize(s32 time, s32 target_w, s32 target_h, s32 initial_w,
                          s32 initial_h)
{
	mSizeInterpolator.setValue(time, target_w, target_h, initial_w, initial_h);
	mPane->resize(initial_w, initial_h);
	mSizeAnimPending = true;
}

void TExPane::setPaneAlpha(s32 time, s16 target_alpha, s16 initial_alpha)
{
	mPane->setAlpha(clapAlpha(initial_alpha));

	mCurrentAlpha     = initial_alpha;
	mAlphaStep        = f32(target_alpha - initial_alpha) / time;
	mTargetAlpha      = target_alpha;
	mAlphaAnimPending = true;
}

void TGuide::appearGuidePane(int idx)
{
	_424 = _1C0[idx];
	_428 = _378[idx];
	JUTRect rect1(_218[idx]);
	JUTRect rect2(_168[idx]->unk14.x1, _168[idx]->unk14.y1,
	              _168[idx]->unk14.x2, _168[idx]->unk14.y2);

	_424->mPane->mVisible = true;

	int widthR1  = rect1.x2 - rect1.x1;
	int heightR1 = rect1.y2 - rect1.y1;
	_424->setCenteredSize(20, widthR1, heightR1, 0, 0);

	_424->setPaneOffset(20, 0, 0, rect2.x1 - rect1.x1,
	                    rect2.y1 - rect1.x1 - 40);

	_428->mPane->mAlpha   = 0;
	_428->mPane->mVisible = true;
	_428->setPaneAlpha(20, 255, 0);

	_128->setPaneAlpha(20, 0, 255);
	_12C->setPaneAlpha(20, 0, 80);

	if (idx == 1)
		placeMario();

	gpMSound->startSoundSystemSE(0x4804, 0, nullptr, 0);

	_42C   = (s16)idx;
	mState = 1;
	if (idx != -1 && idx < 10) {
		unk164             = 0;
		_44C[idx]->mAlpha = 255;
	}
}

void TGuide::placeMario()
{
	if (SMS_getShineStage(gpMarDirector->mMap) != 1) {
		_430->hide();
		return;
	}

	JGeometry::TVec3<f32> mpos = *gpMarioPos;
	int rangeX                 = _434.x2 - _434.x1;
	int rangeY                 = _434.y2 - _434.y1;
	mpos.x                     = mpos.x * (f32)rangeX / 25000.0f;
	mpos.y                     = 0.0f;
	mpos.z                     = mpos.z * (f32)rangeY / 21200.0f;

	J2DPane* pane = _430;
	int paneW     = pane->mBounds.x2 - pane->mBounds.x1;
	int paneH     = pane->mBounds.y2 - pane->mBounds.y1;
	int destX
	    = (int)(mpos.x + 0.5f * (f32)rangeX - 0.5f * (f32)paneW - 2.0f);
	int destY = (int)(mpos.z + 0.5f * (f32)rangeY + 0.5f * (f32)paneH);

	if (destX > rangeX - paneW)
		destX = rangeX - paneW;
	if (destX < 0)
		destX = 0;
	if (destY > rangeY - paneH)
		destY = rangeY - paneH;
	if (destY < 0)
		destY = 0;

	pane->show();
	_430->move(destX, destY);

	for (int i = 2; i < 10; i++) {
		if (TFlagManager::smInstance->getBool(0x103A5 + i)) {
			unkBC->search('01g1' + (i - 2))->show();
		} else {
			unkBC->search('01g1' + (i - 2))->hide();
		}
	}
}

void TGuide::changeBotStatus(int idx)
{
	if (idx == -1 || idx >= 10) {
		_124->mVisible = false;
		return;
	}

	u8* stageData = (u8*)this + idx * 8;

	if (stageData[0x14] == 0) {
		_124->mVisible = true;
		_F4->mVisible  = true;

		const char* msg = SMSGetMessageData(_474, idx);
		strncpy(_124->getStringPtr(), msg, 0x1A);

		s32 shineCount = stageData[0x15];
		if (shineCount < 0)
			shineCount = 0;
		if (shineCount > 99)
			shineCount = 99;

		if (shineCount < 10) {
			_FC->mVisible = false;
			_F8->changeTexture(_C8[shineCount]->mTexInfo, 0);
		} else {
			_FC->mVisible = true;
			_F8->changeTexture(_C8[shineCount / 10]->mTexInfo, 0);
			_FC->changeTexture(_C8[shineCount % 10]->mTexInfo, 0);
		}

		if (idx <= 1 || stageData[0x16] == 0) {
			_100->mVisible = false;
			_104->mVisible = false;
			_108->mVisible = false;
		} else if (stageData[0x16] == 1) {
			_100->mVisible = true;
			_104->mVisible = true;
			_108->mVisible = false;
		} else {
			_100->mVisible = true;
			_104->mVisible = true;
			_108->mVisible = true;
		}

		s32 deaths = *(u16*)(stageData + 0x18);
		if (deaths < 0)
			deaths = 0;
		if (deaths > 999)
			deaths = 999;

		if (deaths < 100) {
			_114->mVisible = false;
			_10C->changeTexture(_C8[deaths / 10]->mTexInfo, 0);
			_110->changeTexture(_C8[deaths % 10]->mTexInfo, 0);
		} else {
			_114->mVisible = true;
			s32 hundreds = deaths / 100;
			_10C->changeTexture(_C8[hundreds]->mTexInfo, 0);
			deaths -= hundreds * 100;
			_110->changeTexture(_C8[deaths / 10]->mTexInfo, 0);
			_114->changeTexture(_C8[deaths % 10]->mTexInfo, 0);
		}

		if (stageData[0x1A] != 0) {
			_118->mVisible = true;
		} else {
			_118->mVisible = false;
		}

		s32 blueCount = stageData[0x1B];
		if (blueCount < 0)
			blueCount = 0;
		if (blueCount > 99)
			blueCount = 99;

		if (idx != 0) {
			unkBC->search('sb_i')->show();
			unkBC->search('sc_t')->show();
		} else {
			unkBC->search('sb_i')->hide();
			unkBC->search('sc_t')->hide();
		}

		if (blueCount < 10) {
			_120->mVisible = false;
			_11C->changeTexture(_C8[blueCount % 10]->mTexInfo, 0);
		} else {
			_120->mVisible = true;
			_11C->changeTexture(_C8[blueCount / 10]->mTexInfo, 0);
			_120->changeTexture(_C8[blueCount % 10]->mTexInfo, 0);
		}
	} else {
		_124->mVisible = true;
		_F4->mVisible  = false;

		s32 blueCount = stageData[0x1B];
		if (blueCount < 0)
			blueCount = 0;
		if (blueCount > 99)
			blueCount = 99;

		if (blueCount < 10) {
			_120->mVisible = false;
			_11C->changeTexture(_C8[blueCount % 10]->mTexInfo, 0);
		} else {
			_120->mVisible = true;
			_11C->changeTexture(_C8[blueCount / 10]->mTexInfo, 0);
			_120->changeTexture(_C8[blueCount % 10]->mTexInfo, 0);
		}

		unkBC->search('sb_i')->show();
		unkBC->search('sc_t')->hide();
		unkBC->search('sq_i')->hide();

		const char* msg = SMSGetMessageData(_474, idx);
		strncpy(_124->getStringPtr(), msg, 0x1A);
	}
}

int TGuide::checkPoint(int x, int y)
{
	int result = -1;
	int i;
	for (i = 0; i < 14; i++) {
		TBoundPane* p
		    = *(TBoundPane**)((u8*)this + 0x168 + i * 4);
		JUTRect rect(*(JUTRect*)((u8*)p + 0x14));
		if (x > rect.x1 && x < rect.x2 && y > rect.y1
		    && y < rect.y2) {
			result = i;
			break;
		}
	}
	if (result == -1) {
		for (i = 0; i < 10; i++) {
			TBoundPane* p
			    = *(TBoundPane**)((u8*)this + 0x44C + i * 4);
			JUTRect rect(*(JUTRect*)((u8*)p + 0x14));
			if (x > rect.x1 && x < rect.x2 && y > rect.y1
			    && y < rect.y2) {
				result = i;
				break;
			}
		}
	}
	if (result >= 0 && result < 10) {
		TBoundPane* p
		    = ((TBoundPane**)((u8*)this + 0x44C))[result];
		if (*((u8*)p + 0xC) == 0)
			result = -1;
	}
	return result;
}

void TGuide::linkSelect()
{
	unkC0->mFlags |= 0x80;

	if ((unkC0->mEnabledFrameMeaning & 0x40)
	    || (unkC0->mButton.mTrigger & 0x10)) {
		mState = 7;
	}

	J2DPane* pane128 = _128->mPane;
	s32 cx
	    = pane128->mBounds.x1 + (s16)(s32)(3.2f * unkC0->mCompSPos[8]);
	s32 cy
	    = pane128->mBounds.y1 + (s16)(s32)(-3.2f * unkC0->mCompSPos[9]);
	if (cx > 0x238)
		cx = 0x238;
	if (cx < 0)
		cx = 0;
	if (cy > 0x168)
		cy = 0x168;
	if (cy < 0x38)
		cy = 0x38;

	pane128->move(cx, cy);
	_12C->mPane->move(cx + 7, cy + 4);

	int idx = checkPoint(cx - 2, cy + 10);
	if (idx != -1 && (unkC0->mMeaning & 0x20)) {
		appearGuidePane(idx);
	}

	if (idx != -1 && idx < 10) {
		J2DPane*& slot = _44C[idx];
		int newAlpha;
		if (unk164 != 0) {
			newAlpha = slot->mAlpha + 4;
		} else {
			newAlpha = slot->mAlpha - 4;
		}
		if (newAlpha < 0x1e) {
			unk164   = 1;
			newAlpha = 0x1e;
		} else if (newAlpha > 0xff) {
			unk164   = 0;
			newAlpha = 0xff;
		}
		slot->mAlpha = newAlpha;
	}

	{
		J2DPicture* pic = (J2DPicture*)_128->mPane;
		if ((u32)_F0 % 45 == 0) {
			if (((u32)_F0 / 45) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	{
		J2DPicture* pic = (J2DPicture*)_12C->mPane;
		if ((u32)_F0 % 45 == 0) {
			if (((u32)_F0 / 45) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	if (_480 != idx) {
		changeBotStatus(idx);
		if (_480 != -1 && _480 < 10) {
			_44C[_480]->mAlpha = 0xff;
		}
		if (idx == -1) {
			J2DPicture* pic128 = (J2DPicture*)_128->mPane;
			pic128->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
			pic128->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			J2DPicture* pic12C = (J2DPicture*)_12C->mPane;
			pic12C->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
			pic12C->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
		}
		unk164 = 0;
		_480   = idx;
	}

	{
		J2DPicture* pic = _134;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	{
		J2DPicture* p = (J2DPicture*)_148;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				p->mMirror = (J2DMirror)0;
			} else {
				p->mMirror = J2DMirror_X;
			}
		}
	}

	{
		J2DPane* p = _154;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				p->mRotation = 0.0f;
			} else {
				p->mRotation = 30.0f;
			}
		}
	}

	{
		J2DPicture* pic = _138;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	{
		J2DPicture* p = (J2DPicture*)_14C;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				p->mMirror = (J2DMirror)0;
			} else {
				p->mMirror = J2DMirror_X;
			}
		}
	}

	{
		J2DPicture* p = (J2DPicture*)_150;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				p->mMirror = (J2DMirror)0;
			} else {
				p->mMirror = J2DMirror_X;
			}
		}
	}

	{
		J2DPicture* pic = _13C;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	{
		J2DPicture* pic = _140;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	{
		J2DPicture* pic = _144;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				pic->setBlendKonstColor(0.0f, 1.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(0.0f, 1.0f, 0.0f, 0.0f);
			} else {
				pic->setBlendKonstColor(1.0f, 0.0f, 0.0f, 0.0f);
				pic->setBlendKonstAlpha(1.0f, 0.0f, 0.0f, 0.0f);
			}
		}
	}

	{
		J2DPane* p = _158;
		if ((u32)_F0 % 90 == 0) {
			if (((u32)_F0 / 90) % 2 == 0) {
				p->mRotation = 0.0f;
			} else {
				p->mRotation = -45.0f;
			}
		}
	}

	{
		TBoundPane* bp = _444;
		if ((u32)_F0 % 90 == 0) {
			JUTPoint p1(0, 0);
			JUTPoint p2(0, -5);
			JUTPoint p3(0, 0);
			bp->setPanePosition(45, p1, p2, p3);
		} else if ((u32)_F0 % 90 == 45) {
			JUTPoint p1(0, 0);
			JUTPoint p2(0, 5);
			JUTPoint p3(0, 0);
			bp->setPanePosition(45, p1, p2, p3);
		}

		_448->mAlpha = ((u32)_F0 % 180 < 0x82) ? 0xff : 0;
		bp->update();
	}

	{
		TExPane* ep = _478;
		if ((u32)_F0 % 270 == 0) {
			if (((u32)_F0 / 270) % 2 == 0) {
				ep->setPaneAlpha(270, 0, _47C);
			} else {
				ep->setPaneAlpha(270, _47C, 0);
			}
		}
		ep->update();
	}

	if (_15C != 0) {
		unk160 += 3;
		if (unk160 > 0x12c) {
			_15C = 0;
		}
	} else {
		unk160 -= 3;
		if (unk160 < 0x1e) {
			_15C = 1;
		}
	}

	int alpha;
	if ((int)unk160 < 0x1e) {
		alpha = 0x1e;
	} else if ((int)unk160 > 0xff) {
		alpha = 0xff;
	} else {
		alpha = (u8)unk160;
	}

	for (int i = 0; i < 10; i++) {
		((J2DPane*)_168[i])->mAlpha = alpha;
	}

	_F0++;
	if (_F0 > 0x21c) {
		_F0 = 0;
	}
}

void TGuide::startMoveCursor()
{
	mState  = 9;
	unk164 = 0;
}

JKRMemArchive* TGuide::setup(JKRMemArchive* archive)
{
	if (archive)
		SMSMountAramArchive(archive, gArBkGuide);
	else
		setup_wait = 0x10;
	unkC4 = 0;
	return archive;
}

void TGuide::resetScore()
{
	int redCoinTotal = 0;
	int totalAccum   = 0;

	for (int stage = 0; stage < 10; stage++) {
		if (stage == 9)
			continue;

		if (TFlagManager::smInstance->getBool(stage + 0x103A5)) {
			unkBC->search('0_mn' + (stage << 24))->show();
		} else {
			unkBC->search('0_mn' + (stage << 24))->hide();
		}

		if (stage <= 1)
			continue;

		u8* stageData = (u8*)this + stage * 8;

		for (int i = 0; i < 8; i++) {
			if (i < stageData[0x15]) {
				unkBC->search('0ss1' + (stage << 24) + i)->show();
			} else {
				unkBC->search('0ss1' + (stage << 24) + i)->hide();
			}
		}

		J2DPane* sq1 = unkBC->search('0sq1' + (stage << 24));
		sq1->hide();
		J2DPane* sq2 = unkBC->search('0sq2' + (stage << 24));
		sq2->hide();

		u8* p_red = &stageData[0x16];
		if (*p_red != 0)
			sq1->show();
		if (*p_red > 1)
			sq2->show();
		redCoinTotal += *p_red;
		totalAccum += stageData[0x15];
	}

	totalAccum += redCoinTotal;
	if ((u8)redCoinTotal == 0) {
		unkBC->search('lqus')->hide();
	} else {
		unkBC->search('lqus')->show();
	}

	for (int stage = 1; stage < 10; stage++) {
		_3D0[stage]
		    = (J2DPicture*)unkBC->search('mi00' + stage);

		if (stage == 9)
			continue;

		u8* stageData = (u8*)this + stage * 8;
		u16 deaths    = *(u16*)(stageData + 0x18);
		if (deaths > 999)
			deaths = 999;

		u32 cTag = '0c_1' + (stage << 24);
		J2DPicture* h = (J2DPicture*)unkBC->search(cTag);
		J2DPicture* t = (J2DPicture*)unkBC->search(cTag + 1);
		J2DPicture* o = (J2DPicture*)unkBC->search(cTag + 2);

		if (deaths < 100) {
			h->mVisible = false;
			t->changeTexture(_C8[deaths / 10]->mTexInfo, 0);
			o->changeTexture(_C8[deaths % 10]->mTexInfo, 0);
		} else {
			h->mVisible    = true;
			int hundreds = deaths / 100;
			h->changeTexture(_C8[hundreds]->mTexInfo, 0);
			deaths -= hundreds * 100;
			t->changeTexture(_C8[deaths / 10]->mTexInfo, 0);
			o->changeTexture(_C8[deaths % 10]->mTexInfo, 0);
		}

		if (stageData[0x1A] != 0) {
			unkBC->search('0c_s' + (stage << 24))->show();
			totalAccum++;
		} else {
			unkBC->search('0c_s' + (stage << 24))->hide();
		}
	}

	_3D0[0] = (J2DPicture*)unkBC->search('mi00');
	_448    = (J2DPicture*)unkBC->search('clic');

	s16 u22 = 0;
	if (TFlagManager::smInstance->getBool(0x10056))
		u22 = 1;
	if (TFlagManager::smInstance->getBool(0x10058))
		u22++;

	((J2DPicture*)unkBC->search('0s_1'))
	    ->changeTexture(_C8[u22]->mTexInfo, 0);
	totalAccum += u22;

	s32 totalFlag = TFlagManager::smInstance->getFlag(0x40000);
	s32 remaining = totalFlag - (u8)totalAccum;
	if ((u8)remaining > 99)
		remaining = 99;

	((J2DPicture*)unkBC->search('1s_1'))
	    ->changeTexture(_C8[(u8)remaining / 10]->mTexInfo, 0);
	((J2DPicture*)unkBC->search('1s_2'))
	    ->changeTexture(_C8[(u8)remaining % 10]->mTexInfo, 0);

	s32 totalClamped = totalFlag;
	if (totalClamped > 999)
		totalClamped = 999;

	J2DPicture* lh = (J2DPicture*)unkBC->search('lt_1');
	J2DPicture* lt = (J2DPicture*)unkBC->search('lt_2');
	J2DPicture* lo = (J2DPicture*)unkBC->search('lt_3');

	if (totalClamped < 100) {
		lh->mVisible = false;
		lt->changeTexture(_C8[totalClamped / 10]->mTexInfo, 0);
		lo->changeTexture(_C8[totalClamped % 10]->mTexInfo, 0);
	} else {
		lh->mVisible    = true;
		s32 lhundreds = totalClamped / 100;
		lh->changeTexture(_C8[lhundreds]->mTexInfo, 0);
		s32 lrem = totalClamped - lhundreds * 100;
		lt->changeTexture(_C8[lrem / 10]->mTexInfo, 0);
		lo->changeTexture(_C8[lrem % 10]->mTexInfo, 0);
	}

	switch (gpApplication.mSaveFile) {
	case 0:
		unkBC->search('ld_a')->show();
		unkBC->search('ld_b')->hide();
		unkBC->search('ld_c')->hide();
		break;
	case 1:
		unkBC->search('ld_a')->hide();
		unkBC->search('ld_b')->show();
		unkBC->search('ld_c')->hide();
		break;
	case 2:
		unkBC->search('ld_a')->hide();
		unkBC->search('ld_b')->hide();
		unkBC->search('ld_c')->show();
		break;
	}

	s32 finalFlag = TFlagManager::smInstance->getFlag(0x40000);
	f32 alphaF
	    = 255.0f * (1.0f - (f32)(finalFlag / 30) * 0.25f);
	_47C                  = (u8)alphaF;
	_478->mPane->mAlpha = _47C;
}

void TGuide::resetObjects()
{
	int totalAccum = 0;

	for (u32 stage = 0; stage < 13; stage++) {
		if (stage >= 10)
			continue;

		u8* stageData = (u8*)this + stage * 8;
		stageData[0x14] = 0;

		int shineCount = 0;
		if (stage != 0 && stage != 1) {
			for (int i = 0; i < 8; i++) {
				s16 sid = getShineID(stage, i, false);
				u8 got  = (sid == -1)
				             ? 0
				             : TFlagManager::getInstance()->getShineFlag((u8)sid);
				if (got)
					shineCount++;
			}
		}
		int clampedShine = (shineCount >= 100) ? 99 : shineCount;
		stageData[0x15] = (u8)clampedShine;
		totalAccum += clampedShine;

		int redCoin = 0;
		if (stage != 0 && stage != 1) {
			s16 sid1 = getShineID(stage, 1, true);
			u8 got1  = (sid1 == -1)
			              ? (u8)redCoin
			              : TFlagManager::getInstance()->getShineFlag((u8)sid1);
			if (got1)
				redCoin = 1;

			s16 sid2 = getShineID(stage, 2, true);
			u8 got2  = (sid2 == -1)
			              ? 0
			              : TFlagManager::getInstance()->getShineFlag((u8)sid2);
			if (got2)
				redCoin++;
		}
		if (redCoin >= 10)
			redCoin = 9;
		stageData[0x16] = (u8)redCoin;
		totalAccum += redCoin;

		u16 deaths = (u16)TFlagManager::smInstance->getFlag(stage + 0x20005);
		if (deaths >= 1000)
			deaths = 999;
		*(s16*)(stageData + 0x18) = (s16)deaths;

		s16 bossID  = getShineID(stage, 0, true);
		u8 bossFlag = (bossID == -1)
		                 ? 0
		                 : TFlagManager::getInstance()->getShineFlag((u8)bossID);
		stageData[0x1A] = bossFlag;
		if (stageData[0x1A] != 0)
			totalAccum++;

		int blueCount = 0;
		if (stage != 0) {
			for (u8 c = 0; c < 50; c++) {
				if (TFlagManager::smInstance->getBlueCoinFlag(
				        (u8)scNormalStageTable[stage], c))
					blueCount++;
			}
		}
		if (blueCount >= 1000)
			blueCount = 999;
		stageData[0x1B] = (u8)blueCount;

		if (TFlagManager::smInstance->getBool(stage + 0x103A5)) {
			((J2DPane*)_44C[stage])->mVisible = true;
			((J2DPane*)_168[stage])->mVisible = true;
		} else {
			((J2DPane*)_44C[stage])->mVisible = false;
			((J2DPane*)_168[stage])->mVisible = false;
		}
	}

	*((u8*)this + 0x5C) = 1;

	int sirenaBlue = 0;
	for (u8 c = 0; c < 50; c++) {
		if (TFlagManager::smInstance->getBlueCoinFlag(
		        (u8)scNormalStageTable[9], c))
			sirenaBlue++;
	}
	*((u8*)this + 0x63) = (u8)sirenaBlue;

	int u22 = 0;
	if (TFlagManager::smInstance->getBool(0x10056))
		u22 = 1;
	if (TFlagManager::smInstance->getBool(0x10058))
		u22++;
	*((u8*)this + 0x15) = (u8)u22;
	totalAccum += (s16)u22;

	s32 maxFlag         = TFlagManager::smInstance->getFlag(0x40000);
	*((u8*)this + 0x1D) = (u8)(maxFlag - totalAccum);

	changeBotStatus(-1);
	resetScore();

	_128->mPane->mVisible = true;
	_12C->mPane->mVisible = true;
}

void TGuide::load(JSUMemoryInputStream& stream)
{
	unkC5 = 0;
	JDrama::TNameRef::load(stream);

	JKRMemArchive* archive;
	if ((archive = gpMarDirector->unkD8) != nullptr) {
		SMSMountAramArchive(archive, gArBkGuide);
	} else {
		setup_wait = 0x10;
	}
	unkC4 = 0;

	unkBC = new J2DSetScreen("guide_1.blo", (JKRArchive*)archive);

	((J2DTextBox*)unkBC->search('a_ic'))->setFont(gpSystemFont);
	((J2DTextBox*)unkBC->search('a_tx'))->setFont(gpSystemFont);
	((J2DTextBox*)unkBC->search('b_ic'))->setFont(gpSystemFont);
	((J2DTextBox*)unkBC->search('b_tx'))->setFont(gpSystemFont);

	_124 = nullptr;
	_124 = (J2DTextBox*)unkBC->search('s_mn');
	SMSMakeTextBuffer(_124, 0x1a);
	_124->setFont(gpSystemFont);

	for (int i = 0; i < 10; i++) {
		char buf[0xff];
		snprintf(buf, 0xff, "/guide/timg/coin_number_%d.bti", i);
		JUTTexture* tex
		    = (JUTTexture*)::operator new(sizeof(JUTTexture));
		if (tex != nullptr) {
			const ResTIMG* img
			    = (ResTIMG*)JKRFileLoader::getGlbResource(buf);
			tex->mEmbPalette = nullptr;
			tex->storeTIMG(img);
			tex->unk50 = 0;
		}
		_C8[i] = tex;
	}

	_F4 = (J2DPicture*)unkBC->search('ss_i');
	for (int i = 0; i < 2; i++) {
		J2DPicture* p
		    = (J2DPicture*)unkBC->search('ss_1' + i);
		*(J2DPicture**)((u8*)this + 0xF8 + i * 4) = p;
	}

	_100 = unkBC->search('sq_i');
	for (int i = 0; i < 2; i++) {
		J2DPane* p = unkBC->search('sq_1' + i);
		*(J2DPane**)((u8*)this + 0x104 + i * 4) = p;
	}

	for (int i = 0; i < 3; i++) {
		J2DPicture* p
		    = (J2DPicture*)unkBC->search('sc_1' + i);
		*(J2DPicture**)((u8*)this + 0x10C + i * 4) = p;
	}

	_118 = (J2DPicture*)unkBC->search('sc_s');

	for (int i = 0; i < 2; i++) {
		J2DPicture* p
		    = (J2DPicture*)unkBC->search('sb_1' + i);
		*(J2DPicture**)((u8*)this + 0x11C + i * 4) = p;
	}

	JUTTexture* cuiTex
	    = (JUTTexture*)::operator new(sizeof(JUTTexture));
	if (cuiTex != nullptr) {
		const ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
		    "/guide/timg/guide_cursor_2.bti");
		cuiTex->mEmbPalette = nullptr;
		cuiTex->storeTIMG(img);
		cuiTex->unk50 = 0;
	}

	for (int i = 0; i < 2; i++) {
		TExPane* p = new TExPane(unkBC, 'cu_a' + i);
		*(TExPane**)((u8*)this + 0x128 + i * 4) = p;
		J2DPicture* pic = (J2DPicture*)p->getPane();
		pic->insert(cuiTex, pic->mTextureNum, 0.0f);
	}

	for (int i = 0; i < 13; i++) {
		int hi      = i / 10;
		int lo      = i % 10;
		u32 ddTag   = (hi << 8) + lo + 0x3030;
		u32 baseTag = ddTag << 16;

		_168[i] = (TBoundPane*)unkBC->search(ddTag);

		_1C0[i] = new TExPane(unkBC, baseTag + 0x5f30);
		_218[i] = _1C0[i]->getPane()->mBounds;

		_378[i] = new TExPane(unkBC, baseTag + 0x5f31);

		J2DTextBox* tb1
		    = (J2DTextBox*)unkBC->search(baseTag + 0x5f33);
		tb1->setFont(gpSystemFont);

		J2DTextBox* tb2
		    = (J2DTextBox*)unkBC->search(baseTag + 0x5f35);
		tb2->setFont(gpSystemFont);
	}

	_168[13] = (TBoundPane*)unkBC->search(0x3230);

	_1C0[13] = new TExPane(unkBC, 'lwin');
	_218[13] = _1C0[13]->getPane()->mBounds;

	_378[13] = new TExPane(unkBC, 'llin');

	for (int i = 0; i < 10; i++) {
		J2DPane* p = unkBC->search('pn00' + i);
		*(J2DPane**)((u8*)this + 0x44C + i * 4) = p;
	}

	_430 = unkBC->search('01mi');
	_434 = unkBC->search('01_9')->mBounds;
	_474 = JKRFileLoader::getGlbResource("/common/2d/stagename.bmg");

	_134 = (J2DPicture*)unkBC->search('10');
	JUTTexture* texSun = (JUTTexture*)::operator new(sizeof(JUTTexture));
	if (texSun != nullptr) {
		const ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
		    "/guide/timg/guide_draw_sun_2.bti");
		texSun->mEmbPalette = nullptr;
		texSun->storeTIMG(img);
		texSun->unk50 = 0;
	}
	_134->insert(texSun, _134->mTextureNum, 0.0f);

	_138 = (J2DPicture*)unkBC->search('13');
	JUTTexture* texShip = (JUTTexture*)::operator new(sizeof(JUTTexture));
	if (texShip != nullptr) {
		const ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
		    "/guide/timg/guide_draw_ship_2.bti");
		texShip->mEmbPalette = nullptr;
		texShip->storeTIMG(img);
		texShip->unk50 = 0;
	}
	_138->insert(texShip, _138->mTextureNum, 0.0f);

	_13C = (J2DPicture*)unkBC->search('16');
	JUTTexture* texPalm2 = (JUTTexture*)::operator new(sizeof(JUTTexture));
	if (texPalm2 != nullptr) {
		const ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
		    "/guide/timg/guide_draw_palmtree_2.bti");
		texPalm2->mEmbPalette = nullptr;
		texPalm2->storeTIMG(img);
		texPalm2->unk50 = 0;
	}
	_13C->insert(texPalm2, _13C->mTextureNum, 0.0f);

	_140 = (J2DPicture*)unkBC->search('17');
	JUTTexture* texPalm1 = (JUTTexture*)::operator new(sizeof(JUTTexture));
	if (texPalm1 != nullptr) {
		const ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
		    "/guide/timg/guide_draw_palmtree_1.bti");
		texPalm1->mEmbPalette = nullptr;
		texPalm1->storeTIMG(img);
		texPalm1->unk50 = 0;
	}
	_140->insert(texPalm1, _140->mTextureNum, 0.0f);

	_144 = (J2DPicture*)unkBC->search('18');
	JUTTexture* texFish = (JUTTexture*)::operator new(sizeof(JUTTexture));
	if (texFish != nullptr) {
		const ResTIMG* img = (ResTIMG*)JKRFileLoader::getGlbResource(
		    "/guide/timg/guide_draw_fish_2.bti");
		texFish->mEmbPalette = nullptr;
		texFish->storeTIMG(img);
		texFish->unk50 = 0;
	}
	_144->insert(texFish, _144->mTextureNum, 0.0f);

	_148 = unkBC->search('11');
	_14C = unkBC->search('14');
	_150 = unkBC->search('15');
	_154 = unkBC->search('12');
	_154->setBasePosition((J2DBasePosition)4);
	_158 = unkBC->search('19');
	_158->setBasePosition((J2DBasePosition)4);

	void* msgData = JKRFileLoader::getGlbResource("/guide/guidemess.bmg");

	for (int i = 0; i < 13; i++) {
		int hi      = i / 10;
		int lo      = i % 10;
		u32 ddTag   = (hi << 8) + lo + 0x3030;
		u32 baseTag = ddTag << 16;

		J2DTextBox* tb1
		    = (J2DTextBox*)unkBC->search(baseTag + 0x5f33);
		SMSMakeTextBuffer(tb1, 0x1e);
		tb1->setFont(gpSystemFont);
		const char* msg1 = SMSGetMessageData(msgData, i + 0xd);
		strncpy(tb1->getStringPtr(), msg1, 0x1e);

		J2DTextBox* tb2
		    = (J2DTextBox*)unkBC->search(baseTag + 0x5f35);
		SMSMakeTextBuffer(tb2, 0x200);
		tb2->setFont(gpSystemFont);
		const char* msg2 = SMSGetMessageData(msgData, i);
		strncpy(tb2->getStringPtr(), msg2, 0x200);
	}

	_478 = new TExPane(unkBC, 'mark');
	_444 = new TBoundPane(unkBC, 0x3230);

	resetObjects();
	unkC5 = 1;
}

TGuide::TGuide(const char* name)
    : JDrama::TViewObj(name)
    , mState(8)
    , unkBC(nullptr)
    , unkC0(nullptr)
    , unkC4(0)
    , unkC5(0)
    , unk160(0xFF)
    , unk164(1)
    , _480(-1)
{
}
