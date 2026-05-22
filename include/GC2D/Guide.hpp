#ifndef GC2D_GUIDE_HPP
#define GC2D_GUIDE_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JUtility/JUTRect.hpp>

class JKRMemArchive;
class J2DScreen;
class J2DPicture;
class TBoundPane;
class TExPane;
class TMarioGamePad;

class TGuide : public JDrama::TViewObj {
public:
	TGuide(const char* name = "<Guide>");

	virtual void load(JSUMemoryInputStream& stream);
	virtual void perform(unsigned long, JDrama::TGraphics*);

	void resetObjects();
	void resetScore();
	JKRMemArchive* setup(JKRMemArchive*);
	void startMoveCursor();
	void linkSelect();
	int checkPoint(int, int);
	void changeBotStatus(int);
	void placeMario();
	void appearGuidePane(int);

public:
	/* 0x10 */ u32 mState;
	/* 0x14 */ u8 _14[0xBC - 0x14];
	/* 0xBC */ J2DScreen* unkBC;
	/* 0xC0 */ TMarioGamePad* unkC0;
	/* 0xC4 */ u8 unkC4;
	/* 0xC5 */ u8 unkC5;
	/* 0xC6 */ u8 _C6[0x160 - 0xC6];
	/* 0x160 */ u32 unk160;
	/* 0x164 */ u8 unk164;
	/* 0x165 */ u8 _165[0x218 - 0x165];
	/* 0x218 */ JUTRect _218[22];
	/* 0x378 */ u8 _378[0x434 - 0x378];
	/* 0x434 */ JUTRect _434;
	/* 0x444 */ u8 _444[0x480 - 0x444];
	/* 0x480 */ s32 _480;
	/* 0x484 */ u8 _484[0x48C - 0x484];
	/* 0x48C */ JUTRect _48C;
	/* 0x49C */ u8 _49C[0x6F8 - 0x49C];
};

#endif
