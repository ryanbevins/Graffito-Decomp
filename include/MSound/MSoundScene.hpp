#ifndef MSOUND_SCENE_HPP
#define MSOUND_SCENE_HPP

#include <dolphin/types.h>
#include <dolphin/mtx.h>

class MSSceneSE {
public:
	MSSceneSE(u32);
	void frameLoop(u32, Vec*, u8);
	void sortMaxTrans(Vec*, u8, u8);

	/* 0x000 */ u8 mFlag;
	/* 0x004 */ Vec* mPosPtrs[256];
	/* 0x404 */ Vec mSumPos[3];
	/* 0x428 */ Vec* mPool[3][3];
};

#endif
