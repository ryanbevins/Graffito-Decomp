#include <NPC/NpcTrample.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcSave.hpp>
#include <JSystem/JMath.hpp>

f32 TNpcTrample::msAmpDecrease = 0.0f;

bool TNpcTrample::updateTrample(f32 dt, f32* out)
{
	bool result = false;
	if (unk6 == 0 && unk4 == 0) {
		*out = dt;
	} else if (unk6 > 0) {
		*out = dt;
		unk4 = 0;
		unk6 -= 1;
		if (unk6 == 0)
			result = true;
	} else if (unk4 > 0) {
		unk4 -= 1;
		if (unk4 == 0) {
			*out = dt;
			unk6 = TBaseNPC::mPtrSaveNormal->mSLTrampleToMadFrames.value;
		} else {
			unk0 -= msAmpDecrease;
			s16 angle = (s16)((TBaseNPC::mPtrSaveNormal->mSLTrampleShakeFrames.get() - unk4)
			                  * TBaseNPC::mPtrSaveNormal->mSLTrampleVelocity.value);
			f32 mod = -JMASSin(angle);
			mod *= unk0;
			mod += 1.0f;
			*out = dt * mod;
		}
	}
	return result;
}

void TNpcTrample::startTrample()
{
	unk0          = TBaseNPC::mPtrSaveNormal->mSLTrampleAmplitude.value;
	unk4          = TBaseNPC::mPtrSaveNormal->mSLTrampleShakeFrames.value;
	unk6          = 0;
	msAmpDecrease = unk0 * (1.0f / (f32)unk4);
}
