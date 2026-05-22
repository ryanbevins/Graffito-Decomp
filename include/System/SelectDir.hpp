#ifndef SYSTEM_SELECT_DIR_HPP
#define SYSTEM_SELECT_DIR_HPP

#include <JSystem/JDrama/JDRDirector.hpp>
#include <dolphin/types.h>

namespace JDrama {
class TDisplay;
class TScreen;
}

class TMarioGamePad;
class JKRArchive;
class JPAEmitterManager;
class TSelectMenu;
class TSelectGrad;
class TSelectShineManager;

class TSelectDir : public JDrama::TDirector {
public:
	TSelectDir();
	virtual ~TSelectDir();

	virtual int direct();

	void setup(JDrama::TDisplay*, TMarioGamePad*, u8);
	static void* setupThreadFunc(void*);
	int rsetup();
	void changeOrder();

public:
	// unk10/unk14 belong to JDrama::TDirector — TViewObj* root + TDStageGroup*
	/* 0x18 */ TMarioGamePad* mGamePad;
	/* 0x1C */ JDrama::TDisplay* mDisplay;
	/* 0x20 */ TSelectMenu* mMenu;
	/* 0x24 */ TSelectGrad* mGrad;
	/* 0x28 */ TSelectShineManager* mShineManager;
	/* 0x2C */ JKRArchive* mArchive;
	/* 0x30 */ JPAEmitterManager* mEmitterMgr1;
	/* 0x34 */ JPAEmitterManager* mEmitterMgr2;
	/* 0x38 */ u8 mInitDone;
	/* 0x3C */ s32 unk3C;
	/* 0x40 */ u8 mCupId;
	/* 0x44 */ JDrama::TScreen* mScreenGrad;
	/* 0x48 */ JDrama::TScreen* mScreen2D;
	/* 0x4C */ u8 mResetTriggered;
};

#endif
