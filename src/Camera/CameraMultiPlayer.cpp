#include <Camera/Camera.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>

template <> f32 CLBCalcRatio<f32>(f32, f32, f32);
template <> s16 CLBLinearInbetween<s16>(s16, s16, f32);

struct TMultiPlayerData {
	TMultiPlayerData()
	{
		unk0 = 0;
		unk4 = 0.0f;
		unk8 = 0.0f;
	}
	/* 0x0 */ const JGeometry::TVec3<f32>* unk0;
	/* 0x4 */ f32 unk4;
	/* 0x8 */ f32 unk8;
};

struct TMultiPlayerContainer {
	/* 0x0 */ u8 mCapacity;
	/* 0x1 */ u8 mCount;
	/* 0x4 */ TMultiPlayerData* mData;
};

void CPolarSubCamera::ctrlMultiPlayerCamera_()
{
	TMultiPlayerContainer* c = unk2BC;
	int count = c->mCount;
	if (count <= 0) {
		// Fall back to current target / pos
		*(f32*)((u8*)this + 0x8C) = *(f32*)((u8*)this + 0x148);
		*(f32*)((u8*)this + 0x90) = *(f32*)((u8*)this + 0x14C);
		*(f32*)((u8*)this + 0x94) = *(f32*)((u8*)this + 0x150);
		*(f32*)((u8*)this + 0x98) = *(f32*)((u8*)this + 0x124);
		*(f32*)((u8*)this + 0x9C) = *(f32*)((u8*)this + 0x128);
		*(f32*)((u8*)this + 0xA0) = *(f32*)((u8*)this + 0x12C);
	} else {
		JGeometry::TVec3<f32> sum(0.0f, 0.0f, 0.0f);
		TMultiPlayerData* data = c->mData;
		for (int i = 0; i < count; i++) {
			const JGeometry::TVec3<f32>* p = data[i].unk0;
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
		for (int i = 0; i < count; i++) {
			const JGeometry::TVec3<f32>* a = data[i].unk0;
			for (int j = i + 1; j < count; j++) {
				const JGeometry::TVec3<f32>* b = data[j].unk0;
				f32 dx = a->x - b->x;
				f32 dy = a->y - b->y;
				f32 dz = a->z - b->z;
				f32 d  = dx * dx + dy * dy + dz * dz;
				if (d > maxDistSq)
					maxDistSq = d;
			}
		}

		f32 dist = maxDistSq;
		if (maxDistSq > 0.0f) {
			f64 root = __frsqrte(maxDistSq);
			dist     = 0.5 * root
			       * (3.0 - maxDistSq * (root * root)) * maxDistSq;
		}
		f32 r    = 300.0f + 1.5f * dist;

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

		JGeometry::TVec3<f32>* target = (JGeometry::TVec3<f32>*)((u8*)this + 0x98);
		*(f32*)((u8*)this + 0x8C)     = sum.x;
		*(f32*)((u8*)this + 0x90)     = sum.y;
		*(f32*)((u8*)this + 0x94)     = sum.z;
		CLBPolarToCross(sum, target, r, outAng, 0);
	}
	calcPosAndAt_();
}

bool CPolarSubCamera::removeMultiPlayer(const JGeometry::TVec3<f32>* p)
{
	TMultiPlayerContainer* c = unk2BC;
	if (c == nullptr)
		return false;

	bool found = false;
	TMultiPlayerData* data = c->mData;
	for (int i = 0; i < c->mCount; i++, data++) {
		if (found || data->unk0 == p) {
			if (i != c->mCount - 1) {
				*data = data[1];
			}
			found = true;
		}
	}
	if (found) {
		c->mCount -= 1;
	}
	return found;
}

bool CPolarSubCamera::addMultiPlayer(const JGeometry::TVec3<f32>* p, f32 a,
                                     f32 b)
{
	TMultiPlayerContainer* c = unk2BC;
	if (c == nullptr)
		return false;

	bool added;
	if (c->mCount >= c->mCapacity) {
		added = false;
	} else {
		TMultiPlayerData* slot = &c->mData[c->mCount];
		slot->unk0             = p;
		slot->unk4             = a;
		slot->unk8             = b;
		c->mCount += 1;
		added = true;
	}
	return added;
}

void CPolarSubCamera::createMultiPlayer(u8 capacity)
{
	if (unk2BC != nullptr)
		return;

	TMultiPlayerContainer* c = new TMultiPlayerContainer();
	if (c != nullptr) {
		c->mCapacity = capacity;
		c->mCount    = 0;
		c->mData     = nullptr;
		c->mData     = new TMultiPlayerData[capacity];
	}

	unk2BC = c;
}
