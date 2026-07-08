#include <Camera/CameraMultiPlayer.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Camera/Camera.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>

template <> f32 CLBCalcRatio<f32>(f32, f32, f32);
template <> s16 CLBLinearInbetween<s16>(s16, s16, f32);

inline
TCameraMultiPlayer::TCameraMultiPlayer(u8 max_player_count)
    : mMaxPlayers(max_player_count)
    , mPlayerCount(0)
    , mPlayers(nullptr)
{
	mPlayers = new TMultiPlayerData[max_player_count];
}

inline
bool TCameraMultiPlayer::addPlayer(const JGeometry::TVec3<f32>* param_1,
                                   f32 param_2, f32 param_3)
{
	if (mPlayerCount >= mMaxPlayers)
		return false;

	TMultiPlayerData* data = &mPlayers[mPlayerCount];
	data->unk0             = param_1;
	data->unk4             = param_2;
	data->unk8             = param_3;
	mPlayerCount += 1;

	return true;
}

inline bool
TCameraMultiPlayer::removePlayer(const JGeometry::TVec3<f32>* param_1)
{
	bool found = false;
	int i;
	TMultiPlayerData* it = mPlayers;
	for (i = 0; i < mPlayerCount; ++i, ++it) {
		if (found == true || (found == false && it->unk0 == param_1)) {
			if (i != mPlayerCount - 1)
				*it = *(it + 1);
			found = true;
		}
	}

	if (found != true)
		return found;

	mPlayerCount -= 1;
	return found;
}

void CPolarSubCamera::createMultiPlayer(u8 param_1)
{
	if (!unk2BC)
		unk2BC = new TCameraMultiPlayer(param_1);
}

bool CPolarSubCamera::addMultiPlayer(const JGeometry::TVec3<f32>* param_1,
                                     f32 param_2, f32 param_3)
{
	TCameraMultiPlayer* c = unk2BC;
	if (c == nullptr)
		return false;

	bool added;
	if (c->mPlayerCount >= c->mMaxPlayers) {
		added = false;
	} else {
		TMultiPlayerData* slot = &c->mPlayers[c->mPlayerCount];
		slot->unk0             = param_1;
		slot->unk4             = param_2;
		slot->unk8             = param_3;
		c->mPlayerCount += 1;
		added = true;
	}
	return added;
}

bool CPolarSubCamera::removeMultiPlayer(const JGeometry::TVec3<f32>* param_1)
{
	if (!unk2BC)
		return false;

	return unk2BC->removePlayer(param_1);
}

void CPolarSubCamera::ctrlMultiPlayerCamera_()
{
	TCameraMultiPlayer* c = unk2BC;
	int count              = c->mPlayerCount;
	if (count <= 0) {
		*(f32*)((u8*)this + 0x8C) = *(f32*)((u8*)this + 0x148);
		*(f32*)((u8*)this + 0x90) = *(f32*)((u8*)this + 0x14C);
		*(f32*)((u8*)this + 0x94) = *(f32*)((u8*)this + 0x150);
		*(f32*)((u8*)this + 0x98) = *(f32*)((u8*)this + 0x124);
		*(f32*)((u8*)this + 0x9C) = *(f32*)((u8*)this + 0x128);
		*(f32*)((u8*)this + 0xA0) = *(f32*)((u8*)this + 0x12C);
	} else {
		JGeometry::TVec3<f32> sum(0.0f, 0.0f, 0.0f);
		TMultiPlayerData* data = unk2BC->mPlayers;
		for (int i = count; i > 0; i--, data++) {
			const JGeometry::TVec3<f32>* p = data->unk0;
			sum.x += p->x;
			sum.y += p->y;
			sum.z += p->z;
		}

		f32 invCount = 1.0f / (f32)count;
		sum.x *= invCount;
		sum.y *= invCount;
		sum.z *= invCount;

		sum.y += unk68->unk24;

		f32 maxDistSq = 0.0f;
		data           = unk2BC->mPlayers;
		for (int i = 0; i < count; i++) {
			const JGeometry::TVec3<f32>* a = data[i].unk0;
			for (int j = i + 1; j < count; j++) {
				const JGeometry::TVec3<f32>* b = data[j].unk0;
				f32 dx = a->x - b->x;
				f32 dy = a->y - b->y;
				f32 dz = a->z - b->z;
				f32 dxSq = dx * dx;
				f32 dySq = dy * dy;
				f32 dzSq = dz * dz;
				f32 d    = dxSq + dySq;
				d        = dzSq + d;
				if (d > maxDistSq)
					maxDistSq = d;
			}
		}

		f32 dist;
		if (maxDistSq > 0.0f) {
			f64 root = __frsqrte(maxDistSq);
			dist     = 0.5 * root
			       * (3.0 - maxDistSq * (root * root)) * maxDistSq;
		} else {
			dist = maxDistSq;
		}
		f32 r = 300.0f + 1.5f * dist;

		f32 lo = unk68->unk08;
		f32 hi = unk68->unk0C;
		if (r > hi)
			r = hi;
		else if (r < lo)
			r = lo;

		f32 ratio  = CLBCalcRatio<f32>(lo, hi, r);
		s16 angA   = unk68->unk18;
		s16 angB   = unk68->unk1A;
		s16 outAng = (s16)CLBLinearInbetween<s16>(angA, angB, ratio);

		JGeometry::TVec3<f32>* target
		    = (JGeometry::TVec3<f32>*)((u8*)this + 0x98);
		*(f32*)((u8*)this + 0x8C) = sum.x;
		*(f32*)((u8*)this + 0x90) = sum.y;
		*(f32*)((u8*)this + 0x94) = sum.z;
		CLBPolarToCross(sum, target, r, outAng, 0);
	}

	calcPosAndAt_();
}
