#ifndef MOVE_BG_MAP_OBJ_FLAG_HPP
#define MOVE_BG_MAP_OBJ_FLAG_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/ResTIMG.hpp>
#include <Strategic/HitActor.hpp>

class TMapObjFlag : public THitActor {
public:
	TMapObjFlag(const char* name);
	virtual ~TMapObjFlag();
	virtual void load(JSUMemoryInputStream&);

	void init(const char*);
	virtual void updateVertex();
	void draw();

	static f32 mFlutterSpeed;

public:
	/* 0x68 */ f32 mScaledHeight;
	/* 0x6C */ f32 mScaledWidth;
	/* 0x70 */ int mNumRows;
	/* 0x74 */ int mNumCols;
	/* 0x78 */ Vec** mVertexGrid;
	/* 0x7C */ f32 mFlagHeight;
	/* 0x80 */ f32 mFlagWidth;
	/* 0x84 */ f32 mSegmentSize;
	/* 0x88 */ f32 mPhase;
	/* 0x8C */ Mtx mLocalMtx;
	/* 0xBC */ int mStepSize;
};

class TMapObjFlagManager;
extern TMapObjFlagManager* gpMapObjFlagManager;

class TMapObjFlagManager : public JDrama::TViewObj {
public:
	TMapObjFlagManager(const char* name);
	virtual ~TMapObjFlagManager();
	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

	void registerObj(TMapObjFlag*, const char*);
	void initDraw();

	struct TMapObjFlagInfo {
		TMapObjFlagInfo();

		/* 0x00 */ int mCount;
		/* 0x04 */ TMapObjFlag* mFlags[16];
		/* 0x44 */ u32 _44;
		/* 0x48 */ u32 _48;
		/* 0x4C */ u32 _4C;
		/* 0x50 */ u32 _50;
		/* 0x54 */ ResTIMG* mTexture;
	};

public:
	/* 0x10 */ TMapObjFlagInfo mInfo[15];
};

#endif
