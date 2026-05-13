#include <Player/MarioMain.hpp>
#include <Player/MarioEffect.hpp>
#include <System/EmitterViewObj.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>

bool TMario::askJumpIntoWaterEffectExist() const
{
	TMarioEffect* eff = (TMarioEffect*)mMarioEffect;
	if (eff->unk6C[0] == 1)
		return true;
	if (eff->unk6C[1] == 1)
		return true;
	return false;
}

void TMario::sinkInSandEffect()
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x53, &mPosition, 0, nullptr);
	if (emitter) {
		emitter->unk154.x = 0.0f;
		emitter->unk154.y = 0.0f;
		emitter->unk154.z = 0.0f;
		emitter->unk174.x = 0.0f;
		emitter->unk174.y = 0.0f;
		emitter->unk174.z = 0.0f;
	}
}
