#include <Camera/Camera.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionData.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MapUtil.hpp>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> s16 CLBRoundf<s16>(f32);

namespace {

inline void* getKindOpt(const CPolarSubCamera* cam)
{
	return *(void**)((u8*)cam + 0x2D4);
}

inline TCameraKindParam* getActiveKindParam(const CPolarSubCamera* cam)
{
	return *(TCameraKindParam**)((u8*)cam + 0x68);
}

inline JGeometry::TVec3<f32>* getTrackPos(const CPolarSubCamera* cam)
{
	return *(JGeometry::TVec3<f32>**)((u8*)cam + 0x2AC);
}

inline void updateInHouseTimer(CPolarSubCamera* cam)
{
	s16 newArea = *(s16*)((u8*)cam + 0x2CA);
	if (newArea != -1) {
		*(s16*)((u8*)cam + 0x2C8) = newArea;
		void* opt = *(void**)((u8*)cam + 0x2D4);
		u8 step = *(u8*)((u8*)opt + 0x108);
		u8 cnt  = *(u8*)((u8*)cam + 0x2CC);
		if ((f32)cnt < (f32)step) {
			*(u8*)((u8*)cam + 0x2CC) = cnt + 1;
		}
		return;
	}
	if (*(s16*)((u8*)cam + 0x2C8) != -1) {
		void* opt = *(void**)((u8*)cam + 0x2D4);
		u8 step = *(u8*)((u8*)opt + 0x108);
		u8 cnt  = *(u8*)((u8*)cam + 0x2CC);
		if ((f32)cnt < (f32)step) {
			*(u8*)((u8*)cam + 0x2CC) = cnt + 1;
		} else {
			*(s16*)((u8*)cam + 0x2C8) = -1;
			*(u8*)((u8*)cam + 0x2CC) = 0;
		}
	}
}

inline bool isValidCamClip(const TBGCheckData* data)
{
	bool ok = false;
	if (data != nullptr) {
		if (data->isLegal()) {
			if (data->mBGType & BG_PROPERTY_FLAG_CAMERA_WONT_CLIP ? true
			                                                     : false)
				ok = true;
		}
	}
	return ok;
}

} // namespace

bool CPolarSubCamera::execGroundCheck_(Vec p)
{
	bool didSnap                 = false;
	void* opt                    = getKindOpt(this);
	JGeometry::TVec3<f32>* track = getTrackPos(this);
	f32 baseGap                  = *(f32*)((u8*)opt + 0xA4);
	f32 interp                   = CLBLinearInbetween<f32>(
        *(f32*)((u8*)opt + 0xB8), *(f32*)((u8*)opt + 0xCC), track->z);
	if (mMode == 0x2A) {
		f32 a = 200.0f;
		if (baseGap > a)
			a = baseGap;
		baseGap = a;

		f32 b = 400.0f;
		if (interp > b)
			b = interp;
		interp = b;
	}

	f32 camBaseY            = *(f32*)((u8*)this + 0xB8);
	const TBGCheckData* gnd;
	f32 groundY             = gpMap->checkGroundIgnoreWaterSurface(
        p.x, camBaseY + baseGap, p.z, &gnd);
	if (isValidCamClip(gnd)) {
		f32 newY = groundY + interp;
		if (*(f32*)((u8*)this + 0x84) < newY) {
			*(f32*)((u8*)this + 0x84) = newY;
			didSnap                   = true;
		}
	}
	return didSnap;
}

bool CPolarSubCamera::execRoofCheck_(Vec p)
{
	bool didSnap             = false;
	bool inMonte             = false;
	f32 roofY                = 0.0f;
	const TBGCheckData* roof = nullptr;
	if (SMS_GetMonteVillageAreaInMario() == 0 && gpCamera->mMode != 0x3E) {
		roofY   = -512.5f;
		inMonte = true;
	} else {
		void* opt    = getKindOpt(this);
		f32 camBaseY = *(f32*)((u8*)this + 0xB8);
		f32& offset  = *(f32*)((u8*)opt + 0xE0);
		roofY        = gpMap->checkRoof(p.x, camBaseY - offset, p.z, &roof);
	}

	if (inMonte || isValidCamClip(roof)) {
		void* opt = getKindOpt(this);
		f32 shift = *(f32*)((u8*)opt + 0xF4);
		f32 newY  = roofY - shift;
		if (mCurrentTarget.mPosition.y > newY) {
			mCurrentTarget.mPosition.y = newY;
			didSnap                     = true;
		}
	}
	return didSnap;
}

bool CPolarSubCamera::execWallCheck_(Vec* p)
{
	bool didSnap = false;
	f32 radius   = *(f32*)((u8*)unk2D4 + 0x7C);
	if (radius > 0.0f) {
		TBGWallCheckRecord record(
		    mCurrentTarget.mPosition.x,
		    10.0f + mPreviousTarget.mPosition.y,
		    mCurrentTarget.mPosition.z, radius, 4, 0);

		if (gpMap->isTouchedWallsAndMoveXZ(&record)) {
			int count = record.mResultWallsNum;
			for (int i = 0; i < count; ++i) {
				TBGCheckData* wall = record.mResultWalls[i];
				if (!isValidCamClip(wall))
					continue;

				JGeometry::TVec3<f32> cam = mCurrentTarget.mPosition;
				JGeometry::TVec3<f32> trg = cam;

				f32 nx    = wall->mNormal.x;
				f32 ny    = wall->mNormal.y;
				f32 nz    = wall->mNormal.z;
				f32 sdist = trg.x * nx + trg.y * ny + trg.z * nz
				            + wall->mPlaneDistance;
				f32 absD = sdist >= 0.0f ? sdist : -sdist;
				if (!(absD < radius))
					continue;

				void* opt   = unk2D4;
				f32 push    = radius - sdist;
				f32 camRate = *(f32*)((u8*)opt + 0x90);
				f32 camPush = push * camRate;
				trg.x += camPush * nx;
				trg.z += camPush * nz;
				mCurrentTarget.mPosition.x = trg.x;
				mCurrentTarget.mPosition.z = trg.z;

				cam.x += push * wall->mNormal.x;
				cam.z += push * wall->mNormal.z;
				p->x    = cam.x;
				p->z    = cam.z;
				didSnap = true;
			}
		}
	}
	return didSnap;
}

bool CPolarSubCamera::isNeedWallCheck_() const
{
	bool result = true;
	if (mMode == 0x49 || isLButtonCameraSpecifyMode(mMode)
	    || isLButtonCameraInbetween() || isTalkCameraSpecifyMode(mMode)
	    || isTalkCameraInbetween() || isRailCameraSpecifyMode(mMode)
	    || mMode == 2 || mMode == 0xD
	    || (*(u16*)((u8*)this + 0x64) & 4) != 0) {
		result = false;
	}
	return result;
}

bool CPolarSubCamera::isNeedRoofCheck_() const
{
	bool result = true;
	if (mMode == 0x49
	    || (isLButtonCameraSpecifyMode(mMode) && !isNowInbetween() ? true
	                                                               : false)
	    || isRailCameraSpecifyMode(mMode) || mMode == 2
	    || *(u16*)((u8*)this + 0x27A) != 0) {
		result = false;
	}
	return result;
}

bool CPolarSubCamera::isNeedGroundCheck_()
{
	bool result = true;
	if (mMode == 0x49
	    || (isLButtonCameraSpecifyMode(mMode) && !isNowInbetween() ? true
	                                                               : false)
	    || isRailCameraSpecifyMode(mMode) || mMode == 2
	    || unk278 != 0) {
		result = false;
	} else if (mMode != 0x2A) {
		if (isNormalCameraSpecifyMode(mMode)
		    || isTowerCameraSpecifyMode(mMode)) {
			TCameraKindParam* pad = getActiveKindParam(this);
			f32 sY                = pad->unk0C * JMASSin(pad->unk1A);
			f32 sX                = pad->unk08 * JMASSin(pad->unk18);
			f32 dy                = mPosition.y - mTarget.y;
			f32 mx                = sY;
			if (sX > sY)
				mx = sX;
			if (dy > 1.25f * mx) {
				result = false;
				if (unk278 < 0x78)
					unk278 = 0x78;
			}
		}
	}
	return result;
}

void CPolarSubCamera::calcInHouseNo_(bool flag)
{
	bool needsRecalc = true;
	if (!flag) {
		bool match1 = (*(f32*)((u8*)this + 0x13C) == *(f32*)((u8*)this + 0x124)
		               && *(f32*)((u8*)this + 0x140)
		                      == *(f32*)((u8*)this + 0x128)
		               && *(f32*)((u8*)this + 0x144)
		                      == *(f32*)((u8*)this + 0x12C));
		if (match1) {
			bool match2
			    = (*(f32*)((u8*)this + 0x160) == *(f32*)((u8*)this + 0x148)
			       && *(f32*)((u8*)this + 0x164)
			              == *(f32*)((u8*)this + 0x14C)
			       && *(f32*)((u8*)this + 0x168)
			              == *(f32*)((u8*)this + 0x150));
			if (match2)
				needsRecalc = false;
		}
	}

	if (needsRecalc) {
		bool skip = false;
		if (getTrackPos(this)->z > 0.3f
		    || (isTalkCameraSpecifyMode(mMode) && !isNowInbetween() ? true
		                                                            : false)
		    || (isLButtonCameraSpecifyMode(mMode) && !isNowInbetween()
		            ? true
		            : false)) {
			skip = true;
		}

		if (skip) {
			*(s16*)((u8*)this + 0x2CA) = -1;
			updateInHouseTimer(this);
			return;
		}

		JGeometry::TVec3<f32> samples[18];

		f32 aspect     = *(f32*)((u8*)this + 0x4C);
		f32 fovy       = *(f32*)((u8*)this + 0x48);
		f32 nearClip   = *(f32*)((u8*)this + 0x28);
		s16 angleZ     = getFinalAngleZ();
		s16 halfFovS16 = CLBRoundf<s16>(182.04445f * (0.5f * fovy));
		f32 halfH      = nearClip * (1.0f / JMASCos(halfFovS16));
		halfH          = JMASSin(halfFovS16) * halfH;
		f32 fullH      = halfH * 2.0f;

		JGeometry::TVec2<f32> halfPlane;
		halfPlane.y = fullH;
		halfPlane.x = halfPlane.y * aspect;

		S16Vec rotBuf[6];
		CLBCalcNearNinePos(samples, rotBuf,
		                   *(JGeometry::TVec3<f32>*)((u8*)this + 0x124),
		                   *(JGeometry::TVec3<f32>*)((u8*)this + 0x148),
		                   angleZ, nearClip, halfPlane);

		f32 sampleOffset                 = *(f32*)((u8*)this + 0x2C4);
		JGeometry::TVec3<f32>* srcSample = samples;
		JGeometry::TVec3<f32>* dstSample = samples + 9;
		for (int i = 0; i < 3; ++i) {
			dstSample[0].x = srcSample[0].x
			                 + *(f32*)((u8*)this + 0x25C) * sampleOffset;
			dstSample[0].y = srcSample[0].y
			                 + *(f32*)((u8*)this + 0x260) * sampleOffset;
			dstSample[0].z = srcSample[0].z
			                 + *(f32*)((u8*)this + 0x264) * sampleOffset;
			dstSample[1].x = srcSample[1].x
			                 + *(f32*)((u8*)this + 0x25C) * sampleOffset;
			dstSample[1].y = srcSample[1].y
			                 + *(f32*)((u8*)this + 0x260) * sampleOffset;
			dstSample[1].z = srcSample[1].z
			                 + *(f32*)((u8*)this + 0x264) * sampleOffset;
			dstSample[2].x = srcSample[2].x
			                 + *(f32*)((u8*)this + 0x25C) * sampleOffset;
			dstSample[2].y = srcSample[2].y
			                 + *(f32*)((u8*)this + 0x260) * sampleOffset;
			dstSample[2].z = srcSample[2].z
			                 + *(f32*)((u8*)this + 0x264) * sampleOffset;
			srcSample += 3;
			dstSample += 3;
		}

		f32 stepY  = *(f32*)((u8*)this + 0x2C0);
		f32 baseY  = -78.0f;

		JGeometry::TVec3<f32>* baseSample = samples;
		for (int i = 0; i < 9; ++i, ++baseSample) {
			JGeometry::TVec3<f32>* p = baseSample;
			for (int j = 0; j < 2; ++j, p += 9) {
				f32 yOff                 = 0.0f;
				for (int k = 0; k < 2; ++k) {
					JGeometry::TVec3<f32> query;
					query.x = p->x;
					query.y = p->y - yOff + baseY;
					query.z = p->z;

					const TBGCheckData* hit;
					gpMap->checkGroundIgnoreWaterSurface(query, &hit);
					if (hit != nullptr) {
						bool inHouse = hit->mBGType == 0x600 ? true : false;
						if (inHouse) {
							*(s16*)((u8*)this + 0x2CA) = hit->mData;
							updateInHouseTimer(this);
							return;
						}
					}
					yOff += stepY;
				}
			}
		}

		*(s16*)((u8*)this + 0x2CA) = -1;
	}
	updateInHouseTimer(this);
}
