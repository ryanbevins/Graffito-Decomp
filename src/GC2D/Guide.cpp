#include <GC2D/Guide.hpp>
#include <GC2D/BoundPane.hpp>
#include <System/Application.hpp>
#include <System/StageUtil.hpp>
#include <JSystem/JUtility/JUTRect.hpp>

static u8 setup_wait;

void TGuide::perform(unsigned long flags, JDrama::TGraphics* gfx) { }

void TGuide::appearGuidePane(int idx) { }

void TGuide::placeMario() { }

void TGuide::changeBotStatus(int idx) { }

int TGuide::checkPoint(int x, int y)
{
	int result = -1;
	JUTRect rect;
	int i;
	for (i = 0; i < 14; i++) {
		TBoundPane* p
		    = ((TBoundPane**)((u8*)this + 0x168))[i];
		rect.copy(*(JUTRect*)((u8*)p + 0x14));
		if (x > rect.x1 && x < rect.x2 && y > rect.y1
		    && y < rect.y2) {
			result = i;
			break;
		}
	}
	if (result == -1) {
		for (i = 0; i < 10; i++) {
			TBoundPane* p
			    = ((TBoundPane**)((u8*)this + 0x44C))[i];
			rect.copy(*(JUTRect*)((u8*)p + 0x14));
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

void TGuide::setup(JKRMemArchive* archive)
{
	if (archive)
		SMSMountAramArchive(archive, gArBkGuide);
	else
		setup_wait = 0x10;
	unkC4 = 0;
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
