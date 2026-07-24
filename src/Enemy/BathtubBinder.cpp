#include <Enemy/BathtubBinder.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#define JG_TUTIL_SQRT_OUT_OF_LINE
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JGeometry/JGVec3.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Map/BathWaterManager.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <PowerPC_EABI_Support/Msl/MSL_C/MSL_Common/math.h>
#include <Strategic/LiveActor.hpp>

static inline JGeometry::TVec3<f32> getBathtubCenter(u8* bathtubData)
{
	return JGeometry::TVec3<f32>(
	    *(f32*)(bathtubData + 0x00),
	    *(f32*)(bathtubData + 0x04) - *(f32*)(bathtubData + 0x44),
	    *(f32*)(bathtubData + 0x08));
}

TBathtubBinder::TBathtubBinder()
{
	mBathtub      = nullptr;
	mBathWaterMgr = nullptr;
}

TBathtubBinder::~TBathtubBinder() { }

bool TBathtubBinder::init(f32 a, f32 b, f32 c, f32 d, f32 e)
{
	mBathtub      = JDrama::TNameRefGen::search<TBathtub>("\x83\x6F\x83\x58\x83\x5E\x83\x75");
	mBathWaterMgr = JDrama::TNameRefGen::search<TBathWaterManager>(
	    "\x83\x6F\x83\x58\x83\x5E\x83\x75\x82\xCC\x90\x85");

	mUnk20 = e;
	mUnk0C = a;
	mUnk10 = b;
	mUnk14 = c;
	mUnk18 = d;
	mUnk1C = mUnk18 / (mUnk10 + mUnk18);

	if (mBathtub == nullptr)
		mBathWaterMgr = nullptr;

	return mBathtub != nullptr;
}

void TBathtubBinder::bind(TLiveActor* actor)
{
	if (mBathtub == nullptr || *((u8*)mBathtub + 0x29A) == 0)
		float_(actor);
}

void TBathtubBinder::float_(TLiveActor* actor)
{
	if (mBathWaterMgr == nullptr)
		return;

	Mtx rotMtx;
	MsMtxSetRotRPH(rotMtx, actor->mRotation.x, actor->mRotation.y,
	               actor->mRotation.z);

	f32 sinY = rotMtx[0][2];
	f32 cosY = rotMtx[2][2];

	f32 worldX = sinY * mUnk0C + actor->mPosition.x;
	f32 worldZ = cosY * mUnk0C + actor->mPosition.z;
	f32 localR = mUnk10;

	if (mBathtub != nullptr) {
		u8* tub = (u8*)mBathtub + 0x170;
		JGeometry::TVec3<f32> base = getBathtubCenter(tub);

		f32 cap   = *(f32*)(tub + 0x3C);
		f32 lim   = *(f32*)(tub + 0x44);
		f32 capSq = cap * cap;
		f32 limSq = lim * lim;
		f32 R     = JGeometry::TUtil<f32>::sqrt(capSq - limSq);
		f32 clipR = R - localR;
		f32 dx = worldX - base.x;
		f32 dz = worldZ - base.z;
		f32 distSq = dz * dz + dx * dx;
		if (distSq > clipR * clipR) {
			f32 inv;
			if (distSq <= 0.0f) {
				inv = distSq;
			} else {
				f32 root = __frsqrte(distSq);
				inv = 0.5f * root * (3.0f - distSq * (root * root));
			}
			f32 scale = clipR * inv;
			worldX = base.x + scale * dx;
			worldZ = base.z + scale * dz;
		}
	}

	f32 wH1 = mBathWaterMgr->getWaterHeight(worldX, worldZ);
	JGeometry::TVec3<f32> front(worldX, mUnk20 + wH1, worldZ);

	f32 worldX2 = sinY * -mUnk14 + actor->mPosition.x;
	f32 worldZ2 = cosY * -mUnk14 + actor->mPosition.z;
	f32 localR2 = mUnk18;

	if (mBathtub != nullptr) {
		u8* tub = (u8*)mBathtub + 0x170;
		JGeometry::TVec3<f32> base = getBathtubCenter(tub);

		f32 cap   = *(f32*)(tub + 0x3C);
		f32 lim   = *(f32*)(tub + 0x44);
		f32 capSq = cap * cap;
		f32 limSq = lim * lim;
		f32 R     = JGeometry::TUtil<f32>::sqrt(capSq - limSq);
		f32 clipR = R - localR2;
		f32 dx = worldX2 - base.x;
		f32 dz = worldZ2 - base.z;
		f32 distSq = dz * dz + dx * dx;
		if (distSq > clipR * clipR) {
			f32 inv;
			if (distSq <= 0.0f) {
				inv = distSq;
			} else {
				f32 root = __frsqrte(distSq);
				inv = 0.5f * root * (3.0f - distSq * (root * root));
			}
			f32 scale = clipR * inv;
			worldX2 = base.x + scale * dx;
			worldZ2 = base.z + scale * dz;
		}
	}

	f32 wH2 = mBathWaterMgr->getWaterHeight(worldX2, worldZ2);
	JGeometry::TVec3<f32> rear(worldX2, mUnk20 + wH2, worldZ2);
	JGeometry::TVec3<f32> delta;
	delta.sub(front, rear);

	actor->mPosition.y
	    = 0.2f
	          * (mUnk1C * delta.y + rear.y - actor->mPosition.y)
	      + actor->mPosition.y;

	if (delta.squared() <= 0.0000038146973f)
		return;

	f32 hSq = delta.x * delta.x + delta.z * delta.z;
	f32 hLen;
	if (hSq <= 0.0f) {
		hLen = hSq;
	} else {
		f32 root = __frsqrte(hSq);
		hLen = hSq * (0.5f * root * (3.0f - hSq * (root * root)));
	}

	s16 ang = matan(delta.y, hLen);
	f32 deg = ang * 0.005493164f;

	f32 clamped = deg;
	if (deg < -15.0f)
		clamped = -15.0f;
	else if (deg > 15.0f)
		clamped = 15.0f;

	f32 stepX = 0.1f * (clamped - actor->mRotation.x) + actor->mRotation.x;
	actor->mRotation.x = stepX;
	actor->mRotation.z = 0.0f;

	f32 worldX3 = (mUnk10 + mUnk18) * 0.5f;
	if (mBathtub == nullptr)
		return;

	{
		u8* tub = (u8*)mBathtub + 0x170;
		JGeometry::TVec3<f32> base = getBathtubCenter(tub);

		f32 cap   = *(f32*)(tub + 0x3C);
		f32 lim   = *(f32*)(tub + 0x44);
		f32 capSq = cap * cap;
		f32 limSq = lim * lim;
		f32 R     = JGeometry::TUtil<f32>::sqrt(capSq - limSq);
		f32 clipR = R - worldX3;
		f32 dx = actor->mPosition.x - base.x;
		f32 dz = actor->mPosition.z - base.z;
		f32 distSq = dz * dz + dx * dx;
		if (distSq > clipR * clipR) {
			f32 inv;
			if (distSq <= 0.0f) {
				inv = distSq;
			} else {
				f32 root = __frsqrte(distSq);
				inv = 0.5f * root * (3.0f - distSq * (root * root));
			}
			f32 scale = clipR * inv;
			actor->mPosition.x = base.x + scale * dx;
			actor->mPosition.z = base.z + scale * dz;
		}

		if (actor->mPosition.y < base.y)
			actor->mPosition.y = base.y;
	}
}
