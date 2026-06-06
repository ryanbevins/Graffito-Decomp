#include <Camera/Camera.hpp>
#include <Camera/CameraKindParam.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <Player/MarioAccess.hpp>

extern void* gpMarioOriginal;

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);

void CPolarSubCamera::execSecureView_(s16 angle, Vec* out)
{
	s16 marioBack = (s16)(*gpMarioAngleY - 0x8000);

	f32 nearVal = CLBLinearInbetween<f32>(
	    unk68->unk3C, unk68->unk44, this->unkA8);

	u16 delta    = (u16)(angle - marioBack);
	f32 cosDelta = jmaCosTable[delta >> jmaSinShift];

	f32 farVal = (cosDelta >= 0.0f)
	                 ? 0.0f
	                 : CLBLinearInbetween<f32>(unk68->unk40, unk68->unk48,
	                                           this->unkA8);

	f32 sinDelta = jmaSinTable[delta >> jmaSinShift];
	f32 sum      = nearVal * sinDelta + farVal * cosDelta;
	if (sum < 0.0f) {
		sum = -(nearVal * sinDelta + farVal * cosDelta);
	}
	sum = -sum;

	u16 marioBackU = (u16)marioBack;
	f32 dx         = sum * jmaSinTable[marioBackU >> jmaSinShift];
	f32 dz         = sum * jmaCosTable[marioBackU >> jmaSinShift];

	s16 prevAngle = *(s16*)((u8*)gpMarioOriginal + 0x9C);
	s16 curAngle  = *gpMarioAngleY;
	s32 ad;
	if (curAngle - prevAngle >= 0)
		ad = curAngle - prevAngle;
	else
		ad = -(curAngle - prevAngle);

	f32 deg = (f32)(s16)ad * (360.0f / 65536.0f);
	f32 factor;
	if (deg <= 1.0f)
		factor = 1.0f;
	else
		factor = 1.0f / deg;

	f32 clamped = unk68->unk38 * factor;
	if (clamped > 1.0f)
		clamped = 1.0f;
	else if (clamped < 0.0f)
		clamped = 0.0f;

	CLBChaseDecrease(&unk294, dx, clamped, 0.0f);
	CLBChaseDecrease(&unk298, dz, clamped, 0.0f);

	out->x += unk294;
	out->z += unk298;
}

void CPolarSubCamera::calcSecureViewTarget_(s16 angle, f32* outX, f32* outZ)
{
	s16 marioBack = (s16)(*gpMarioAngleY - 0x8000);

	f32 nearVal = CLBLinearInbetween<f32>(unk68->unk3C, unk68->unk44,
	                                      this->unkA8);

	u16 delta    = (u16)(angle - marioBack);
	f32 cosDelta = jmaCosTable[delta >> jmaSinShift];

	f32 farVal;
	if (cosDelta >= 0.0f) {
		farVal = 0.0f;
	} else {
		farVal = CLBLinearInbetween<f32>(unk68->unk40, unk68->unk48,
		                                 this->unkA8);
	}

	f32 sinDelta = jmaSinTable[delta >> jmaSinShift];
	f32 sum      = farVal * cosDelta + nearVal * sinDelta;
	if (sum < 0.0f)
		sum = -sum;
	sum = -sum;

	u16 marioBackU = (u16)marioBack;
	*outX          = sum * jmaSinTable[marioBackU >> jmaSinShift];
	*outZ          = sum * jmaCosTable[marioBackU >> jmaSinShift];
}
