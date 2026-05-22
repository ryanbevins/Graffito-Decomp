#ifndef GC2D_GUIDE_HPP
#define GC2D_GUIDE_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JUtility/JUTRect.hpp>

class JKRMemArchive;
class J2DPane;
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
	/* 0xC6 */ u8 _C6[0x128 - 0xC6];
	/* 0x128 */ TExPane* _128;
	/* 0x12C */ TExPane* _12C;
	/* 0x130 */ u8 _130[0x160 - 0x130];
	/* 0x160 */ u32 unk160;
	/* 0x164 */ u8 unk164;
	/* 0x165 */ u8 _165[0x168 - 0x165];
	/* 0x168 */ TBoundPane* _168[14];
	/* 0x1A0 */ u8 _1A0[0x1C0 - 0x1A0];
	/* 0x1C0 */ TExPane* _1C0[22];
	/* 0x218 */ JUTRect _218[22];
	/* 0x378 */ TExPane* _378[22];
	/* 0x3D0 */ u8 _3D0[0x424 - 0x3D0];
	/* 0x424 */ TExPane* _424;
	/* 0x428 */ TExPane* _428;
	/* 0x42C */ s16 _42C;
	/* 0x42E */ u8 _42E[0x430 - 0x42E];
	/* 0x430 */ J2DPane* _430;
	/* 0x434 */ JUTRect _434;
	/* 0x444 */ u8 _444[0x44C - 0x444];
	/* 0x44C */ J2DPane* _44C[10];
	/* 0x474 */ u8 _474[0x480 - 0x474];
	/* 0x480 */ s32 _480;
	/* 0x484 */ u8 _484[0x48C - 0x484];
	/* 0x48C */ JUTRect _48C;
	/* 0x49C */ u8 _49C[0x6F8 - 0x49C];
};

#endif
