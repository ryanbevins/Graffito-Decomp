#ifndef GC2D_SELECT_MENU_HPP
#define GC2D_SELECT_MENU_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JUtility/JUTColor.hpp>
#include <JSystem/JUtility/JUTRect.hpp>
#include <dolphin/types.h>

class JKRArchive;
class TSelectShineManager;
class TSelectDir;
class TMarioGamePad;

namespace JDrama {
class TGraphics;
}

// sizeof = 0x170 (verified from `__nw__FUl(0x170)` before TSelectMenu ctor)
class TSelectMenu : public JDrama::TViewObj {
public:
	TSelectMenu(const char* name);

	virtual void perform(u32 flags, JDrama::TGraphics* graphics);

	void initData(u8 cup, JKRArchive* archive, TSelectShineManager* shineMgr,
	              TSelectDir* dir);
	void startMove();
	void startOpenWindow();
	int getNextIndex();
	int getPrevIndex();

public:
	/* 0x10 */ u32 mState;
	/* 0x14 */ u8 _14[0x58 - 0x14];
	/* 0x58 */ u8 _58[0x68 - 0x58]; // JUTRect, set() in body
	/* 0x68 */ u8 _68[0x100 - 0x68];
	/* 0x100 */ TMarioGamePad* mGamePad;
	/* 0x104 */ u8 _104[0x110 - 0x104];
	/* 0x110 */ u8 _110[0x120 - 0x110]; // JUTRect, set() in body
	/* 0x120 */ u8 _120[0x130 - 0x120]; // JUTRect, set() in body
	/* 0x130 */ TSelectShineManager* mShineManager;
	/* 0x134 */ TSelectDir* mDir;
	/* 0x138 */ u8 _138;
	/* 0x139 */ u8 _139;
	/* 0x13A */ u8 _13A;
	/* 0x13B */ u8 mScenarioIndex;
	/* 0x13C */ u8 _13C;
	/* 0x13D */ u8 _13D[0x140 - 0x13D];
	/* 0x140 */ s32 _140;
	/* 0x144 */ s32 _144;
	/* 0x148 */ u8 _148;
	/* 0x149 */ u8 _149;
	/* 0x14A */ u8 _14A;
	/* 0x14B */ u8 _14B;
	/* 0x14C */ f32 _14C;
	/* 0x150 */ u8 mStageStates[8];
	/* 0x158 */ s32 _158;
	/* 0x15C */ s32 _15C;
	/* 0x160 */ s32 _160;
	/* 0x164 */ s32 _164;
	/* 0x168 */ s16 _168;
	/* 0x16A */ s16 _16A;
	/* 0x16C */ s16 _16C;
	/* 0x16E */ u8 _16E[0x170 - 0x16E];
};

// sizeof = 0x24 (verified from `__nw__FUl(0x24)` before TSelectGrad ctor)
class TSelectGrad : public JDrama::TViewObj {
public:
	TSelectGrad(const char* name);

	virtual void perform(u32 flags, JDrama::TGraphics* graphics);

	void setStageColor(u8 cup);

public:
	/* 0x10 */ s32 _10;
	/* 0x14 */ s32 _14;
	/* 0x18 */ s32 _18;
	/* 0x1C */ JUtility::TColor mColor1;
	/* 0x20 */ JUtility::TColor mColor2;
};

#endif
