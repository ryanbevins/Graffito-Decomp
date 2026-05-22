#include <Player/MarioMain.hpp>
#include <Player/MarioEffect.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
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

void TMario::sleepingEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x124, &mSleepPos, 1, this);
}

void TMario::sleepingEffectKill()
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emitAndBindToPosPtr(0x124, &mSleepPos, 1, this);
	if (emitter)
		emitter->deleteAllParticle();
}

void TMario::emitSandEffect() { emitFootPrintWithEffect(0x3b, 0x3a); }
void TMario::emitDirtyFootPrint() { emitFootPrintWithEffect(0x50, -1); }

void TMario::emitBlurSpinJump()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x105, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x106, getCenterAnmMtx(), 1, this);
	if (unk134 > 0.0f) {
		JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x126, getCenterAnmMtx(), 1, this);
		if (emitter)
			emitter->mChildSpawnRate = 1.75f * unk134 * (1.0f / 255.0f);
	}
}

void TMario::emitBlurHipDropSuper()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11A, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11B, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11C, getCenterAnmMtx(), 1, this);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x11D, getCenterAnmMtx(), 1, this);
}

void TMario::emitBlurHipDrop()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x104, getCenterAnmMtx(), 1, this);
}

void TMario::blurEffect()
{
	gpMarioParticleManager->emitAndBindToMtxPtr(0x10E, getCenterAnmMtx(), 1, this);
}

void TMario::warpInLight()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x51, &unk160[2], 0, this);
}

void TMario::elecEndEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x8B, &unk160[2], 0, this);
}

void TMario::elecEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x116, &unk160[2], 1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x118, &unk160[2], 1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x117, &unk160[2], 1, this);
}

void TMario::emitRotateShootEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x114, &unk160[2], 1, this);
	gpMarioParticleManager->emitAndBindToPosPtr(0x115, &unk160[2], 1, this);
}

void TMario::wallSlipEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0x102, &mPosition, 1, this);
}

void TMario::emitGetWaterEffect()
{
	JGeometry::TVec3<f32>* p = &unk160[0];
	gpMarioParticleManager->emitAndBindToPosPtr(0xF, p, 0, nullptr);
}

void TMario::emitGetEffect()
{
	gpMarioParticleManager->emitAndBindToPosPtr(0xE, &unk160[0], 0, nullptr);
	startSoundActor(0x1989);
}

void TMario::emitSweatSometimes()
{
	s16 yaw = mFaceAngle.y;
	if ((gpMarDirector->unk58 & 0xF) == 0)
		emitSweat(yaw);
}

void TMario::kickFruitEffect()
{
	JPABaseEmitter* emitter
	    = gpMarioParticleManager->emit(0x39, &mPosition, 0, nullptr);
	if (emitter) {
		emitter->unk154.x = 1.2f;
		emitter->unk154.y = 1.2f;
		emitter->unk154.z = 1.2f;
		emitter->unk174.x = 1.2f;
		emitter->unk174.y = 1.2f;
		emitter->unk174.z = 1.2f;
		JGeometry::TVec3<f32> pos = mPosition;
		pos.y += 30.0f;
		emitter->unk160.x = pos.x;
		emitter->unk160.y = pos.y;
		emitter->unk160.z = pos.z;
	}
}
