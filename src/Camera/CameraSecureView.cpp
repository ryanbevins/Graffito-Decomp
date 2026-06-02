#include <Camera/Camera.hpp>
#include <Camera/cameralib.hpp>
#include <JSystem/JMath.hpp>
#include <Player/MarioAccess.hpp>

extern void* gpMarioOriginal;

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);

void CPolarSubCamera::execSecureView_(s16 angle, Vec* out)
{
	s16 marioBack = (s16)(*gpMarioAngleY - 0x8000);

	f32 nearVal = CLBLinearInbetween<f32>(
	    *(f32*)((u8*)(*(void**)((u8*)this + 0x68)) + 0x3C),
	    *(f32*)((u8*)(*(void**)((u8*)this + 0x68)) + 0x44),
	    this->unkA8);

	u16 delta    = (u16)(angle - marioBack);
	f32 cosDelta = jmaCosTable[delta >> jmaSinShift];

	f32 farVal = (cosDelta >= 0.0f)
	                 ? 0.0f
	                 : CLBLinearInbetween<f32>(
	                       *(f32*)((u8*)(*(void**)((u8*)this + 0x68)) + 0x40),
	                       *(f32*)((u8*)(*(void**)((u8*)this + 0x68)) + 0x48),
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

	s16 ad = *gpMarioAngleY - *(s16*)((u8*)gpMarioOriginal + 0x9C);
	if (ad < 0)
		ad = -ad;

	f32 deg = (f32)ad * (360.0f / 65536.0f);
	f32 factor;
	if (deg <= 1.0f)
		factor = 1.0f;
	else
		factor = 1.0f / deg;

	f32 clamped
	    = *(f32*)((u8*)(*(void**)((u8*)this + 0x68)) + 0x38) * factor;
	if (clamped > 1.0f)
		clamped = 1.0f;
	else if (clamped < 0.0f)
		clamped = 0.0f;

	CLBChaseDecrease((f32*)((u8*)this + 0x294), dx, clamped, 0.0f);
	CLBChaseDecrease((f32*)((u8*)this + 0x298), dz, clamped, 0.0f);

	out->x += *(f32*)((u8*)this + 0x294);
	out->z += *(f32*)((u8*)this + 0x298);
}

void CPolarSubCamera::calcSecureViewTarget_(s16 angle, f32* outX, f32* outZ)
{
	void* params  = *(void**)((u8*)this + 0x68);
	s16 marioBack = (s16)(*gpMarioAngleY - 0x8000);

	f32 nearVal = CLBLinearInbetween<f32>(*(f32*)((u8*)params + 0x3C),
	                                      *(f32*)((u8*)params + 0x44),
	                                      this->unkA8);

	u16 delta    = (u16)(angle - marioBack);
	f32 cosDelta = jmaCosTable[delta >> jmaSinShift];

	f32 farVal;
	if (cosDelta >= 0.0f) {
		farVal = 0.0f;
	} else {
		void* p2 = *(void**)((u8*)this + 0x68);
		farVal   = CLBLinearInbetween<f32>(*(f32*)((u8*)p2 + 0x40),
		                                   *(f32*)((u8*)p2 + 0x48),
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
