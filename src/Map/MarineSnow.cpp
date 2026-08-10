#define JDRAMA_TFLAG_CTOR_DECL_ONLY
#include <Map/MarineSnow.hpp>
#undef JDRAMA_TFLAG_CTOR_DECL_ONLY
#include <Camera/Camera.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/Particles.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>

TMarineSnow::TMarineSnow(const char* name)
    : JDrama::TActor(name)
{
}

void TMarineSnow::loadAfter()
{
	SMS_LoadParticle("/scene/others/marinesnow/ms_mare_marinsnow.jpa", 0x14B);
}

void TMarineSnow::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (param_1 & 2) {
		mPosition.set(gpCamera->unk124);
		gpMarioParticleManager->emitAndBindToPosPtr(0x14B, &mPosition, 1, this);
	}
}
