#include <GC2D/Guide.hpp>
#include <GC2D/BoundPane.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/StageUtil.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <JSystem/J2D/J2DPane.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/JUtility/JUTRect.hpp>

static u8 setup_wait;

void TGuide::perform(unsigned long flags, JDrama::TGraphics* gfx) { }

void TGuide::appearGuidePane(int idx) { }

void TGuide::placeMario()
{
	if (SMS_getShineStage(gpMarDirector->mMap) != 1) {
		_430->mVisible = false;
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

	pane->mVisible = true;
	_430->move(destX, destY);

	for (int i = 2; i < 10; i++) {
		if (TFlagManager::smInstance->getBool(0x103A5 + i)) {
			unkBC->search('01g1' + (i - 2))->mVisible = true;
		} else {
			unkBC->search('01g1' + (i - 2))->mVisible = false;
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

void TGuide::resetScore() { }

void TGuide::resetObjects() { }

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
