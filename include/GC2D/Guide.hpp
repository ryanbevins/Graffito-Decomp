#ifndef GC2D_GUIDE_HPP
#define GC2D_GUIDE_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>

class JKRMemArchive;
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
	void setup(JKRMemArchive*);
	void startMoveCursor();
	void linkSelect();
	int checkPoint(int, int);
	void changeBotStatus(int);
	void placeMario();
	void appearGuidePane(int);

public:
	/* 0x10 */ char unk10[0xC0 - 0x10];
	/* 0xC0 */ TMarioGamePad* unkC0;
	/* 0xC4 */ u8 unkC4;
	/* 0xC5 */ u8 unkC5;
	/* 0xC6 */ char unkC6[0x6f8 - 0xC6];
};

#endif
