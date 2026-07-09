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
	if (!MSGMSound->gateCheck(id))
		return;
	if (count > 256)
		return;

	Vec* p = trans;
	u8 i   = 0;
	if (count > 8) {
		u8 lim = count - 8;
		while (i < lim) {
			mPosPtrs[i + 0] = p;
			mPosPtrs[i + 1] = p + 1;
			mPosPtrs[i + 2] = p + 2;
			mPosPtrs[i + 3] = p + 3;
			mPosPtrs[i + 4] = p + 4;
			mPosPtrs[i + 5] = p + 5;
			mPosPtrs[i + 6] = p + 6;
			mPosPtrs[i + 7] = p + 7;
			p += 8;
			i += 8;
		}
	}
	while (i < count) {
		mPosPtrs[i] = p;
		p++;
		i++;
	}

	u8 s = 0;
	do {
		mSumPos[s].x = 0.0f;
		mSumPos[s].y = 0.0f;
		mSumPos[s].z = 0.0f;
		mPool[s][0]  = 0;
		mPool[s][1]  = 0;
		mPool[s][2]  = 0;
		s++;
	} while (s < 3);

	MtxPtr camMtx = MSGMSound->unk8->unk8;

	for (int i = 0; i < count; i++) {
		Vec out;
		PSMTXMultVec(camMtx, mPosPtrs[i], &out);
		f32 angle = atan2f(out.x, out.z);

		if (angle < -3.1415927f)
			angle = -3.1415927f;
		else if (angle > 3.1415927f)
			angle = 3.1415927f;

		u8 sector;
		if (angle >= -3.1415927f && angle < -1.0470928f) {
			sector = 0;
		} else if (angle >= -1.0470928f && angle < 1.0470928f) {
			sector = 1;
		} else {
			sector = 2;
		}

		sortMaxTrans(mPosPtrs[i], sector, 0);
	}

	u8 sec = 0;
	do {
		u8 j = 0;
		do {
			Vec** slot = &mPool[sec][j];
			if (*slot == 0)
				break;
			mSumPos[sec].x += (*slot)->x;
			j++;
			mSumPos[sec].y += (*slot)->y;
			mSumPos[sec].z += (*slot)->z;
		} while (j < 3);
		if (j != 0) {
			f32 fn = (f32)j;
			mSumPos[sec].x /= fn;
			mSumPos[sec].y /= fn;
			mSumPos[sec].z /= fn;
		}
		sec++;
	} while (sec < 3);

	for (u8 sector = 0; sector < 3; sector++) {
		if (mPool[sector][0] == 0)
			continue;
		if (mFlag == 0) {
			u32 sid = id + sector;
			if (MSGMSound->gateCheck(sid)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    sid, &mSumPos[sector], 0, 0, 0, 4);
			}
		} else {
			MSoundSESystem::MSRandPlay::startSeRandPlay(id, sector);
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
