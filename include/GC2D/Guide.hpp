#ifndef GC2D_GUIDE_HPP
#define GC2D_GUIDE_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JUtility/JUTRect.hpp>

class JKRMemArchive;
class J2DPane;
class J2DScreen;
class J2DPicture;
class J2DTextBox;
class JUTTexture;
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
	/* 0xC6 */ u8 _C6[0xC8 - 0xC6];
	/* 0xC8 */ JUTTexture* _C8[10];
	/* 0xF0 */ u16 _F0;
	/* 0xF2 */ u8 _F2[0xF4 - 0xF2];
	/* 0xF4 */ J2DPicture* _F4;
	/* 0xF8 */ J2DPicture* _F8;
	/* 0xFC */ J2DPicture* _FC;
	/* 0x100 */ J2DPane* _100;
	/* 0x104 */ J2DPane* _104;
	/* 0x108 */ J2DPane* _108;
	/* 0x10C */ J2DPicture* _10C;
	/* 0x110 */ J2DPicture* _110;
	/* 0x114 */ J2DPicture* _114;
	/* 0x118 */ J2DPicture* _118;
	/* 0x11C */ J2DPicture* _11C;
	/* 0x120 */ J2DPicture* _120;
	/* 0x124 */ J2DTextBox* _124;
	/* 0x128 */ TExPane* _128;
	/* 0x12C */ TExPane* _12C;
	/* 0x130 */ J2DPicture* _130;
	/* 0x134 */ J2DPicture* _134;
	/* 0x138 */ J2DPicture* _138;
	/* 0x13C */ J2DPicture* _13C;
	/* 0x140 */ J2DPicture* _140;
	/* 0x144 */ J2DPicture* _144;
	/* 0x148 */ J2DPane* _148;
	/* 0x14C */ J2DPane* _14C;
	/* 0x150 */ J2DPane* _150;
	/* 0x154 */ J2DPane* _154;
	/* 0x158 */ J2DPane* _158;
	/* 0x15C */ u8 _15C;
	/* 0x15D */ u8 _15D[0x160 - 0x15D];
	/* 0x160 */ u32 unk160;
	/* 0x164 */ u8 unk164;
	/* 0x165 */ u8 _165[0x168 - 0x165];
	/* 0x168 */ TBoundPane* _168[14];
	/* 0x1A0 */ u8 _1A0[0x1C0 - 0x1A0];
	/* 0x1C0 */ TExPane* _1C0[22];
	/* 0x218 */ JUTRect _218[22];
	/* 0x378 */ TExPane* _378[22];
	/* 0x3D0 */ J2DPicture* _3D0[10];
	/* 0x3F8 */ u8 _3F8[0x424 - 0x3F8];
	/* 0x424 */ TExPane* _424;
	/* 0x428 */ TExPane* _428;
	/* 0x42C */ s16 _42C;
	/* 0x42E */ u8 _42E[0x430 - 0x42E];
	/* 0x430 */ J2DPane* _430;
	/* 0x434 */ JUTRect _434;
	/* 0x444 */ TBoundPane* _444;
	/* 0x448 */ J2DPicture* _448;
	/* 0x44C */ J2DPane* _44C[10];
	/* 0x474 */ void* _474;
	/* 0x478 */ TExPane* _478;
	/* 0x47C */ u8 _47C;
	/* 0x47D */ u8 _47D[0x480 - 0x47D];
	/* 0x480 */ s32 _480;
	/* 0x484 */ u8 _484[0x48C - 0x484];
	/* 0x48C */ JUTRect _48C;
	/* 0x49C */ u8 _49C[0x6F8 - 0x49C];
};

#endif
