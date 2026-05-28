#include <Enemy/FeetInv.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JGeometry.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <dolphin/mtx.h>

void FeetInvCalc(J3DModel* model, u16 hipIdx, u16 kneeIdx, u16 footIdx,
                 f32 threshold)
{
	JGeometry::TVec3<f32> kneePos;
	JGeometry::TVec3<f32> footPos;
	MtxPtr kneeMtx = model->getAnmMtx(kneeIdx);
	MtxPtr footMtx = model->getAnmMtx(footIdx);

	kneePos.x = kneeMtx[0][3];
	kneePos.y = kneeMtx[1][3];
	kneePos.z = kneeMtx[2][3];
	footPos.x = footMtx[0][3];
	footPos.y = footMtx[1][3];
	footPos.z = footMtx[2][3];

	JGeometry::TVec3<f32> diff;
	diff = footPos;
	diff.x -= kneePos.x;
	diff.y -= kneePos.y;
	diff.z -= kneePos.z;
	f32 shin = JGeometry::TUtil<f32>::sqrt(diff.squared());

	const TBGCheckData* groundData;
	f32 groundY = gpMap->checkGround(footPos.x, footPos.y + shin, footPos.z,
	                                 &groundData);
	if (groundY + threshold < footPos.y)
		return;
	footPos.y = groundY + threshold;

	MtxPtr hipMtx = model->getAnmMtx(hipIdx);
	JGeometry::TVec3<f32> hipPos;
	hipPos.x = hipMtx[0][3];
	hipPos.y = hipMtx[1][3];
	hipPos.z = hipMtx[2][3];

	JGeometry::TVec3<f32> kneeFromHip;
	kneeFromHip = kneePos;
	kneeFromHip.x -= hipPos.x;
	kneeFromHip.y -= hipPos.y;
	kneeFromHip.z -= hipPos.z;
	f32 thigh = JGeometry::TUtil<f32>::sqrt(kneeFromHip.squared());

	JGeometry::TVec3<f32> footFromHip;
	footFromHip = footPos;
	footFromHip.x -= hipPos.x;
	footFromHip.y -= hipPos.y;
	footFromHip.z -= hipPos.z;
	f32 hipFoot = JGeometry::TUtil<f32>::sqrt(footFromHip.squared());

	JGeometry::TVec3<f32> crossV;
	crossV.x = footFromHip.y * kneeFromHip.z - footFromHip.z * kneeFromHip.y;
	crossV.y = footFromHip.z * kneeFromHip.x - footFromHip.x * kneeFromHip.z;
	crossV.z = footFromHip.x * kneeFromHip.y - footFromHip.y * kneeFromHip.x;
	f32 dotV = footFromHip.x * kneeFromHip.x + footFromHip.y * kneeFromHip.y
	         + footFromHip.z * kneeFromHip.z;

	f32 lcAngle
	    = matan(MsVECMag2((Vec*)&crossV), dotV) * (360.0f / 65536.0f);
	if (lcAngle < 0.0f)
		lcAngle = -lcAngle;

	f32 cosLaw = (thigh * thigh + shin * shin - hipFoot * hipFoot)
	           / (2.0f * thigh * shin);
	f32 ankleAngle;
	if (cosLaw == 1.0f) {
		ankleAngle = 0.0f;
	} else if (cosLaw == -1.0f) {
		ankleAngle = 180.0f;
	} else {
		f32 sinLaw = JGeometry::TUtil<f32>::sqrt(1.0f - cosLaw * cosLaw);
		ankleAngle
		    = 90.0f - matan(sinLaw, cosLaw) * (360.0f / 65536.0f);
	}

	f32 sinAng = shin * JMASSin((s16)(ankleAngle * (65536.0f / 360.0f)))
	           / hipFoot;
	f32 sinDeg;
	if (sinAng == 1.0f) {
		sinDeg = 90.0f;
	} else if (sinAng == -1.0f) {
		sinDeg = -90.0f;
	} else {
		f32 cosAng = JGeometry::TUtil<f32>::sqrt(1.0f - sinAng * sinAng);
		sinDeg = matan(cosAng, sinAng) * (360.0f / 65536.0f);
	}
	f32 finalAngle = sinDeg - lcAngle;

	s16 rotS = (s16)(-finalAngle * (65536.0f / 360.0f));
	f32 sinR = JMASSin(rotS);
	f32 cosR = JMASCos(rotS);

	Mtx zRot;
	zRot[0][0] = cosR;
	zRot[0][1] = -sinR;
	zRot[0][2] = 0.0f;
	zRot[0][3] = 0.0f;
	zRot[1][0] = sinR;
	zRot[1][1] = cosR;
	zRot[1][2] = 0.0f;
	zRot[1][3] = 0.0f;
	zRot[2][0] = 0.0f;
	zRot[2][1] = 0.0f;
	zRot[2][2] = 1.0f;
	zRot[2][3] = 0.0f;
	PSMTXConcat(hipMtx, zRot, hipMtx);

	// new knee direction = hip x-axis, normalized, scaled by thigh
	JGeometry::TVec3<f32> hipXAxis;
	hipXAxis.x = hipMtx[0][0];
	hipXAxis.y = hipMtx[1][0];
	hipXAxis.z = hipMtx[2][0];
	if (hipXAxis.squared() <= 3.81469727e-06f) {
		hipXAxis.zero();
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(hipXAxis.squared());
		hipXAxis.scale(inv);
	}
	hipXAxis.scale(thigh);

	JGeometry::TVec3<f32> newKneePos;
	newKneePos.x = hipPos.x;
	newKneePos.y = hipPos.y;
	newKneePos.z = hipPos.z;
	newKneePos.add(hipXAxis);

	kneeMtx[0][3] = newKneePos.x;
	kneeMtx[1][3] = newKneePos.y;
	kneeMtx[2][3] = newKneePos.z;

	// New knee X-axis = footPos - newKneePos, normalized, scaled by len-of-y
	JGeometry::TVec3<f32> kneeXNew;
	kneeXNew.x = footPos.x;
	kneeXNew.y = footPos.y;
	kneeXNew.z = footPos.z;

	f32 yAxisLenSq = kneeMtx[0][1] * kneeMtx[0][1]
	               + kneeMtx[1][1] * kneeMtx[1][1]
	               + kneeMtx[2][1] * kneeMtx[2][1];
	f32 yAxisLen
	    = (yAxisLenSq > 0.0f) ? JGeometry::TUtil<f32>::sqrt(yAxisLenSq) : 0.0f;

	f32 xAxisLenSq = kneeMtx[0][0] * kneeMtx[0][0]
	               + kneeMtx[1][0] * kneeMtx[1][0]
	               + kneeMtx[2][0] * kneeMtx[2][0];
	f32 xAxisLen
	    = (xAxisLenSq > 0.0f) ? JGeometry::TUtil<f32>::sqrt(xAxisLenSq) : 0.0f;

	kneeXNew.x -= newKneePos.x;
	kneeXNew.y -= newKneePos.y;
	kneeXNew.z -= newKneePos.z;

	if (kneeXNew.squared() <= 3.81469727e-06f) {
		kneeXNew.zero();
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(kneeXNew.squared());
		kneeXNew.scale(inv);
	}
	kneeXNew.scale(xAxisLen);

	kneeMtx[0][0] = kneeXNew.x;
	kneeMtx[1][0] = kneeXNew.y;
	kneeMtx[2][0] = kneeXNew.z;

	// new knee Y-axis = cross(zAxis, xAxis) scaled by yAxisLen
	f32 fx = kneeXNew.x;
	f32 fy = kneeMtx[1][0];
	f32 fz = kneeMtx[2][0];
	f32 zx = kneeMtx[0][2];
	f32 zy = kneeMtx[1][2];
	f32 zz = kneeMtx[2][2];

	f32 nyX = fz * zy - fy * zz;
	f32 nyY = fx * zz - fz * zx;
	f32 nyZ = fy * zx - fx * zy;
	f32 yMag2 = nyX * nyX + nyY * nyY + nyZ * nyZ;
	if (yMag2 <= 3.81469727e-06f) {
		nyX = nyY = nyZ = 0.0f;
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(yMag2);
		nyX *= inv;
		nyY *= inv;
		nyZ *= inv;
	}
	kneeMtx[0][1] = nyX * yAxisLen;
	kneeMtx[1][1] = nyY * yAxisLen;
	kneeMtx[2][1] = nyZ * yAxisLen;

	// foot col[3] = footPos
	footMtx[0][3] = footPos.x;
	footMtx[1][3] = footPos.y;
	footMtx[2][3] = footPos.z;

	// foot col[0] = ground normal data (negated, scaled)
	JGeometry::TVec3<f32> negNorm;
	negNorm.x = groundData->mNormal.x;
	negNorm.y = groundData->mNormal.y;
	negNorm.z = groundData->mNormal.z;

	f32 footYLenSq = footMtx[0][1] * footMtx[0][1]
	               + footMtx[1][1] * footMtx[1][1]
	               + footMtx[2][1] * footMtx[2][1];
	f32 footYLen = (footYLenSq > 0.0f)
	                   ? JGeometry::TUtil<f32>::sqrt(footYLenSq)
	                   : 0.0f;

	f32 footXLenSq = footMtx[0][0] * footMtx[0][0]
	               + footMtx[1][0] * footMtx[1][0]
	               + footMtx[2][0] * footMtx[2][0];
	f32 footXLen = (footXLenSq > 0.0f)
	                   ? JGeometry::TUtil<f32>::sqrt(footXLenSq)
	                   : 0.0f;

	negNorm.x = -negNorm.x;
	negNorm.y = -negNorm.y;
	negNorm.z = -negNorm.z;
	negNorm.x *= footYLen;
	negNorm.y *= footYLen;
	negNorm.z *= footYLen;

	footMtx[0][1] = negNorm.x;
	footMtx[1][1] = negNorm.y;
	footMtx[2][1] = negNorm.z;

	// foot X-axis = cross(Y, Z) normalized * xLen
	f32 ffx = negNorm.x;
	f32 ffy = footMtx[1][1];
	f32 ffz = footMtx[2][1];
	f32 fzx = footMtx[0][2];
	f32 fzy = footMtx[1][2];
	f32 fzz = footMtx[2][2];

	f32 nxX = ffz * fzy - ffy * fzz;
	f32 nxY = ffx * fzz - ffz * fzx;
	f32 nxZ = ffy * fzx - ffx * fzy;
	f32 xMag2 = nxX * nxX + nxY * nxY + nxZ * nxZ;
	if (xMag2 <= 3.81469727e-06f) {
		nxX = nxY = nxZ = 0.0f;
	} else {
		f32 inv = JGeometry::TUtil<f32>::inv_sqrt(xMag2);
		nxX *= inv;
		nxY *= inv;
		nxZ *= inv;
	}
	footMtx[0][0] = nxX * footXLen;
	footMtx[1][0] = nxY * footXLen;
	footMtx[2][0] = nxZ * footXLen;
}

TMtxCalcFootInv::TMtxCalcFootInv(u16 a, u16 b, u16 c, u16 d, u16 e, u16 f,
                                 f32 g)
    : J3DMtxCalcSoftimageAnm(nullptr)
{
	unk68 = a;
	unk6A = b;
	unk6C = c;
	unk6E = d;
	unk70 = e;
	unk72 = f;
	unk74 = g;
}

void TMtxCalcFootInv::calc(u16 idx)
{
	J3DMtxCalcAnm::calc(idx);
	if (unk6C == idx) {
		FeetInvCalc(j3dSys.getModel(), unk68, unk6A, unk6C, unk74);
	}
	if (unk72 == idx) {
		FeetInvCalc(j3dSys.getModel(), unk6E, unk70, unk72, unk74);
	}
}
