#include <GC2D/Guide.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/ExPane.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/StageUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J2D/J2DPane.hpp>
#include <JSystem/J2D/J2DPicture.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/JUtility/JUTRect.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>

static u8 setup_wait;

static const u32 scNormalStageTable[] = {
	0x0, 0x1, 0x2, 0x3, 0x4, 0xD, 0x6, 0x8, 0x9, 0xA,
};

void TGuide::perform(unsigned long flags, JDrama::TGraphics* gfx) { }

void TGuide::appearGuidePane(int idx)
{
	_424 = _1C0[idx];
	_428 = _378[idx];
	JUTRect rect1(_218[idx]);
	JUTRect rect2(_168[idx]->unk14);

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

void TGuide::changeBotStatus(int idx) { }

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

void TGuide::linkSelect() { }

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

		u8* stageData = (u8*)this + 0x14 + stage * 8;

		if (TFlagManager::smInstance->getBool(stage + 0x103A5)) {
			unkBC->search('0_mn' + (stage << 24))->show();
		} else {
			unkBC->search('0_mn' + (stage << 24))->hide();
		}

		if (stage <= 1)
			continue;

		for (int i = 0; i < 8; i++) {
			if (i < stageData[1]) {
				unkBC->search('0ss1' + (stage << 24) + i)->show();
			} else {
				unkBC->search('0ss1' + (stage << 24) + i)->hide();
			}
		}

		J2DPane* sq1 = unkBC->search('0sq1' + (stage << 24));
		sq1->hide();
		J2DPane* sq2 = unkBC->search('0sq2' + (stage << 24));
		sq2->hide();

		u8* p_red = &stageData[2];
		if (*p_red != 0)
			sq1->show();
		if (*p_red > 1)
			sq2->show();
		redCoinTotal += *p_red;
		totalAccum += stageData[1];
	}

	totalAccum += redCoinTotal;
	if ((u8)redCoinTotal != 0) {
		unkBC->search('lqus')->show();
	} else {
		unkBC->search('lqus')->hide();
	}

	for (int stage = 1; stage < 10; stage++) {
		_3D0[stage]
		    = (J2DPicture*)unkBC->search('mi00' + stage);

		if (stage == 9)
			continue;

		u8* stageData = (u8*)this + 0x14 + stage * 8;
		u16 deaths    = *(u16*)(stageData + 4);
		if (deaths > 999)
			deaths = 999;

		J2DPicture* h
		    = (J2DPicture*)unkBC->search('0c_1' + (stage << 24));
		J2DPicture* t
		    = (J2DPicture*)unkBC->search('0c_2' + (stage << 24));
		J2DPicture* o
		    = (J2DPicture*)unkBC->search('0c_3' + (stage << 24));

		if (deaths < 100) {
			h->mVisible = false;
			t->changeTexture(_C8[deaths / 10]->mTexInfo, 0);
			o->changeTexture(_C8[deaths % 10]->mTexInfo, 0);
		} else {
			h->mVisible    = true;
			u16 hundreds = deaths / 100;
			h->changeTexture(_C8[hundreds]->mTexInfo, 0);
			u16 rem = deaths - hundreds * 100;
			t->changeTexture(_C8[rem / 10]->mTexInfo, 0);
			o->changeTexture(_C8[rem % 10]->mTexInfo, 0);
		}

		if (stageData[6] != 0) {
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
	u8 remaining  = (u8)(totalFlag - (u8)totalAccum);
	if (remaining > 99)
		remaining = 99;

	((J2DPicture*)unkBC->search('1s_1'))
	    ->changeTexture(_C8[remaining / 10]->mTexInfo, 0);
	((J2DPicture*)unkBC->search('1s_2'))
	    ->changeTexture(_C8[remaining % 10]->mTexInfo, 0);

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

	for (int stage = 0; stage < 13; stage++) {
		if (stage >= 10)
			continue;

		u8* stageData = (u8*)this + 0x14 + stage * 8;
		stageData[0]  = 0;

		int shineCount = 0;
		if (stage != 0 && stage != 1) {
			for (int i = 0; i < 8; i++) {
				s16 sid = SMS_getShineID(stage, i, false);
				u8 got  = (sid == -1)
				             ? 0
				             : TFlagManager::smInstance->getShineFlag((u8)sid);
				if (got)
					shineCount++;
			}
		}
		u8 clampedShine = (shineCount >= 100) ? 99 : (u8)shineCount;
		stageData[1]    = clampedShine;
		totalAccum += clampedShine;

		int redCoin = 0;
		if (stage != 0 && stage != 1) {
			s16 sid1 = SMS_getShineID(stage, 1, true);
			u8 got1  = (sid1 == -1)
			              ? (u8)redCoin
			              : TFlagManager::smInstance->getShineFlag((u8)sid1);
			if (got1)
				redCoin = 1;

			s16 sid2 = SMS_getShineID(stage, 2, true);
			u8 got2  = (sid2 == -1)
			              ? 0
			              : TFlagManager::smInstance->getShineFlag((u8)sid2);
			if (got2)
				redCoin++;
		}
		if (redCoin >= 10)
			redCoin = 9;
		stageData[2] = (u8)redCoin;
		totalAccum += redCoin;

		u16 deaths = (u16)TFlagManager::smInstance->getFlag(stage + 0x20005);
		if (deaths >= 1000)
			deaths = 999;
		*(s16*)(stageData + 4) = (s16)deaths;

		s16 bossID  = SMS_getShineID(stage, 0, true);
		u8 bossFlag = (bossID == -1)
		                 ? 0
		                 : TFlagManager::smInstance->getShineFlag((u8)bossID);
		stageData[6] = bossFlag;
		if (stageData[6] != 0)
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
		stageData[7] = (u8)blueCount;

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

void TGuide::load(JSUMemoryInputStream& stream) { }

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
