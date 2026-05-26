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
	mFlag = 0;
	for (int i = 0; i < 256; i++) {
		mPosPtrs[i] = 0;
	}
}

void MSSceneSE::frameLoop(u32 id, Vec* trans, u8 count)
{
	if (!MSGMSound->gateCheck(id))
		return;
	if (count > 256)
		return;

	for (int i = 0; i < count; i++) {
		mPosPtrs[i] = &trans[i];
	}

	for (int i = 0; i < 3; i++) {
		mSumPos[i].x = 0.0f;
		mSumPos[i].y = 0.0f;
		mSumPos[i].z = 0.0f;
		mPool[i][0]  = 0;
		mPool[i][1]  = 0;
		mPool[i][2]  = 0;
	}

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

		Vec* trans_i = mPosPtrs[i];
		if (mPool[sector][0] == 0) {
			mPool[sector][0] = trans_i;
		} else {
			f32 d_trans = MSGMSound->getDistFromCamera(trans_i);
			f32 d_cur   = MSGMSound->getDistFromCamera(mPool[sector][0]);
			if (d_cur < d_trans) {
				sortMaxTrans(mPool[sector][0], sector, 1);
				mPool[sector][0] = trans_i;
			} else {
				sortMaxTrans(trans_i, sector, 1);
			}
		}
	}

	for (int sector = 0; sector < 3; sector++) {
		u8 n = 0;
		for (int j = 0; j < 3; j++) {
			if (mPool[sector][j] == 0)
				break;
			mSumPos[sector].x += mPool[sector][j]->x;
			n++;
			mSumPos[sector].y += mPool[sector][j]->y;
			mSumPos[sector].z += mPool[sector][j]->z;
		}
		if (n != 0) {
			f32 fn = (f32)n;
			mSumPos[sector].x /= fn;
			mSumPos[sector].y /= fn;
			mSumPos[sector].z /= fn;
		}
	}

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
	Vec* cur = mPool[idx][depth];
	f32 d_cur   = MSGMSound->getDistFromCamera(cur);
	f32 d_trans = MSGMSound->getDistFromCamera(trans);
	if (d_cur < d_trans) {
		u8 nd = depth + 1;
		if (nd < 3) {
			if (mPool[idx][nd] == 0) {
				mPool[idx][nd] = cur;
			} else {
				f32 d_cur2 = MSGMSound->getDistFromCamera(cur);
				f32 d_next = MSGMSound->getDistFromCamera(mPool[idx][nd]);
				if (d_next < d_cur2) {
					u8 nd2 = nd + 1;
					if (nd2 < 3)
						sortMaxTrans(mPool[idx][nd], idx, nd2);
					mPool[idx][nd] = cur;
				} else {
					u8 nd2 = nd + 1;
					if (nd2 < 3)
						sortMaxTrans(cur, idx, nd2);
				}
			}
		}
		mPool[idx][depth] = trans;
	} else {
		u8 nd = depth + 1;
		if (nd < 3) {
			if (mPool[idx][nd] == 0) {
				mPool[idx][nd] = trans;
			} else {
				f32 d_trans2 = MSGMSound->getDistFromCamera(trans);
				f32 d_next   = MSGMSound->getDistFromCamera(mPool[idx][nd]);
				if (d_next < d_trans2) {
					u8 nd2 = nd + 1;
					if (nd2 < 3)
						sortMaxTrans(mPool[idx][nd], idx, nd2);
					mPool[idx][nd] = trans;
				} else {
					u8 nd2 = nd + 1;
					if (nd2 < 3)
						sortMaxTrans(trans, idx, nd2);
				}
			}
		}
	}
}
