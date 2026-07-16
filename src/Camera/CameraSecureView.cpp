#include <Camera/Camera.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Camera/cameralib.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/JMath.hpp>

extern void* gpMarioOriginal;

#define ABS(x) ((x) >= 0 ? (x) : -(x))

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);

void CPolarSubCamera::calcSecureViewTarget_(s16 angle, f32* outX, f32* outZ)
{
	s16 base = *gpMarioAngleY - 0x8000;
	s16 diff = angle - base;

	f32 first = CLBLinearInbetween<f32>(mCurrentParams->mSecureViewDistXMin,
	                                    mCurrentParams->mSecureViewDistXMax,
	                                    mCurrentTarget.unk28);
	f32 cos_d = JMASCos(diff);

	f32 second;
	if (cos_d >= 0.0f) {
		second = 0.0f;
	} else {
		second = CLBLinearInbetween<f32>(mCurrentParams->mSecureViewDistZMin,
		                                 mCurrentParams->mSecureViewDistZMax,
		                                 mCurrentTarget.unk28);
	}

	f32 mag = -ABS(first * JMASSin(diff) + second * cos_d);

	*outX = mag * JMASSin(base);
	*outZ = mag * JMASCos(base);
}

void CPolarSubCamera::execSecureView_(s16 angle, Vec* out)
{
	f32 px;
	f32 pz;
	calcSecureViewTarget_(angle, &px, &pz);

	s16 prevAngle = *(s16*)((u8*)gpMarioOriginal + 0x9C);
	s16 curAngle  = *gpMarioAngleY;
	s32 ad;
	if (curAngle - prevAngle >= 0)
		ad = curAngle - prevAngle;
	else
		ad = -(curAngle - prevAngle);

	f32 ratio = SHORTANGLE2DEG((s16)ad);
	f32 inv;
	if (ratio <= 1.0f)
		inv = 1.0f;
	else
		inv = 1.0f / ratio;

	f32 v = MsClamp(mCurrentParams->mSecureViewChase * inv, 0.0f, 1.0f);

	CLBChaseDecrease(&unk294, px, v, 0.0f);
	CLBChaseDecrease(&unk298, pz, v, 0.0f);

	out->x += unk294;
	out->z += unk298;
}
