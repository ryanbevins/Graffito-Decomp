#ifndef GC2D_TALK_2D_2_HPP
#define GC2D_TALK_2D_2_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <dolphin/gx/GXStruct.h>

class J2DPane;
class J2DSetScreen;
class JUTPoint;
class JUTTexture;
class J2DTextBox;
class JMSMesgEntry;
class TBoundPane;
class TBaseNPC;
class TMessageLoader;

class TTalk2D2;

extern TTalk2D2* gpTalk2D;

struct TTalk2DNameRefEntry {
	/* 0x0 */ JDrama::TNameRef* unk0;
	/* 0x4 */ u32 unk4;
};

class TTalk2D2 : public JDrama::TViewObj {
public:
	TTalk2D2(const char* name = "<TTalk2D2>");

	virtual void load(JSUMemoryInputStream&);
	virtual void loadAfter();
	virtual void perform(u32, JDrama::TGraphics*);

	void setMessageID(u32, u32);
	void forceCloseTalk();
	void closeTalkWindow();
	void openTalkWindow(TBaseNPC*);
	void makeBoxLine(s8, char*);
	bool openBoardWindow();
	bool openNormalWindow();
	void moveBoardWindow();
	void checkBoardControler();
	void moveTalkWindow();
	void checkControler();
	bool closeNormalWindow();
	void closeBoardWindow();
	bool eraseNormalWindow();
	bool eraseBoardWindow();
	void appearBoardBoxWindow();
	void makeLine(f32*, f32*, f32, JUTPoint&, JUTPoint&, JUTPoint&);
	void setupBoardTextBox(const void*, JMSMesgEntry*);
	void setupTextBox(const void*, JMSMesgEntry*);
	void setTagParam(JSUMemoryInputStream&, J2DTextBox&, int*, int*);
	void openWindow(s8, f32);

	static GXColor cColorTable[6];

	int getTalkMode() const { return unk248; }
	s8 getSelectedValue() const { return unk214; }

public:
	/* 0x10 */ J2DSetScreen* unk10;
	/* 0x14 */ TBoundPane* unk14;
	/* 0x18 */ J2DTextBox* unk18;
	/* 0x1C */ J2DPane* unk1C;
	/* 0x20 */ J2DPane* unk20;
	/* 0x24 */ J2DPane* unk24;
	/* 0x28 */ u8 unk28;
	/* 0x29 */ u8 unk29[3];
	/* 0x2C */ J2DSetScreen* unk2C;
	/* 0x30 */ J2DPane* unk30[3];
	/* 0x3C */ J2DPane* unk3C[3];
	/* 0x48 */ J2DPane* unk48[3];
	/* 0x54 */ J2DPane* unk54[3];
	/* 0x60 */ J2DPane* unk60[3];
	/* 0x6C */ J2DPane* unk6C[3];
	/* 0x78 */ J2DPane* unk78[3];
	/* 0x84 */ J2DPane* unk84[3];
	/* 0x90 */ J2DPane* unk90;
	/* 0x94 */ f32 unk94;
	/* 0x98 */ u8 unk98[4];
	/* 0x9C */ J2DTextBox* unk9C[90];
	/* 0x204 */ J2DPane* unk204;
	/* 0x208 */ J2DTextBox* unk208;
	/* 0x20C */ J2DTextBox* unk20C[2];
	/* 0x214 */ s8 unk214;
	/* 0x215 */ u8 unk215[3];
	/* 0x218 */ char* unk218[2];
	/* 0x220 */ s16 unk220;
	/* 0x222 */ s16 unk222;
	/* 0x224 */ u8 unk224;
	/* 0x225 */ u8 unk225;
	/* 0x226 */ u8 unk226;
	/* 0x227 */ u8 unk227;
	/* 0x228 */ s32 unk228[3];
	/* 0x234 */ f32 unk234;
	/* 0x238 */ f32 unk238;
	/* 0x23C */ f32 unk23C;
	/* 0x240 */ u8 unk240[4];
	/* 0x244 */ JUTTexture* unk244;
	/* 0x248 */ u32 unk248; // talk mode
	/* 0x24C */ u32 unk24C;
	/* 0x250 */ u8 unk250;
	/* 0x251 */ u8 unk251;
	/* 0x252 */ u8 unk252;
	/* 0x253 */ u8 unk253;
	/* 0x254 */ JMSMesgEntry* unk254;
	/* 0x258 */ TMessageLoader* unk258;
	/* 0x25C */ TMessageLoader* unk25C;
	/* 0x260 */ TMessageLoader* unk260;
	/* 0x264 */ u32 unk264;
	/* 0x268 */ u8 unk268[2];
	/* 0x26A */ u8 unk26A;
	/* 0x26B */ u8 unk26B;
	/* 0x26C */ u8 unk26C;
	/* 0x26D */ u8 unk26D;
	/* 0x26E */ u8 unk26E[2];
	/* 0x270 */ s32 unk270;
	/* 0x274 */ s32 unk274;
	/* 0x278 */ s32 unk278;
	/* 0x27C */ GXColor unk27C;
	/* 0x280 */ u8 unk280;
	/* 0x281 */ u8 unk281[0x2DC - 0x281];
	/* 0x2DC */ s16 unk2DC;
	/* 0x2DE */ u16 unk2DE;
	/* 0x2E0 */ TTalk2DNameRefEntry unk2E0[10];
	/* 0x330 */ s16 unk330;
	/* 0x332 */ s16 unk332;
	/* 0x334 */ s16 unk334;
	/* 0x336 */ u8 unk336[2];
	/* 0x338 */ f32 unk338;
	/* 0x33C */ f32 unk33C;
	/* 0x340 */ s16 unk340;
	/* 0x342 */ u8 unk342[2];
};

#endif
