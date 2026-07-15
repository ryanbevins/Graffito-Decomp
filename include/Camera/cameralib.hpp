#ifndef CAMERA_CAMERALIB_HPP
#define CAMERA_CAMERALIB_HPP

#include <JSystem/JGeometry.hpp>
#include <MarioUtil/MathUtil.hpp>

extern f32 SMSGetAnmFrameRate(); // avoid including Application.hpp
extern JGeometry::TVec3<f32> CLBConstUpVec;

void CLBCalc2DFPos(JGeometry::TVec2<f32>*, const f32 (*)[4],
                   const f32 (*)[4], const Vec&, u32*, bool);
void CLBCalcNearClipAngle(JGeometry::TVec3<f32>*, S16Vec*,
                          const JGeometry::TVec3<f32>&,
                          const JGeometry::TVec3<f32>&, s16, f32);
void CLBCalcNearFourPos(JGeometry::TVec3<f32>*, JGeometry::TVec3<f32>*, S16Vec*,
                        const JGeometry::TVec3<f32>&,
                        const JGeometry::TVec3<f32>&, s16, f32,
                        const JGeometry::TVec2<f32>&);
void CLBCalcNearNinePos(JGeometry::TVec3<f32>*, S16Vec*,
                        const JGeometry::TVec3<f32>&,
                        const JGeometry::TVec3<f32>&, s16, f32,
                        const JGeometry::TVec2<f32>&);
void CLBCalcPointInCubeRatio(const Vec&, const Vec&, const Vec&, const Vec&,
                             f32*, f32*, f32*);
void CLBCalcRotateZXYTranslateMatrix(MtxPtr, const Vec&, const Vec&);
void CLBCalcScaleTranslateMatrix(MtxPtr, const Vec&, const Vec&);
BOOL CLBChaseAngleDecrease(s16*, s16, s16);
BOOL CLBChaseDecrease(f32*, f32, f32, f32);
BOOL CLBChaseSpecialDecrease(f32*, f32, f32, f32);
void CLBCrossToPolar(const Vec&, const Vec&, f32*, s16*, s16*);
inline void CLBCrossToPolar(const Vec& origin, const Vec& in, s16* outPitch,
                            s16* outYaw)
{
	f32 dx    = in.x - origin.x;
	f32 dz    = in.z - origin.z;
	*outPitch = matan(MsSqrtf(dx * dx + dz * dz), in.y - origin.y);
	*outYaw   = matan(dz, dx);
}
bool CLBIsPointInCube(const Vec&, const Vec&, const Vec&, const Vec&);
void CLBPolarToCross(const Vec&, Vec*, f32, s16, s16);
void CLBRevisionLookatByAngleX(s16, s16, const Vec&, Vec*);
void CLBRotatePosAndUp(s16, s16, const JGeometry::TVec3<f32>&,
                       const JGeometry::TVec3<f32>&,
                       const JGeometry::TVec3<f32>&, JGeometry::TVec3<f32>*,
                       JGeometry::TVec3<f32>*);
void CLBScreenFPosToSPos(JGeometry::TVec2<s16>*, const JGeometry::TVec2<f32>&);

template <class T> void CLBChaseConstantSpecifyFrame(T*, T, T);
template <class T>
BOOL CLBChaseGeneralConstantSpecifySpeed(T* param_1, T param_2, T param_3)
{
	T sVar1 = param_2 - *param_1;
	if (param_3 < 0)
		param_3 = -param_3;

	if (sVar1 > 0) {
		sVar1 -= param_3;
		if (sVar1 > 0)
			*param_1 = param_2 - sVar1;
		else
			*param_1 = param_2;
	} else {
		sVar1 += param_3;
		if (sVar1 < 0)
			*param_1 = param_2 - sVar1;
		else
			*param_1 = param_2;
	}

	if (*param_1 == param_2)
		return FALSE;

	return TRUE;
}
template <class T> T CLBEaseInInbetween(T, T, f32);

template <class T> T CLBTwoDegreeGeneralInbetween(T a, T b, f32 t, f32 ba)
{
	return (T)(ba * t * t + t * ((f32)(b - a) - ba) + (f32)a);
}

template <class T> T CLBEaseOutInbetween(T a, T b, f32 t)
{
	return CLBTwoDegreeGeneralInbetween<T>(a, b, t, a - b);
}

template <class T> T CLBLinearInbetween(T a, T b, f32 f)
{
	return (T)(a + f * (b - a));
}

template <class T> T CLBRoundf(f32 param_1)
{
	return (T)(param_1 + (param_1 > 0.0f ? 0.5f : -0.5f));
}

template <class T> T CLBSquared(T param_1) { return param_1 * param_1; }

template <class T> T CLBPalFrame(T param_1)
{
	f32 rate = SMSGetAnmFrameRate();
	return CLBRoundf<T>(param_1 * (1.0f / rate));
}

template <class T> T CLBPalIntSpeed(T param_1)
{
	f32 rate = SMSGetAnmFrameRate();
	return CLBRoundf<T>((f32)param_1 * rate);
}

template <class T> f32 CLBCalcRatio(T a, T b, T c)
{
	f32 result = 0.0f;
	if (a != b) {
		result = (f32)(c - a) * (1.0f / (f32)(b - a));
	}
	return result;
}

// fabricated
inline s16 CLBDegToShortAngle(f32 deg)
{
	return CLBRoundf<s16>(deg * (65536.0f / 360.0f));
}

#endif
