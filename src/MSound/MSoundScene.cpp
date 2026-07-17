#include <MSound/MSoundScene.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
#include <JSystem/JAudio/JALibrary/JALModSe.hpp>
#include <dolphin/mtx.h>
#include <math.h>

MSSceneSE::MSSceneSE(u32 param)
{
	for (int i = 0; i < 256; i++) {
		mPosPtrs[i] = 0;
	}
	mFlag = 0;
}

void MSSceneSE::frameLoop(u32 id, Vec* trans, u8 count)
{
	if (MSGMSound->gateCheck(id) && count <= 0x100) {
		Vec* ptr = trans;
		for (u8 i = 0; i < count; ++i) {
			mPosPtrs[i] = ptr;
			++ptr;
		}

		MtxPtr m = MSGMSound->unk8->unk8;
		for (u8 i = 0; i < 3; ++i) {
			mSumPos[i].x = 0.0f;
			mSumPos[i].y = 0.0f;
			mSumPos[i].z = 0.0f;
			for (int j = 0; j < 3; ++j)
				mPool[i][j] = 0;
		}

		for (u8 i = 0; i < count; ++i) {
			Vec out;
			PSMTXMultVec(m, mPosPtrs[i], &out);

			f32 angle = atan2f(out.x, out.z);
			f32 clamped;
			if (angle < -3.1415927f)
				clamped = -3.1415927f;
			else if (angle > 3.1415927f)
				clamped = 3.1415927f;
			else
				clamped = angle;

			u8 sector;
			if (clamped >= -3.1415927f && clamped < -1.0470928f)
				sector = 0;
			else if (clamped >= -1.0470928f && clamped < 1.0470928f)
				sector = 1;
			else
				sector = 2;

			sortMaxTrans(mPosPtrs[i], sector, 0);
		}

		for (u8 i = 0; i < 3; ++i) {
			u8 j;
			for (j = 0; j < 3; ++j) {
				if (mPool[i][j] == 0)
					break;
				mSumPos[i].x += mPool[i][j]->x;
				mSumPos[i].y += mPool[i][j]->y;
				mSumPos[i].z += mPool[i][j]->z;
			}

			if (j != 0) {
				mSumPos[i].x /= j;
				mSumPos[i].y /= j;
				mSumPos[i].z /= j;
			}
		}

		for (u8 i = 0; i < 3; ++i) {
			if (mPool[i][0] != 0) {
				if (!mFlag) {
					if (MSGMSound->gateCheck(id + i))
						MSoundSESystem::MSoundSE::startSoundActor(
						    id + i, &mSumPos[i], 0, 0, 0, 4);
				} else {
					MSoundSESystem::MSRandPlay::startSeRandPlay(id, i);
				}
			}
		}
	}
}

void MSSceneSE::sortMaxTrans(Vec* trans, u8 idx, u8 depth)
{
	if (mPool[idx][depth] == 0) {
		mPool[idx][depth] = trans;
		return;
	}

	if (MSGMSound->getDistFromCamera(mPool[idx][depth])
	    >= MSGMSound->getDistFromCamera(trans)) {
		if (depth + 1 < 3)
			sortMaxTrans(mPool[idx][depth], idx, depth + 1);
		mPool[idx][depth] = trans;
	} else {
		if (depth + 1 < 3)
			sortMaxTrans(trans, idx, depth + 1);
	}
}
