#ifndef GC2D_SELECT_MENU_HPP
#define GC2D_SELECT_MENU_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <dolphin/types.h>

class JKRArchive;
class TSelectShineManager;
class TSelectDir;

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
	void startCloseWindow();
	int getNextIndex();
	int getPrevIndex();

public:
	/* 0x10 */ u8 _10[0x100 - 0x10];
	/* 0x100 */ class TMarioGamePad* mGamePad;
	/* 0x104 */ u8 _104[0x170 - 0x104];
};

// sizeof = 0x24 (verified from `__nw__FUl(0x24)` before TSelectGrad ctor)
class TSelectGrad : public JDrama::TViewObj {
public:
	TSelectGrad(const char* name);

	virtual void perform(u32 flags, JDrama::TGraphics* graphics);

	void setStageColor(u8 cup);

public:
	/* 0x10 */ u8 _10[0x24 - 0x10];
};

#endif
