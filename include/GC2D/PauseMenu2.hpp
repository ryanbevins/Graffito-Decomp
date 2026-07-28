#ifndef GC2D_PAUSE_MENU_2_HPP
#define GC2D_PAUSE_MENU_2_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JUtility/JUTRect.hpp>

class J2DPicture;
class J2DSetScreen;
class J2DTextBox;
class J2DPane;
class TMarioGamePad;
class JPABaseEmitter;
class TCardSave;

class TPauseMenu2 : public JDrama::TViewObj {
public:
	TPauseMenu2(const char* = "<TPauseMenu>");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);

	void appearWindow();
	void disappearWindow();
	u8 getNextState();
	void setDrawStart();
	void setDrawEnd();
	void drawAppearPane(J2DPicture*, f32, JUTRect&, f32);

public:
	/* 0x10 */ int unk10;
	/* 0x14 */ J2DSetScreen* unk14;
	/* 0x18 */ J2DPane* unk18;
	/* 0x1C */ J2DPane* unk1C;
	/* 0x20 */ J2DPane* unk20[5];
	/* 0x34 */ JUTRect unk34[5];
	/* 0x84 */ f32 unk84[5];
	/* 0x98 */ J2DPicture* unk98[3];
	/* 0xA4 */ JUTRect unkA4[3];
	/* 0xD4 */ J2DTextBox* unkD4;
	/* 0xD8 */ J2DPane* unkD8;
	/* 0xDC */ J2DTextBox* unkDC;
	/* 0xE0 */ u8 unkE0;
	/* 0xE4 */ u32 unkE4;
	/* 0xE8 */ f32 unkE8;
	/* 0xEC */ f32 unkEC;
	/* 0xF0 */ u8 unkF0;
	/* 0xF4 */ f32 unkF4;
	/* 0xF8 */ s16 unkF8;
	/* 0xFC */ s32 unkFC;
	/* 0x100 */ s16 unk100;
	/* 0x102 */ u16 unk102;
	/* 0x104 */ int unk104;
	/* 0x108 */ u8 unk108;
	/* 0x109 */ u8 unk109;
	/* 0x10C */ TMarioGamePad* unk10C;
	/* 0x110 */ JPABaseEmitter* unk110;
	/* 0x114 */ u32 unk114;
	/* 0x118 */ TCardSave* unk118;
};

#endif
