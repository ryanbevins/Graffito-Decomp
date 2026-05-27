#include <NPC/NpcBase.hpp>
#include <NPC/NpcParts.hpp>

#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <M3DUtil/LodAnm.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/EffectUtil.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <NPC/NpcSave.hpp>
#include <Strategic/LiveActor.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>

// File-local helper used by isPolWaitC/L/REffectEmitTime_.
static BOOL IsCheckPassFrame(J3DFrameCtrl* fc, const f32* table)
{
	BOOL result = FALSE;
	for (s32 off = 0; *(const f32*)((const u8*)table + off) >= 0.0f;
	     off += sizeof(f32)) {
		if (fc->checkPass(*(const f32*)((const u8*)table + off))) {
			result = TRUE;
			break;
		}
	}
	return result;
}

void TBaseNPC::setHappyEffectMtxPtr_(const JUTNameTab* nameTab)
{
	const char* sKoshiNull = "koshi_null";
	const char* sKoshi     = "koshi";
	const char* sJntBody   = "jnt_body";
	const char* jntName    = (const char*)NULL;
	bool isMonte           = true;
	if (!isNormalMonteM() && !isNormalMonteW())
		isMonte = false;
	if (isMonte) {
		jntName = sKoshiNull;
	} else {
		bool isMare = true;
		if (!isNormalMareM() && !isNormalMareW())
			isMare = false;
		if (isMare) {
			jntName = sKoshi;
		} else if (mActorType == 0x04000016) {
			jntName = sJntBody;
		}
	}
	if (jntName) {
		s32 idx       = nameTab->getIndex(jntName);
		J3DModel* mdl = getModel();
		mPtrHappyEffectMtx
		    = (MtxPtr)((u8*)mdl->mNodeMatrices + (u16)idx * sizeof(Mtx));
	}
}

void TBaseNPC::setNoteEffectMtxPtr_(const JUTNameTab* nameTab)
{
	const char* sNoseJnt = "nose_jnt";
	const char* sKuchi   = "kuchi";
	const char* jntName  = (const char*)NULL;
	switch (mActorType) {
	case 0x04000009:
		jntName = sNoseJnt;
		break;
	case 0x04000012:
		jntName = sKuchi;
		break;
	}
	if (jntName) {
		s32 idx       = nameTab->getIndex(jntName);
		J3DModel* mdl = getModel();
		mPtrNoteEffectMtx
		    = (MtxPtr)((u8*)mdl->mNodeMatrices + (u16)idx * sizeof(Mtx));
	}
}

void TBaseNPC::setPollutionEffectMtxPtr_(const JUTNameTab* nameTab)
{
	const char* sKoshiNull = "koshi_null";
	const char* sJntBody   = "jnt_body";
	const char* sFootL     = "footL_jnt";
	const char* sFootR     = "footR_jnt";
	const char* sKoshi     = "koshi";
	const char* jntName    = (const char*)NULL;
	bool isMonte           = true;
	if (!isNormalMonteM() && !isNormalMonteW())
		isMonte = false;
	if (isMonte) {
		s32 idxL       = nameTab->getIndex(sFootL);
		J3DModel* mdlL = getModel();
		mPtrPollutionLEffectMtx
		    = (MtxPtr)((u8*)mdlL->mNodeMatrices + (u16)idxL * sizeof(Mtx));
		s32 idxR       = nameTab->getIndex(sFootR);
		J3DModel* mdlR = getModel();
		mPtrPollutionREffectMtx
		    = (MtxPtr)((u8*)mdlR->mNodeMatrices + (u16)idxR * sizeof(Mtx));
		jntName = sKoshiNull;
	} else {
		bool isMare = true;
		if (!isNormalMareM() && !isNormalMareW())
			isMare = false;
		if (isMare) {
			jntName = sKoshi;
		} else if (mActorType == 0x04000016) {
			jntName = sJntBody;
		}
	}
	if (jntName) {
		s32 idx       = nameTab->getIndex(jntName);
		J3DModel* mdl = getModel();
		mPtrPollutionEffectMtx
		    = (MtxPtr)((u8*)mdl->mNodeMatrices + (u16)idx * sizeof(Mtx));
	}
}

void TBaseNPC::setSmokeEffectMtxPtr_(bool isSmoke)
{
	J3DModel* mdl;
	const char* jntName;
	if (isSmoke) {
		mdl     = mNpcParts->getPartsMActor(11, 0)->getModel();
		jntName = "ef_null";
	} else {
		mdl     = getModel();
		jntName = "yashi_jnt";
	}
	u16 idx = mdl->mModelData->unkB0->getIndex(jntName);
	mPtrSmokeEffectMtx
	    = (MtxPtr)((u8*)mdl->mNodeMatrices + idx * sizeof(Mtx));
}

JGeometry::TVec3<f32> TBaseNPC::getEffectScale_() const
{
	JGeometry::TVec3<f32> result;
	if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
		result.x = 1.0f;
		result.y = 1.0f;
		result.z = 1.0f;
	} else {
		result = mEffectScaleBase;
	}
	return result;
}

void TBaseNPC::emitSinkEffect_()
{
	JGeometry::TVec3<f32> pos(mPosition.x, mSinkBaseY, mPosition.z);

	bool flag = true;
	if (mLiveFlag & 0x00800000)
		flag = false;

	JGeometry::TVec3<f32>* planeNormal
	    = (JGeometry::TVec3<f32>*)((u8*)mGroundPlane + 0x34);
	SMS_EmitSinkInPollutionEffect(pos, *planeNormal, flag);
}

void TBaseNPC::emitHappyEffect_()
{
	JGeometry::TVec3<f32> scale;
	if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
		scale.set(1.0f, 1.0f, 1.0f);
	} else {
		scale = mEffectScaleBase;
	}

	JGeometry::TVec3<f32> scale2 = scale;
	f32 mult                     = mPtrSaveNormal->mSLCleanEffectScale.value;
	scale2.x *= mult;
	scale2.y *= mult;
	scale2.z *= mult;

	bool isMonte = true;
	if (!isNormalMonteM() && !isNormalMonteW())
		isMonte = false;
	if (isMonte) {
		SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0x70,
		                     mPtrHappyEffectMtx, this, scale2);
	} else {
		bool isMare = true;
		if (!isNormalMareM() && !isNormalMareW())
			isMare = false;
		if (isMare || mActorType == 0x04000016) {
			SMS_EasyEmitParticle((E_SMS_EFFECT_ONETIME_NORMAL)0x71,
			                     mPtrHappyEffectMtx, this, scale2);
		}
	}
}

BOOL TBaseNPC::isPolWaitCEffectEmitTime_() const
{
	static const f32 sCheckFrameMonte[7]
	    = { 28.0f, 52.0f, 76.0f, 128.0f, 152.0f, 176.0f, -1.0f };
	static const f32 sCheckFrameMare[3] = { 126.0f, 156.0f, -1.0f };
	static const f32 sCheckFrameKino[3] = { 22.0f, 44.0f, -1.0f };

	BOOL result      = FALSE;
	const f32* table = (const f32*)NULL;

	bool isMonte = true;
	if (!isNormalMonteM() && !isNormalMonteW())
		isMonte = false;
	if (isMonte) {
		table = sCheckFrameMonte;
	} else {
		bool isMare = true;
		if (!isNormalMareM() && !isNormalMareW())
			isMare = false;
		if (isMare) {
			table = sCheckFrameMare;
		} else if (mActorType == 0x04000016) {
			table = sCheckFrameKino;
		}
	}

	if (table != NULL) {
		J3DFrameCtrl* fc = mMActor->getFrameCtrl(0);
		for (s32 i = 0; table[i] >= 0.0f; i++) {
			if (fc->checkPass(table[i])) {
				result = TRUE;
				break;
			}
		}
	}
	return result;
}

inline BOOL TBaseNPC::isPolWaitLEffectEmitTime_() const
{
	static const f32 sCheckFrameMonte[4] = { 28.0f, 52.0f, 76.0f, -1.0f };
	return IsCheckPassFrame(mMActor->getFrameCtrl(0), sCheckFrameMonte);
}

inline BOOL TBaseNPC::isPolWaitREffectEmitTime_() const
{
	static const f32 sCheckFrameMonte[4] = { 128.0f, 152.0f, 176.0f, -1.0f };
	return IsCheckPassFrame(mMActor->getFrameCtrl(0), sCheckFrameMonte);
}

void TBaseNPC::emitParticle_()
{
	// Block 1: smoke + fire chain
	if (mPtrSmokeEffectMtx && (mActionFlag & 0x4000)) {
		JGeometry::TVec3<f32> scale;
		if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
			scale.set(1.0f, 1.0f, 1.0f);
		} else {
			scale = mEffectScaleBase;
		}
		JGeometry::TVec3<f32> scaleSmoke = scale;
		mSmokeEffectPos.x                = mPtrSmokeEffectMtx[0][3];
		mSmokeEffectPos.y                = mPtrSmokeEffectMtx[1][3];
		mSmokeEffectPos.z                = mPtrSmokeEffectMtx[2][3];
		SMS_EasyEmitParticle((E_SMS_EFFECT_LOOP_NORMAL)0x170,
		                     &mSmokeEffectPos, this, scaleSmoke);

		JGeometry::TVec3<f32> scaleFire = scaleSmoke;
		scaleFire.x *= mFireScaleMul;
		scaleFire.y *= mFireScaleMul;
		scaleFire.z *= mFireScaleMul;
		SMS_EasyEmitParticle(PARTICLE_MS_MOE_FIRE_C, &mSmokeEffectPos, this,
		                     scaleFire);
		SMS_EasyEmitParticle(PARTICLE_MS_MOE_FIRE_A, &mSmokeEffectPos, this,
		                     scaleFire);
		SMS_EasyEmitParticle(PARTICLE_MS_MOE_FIRE_B, &mSmokeEffectPos, this,
		                     scaleFire);
	}

	// Block 2: note effect
	if (mPtrNoteEffectMtx) {
		if (mActorType == 0x04000012 && unkD0->mCurrentAnmKind != 5) {
			JGeometry::TVec3<f32> scale;
			if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
				scale.set(1.0f, 1.0f, 1.0f);
			} else {
				scale = mEffectScaleBase;
			}
			JGeometry::TVec3<f32> scaleNote = scale;
			scaleNote.x *= 0.75f;
			scaleNote.y *= 0.75f;
			scaleNote.z *= 0.75f;
			mNoteEffectPos.x = mPtrNoteEffectMtx[0][3];
			mNoteEffectPos.y = mPtrNoteEffectMtx[1][3];
			mNoteEffectPos.z = mPtrNoteEffectMtx[2][3];
			SMS_EasyEmitParticle((E_SMS_EFFECT_LOOP_NORMAL)0x18B,
			                     &mNoteEffectPos, this, scaleNote);
		}
	}

	// Block 3: water hamon
	bool doWater = false;
	if (mActorType == 0x04000007) {
		doWater = true;
	} else if (gpMarDirector->mMap == 4) {
		doWater = true;
	}
	if (doWater) {
		f32 waveY = 0.0f;
		JGeometry::TVec3<f32> scale;
		if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
			scale.set(1.0f, 1.0f, 1.0f);
		} else {
			scale = mEffectScaleBase;
		}
		JGeometry::TVec3<f32> scaleW = scale;

		bool emit = false;
		if (mActorType == 0x04000007) {
			scaleW.x *= 1.5f;
			scaleW.y *= 1.5f;
			scaleW.z *= 1.5f;
			emit = true;
		} else if (!(mPosition.y > 30.0f)) {
			if (mLinearVelocity.x != 0.0f || mLinearVelocity.z != 0.0f) {
				waveY = gpMapObjWave->getWaveHeight(mPosition.x, mPosition.z);
				if (!(mPosition.y > waveY))
					emit = true;
			}
		}
		if (emit) {
			mWaterEffectPos.x = mPosition.x;
			mWaterEffectPos.y = waveY;
			mWaterEffectPos.z = mPosition.z;
			SMS_EasyEmitParticle((E_SMS_EFFECT_LOOP_INDIRECT)0x1F7,
			                     &mWaterEffectPos, this, scaleW);
			SMS_EasyEmitParticle((E_SMS_EFFECT_LOOP_NORMAL)0x171,
			                     &mWaterEffectPos, this, scaleW);
		}
	}

	// Block 4: pollution emission gated on anm state
	s32 anmKind = unkD0->mCurrentAnmKind;
	if (anmKind == 0xF) {
		if (isPolWaitCEffectEmitTime_()) {
			s32 idx = -1;
			if (isNormalMonteM() || isNormalMonteW())
				idx = 0x72;
			else if (isNormalMareW())
				idx = 0x74;
			else if (mActorType == 0x04000016)
				idx = 0x75;
			if (idx != -1) {
				JPABaseEmitter* em
				    = gpMarioParticleManager->emitAndBindToMtxPtr(
				        idx, mPtrPollutionEffectMtx, 0, NULL);
				if (em) {
					JGeometry::TVec3<f32> sc = getEffectScale_();
					em->unk154.set(sc);
					em->unk174.set(sc);
					SMSSetEmitterPolColor(em, 6);
				}
			}
		}
		if (isNormalMonteM() || isNormalMonteW()) {
			if (isPolWaitLEffectEmitTime_()) {
				JPABaseEmitter* em
				    = gpMarioParticleManager->emitAndBindToMtxPtr(
				        0x73, mPtrPollutionLEffectMtx, 0, NULL);
				if (em) {
					JGeometry::TVec3<f32> sc = getEffectScale_();
					em->unk154.set(sc);
					em->unk174.set(sc);
					SMSSetEmitterPolColor(em, 6);
				}
			}
			if (isPolWaitREffectEmitTime_()) {
				JPABaseEmitter* em
				    = gpMarioParticleManager->emitAndBindToMtxPtr(
				        0x73, mPtrPollutionREffectMtx, 0, NULL);
				if (em) {
					JGeometry::TVec3<f32> sc = getEffectScale_();
					em->unk154.set(sc);
					em->unk174.set(sc);
					SMSSetEmitterPolColor(em, 6);
				}
			}
		}
	} else if (anmKind == 0x19) {
		s32 idx = -1;
		if (isNormalMonteM() || isNormalMonteW())
			idx = 0x172;
		else if (isNormalMareW() || mActorType == 0x04000016)
			idx = 0x173;
		if (idx != -1) {
			JPABaseEmitter* em = gpMarioParticleManager->emitAndBindToMtxPtr(
			    idx, mPtrPollutionEffectMtx, 1, this);
			if (em) {
				JGeometry::TVec3<f32> sc;
				if ((s32)mActorType < 0x04000018 && (s32)mActorType >= 0x04000016) {
					sc.set(1.0f, 1.0f, 1.0f);
				} else {
					sc = mEffectScaleBase;
				}
				em->unk154.x = sc.x;
				em->unk154.y = sc.y;
				em->unk154.z = sc.z;
				em->unk174.x = sc.x;
				em->unk174.y = sc.y;
				em->unk174.z = sc.z;
				SMSSetEmitterPolColor(em, 6);
			}
		}
	}
}
