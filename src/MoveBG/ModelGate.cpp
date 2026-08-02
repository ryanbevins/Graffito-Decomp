#include <MoveBG/ModelGate.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorUtil.hpp>
#include <M3DUtil/SampleCtrlModel.hpp>
#include <M3DUtil/SampleCtrlNode.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <Camera/Camera.hpp>
#include <Player/MarioAccess.hpp>
#include <JSystem/JGeometry/JGUtil.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGadget/std-list.hpp>
#include <Strategic/LiveActor.hpp>
#include <Strategic/Strategy.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/Particles.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTexture.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <THPPlayer/THPPlayer.h>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <dolphin/mtx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

#include <M3DUtil/InfectiousStrings.hpp>

static const char* gateMActorNames[] = {
	"05_gate01",       "05_gate02rico", "05_gate03manma",
	"05_gate04monte",  "05_gate05mare",
};

MtxPtr TModelGate::getTakingMtx() { return nullptr; }

void TModelGate::startOpen()
{
	unk70 |= 1;
	unkC4 = 0;
	unk78->setBpk(gateMActorNames[unk71]);
	unk64 &= ~1;
	unk70 |= 2;
}

BOOL TModelGate::receiveMessage(THitActor* sender, u32 message)
{
	if (sender->mActorType == 0x80000001u && message == 0xE) {
		unkC8 = 0;
		unkC4 = 2;
		return TRUE;
	}

	if (sender->mActorType == 0x01000001u) {
		JGeometry::TVec3<f32> localPos;
		PSMTXMultVec(unk7C, &sender->mPosition, &localPos);

		Mtx tmp;
		PSMTXCopy(unk78->getModel()->getAnmMtx(unk72), tmp);

		if ((localPos.x * localPos.x + localPos.y * localPos.y) < 40000.0f
		    && -100.0f < localPos.z && localPos.z < unkFC) {
			if (unk70 & 2) {
				unkD0 += unkD4;
				if (unkD0 > 1.0f) {
					unkCA = unkC8;
					unkD0 = 1.0f;
					unk70 &= ~2;
				}
			}

			f32 randf = (f32)(rand() * 0.000030517578f);
			if (randf < unkF8) {
				gpMarioParticleManager->emitWithRotate(
				    0x1DD, &sender->mPosition, 0, unk74, 0, 2,
				    nullptr);
				gpMarioParticleManager->emitWithRotate(
				    0x1DE, &sender->mPosition, 0, unk74, 0, 2,
				    nullptr);
			}
			return TRUE;
		}
	}
	return FALSE;
}

void TModelGate::screenBlur(JDrama::TGraphics* graphics)
{
	JGeometry::TVec3<f32> diff;
	diff.x = gpMarioPos->x - mPosition.x;
	diff.y = 0.0f;
	diff.z = gpMarioPos->z - mPosition.z;
	PSVECNormalize(&diff, &diff);

	JGeometry::TVec3<f32> rotated;
	PSMTXMultVecSR(graphics->getUnkB4(), &diff, &rotated);

	JGeometry::TVec3<f32> marioPos = *gpMarioPos;
	JGeometry::TVec3<f32> localPos;
	PSMTXMultVec(unk7C, &marioPos, &localPos);

	f32 alpha = 0.0f;
	if (-250.0f <= localPos.x && localPos.x <= 250.0f && -250.0f <= localPos.y
	    && localPos.y <= 250.0f && 0.0f <= localPos.z && localPos.z <= unkF4) {
		if (localPos.z < unkF0) {
			alpha = 1.0f;
		}
		if (unkF0 <= localPos.z && localPos.z <= unkF4) {
			alpha = 1.0f - (localPos.z - unkF0) / (unkF4 - unkF0);
		}
		if (unkF4 < localPos.z) {
			alpha = 0.0f;
		}
	}

	f32 mult       = (f32)unkE0 * alpha;
	s16 relAngle   = (s16)(182.04445f * mRotation.y - (f32)gpCamera->unk258);
	if (relAngle < -0x2AAA || relAngle > 0x2AAA) {
		mult = 0.0f;
	}
	unkE4 = unkE8 * (mult - unkE4) + unkE4;

	f32 blurFactor = unkE4 * (1.0f - gpCamera->unk270);
	f32 blurTarget = unkEC;
	JGeometry::TVec3<f32> tmp(rotated.x, rotated.y, rotated.z);
	TAfterEffect* effect = gpAfterEffect;
	effect->unk15        = 2;
	effect->unk1C        = (u8)(s32)blurFactor;
	effect->unk50        = blurTarget;
	effect->unk5C        = tmp;
}

void TModelGate::perform(u32 perf_flags, JDrama::TGraphics* graphics)
{
	if (!(unk70 & 1))
		return;

	if (perf_flags & 8) {
		THPTextureSet* textureSet = ActivePlayer.dispTextureSet;
		if (ActivePlayer.open && textureSet != nullptr) {
			ResTIMG* textures
			    = unk78->getModel()->getModelData()->getTexture()->mResources;
			textures[0].imageDataOffset
			    = (u32)textureSet->ytexture - (u32)&textures[0];
			textures[1].imageDataOffset
			    = (u32)textureSet->utexture - (u32)&textures[1];
			textures[2].imageDataOffset
			    = (u32)textureSet->vtexture - (u32)&textures[2];
		}
	}

	unk78->perform(perf_flags, graphics);

	if (perf_flags & 1) {
		if (!(unk70 & 2)) {
			JGeometry::TVec3<f32> rel  = *gpMarioPos;
			rel.x                      -= mPosition.x;
			rel.y                      -= mPosition.y;
			rel.z                      -= mPosition.z;
			JGeometry::TVec3<f32> sqv  = rel;
			f32 dist
			    = JGeometry::TUtil<f32>::sqrt(sqv.x * sqv.x + sqv.y * sqv.y + sqv.z * sqv.z);
			if (dist < 1000.0f) {
				unkD0 += 0.01f;
				if (unkD0 > 1.0f) {
					unkD0 = 1.0f;
					unkCA = unkC8;
				}
			} else {
				unkCA -= 1;
				if (unkCA <= 0)
					unkCA = 0;
				unkD0 = (f32)unkCA / 1000.0f;
			}
		}

		JGeometry::TVec3<f32> localMario;
		PSMTXMultVec(unk7C, gpMarioPos, &localMario);
		if (-unk104 < localMario.x && localMario.x < unk104
		    && unk108 < localMario.y && localMario.y < unk10C
		    && unk110 < localMario.z && localMario.z < unk114) {
			if (unkCA > 0) {
				bool jumping = false;
				if (SMS_IsMarioStatusTypeJumping())
					jumping = true;
				if (jumping == true) {
					TLiveActor* mario = SMS_GetMarioLiveActor();
					if (mario->receiveMessage(this, 4) == 1) {
						mHeldObject = (TTakeActor*)SMS_GetMarioLiveActor();
					}
				}
			} else {
				JGeometry::TVec3<f32> diff;
				diff.x = gpMarioPos->x - mPosition.x;
				diff.z = gpMarioPos->z - mPosition.z;
				f32 dist = diff.x * diff.x + diff.z * diff.z;
				if (dist > 0.0f)
					dist = JGeometry::TUtil<f32>::sqrt(dist);
				if (dist < unk100) {
					f32 nx                       = diff.x / dist;
					f32 nz                       = diff.z / dist;
					JGeometry::TVec3<f32> outPos = *gpMarioPos;
					outPos.x                     = outPos.x + 10.0f * nx;
					outPos.z                     = outPos.z + 10.0f * nz;
					SMS_MarioMoveRequest(outPos);
				}
			}
		}
	}

	if (perf_flags & 2) {
		switch (unkC4) {
		case 0: {
			J3DFrameCtrl* fc = unk78->getFrameCtrl(2);
			if (fc->checkState(3)) {
				unkC4 = 1;
			}
			break;
		}
		case 1: {
			if (unkCA > 0) {
				for (int i = 0; i < 4; ++i) {
					gpMarioParticleManager->emitAndBindToMtxPtr(
					    0x131 + i, unk78->getModel()->getAnmMtx(unk72), 1,
					    this);
				}
				unkCA -= 1;
				unkCC += 1;
				if (gpMSound->gateCheck(0x3076)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x3076, (const Vec*)&mPosition, 0, nullptr, 0, 4);
				}
				if (gpMSound->gateCheck(0x3077)) {
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x3077, (const Vec*)&mPosition, 0, nullptr, 0, 4);
				}
			} else {
				unkCA = 0;
				unkCC = 0;
				unkD0 -= unkD8;
				if (unkD0 < 0.0f)
					unkD0 = 0.0f;
			}
			break;
		}
		case 2:
		default:
			unkD0 -= unkDC;
			break;
		}

		if (unkD0 > 1.0f)
			unkD0 = 1.0f;
		if (unkD0 < 0.0f)
			unkD0 = 0.0f;

		J3DTevBlock* tevBlock
		    = unk78->getModel()->getModelData()->getMaterialNodePointer(0)
		          ->getTevBlock();
		if (unkB8 == 1) {
			unkBA = unkB9;
			--unkBC;
			if (unkBC == 0) {
				++unkB9;
				unkBC = unkBE;
				if (unkB9 >= 8)
					unkB9 = 0;
			}

			J3DTevStageInfo* infos = unkC0->unkC[0]->unk3C;
			infos[0].field_0x5    = 0;
			infos[2].field_0x5    = 0;
			infos[3].field_0x5    = 0;
			infos[3].field_0x6    = 0;
			infos[3].field_0x7    = 0;
			infos[3].field_0x8    = 1;
			infos[5].field_0x11   = 1;

			switch (unkB9) {
			case 1:
				infos[3].field_0x5 = 1;
				infos[3].field_0x8 = 0;
				break;
			case 2:
				infos[0].field_0x5 = 8;
				break;
			case 3:
				infos[3].field_0x5 = 8;
				break;
			case 4:
				infos[3].field_0x7 = 1;
				break;
			case 5:
				infos[5].field_0x11 = 0;
				break;
			case 6:
				infos[2].field_0x5 = 1;
				break;
			case 7:
				infos[3].field_0x5 = 0;
				infos[3].field_0x6 = 1;
				infos[3].field_0x8 = 0;
				break;
			}

			tevBlock->getTevStage(0)->setTevStageInfo(infos[0]);
			tevBlock->getTevStage(2)->setTevStageInfo(infos[2]);
			tevBlock->getTevStage(3)->setTevStageInfo(infos[3]);
			tevBlock->getTevStage(5)->setTevStageInfo(infos[5]);
		}

		s16 endFrame = unk78->getFrameCtrl(5)->getEnd();
		f32 frame    = unkD0 * (f32)endFrame;
		unk78->getFrameCtrl(5)->setFrame(frame);
	}

	if (perf_flags & 4) {
		screenBlur(graphics);
	}

	THitActor::perform(perf_flags, graphics);
}

void TModelGate::loadAfter()
{
	initHitActor(0x080000C0, 5, 0x80000000, 300.0f, 400.0f, 300.0f,
	             400.0f);
	unk64 |= 1;
	unk70 = 0;
	unk71 = 0;

	static const char* gateNames[] = {
		"Gate",         "GateToRicco", "GateToMamma",
		"GateToMonte",  "GateToMare",  nullptr,
	};

	for (u8 i = 0; i < 5; ++i) {
		if (strcmp(gateNames[i], mName) == 0) {
			unk71 = i;
			break;
		}
	}

	char buf[256];
	snprintf(buf, sizeof(buf), "/scene/map/map/gate/%s.bmd",
	         gateMActorNames[unk71]);
	unk78 = SMS_MakeMActor("/scene/map/map/gate", buf, 0, 0x11100000);
	unk72 = unk78->getModel()->getModelData()->getJointName()->getIndex("center");

	if (ActivePlayer.open) {
		THPVideoInfo info;
		THPPlayerGetVideoInfo(&info);

		ResTIMG* textures
		    = unk78->getModel()->getModelData()->getTexture()->mResources;
		textures[0].format = 1;
		textures[0].width  = info.xSize;
		textures[0].height = info.ySize;
		textures[1].format = 1;
		textures[1].width  = info.xSize >> 1;
		textures[1].height = info.ySize >> 1;
		textures[2].format = 1;
		textures[2].width  = info.xSize >> 1;
		textures[2].height = info.ySize >> 1;
	}

	unkB8 = 0;
	unkB9 = 0;
	unkBA = 0;
	unkBE = 0xF0;
	unkBC = unkBE;

	unk78->setBtk(gateMActorNames[unk71]);
	unk78->setBrk(gateMActorNames[unk71]);
	unk78->getFrameCtrl(5)->setRate(0.0f);

	SampleCtrlModelData* ctrl
	    = new SampleCtrlModelData(unk78->getModel()->getModelData());
	unkC0 = ctrl;

	unkC5  = 0x20;
	unkC6  = 0xFF;
	unkC4  = 1;
	unkC8  = 0x168;
	unkCA  = 0;
	unkCC  = 0;
	unkCE  = 0x3C;
	unkD0  = 0.0f;
	unkD4  = 0.1f;
	unkD8  = 0.02f;
	unkDC  = 0.025f;
	mScaling.x = 1.0f;
	mScaling.y = 1.0f;
	mScaling.z = 1.0f;
	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("マップグループ");
	group->getChildren().push_back(this);
	{
		Mtx tmp;
		SMS_GetActorMtx(*this, tmp);
		PSMTXCopy(tmp, unk78->getModel()->unk20);
		unk78->getModel()->calc();
		PSMTXTrans(tmp, 0.0f, 0.0f, 250.0f);
		PSMTXConcat(unk78->getModel()->getAnmMtx(unk72), tmp, tmp);
		unkAC.x = 0.0f;
		unkAC.y = 0.0f;
		unkAC.z = 0.0f;
		PSMTXMultVec(tmp, &unkAC, &unkAC);
		PSMTXInverse(unk78->getModel()->getAnmMtx(unk72), unk7C);
		unk74 = matan(unk78->getModel()->getAnmMtx(unk72)[2][2],
		              unk78->getModel()->getAnmMtx(unk72)[0][2]);
	}

	unkE0  = 0;
	unkE4  = 0.0f;
	unkE8  = 0.01f;
	unkEC  = 0.02f;
	unkF0  = 500.0f;
	unkF4  = 1000.0f;
	unkF8  = 0.7f;
	unkFC  = 0.0f;
	unk100 = 200.0f;
	unk104 = 150.0f;
	unk108 = -1000.0f;
	unk10C = 150.0f;
	unk110 = 0.0f;
	unk114 = 300.0f;

	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_body.jpa", 0x1A);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_head.jpa", 0x1B);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_cap.jpa", 0x1C);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_rhand.jpa", 0x1D);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_lhand.jpa", 0x1E);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_rleg.jpa", 0x1F);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_rfoot.jpa", 0x20);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_lleg.jpa", 0x21);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_lfoot.jpa", 0x22);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_watgun.jpa", 0x23);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_dust.jpa", 0x3C);
	SMS_LoadParticle("/scene/map/map/gate/ms_mariowp_senko.jpa", 0x51);
	SMS_LoadParticle("/scene/map/map/gate/ms_gatewind_a.jpa", 0x131);
	SMS_LoadParticle("/scene/map/map/gate/ms_gatewind_a2.jpa", 0x132);
	SMS_LoadParticle("/scene/map/map/gate/ms_gatewind_a3.jpa", 0x133);
	SMS_LoadParticle("/scene/map/map/gate/ms_gatewind_b.jpa", 0x134);
	SMS_LoadParticle("/scene/map/map/gate/ms_gatehit_a.jpa", 0x1DD);
	SMS_LoadParticle("/scene/map/map/gate/ms_gatehit_b.jpa", 0x1DE);

	bool opened;
	switch (unk71) {
	case 0:  opened = TFlagManager::smInstance->getBool(0x10385); break;
	case 1:  opened = TFlagManager::smInstance->getBool(0x10386); break;
	case 2:  opened = TFlagManager::smInstance->getBool(0x10387); break;
	case 3:  opened = TFlagManager::smInstance->getBool(0x10387); break;
	case 4:  opened = TFlagManager::smInstance->getBool(0x10387); break;
	default: opened = false; break;
	}

	if (opened == true) {
		unk70 |= 1;
		unk64 &= ~1;
	} else {
		unk70 &= ~1;
		unk70 |= 2;
		unk64 |= 1;
	}
}

// rogue includes for static init (JALList<*> instantiations)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
