#ifndef PLAYER_SPLASH_MANAGER_HPP
#define PLAYER_SPLASH_MANAGER_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JSupport/JSUList.hpp>
#include <JSystem/JGeometry.hpp>

class JUTTexture;
class TDLColorTexQuad;

struct TWaterSplash {
	/* 0x00 */ JGeometry::TVec3<f32> mPos;
	/* 0x0C */ f32 mVelY;
	/* 0x10 */ u8 mLife;
	/* 0x11 */ u8 mIndex;
	/* 0x12 */ u8 _12[2];
};

class TSplashManager;
extern TSplashManager* gpSplashManager;

class TSplashManager : public JDrama::TViewObj {
public:
	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

	void newSplash(JGeometry::TVec3<f32>, f32);
	void move();
	void makeDL(JDrama::TGraphics*) const;
	void draw() const;

public:
	/* 0x010 */ u16 mFlags;
	/* 0x012 */ u8 _12[2];
	/* 0x014 */ JUTTexture* mTexture;
	/* 0x018 */ JSULink<TWaterSplash>* mLinks[0x40];
	/* 0x118 */ JSUList<TWaterSplash> mActiveList;
	/* 0x124 */ JSUList<TWaterSplash> mFreeList;
	/* 0x130 */ TWaterSplash mSplashes[0x40];
	/* 0x630 */ f32 mUnk630;
	/* 0x634 */ f32 mUnk634;
	/* 0x638 */ f32 mGravity;
	/* 0x63C */ u32 mUnk63C;
	/* 0x640 */ TDLColorTexQuad* mQuad;
	/* 0x644 */ f32 mUnk644;
	/* 0x648 */ u8 mInitLife;
	/* 0x649 */ u8 _649[3];
};

#endif
