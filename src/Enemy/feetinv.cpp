#define JGEOMETRY_TVEC3_ADD_OUT_OF_LINE
#include <Enemy/FeetInv.hpp>
#undef JGEOMETRY_TVEC3_ADD_OUT_OF_LINE
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JGeometry.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <dolphin/mtx.h>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

static inline f32 sqrtPositive(f32 mag)
{
	if (mag > 0.0f) {
		f64 root = __frsqrte(mag);
		volatile f32 result
		    = 0.5f * root * (3.0f - mag * (root * root)) * mag;
		return result;
	}
	return mag;
}

static inline f32 sqrtEstimate(f32 mag)
{
	volatile f32 result = mag * __frsqrte(mag);
	return result;
}

void FeetInvCalc(J3DModel* model, u16 hipIdx, u16 kneeIdx, u16 footIdx,
                 f32 threshold)
{
	const TBGCheckData* groundData;
	JGeometry::TVec3<f32> kneePos;
	JGeometry::TVec3<f32> footPos;
	JGeometry::TVec3<f32> hipPos;
	JGeometry::TVec3<f32> diff;
	JGeometry::TVec3<f32> kneeFromHip;
	JGeometry::TVec3<f32> footFromHip;
	JGeometry::TVec3<f32> cross;
	JGeometry::TVec3<f32> newKneeXAxis;
	JGeometry::TVec3<f32> newKneeZAxis;
	Vec newFootYAxis;
	JGeometry::TVec3<f32> newFootXAxis;
	Mtx zRot;

	MtxPtr kneeMtx = model->getAnmMtx(kneeIdx);
	MtxPtr footMtx = model->getAnmMtx(footIdx);

	kneePos.x = kneeMtx[0][3];
	kneePos.y = kneeMtx[1][3];
	kneePos.z = kneeMtx[2][3];
	footPos.x = footMtx[0][3];
	footPos.y = footMtx[1][3];
	footPos.z = footMtx[2][3];

	diff = footPos;
	diff.x -= kneePos.x;
	diff.y -= kneePos.y;
	diff.z -= kneePos.z;
	f32 shin = JGeometry::TUtil<f32>::sqrt(diff.squared());

	f32 groundY = gpMap->checkGround(footPos.x, footPos.y + shin, footPos.z,
	                                 &groundData);
	if (groundY + threshold < footPos.y)
		return;
	footPos.y = groundY + threshold;

	MtxPtr hipMtx = model->getAnmMtx(hipIdx);
	hipPos.x = hipMtx[0][3];
	hipPos.y = hipMtx[1][3];
	hipPos.z = hipMtx[2][3];

	kneeFromHip = kneePos;
	kneeFromHip.x -= hipPos.x;
	kneeFromHip.y -= hipPos.y;
	kneeFromHip.z -= hipPos.z;
	f32 thigh = JGeometry::TUtil<f32>::sqrt(kneeFromHip.squared());

	footFromHip = footPos;
	footFromHip.x -= hipPos.x;
	footFromHip.y -= hipPos.y;
	footFromHip.z -= hipPos.z;
	f32 hipFoot = JGeometry::TUtil<f32>::sqrt(footFromHip.squared());

	cross.x = footFromHip.y * kneeFromHip.z - footFromHip.z * kneeFromHip.y;
	cross.y = footFromHip.z * kneeFromHip.x - footFromHip.x * kneeFromHip.z;
	cross.z = footFromHip.x * kneeFromHip.y - footFromHip.y * kneeFromHip.x;
	f32 dotV = footFromHip.x * kneeFromHip.x + footFromHip.y * kneeFromHip.y
	         + footFromHip.z * kneeFromHip.z;

	s16 lcAngleS = matan(dotV, MsVECMag2((Vec*)&cross));
	f32 lcAngle = __fabsf(lcAngleS * (360.0f / 65536.0f));
	f32 cosLaw = (thigh * thigh + shin * shin - hipFoot * hipFoot)
	           / (2.0f * thigh * shin);

	f32 ankleAngle;
	if (cosLaw == 1.0f) {
		ankleAngle = 0.0f;
	} else if (cosLaw == -1.0f) {
		ankleAngle = 180.0f;
	} else {
		f32 sinLaw = sqrtEstimate(1.0f - cosLaw * cosLaw);
		s16 ankleAngleS = matan(sinLaw, cosLaw);
		ankleAngle = 90.0f - ankleAngleS * (360.0f / 65536.0f);
	}

	f32 sinAng
	    = shin * JMASSin((s16)(ankleAngle * (65536.0f / 360.0f))) / hipFoot;
	f32 sinDeg;
	if (sinAng == 1.0f) {
		sinDeg = 90.0f;
	} else if (sinAng == -1.0f) {
		sinDeg = -90.0f;
	} else {
		f32 cosAng = sqrtEstimate(1.0f - sinAng * sinAng);
		s16 sinDegS = matan(cosAng, sinAng);
		sinDeg = sinDegS * (360.0f / 65536.0f);
	}
	f32 finalAngle = sinDeg - lcAngle;

	s16 rotS = (s16)(-finalAngle * (65536.0f / 360.0f));
	f32 sinR = JMASSin(rotS);
	f32 cosR = JMASCos(rotS);

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

	// new knee X direction = normalize(hip x-axis) * thigh; new knee pos = hip + that
	newKneeXAxis.x = hipMtx[0][0];
	newKneeXAxis.y = hipMtx[1][0];
	newKneeXAxis.z = hipMtx[2][0];
	newKneeXAxis.normalize();
	newKneeXAxis.scale(thigh);

	JGeometry::TVec3<f32> newKneePos;
	{
		JGeometry::TVec3<f32> newKneePosTemp;
		newKneePosTemp = hipPos;
		newKneePosTemp.add(newKneeXAxis);
		newKneePos = newKneePosTemp;
	}

	kneeMtx[0][3] = newKneePos.x;
	kneeMtx[1][3] = newKneePos.y;
	kneeMtx[2][3] = newKneePos.z;

	// new knee X-axis (col 0) = direction from new knee to foot, scaled to original X length
	JGeometry::TVec3<f32> kneeToFoot;
	kneeToFoot = footPos;

	f32 zAxisLenSq = kneeMtx[0][2] * kneeMtx[0][2]
	               + kneeMtx[1][2] * kneeMtx[1][2]
	               + kneeMtx[2][2] * kneeMtx[2][2];
	f32 zAxisLen = sqrtPositive(zAxisLenSq);

	f32 xAxisLenSq = kneeMtx[0][0] * kneeMtx[0][0]
	               + kneeMtx[1][0] * kneeMtx[1][0]
	               + kneeMtx[2][0] * kneeMtx[2][0];
	f32 xAxisLen = sqrtPositive(xAxisLenSq);

	kneeToFoot.x -= newKneePos.x;
	kneeToFoot.y -= newKneePos.y;
	kneeToFoot.z -= newKneePos.z;

	kneeToFoot.normalize();
	kneeToFoot.scale(xAxisLen);

	kneeMtx[0][0] = kneeToFoot.x;
	kneeMtx[1][0] = kneeToFoot.y;
	kneeMtx[2][0] = kneeToFoot.z;

	// new knee Z-axis = normalize(cross(xAxis, yAxis)) scaled by old Z length
	{
		f32 fx = kneeMtx[0][0];
		f32 fy = kneeMtx[1][0];
		f32 fz = kneeMtx[2][0];
		f32 yx = kneeMtx[0][1];
		f32 yy = kneeMtx[1][1];
		f32 yz = kneeMtx[2][1];

		newKneeZAxis.x = fy * yz - fz * yy;
		newKneeZAxis.y = fz * yx - fx * yz;
		newKneeZAxis.z = fx * yy - fy * yx;
		newKneeZAxis.normalize();
		newKneeZAxis.scale(zAxisLen);
	}
	kneeMtx[0][2] = newKneeZAxis.x;
	kneeMtx[1][2] = newKneeZAxis.y;
	kneeMtx[2][2] = newKneeZAxis.z;

	// foot col[3] = footPos
	footMtx[0][3] = footPos.x;
	footMtx[1][3] = footPos.y;
	footMtx[2][3] = footPos.z;

	// foot Y-axis = -groundNormal * footYLen
	newFootYAxis = groundData->mNormal;

	f32 footYLenSq = footMtx[0][1] * footMtx[0][1]
	               + footMtx[1][1] * footMtx[1][1]
	               + footMtx[2][1] * footMtx[2][1];
	f32 footYLen = sqrtPositive(footYLenSq);

	f32 footXLenSq = footMtx[0][0] * footMtx[0][0]
	               + footMtx[1][0] * footMtx[1][0]
	               + footMtx[2][0] * footMtx[2][0];
	f32 footXLen = sqrtPositive(footXLenSq);

	newFootYAxis.x = -newFootYAxis.x;
	newFootYAxis.y = -newFootYAxis.y;
	newFootYAxis.z = -newFootYAxis.z;
	newFootYAxis.x *= footYLen;
	newFootYAxis.y *= footYLen;
	newFootYAxis.z *= footYLen;

	footMtx[0][1] = newFootYAxis.x;
	footMtx[1][1] = newFootYAxis.y;
	footMtx[2][1] = newFootYAxis.z;

	// foot X-axis = normalize(cross(Y, Z)) * xLen
	{
		f32 fx = newFootYAxis.x;
		f32 fy = footMtx[1][1];
		f32 fz = footMtx[2][1];
		f32 zx = footMtx[0][2];
		f32 zy = footMtx[1][2];
		f32 zz = footMtx[2][2];

		newFootXAxis.x = fy * zz - fz * zy;
		newFootXAxis.y = fz * zx - fx * zz;
		newFootXAxis.z = fx * zy - fy * zx;
		newFootXAxis.normalize();
		newFootXAxis.scale(footXLen);
	}
	footMtx[0][0] = newFootXAxis.x;
	footMtx[1][0] = newFootXAxis.y;
	footMtx[2][0] = newFootXAxis.z;
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
