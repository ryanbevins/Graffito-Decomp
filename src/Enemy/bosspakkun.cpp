#include <Enemy/BossPakkun.hpp>
#include <Enemy/AreaCylinder.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Walker.hpp>
#include <Camera/CameraShake.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <MSound/MSModBgm.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#include <Map/MapCollisionManager.hpp>
#include <Map/MapData.hpp>
#include <Map/PollutionManager.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/ShadowUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MoveBG/ItemManager.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <Player/ModelWaterManager.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/ObjManager.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/TargetArrow.hpp>
#include <math.h>
#include <stdlib.h>

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

static inline JGeometry::TVec3<f32> polarXZ(s16 angle, f32 radius)
{
	return JGeometry::TVec3<f32>(radius * JMASSin(angle), 0.0f,
	                             radius * JMASCos(angle));
}

static inline f32 calcBossPakkunYaw(f32 x, f32 z)
{
	if (z == 0.0f) {
		if (x >= 0.0f)
			return 90.0f;
		return -90.0f;
	}

	if (z >= 0.0f)
		return matan(z, x) * (360.0f / 65536.0f);

	return 180.0f - matan(-z, x) * (360.0f / 65536.0f);
}

static const char* bosspakkun_bastable[] = {
	nullptr,
	nullptr,
	"/scene/bosspakkun/bas/bosspaku_ball_end.bas",
	"/scene/bosspakkun/bas/bosspaku_down.bas",
	"/scene/bosspakkun/bas/bosspaku_down_end.bas",
	nullptr,
	"/scene/bosspakkun/bas/bosspaku_down_loop.bas",
	nullptr,
	"/scene/bosspakkun/bas/bosspaku_fall_end.bas",
	nullptr,
	"/scene/bosspakkun/bas/bosspaku_fall_start.bas",
	"/scene/bosspakkun/bas/bosspaku_fly.bas",
	"/scene/bosspakkun/bas/bosspaku_fly_pollut.bas",
	"/scene/bosspakkun/bas/bosspaku_fly_start.bas",
	"/scene/bosspakkun/bas/bosspaku_getup.bas",
	"/scene/bosspakkun/bas/bosspaku_head.bas",
	"/scene/bosspakkun/bas/bosspaku_hovering.bas",
	"/scene/bosspakkun/bas/bosspaku_jump_reaction.bas",
	"/scene/bosspakkun/bas/bosspaku_land.bas",
	"/scene/bosspakkun/bas/bosspaku_panpan.bas",
	"/scene/bosspakkun/bas/bosspaku_pollut_end.bas",
	"/scene/bosspakkun/bas/bosspaku_pollut_start.bas",
	"/scene/bosspakkun/bas/bosspaku_return.bas",
	"/scene/bosspakkun/bas/bosspaku_sleep.bas",
	"/scene/bosspakkun/bas/bosspaku_tornado.bas",
	nullptr,
	"/scene/bosspakkun/bas/bosspaku_water_hit.bas",
	nullptr,
	nullptr,
	nullptr,
};

static const TModelDataLoadEntry sLightEntries[] = {
	{ "bosspaku_model.bmd", 0x10010000, 0 },
	{ "pollut_ball.bmd", 0x11040000, 0 },
	{ "pollut_ball_stamp.bmd", 0x10010000, 0 },
	{ nullptr, 0, 0 },
};

static const TModelDataLoadEntry sNormalEntries[] = {
	{ "bosspaku_model.bmd", 0x10010000, 0 },
	{ "bosspaku_end.bmd", 0x10100000, 0 },
	{ "pollut_ball.bmd", 0x11040000, 0 },
	{ "pollut_ball_stamp.bmd", 0x10010000, 0 },
	{ "bosspakuPollut.bmd", 0x11020000, 0 },
	{ "bosspakuPollut_white.bmd", 0x10010000, 0 },
	{ "trunade.bmd", 0x10020000, 0 },
	{ nullptr, 0, 0 },
};

DEFINE_NERVE(TNerveBPSleep, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	if (spine->getTime() == 0)
		boss->changeBck(0x17);

	return false;
}

DEFINE_NERVE(TNerveBPFall, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->offLiveFlag(LIVE_FLAG_UNK10);
		boss->onLiveFlag(LIVE_FLAG_AIRBORNE);
		boss->changeBck(0x0A);
	}

	if (actor->checkCurBckFromIndex(0x0A)) {
		if (actor->curAnmEndsNext(0, nullptr))
			boss->changeBck(0x09);
	} else if (actor->checkCurBckFromIndex(0x09)) {
		BOOL isAirborne;
		if (boss->mLiveFlag & LIVE_FLAG_AIRBORNE)
			isAirborne = TRUE;
		else
			isAirborne = FALSE;

		if (!isAirborne) {
			boss->changeBck(0x08);
			gpCameraShake->startShake((EnumCamShakeMode)0x0F, 1.0f);
			boss->rumblePad(2, boss->mPosition);
		}
	} else if (actor->checkCurBckFromIndex(0x08)) {
		if (actor->curAnmEndsNext(0, nullptr)) {
			boss->changeBck(0x0E);
			gpCameraShake->startShake((EnumCamShakeMode)0x10, 1.0f);
			boss->rumblePad(0, boss->mPosition);
		}
	} else if (actor->checkCurBckFromIndex(0x0E)
	           && actor->curAnmEndsNext(0, nullptr)) {
		bool isBossStage;
		if (gpMarDirector->mMap == 2 && gpMarDirector->unk7D == 4)
			isBossStage = true;
		else
			isBossStage = false;

		if (isBossStage) {
			f32 tornadoProp
			    = boss->getBossPakkunSaveParam()->mSLTornadoProp.value;
			if (boss->mTornado->unk98 != 0
			    || rand() * 0.000030517578f < tornadoProp) {
				spine->pushAfterCurrent(&TNerveBPTakeOff::theNerve());
				spine->pushAfterCurrent(&TNerveBPVomit::theNerve());
			} else if (boss->mTornado->unk98 == 0) {
				spine->pushAfterCurrent(&TNerveBPWait::theNerve());
				spine->pushAfterCurrent(&TNerveBPTornado::theNerve());
			} else {
				spine->pushAfterCurrent(&TNerveBPWait::theNerve());
			}
		} else {
			spine->pushAfterCurrent(&TNerveBPWait::theNerve());
		}
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPJumpReact, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x11);

	if (actor->curAnmEndsNext(0, nullptr))
		return true;
	return false;
}

DEFINE_NERVE(TNerveBPFlyCannon, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x0C);

	if (spine->getTime() == 0xA8)
		boss->launchPolDrop();

	if (actor->curAnmEndsNext(0, nullptr))
		return true;
	return false;
}

DEFINE_NERVE(TNerveBPFlyPivot, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0)
		boss->changeBck(0x0B);

	if (boss->turnToCurPathNode(
	        boss->getBossPakkunSaveParam()->mSLPivotSpeed.get())) {
		if (boss->unk114.size() != 0)
			boss->unkF4 = boss->unk114.pop();
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPStompReact, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->changeBck(0x05);
		boss->mHeadHit->onHitFlag(HIT_FLAG_NO_COLLISION);
	}

	if (spine->getTime() == 0x1E && (s8)boss->unk17C == 0) {
		boss->unk17C = 1;
		boss->unk174 = nullptr;
		boss->unk170 = nullptr;
		boss->unk1B8 = 0x32;

		if (boss->unk18C != nullptr) {
			JGeometry::TVec3<f32> pos;
			boss->getJointTransByIndex(0x12, &pos);
			pos.y += 250.0f;
			boss->unk18C->mPos.value = pos;
			gpModelWaterManager->emitRequest(*boss->unk18C);
		}
	}

	if (spine->getTime() == 0x32)
		boss->unk1BC = 1;

	if (actor->curAnmEndsNext(0, nullptr)) {
		boss->mHeadHit->offHitFlag(HIT_FLAG_NO_COLLISION);
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPSwing, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x0F);

	if (spine->getTime() == 0) {
		gpMarioParticleManager->emitAndBindToSRTMtxPtr(
		    0xAC, boss->getModel()->getAnmMtx(0x12), 0, boss);
	}

	if (actor->curAnmEndsNext(0, nullptr))
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveBPGetUp, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->changeBck(0x0E);
		gpCameraShake->startShake((EnumCamShakeMode)0x10, 1.0f);
		boss->rumblePad(0, boss->mPosition);
	}

	if (actor->curAnmEndsNext(0, nullptr)) {
		if (actor->checkCurBckFromIndex(0x0E)) {
			boss->changeBck(0x13);
		} else {
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPTumbleIn, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x03);

	if (spine->getTime() == 0x150) {
		gpMarioParticleManager->emitAndBindToMtxPtr(
		    0xAA, boss->getModel()->getAnmMtx(0x0E), 0, boss);
	}

	if (spine->getTime() == 0x15C) {
		gpCameraShake->startShake((EnumCamShakeMode)0x0E, 1.0f);
		boss->rumblePad(2, boss->mPosition);
	}

	if (actor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveBPTumble::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPTumble, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0) {
		boss->changeBck(0x06);
		boss->unk16C = 1;
	}

	gpMarioParticleManager->emitAndBindToMtxPtr(
	    0x161, boss->getModel()->getAnmMtx(0), 1, (const u8*)boss + 8);
	gpCameraShake->keepShake((EnumCamShakeMode)0x11, 1.0f);

	if ((spine->getTime() / 60) % 2 != 0)
		boss->rumblePad(0, boss->mPosition);

	if (spine->getTime()
	    >= boss->getBossPakkunSaveParam()->mSLTumbleTime.get()) {
		boss->unk16C = 0;
		spine->pushAfterCurrent(&TNerveBPTumbleOut::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPTumbleOut, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->changeBck(0x0E);
		gpCameraShake->startShake((EnumCamShakeMode)0x10, 1.0f);
		boss->rumblePad(0, boss->mPosition);
	}

	if (actor->curAnmEndsNext(0, nullptr)) {
		if (actor->checkCurBckFromIndex(0x0E)) {
			boss->changeBck(0x16);

			bool isBossStage;
			if (gpMarDirector->mMap == 2 && gpMarDirector->unk7D == 4)
				isBossStage = true;
			else
				isBossStage = false;

			if (!isBossStage) {
				++boss->unk1C4;
				if ((s8)boss->unk1C4 >= 3) {
					gpMarDirector->mConsole->startAppearBalloon(0xE0001,
					                                             true);
					boss->unk1C4 = 0;
				}
			}
		} else {
			spine->pushAfterCurrent(&TNerveBPWait::theNerve());
			return TRUE;
		}
	}

	if (actor->checkCurBckFromIndex(0x16)) {
		f32 frame = actor->getFrameCtrl(0)->getFrame();

		if (140.0f < frame && frame < 160.0f && (s8)boss->unk17C == 0) {
			boss->unk17C = 1;
			boss->unk174 = nullptr;
			boss->unk170 = nullptr;
			boss->unk1B8 = 0x32;

			if (boss->unk18C != nullptr) {
				JGeometry::TVec3<f32> pos;
				boss->getJointTransByIndex(0x12, &pos);
				pos.y += 250.0f;
				boss->unk18C->mPos.value = pos;
				gpModelWaterManager->emitRequest(*boss->unk18C);
			}
		}

		if (35.0f < frame)
			boss->unk1BC = 1;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPSwallow, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0)
		boss->changeBck(0x1A);

	if (boss->unk178
	    >= boss->getBossPakkunSaveParam()->mSLWaterMarkLimit.get()) {
		spine->pushAfterCurrent(&TNerveBPTumbleIn::theNerve());
		boss->unk16C = 0;
		boss->unk170 = nullptr;
		return TRUE;
	}

	MtxPtr mouthMtx = boss->getModel()->getAnmMtx(0x12);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x15D, mouthMtx, 1, boss);
	gpMarioParticleManager->emitAndBindToMtxPtr(0x15E, mouthMtx, 1,
	                                            (const u8*)boss + 1);

	if (boss->unk170 != nullptr) {
		boss->changeBck(0x1A);
		boss->unk170 = nullptr;
		return FALSE;
	}

	boss->unk16C = 0;
	spine->pushAfterCurrent(&TNerveBPWait::theNerve());
	return TRUE;
}

DEFINE_NERVE(TNerveBPPivot, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0)
		boss->changeBck(0x19);

	JGeometry::TVec3<f32> toMario = boss->mPosition;
	toMario -= *gpMarioPos;

	f32 swingLength = boss->getBossPakkunSaveParam()->mSLSwingLength.get();
	f32 pivotSpeed;
	if (toMario.squared() < swingLength * swingLength)
		pivotSpeed = boss->getBossPakkunSaveParam()->mSLPivotSpeedAware.get();
	else
		pivotSpeed = boss->getBossPakkunSaveParam()->mSLPivotSpeed.get();

	if (boss->turnToCurPathNode(pivotSpeed)) {
		if (boss->unk114.size() != 0)
			boss->unkF4 = boss->unk114.pop();
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPTornado, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->changeBck(0x18);
		gpMarioParticleManager->emitAndBindToSRTMtxPtr(
		    0xAB, boss->getModel()->getAnmMtx(3), 0, boss);
		gpMarioParticleManager->emitAndBindToPosPtr(0xA9, &boss->unk194, 0,
		                                            nullptr);
		gpMarioParticleManager->emitAndBindToPosPtr(0xA9, &boss->unk1A0, 0,
		                                            nullptr);
	}

	if (spine->getTime() == 0x96) {
		JGeometry::TVec3<f32>* marioPos = gpMarioPos;
		TBPTornado* tornado = boss->mTornado;
		tornado->unk98      = 1;
		tornado->unk70      = *marioPos;
		tornado->mPosition  = tornado->mOwner->mPosition;
		tornado->unk7C      = tornado->mOwner->mPosition;
		tornado->unk94
		    = tornado->mOwner->getBossPakkunSaveParam()->mSLTornadoMoveInit.value;
		tornado->offHitFlag(HIT_FLAG_NO_COLLISION);

		J3DFrameCtrl* ctrl = tornado->mActor->getFrameCtrl(5);
		ctrl->setFrame(0.0f);
		ctrl->setRate(0.0f);
	}

	if (actor->isCurAnmAlreadyEnd(0))
		return TRUE;
	return FALSE;
}

DEFINE_NERVE(TNerveBPCannon, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x15);

	if (actor->curAnmEndsNext(0, nullptr)) {
		if (actor->checkCurBckFromIndex(0x15)) {
			boss->changeBck(0x02);
			boss->launchPolDrop();
		} else {
			spine->pushAfterCurrent(&TNerveBPWait::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPWait, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	JGeometry::TVec3<f32> toMario = boss->mPosition;
	toMario -= *gpMarioPos;

	f32 swingLength = boss->getBossPakkunSaveParam()->mSLSwingLength.value;
	if (toMario.squared() < swingLength * swingLength) {
		JGeometry::TVec3<f32> yawDir;
		yawDir.x = -toMario.x;
		yawDir.y = -toMario.y;
		yawDir.z = -toMario.z;
		toMario  = yawDir;

		f32 targetYaw;
		if (toMario.z == 0.0f) {
			if (toMario.x >= 0.0f)
				targetYaw = 90.0f;
			else
				targetYaw = -90.0f;
		} else if (toMario.z >= 0.0f) {
			targetYaw = matan(toMario.z, toMario.x) * (360.0f / 65536.0f);
		} else {
			f32 yaw = matan(-toMario.z, toMario.x) * (360.0f / 65536.0f);
			targetYaw = 180.0f - yaw;
		}

		f32 wrappedYaw = callMsWrap(boss->mRotation.y, targetYaw - 180.0f,
		                            targetYaw + 180.0f);
		if (fabsf(targetYaw - wrappedYaw) < 60.0f) {
			spine->pushAfterCurrent(&TNerveBPWait::theNerve());
			spine->pushAfterCurrent(&TNerveBPSwing::theNerve());
			return TRUE;
		}

		spine->pushAfterCurrent(&TNerveBPWait::theNerve());
		spine->pushAfterCurrent(&TNerveBPVomit::theNerve());

		TPathNode marioPath(*gpMarioPos);
		boss->unk114.push(boss->unkF4);
		boss->unkF4 = marioPath;

		spine->pushAfterCurrent(&TNerveBPPivot::theNerve());
		return TRUE;
	}

	if (spine->getTime() == 0)
		boss->changeBck(0x19);

	if (spine->getTime()
	    >= boss->getBossPakkunSaveParam()->mSLWaitFrameStg0.value) {
		MActor* actor = boss->mMActor;
		if (actor->isCurAnmAlreadyEnd(0)) {
			if (gpMarDirector->mMap == 2) {
				if (gpMarDirector->unk7D == 0 || gpMarDirector->unk7D == 1) {
					JGeometry::TVec3<f32>* marioPos = gpMarioPos;
					if (boss->unk188 == nullptr) {
						boss->unk188 = (TAreaCylinderManager*)gpConductor->search(
						    "ゲロエリアマネージャー");
					}

					BOOL marioInArea;
					if (boss->unk188 == nullptr)
						marioInArea = FALSE;
					else
						marioInArea = boss->unk188->contain(*marioPos);

					if (marioInArea) {
						u16 bgType = (*gpMarioGroundPlane)->getBGType();
						bool isWaterSurface;
						if (bgType == 0x100 || (u16)(bgType - 0x101) <= 4
						    || bgType == 0x4104)
							isWaterSurface = true;
						else
							isWaterSurface = false;

						if (!isWaterSurface) {
							spine->pushAfterCurrent(&TNerveBPCannon::theNerve());
							return TRUE;
						}
					}

					spine->pushAfterCurrent(&TNerveBPWait::theNerve());
					return TRUE;
				}
			}

			if (gpMarDirector->unk7D == 4) {
				f32 tornadoProp
				    = boss->getBossPakkunSaveParam()->mSLTornadoProp.value;
				if (boss->mTornado->unk98 != 0
				    || rand() * 0.000030517578f < tornadoProp) {
					spine->pushAfterCurrent(&TNerveBPTakeOff::theNerve());
					spine->pushAfterCurrent(&TNerveBPVomit::theNerve());
					return TRUE;
				}

				if (boss->mTornado->unk98 == 0) {
					spine->pushAfterCurrent(&TNerveBPWait::theNerve());
					spine->pushAfterCurrent(&TNerveBPTornado::theNerve());
					return TRUE;
				}

				spine->pushAfterCurrent(&TNerveBPWait::theNerve());
				return TRUE;
			}

			spine->pushAfterCurrent(&TNerveBPWait::theNerve());
			spine->pushAfterCurrent(&TNerveBPVomit::theNerve());

			JGeometry::TVec3<f32> randomPos = boss->mPosition;
			f32 randX = rand() * 0.000030517578f;
			randX -= 0.5f;
			randomPos.x += 10000.0f * randX;
			f32 randZ = rand() * 0.000030517578f;
			randZ -= 0.5f;
			randomPos.z += 10000.0f * randZ;

			TPathNode randomPath(randomPos);
			boss->unk114.push(boss->unkF4);
			boss->unkF4 = randomPath;

			spine->pushAfterCurrent(&TNerveBPPivot::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPCannonL, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		actor->setBck("bosspaku_pollut_start");

	if (actor->curAnmEndsNext(0, nullptr)) {
		if (actor->checkCurAnm("bosspaku_pollut_start", 0)) {
			actor->setBck("bosspaku_ball_end");
			boss->launchPolDrop();
		} else {
			spine->pushAfterCurrent(&TNerveBPWaitL::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPVomit, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x15);

	if (actor->checkCurBckFromIndex(0x15)) {
		f32 frame = actor->getFrameCtrl(0)->getFrame();
		if (25.0f < frame && frame < 165.0f)
			boss->unk16C = 2;
		else
			boss->unk16C = 0;
	}

	if (actor->checkCurBckFromIndex(0x14)) {
		if (rand() * 0.000030517578f < 0.2f && spine->getTime() == 500) {
			f32 rotY = boss->mRotation.y;
			s16 angle = (s16)(rotY * (65536.0f / 360.0f));

			JGeometry::TVec3<f32> offset = polarXZ(angle, 700.0f);

			gpItemManager->makeObjAppear(boss->mPosition.x + offset.x,
			                             boss->mPosition.y + 1.0f,
			                             boss->mPosition.z + offset.z,
			                             0x20000002, false);
		}
	}

	if (actor->curAnmEndsNext(0, nullptr)) {
		if (actor->checkCurBckFromIndex(0x15)) {
			boss->unk16C = 0;
			boss->changeBck(0x14);

			TBPVomit* vomit = boss->mVomit;
			vomit->unk14->setBckFromIndex(0);
			vomit->unk18->setBckFromIndex(1);

			J3DModel* bossModel = vomit->mOwner->getModel();
			PSMTXCopy(bossModel->getBaseTRMtx(),
			          vomit->unk14->getModel()->getBaseTRMtx());
			vomit->unk14->getModel()->unk14 = vomit->mOwner->mScaling;

			PSMTXCopy(bossModel->getBaseTRMtx(),
			          vomit->unk18->getModel()->getBaseTRMtx());
			vomit->unk18->getModel()->unk14 = vomit->mOwner->mScaling;

			boss->rumblePad(1, boss->mPosition);
		} else {
			bool isBossStage;
			if (gpMarDirector->mMap == 2 && gpMarDirector->unk7D == 4)
				isBossStage = true;
			else
				isBossStage = false;

			if (!isBossStage) {
				if ((boss->unk1C0 & 1) == 0)
					gpMarDirector->mConsole->startAppearBalloon(0xE0000,
					                                             true);
				boss->unk1C0 |= 1;
			}

			return TRUE;
		}
	}

	if (actor->checkCurBckFromIndex(0x14)) {
		s16 angle = (s16)(boss->mRotation.y * (65536.0f / 360.0f));
		JGeometry::TVec3<f32> dir;
		dir.set(JMASSin(angle), 0.0f, JMASCos(angle));
		gpModelWaterManager->wind(dir);
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPWaitL, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		actor->setBck("bosspaku_wait");

	if (spine->getTime()
	    >= boss->getBossPakkunSaveParam()->mSLWaitFrameStg0.value) {
		JGeometry::TVec3<f32>* marioPos = gpMarioPos;
		if (boss->unk188 == nullptr) {
			boss->unk188 = (TAreaCylinderManager*)gpConductor->search(
			    "ゲロエリアマネージャー");
		}

		BOOL marioInArea;
		if (boss->unk188 == nullptr)
			marioInArea = FALSE;
		else
			marioInArea = boss->unk188->contain(*marioPos);

		if (marioInArea) {
			u16 bgType = (*gpMarioGroundPlane)->getBGType();
			bool isWaterSurface;
			if (bgType == 0x100 || (u16)(bgType - 0x101) <= 4
			    || bgType == 0x4104)
				isWaterSurface = true;
			else
				isWaterSurface = false;

			if (!isWaterSurface) {
				spine->pushAfterCurrent(&TNerveBPCannonL::theNerve());
				return TRUE;
			}
		}

		if (actor->curAnmEndsNext(0, nullptr)) {
			spine->pushAfterCurrent(&TNerveBPWaitL::theNerve());
			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPBreakSleep, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0) {
		boss->changeBck(0x0E);
		MSBgm::stopTrackBGMs(7, 10);
	}

	if (boss->mMActor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveBPTakeOff::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPTakeOff, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->onLiveFlag(LIVE_FLAG_UNK10);
		boss->onLiveFlag(LIVE_FLAG_AIRBORNE);
		boss->changeBck(0x0D);
	}

	if (actor->checkCurBckFromIndex(0x0D)
	    && actor->curAnmEndsNext(0, nullptr))
		boss->changeBck(0x0B);

	if (actor->checkCurBckFromIndex(0x0B)) {
		boss->mPosition.y += 5.0f;

		const TPathNode& path       = boss->unk104;
		THitActor* pathActor       = path.unk0;
		const JGeometry::TVec3<f32>* pointRef;
		if (pathActor != nullptr)
			pointRef = &pathActor->getPosition();
		else
			pointRef = &path.unk4;
		JGeometry::TVec3<f32> point = *pointRef;
		if (point.y < boss->mPosition.y) {
			boss->mPosition.y = point.y;

			if (boss->unk124->unk0 != nullptr)
				spine->pushAfterCurrent(&TNerveBPFly::theNerve());
			else
				spine->pushAfterCurrent(&TNerveBPTouchDown::theNerve());

			return TRUE;
		}
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPDie, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		((MSModBgm*)gpMSound->unk98)->modBgm(0, 1);
	else
		((MSModBgm*)gpMSound->unk98)->modBgm(0, 1);

	if (actor->checkCurBckFromIndex(0x07) && spine->getTime() == 0x2A8)
		boss->onLiveFlag(LIVE_FLAG_UNK8);

	if (actor->curAnmEndsNext(0, nullptr)
	    && actor->checkCurBckFromIndex(0x07)) {
		boss->kill();
		gpItemManager->makeShineAppearWithDemo("シャイン（ボス用）",
		                                       "ボスシャインカメラ",
		                                       boss->mPosition.x,
		                                       boss->mPosition.y,
		                                       boss->mPosition.z);
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPPreDie, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0) {
		boss->changeBck(0x05);
		boss->mHeadHit->onHitFlag(HIT_FLAG_NO_COLLISION);

		if ((s8)boss->unk17C == 0) {
			boss->unk17C = 1;
			boss->unk174 = nullptr;
			boss->unk170 = nullptr;
			boss->unk1B8 = 0x32;

			if (boss->unk18C != nullptr) {
				JGeometry::TVec3<f32> pos;
				boss->getJointTransByIndex(0x12, &pos);
				pos.y += 250.0f;
				boss->unk18C->mPos.value = pos;
				gpModelWaterManager->emitRequest(*boss->unk18C);
			}
		}

		TEnemyManager* nameKuriManager
		    = JDrama::TNameRefGen::search<TEnemyManager>("ナメクリマネージャ");
		if (nameKuriManager != nullptr)
			nameKuriManager->killChildren();

		MSBgm::stopTrackBGM(1, 10);
	}

	if (actor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveBPDie::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPTouchDown, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();
	MActor* actor      = boss->mMActor;

	if (spine->getTime() == 0)
		boss->changeBck(0x0B);

	if (actor->checkCurBckFromIndex(0x0B)) {
		boss->mPosition.y -= 5.0f;

		const TPathNode& path       = boss->unk104;
		THitActor* pathActor       = path.unk0;
		const JGeometry::TVec3<f32>* pointRef;
		if (pathActor != nullptr)
			pointRef = &pathActor->getPosition();
		else
			pointRef = &path.unk4;
		JGeometry::TVec3<f32> point = *pointRef;
		if (point.y > boss->mPosition.y) {
			boss->mPosition.y = point.y;
			boss->changeBck(0x12);
			boss->offLiveFlag(LIVE_FLAG_UNK10);
		}
	}

	if (actor->checkCurBckFromIndex(0x12)
	    && actor->curAnmEndsNext(0, nullptr)) {
		spine->pushAfterCurrent(&TNerveBPWait::theNerve());
		return TRUE;
	}

	return FALSE;
}

DEFINE_NERVE(TNerveBPFly, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0) {
		boss->changeBck(0x0B);
		boss->goToRandomNextGraphNode();

		if ((s8)boss->unk1CC == 0) {
			MSBgm::startBGM(0x8001000D);
			boss->unk1CC = 1;
		}
	}

	const TPathNode& goalPath = boss->unk104;
	THitActor* goalActor     = goalPath.unk0;
	const JGeometry::TVec3<f32>* goalRef;
	if (goalActor != nullptr)
		goalRef = &goalActor->getPosition();
	else
		goalRef = &goalPath.unk4;
	JGeometry::TVec3<f32> toGoal = *goalRef;
	toGoal -= boss->mPosition;
	toGoal.y = 0.0f;

	if (PSVECMag((Vec*)&toGoal) < 100.0f) {
		if (!(boss->unk124->unk0->unk0[boss->unk124->mCurrIdx].unk0->mFlags
		      & 0x800)) {
			boss->goToRandomNextGraphNode();
		} else {
			spine->pushAfterCurrent(&TNerveBPHover::theNerve());
			return TRUE;
		}
	}

	f32 turnSpeed = boss->mTurnSpeed;
	f32 flySpeed  = boss->getBossPakkunSaveParam()->mSLFlySpeed.value;
	boss->turnToCurPathNode(turnSpeed);

	const TPathNode& dirPath = boss->unkF4;
	THitActor* dirActor     = dirPath.unk0;
	const JGeometry::TVec3<f32>* dirRef;
	if (dirActor != nullptr)
		dirRef = &dirActor->getPosition();
	else
		dirRef = &dirPath.unk4;
	JGeometry::TVec3<f32> dir = *dirRef;
	dir -= boss->mPosition;
	PSVECNormalize((Vec*)&dir, (Vec*)&dir);
	dir *= flySpeed;

	JGeometry::TVec3<f32> velocity = boss->mLinearVelocity;
	velocity += dir;
	boss->mLinearVelocity = velocity;

	return FALSE;
}

DEFINE_NERVE(TNerveBPHover, TLiveActor)
{
	TBossPakkun* boss = (TBossPakkun*)spine->getBody();

	if (spine->getTime() == 0) {
		boss->changeBck(0x10);
		boss->unk16C = 3;
	}

	f32 polBallRange = boss->getBossPakkunSaveParam()->mSLPollBallRange.get();
	JGeometry::TVec3<f32>* marioPos = gpMarioPos;
	if (boss->unk188 == nullptr) {
		boss->unk188
		    = (TAreaCylinderManager*)gpConductor->search("ゲロエリアマネージャー");
	}

	BOOL marioInArea;
	if (boss->unk188 == nullptr)
		marioInArea = FALSE;
	else
		marioInArea = boss->unk188->contain(*marioPos);

	if (marioInArea
	    && boss->mDistToMarioSquared < polBallRange * polBallRange) {
		spine->pushAfterCurrent(&TNerveBPHover::theNerve());
		spine->pushAfterCurrent(&TNerveBPFlyCannon::theNerve());

		TPathNode marioPath(*gpMarioPos);
		boss->unk114.push(boss->unkF4);
		boss->unkF4 = marioPath;

		spine->pushAfterCurrent(&TNerveBPFlyPivot::theNerve());
		return TRUE;
	}

	if (spine->getTime()
	    >= boss->getBossPakkunSaveParam()->mSLHoverTimer.get()) {
		spine->pushAfterCurrent(&TNerveBPFly::theNerve());
		boss->unk16C = 0;
		return TRUE;
	}

	return FALSE;
}

TBossPakkunParams::TBossPakkunParams(const char* path)
    : TSpineEnemyParams(path)
    , PARAM_INIT(mSLWaitFrameStg0, 400)
    , PARAM_INIT(mSLWaterMarkLimit, 600)
    , PARAM_INIT(mSLSwingLength, 600.0f)
    , PARAM_INIT(mSLPollBallStampScale, 1.0f)
    , PARAM_INIT(mSLTumbleTime, 2400)
    , PARAM_INIT(mSLAnmBlendTime0, 60)
    , PARAM_INIT(mSLFlySpeed, 5.0f)
    , PARAM_INIT(mSLPivotSpeed, 0.7f)
    , PARAM_INIT(mSLPivotSpeedAware, 1.5f)
    , PARAM_INIT(mSLVomitAnmRate, 0.6f)
    , PARAM_INIT(mSLHeadHomingLimit, 30.0f)
    , PARAM_INIT(mSLDamageAngle, 180.0f)
    , PARAM_INIT(mSLTornadoProp, 0.4f)
    , PARAM_INIT(mSLTornadoSpeed, 2.0f)
    , PARAM_INIT(mSLTornadoRollSpeed, 0.06f)
    , PARAM_INIT(mSLTornadoMoveInit, 10000.0f)
    , PARAM_INIT(mSLTornadoMoveInc, 1.0f)
    , PARAM_INIT(mSLTornadoMoveLimit, 10720.0f)
    , PARAM_INIT(mSLWaterHitTimer, 20)
    , PARAM_INIT(mSLHoverTimer, 1200)
    , PARAM_INIT(mSLPollBallRange, 10000.0f)
    , PARAM_INIT(mSLPollBallSpeed, 20.0f)
    , PARAM_INIT(mSLPollBallFront, 1000.0f)
{
	TParams::load(mPrmPath);
}

TBossPakkunManager::TBossPakkunManager(const char* name, int is_light)
    : TEnemyManager(name)
    , mIsLight(is_light)
{
}

void TBossPakkunManager::load(JSUMemoryInputStream& stream)
{
	unk38 = new TBossPakkunParams("/enemy/bosspakkun.prm");
	TEnemyManager::load(stream);

	if (mIsLight == 0) {
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_blur1.jpa", 0xA9);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_down.jpa", 0xAA);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_swing1.jpa", 0xAB);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_swing2.jpa", 0xAC);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_wathit.jpa", 0x15D);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_wathit_w.jpa", 0x15E);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_ase.jpa", 0x15F);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_blur2.jpa", 0x160);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_jita.jpa", 0x161);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_tr_rock.jpa", 0x162);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_tr_smoke.jpa", 0x163);
		SMS_LoadParticle("/scene/bosspakkun/jpa/ms_bopa_tr_weed.jpa", 0x164);
	}
}

void TBossPakkunManager::createModelData()
{
	if (mIsLight != 0)
		createModelDataArray(sLightEntries);
	else
		createModelDataArray(sNormalEntries);
}

TBossPakkun::TBossPakkun(const char* name)
    : TSpineEnemy(name)
{
	unk154   = 0.0f;
	mPolDrop = nullptr;
	mVomit   = nullptr;
	mTornado = nullptr;
	mHeadHit = nullptr;
	mNavel   = nullptr;
	unk16C   = 0;
	unk170   = nullptr;
	unk174   = nullptr;
	unk178   = 0;
	unk17C   = 0;
	unk180   = nullptr;
	unk184   = 0.0f;
	unk188   = nullptr;
	unk18C   = nullptr;
	unk190   = 0;
	unk1B8   = 0;
	unk1BC   = 0;
	unk1C0   = 0;
	unk1C4   = 0;
	unk1CC   = 0;

	offLiveFlag(LIVE_FLAG_UNK100);
	mBinder = new TWalker;
}

const char** TBossPakkun::getBasNameTable() const { return bosspakkun_bastable; }

void TBossPakkun::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mPolDrop != nullptr)
		mPolDrop->perform(flags, graphics);
	if (mVomit != nullptr)
		mVomit->perform(flags, graphics);
	if (mTornado != nullptr)
		mTornado->perform(flags, graphics);

	if (checkLiveFlag(LIVE_FLAG_DEAD))
		return;

	if (mHeadHit != nullptr)
		mHeadHit->perform(flags, graphics);
	if (mNavel != nullptr)
		mNavel->perform(flags, graphics);

	if (((TBossPakkunManager*)mManager)->mIsLight == 0 && (flags & 1)) {
		TBossPakkunMtxCalc* mtxCalc = mMtxCalc;
		mtxCalc->unk50 = -unk154 + mtxCalc->unk50;
		if (mtxCalc->unk50 < 0.0f)
			mtxCalc->unk50 = 0.0f;
		else if (mtxCalc->unk50 > 1.0f)
			mtxCalc->unk50 = 1.0f;

		if ((s8)unk17C != 0) {
			if (unk178 <= 0) {
				unk17C = 0;
				unk178 = 0;
			} else {
				s32 dec
				    = getBossPakkunSaveParam()->mSLWaterMarkLimit.value / 100;
				if (dec == 0)
					dec = 1;
				unk178 -= dec;
				if (unk178 < 0)
					unk178 = 0;
			}
		}

		if ((s8)unk17C == 0 && unk174 > 0) {
			--unk174;
			++unk170;
		}

		if ((s8)unk1BC != 0) {
			if (unk1B8 <= 0) {
				unk1BC = 0;
				unk1B8 = 0;
			} else {
				--unk1B8;
			}
		}

		if ((s8)unk16C == 1 && checkMarioRiding()
		    && &TNerveBPJumpReact::theNerve() != mSpine->getLatestNerve()) {
			mSpine->pushNerve(&TNerveBPJumpReact::theNerve());
		}
	}

	if (((TBossPakkunManager*)mManager)->mIsLight == 0 && (flags & 2)) {
		if (mMActor->checkCurBckFromIndex(0x18)) {
			MtxPtr mtx = mMActor->getModel()->getAnmMtx(0x2B);
			unk194.set(mtx[0][3], mtx[1][3], mtx[2][3]);

			mtx = mMActor->getModel()->getAnmMtx(0x2C);
			unk1A0.set(mtx[0][3], mtx[1][3], mtx[2][3]);
		}

		if (mMActor->checkCurBckFromIndex(0x17)) {
			MtxPtr mtx = mMActor->getModel()->getAnmMtx(0x14);
			unk1AC.set(mtx[0][3], mtx[1][3], mtx[2][3]);

			JPABaseEmitter* emitter = gpMarioParticleManager->emitAndBindToPosPtr(
			    0x124, &unk1AC, 1, this);
			if (emitter != nullptr) {
				static JGeometry::TVec3<f32> scale(2.5f, 2.5f, 2.5f);
				emitter->setScale(scale);
			}
		}

		if (mMActor->checkCurBckFromIndex(0x0B)
		    || mMActor->checkCurBckFromIndex(0x0D)
		    || mMActor->checkCurBckFromIndex(0x0C)) {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x160, getModel()->getAnmMtx(0x26), 1, this);
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x160, getModel()->getAnmMtx(0x2E), 1, (u8*)this + 1);
		}

		if (mMActor->checkCurBckFromIndex(0x0B)
		    || mMActor->checkCurBckFromIndex(0x06)
		    || mMActor->checkCurBckFromIndex(0x1A)) {
			gpMarioParticleManager->emitAndBindToMtxPtr(
			    0x15F, getModel()->getAnmMtx(0x14), 1, (u8*)this + 1);
		}
	}

	if (((TBossPakkunManager*)mManager)->mIsLight == 0
	    && &TNerveBPDie::theNerve() == mSpine->getLatestNerve()) {
		MActor* oldActor = mMActor;
		mMActor          = unk180;
		TSpineEnemy::perform(flags, graphics);
		mMActor = oldActor;
		return;
	}

	if (((TBossPakkunManager*)mManager)->mIsLight == 0 && (flags & 2)) {
		updateSquareToMario();
		getModel()->getModelData()->mJointNodePointer[0]->setMtxCalc(mMtxCalc);
	}

	if (((TBossPakkunManager*)mManager)->mIsLight == 0 && (flags & 2)) {
		if ((s8)unk16C == 1) {
			JGeometry::TVec3<f32> arrowPos = mNavel->mPosition;
			arrowPos.y += 100.0f;
			gpTargetArrow->unk14 = 1;
			gpTargetArrow->setPos(arrowPos);
		} else {
			gpTargetArrow->unk14 = 0;
		}
	}

	if (((TBossPakkunManager*)mManager)->mIsLight == 0 && (flags & 0x200)) {
		if (&TNerveBPPreDie::theNerve() == mSpine->getLatestNerve()
		    || &TNerveBPStompReact::theNerve() == mSpine->getLatestNerve()) {
			mMActor->offMakeDL();
			SMS_AddDamageFogEffect(
			    mMActor->getModel()->getModelData(), mPosition, graphics);
		} else {
			SMS_ResetDamageFogEffect(mMActor->getModel()->getModelData());
		}
	}

	TSpineEnemy::perform(flags, graphics);
}

BOOL TBossPakkun::receiveMessage(THitActor* sender, u32 message)
{
	if (((TBossPakkunManager*)mManager)->mIsLight != 0)
		return FALSE;

	const TNerveBPSleep& sleepNerve = TNerveBPSleep::theNerve();
	if (mSpine->getLatestNerve() == &sleepNerve
	    && sender->getActorType() == 0x1000000D) {
		mSpine->reset();
		mSpine->setNext(&TNerveBPBreakSleep::theNerve());
		return TRUE;
	}

	if ((s8)unk16C == 3
	    && (sender->getActorType() == 0x1000000D
	        || sender->getActorType() == 0x1000001)) {
		if (mPosition.y - 300.0f > sender->mPosition.y)
			return TRUE;

		if (mPosition.y + 1500.0f < sender->mPosition.y)
			return TRUE;

		unk16C = 0;
		mSpine->reset();
		mSpine->setNext(&TNerveBPFall::theNerve());

		if (gpMSound->gateCheck(0x2817)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2817, &mPosition, 0, nullptr, 0, 4);
		}

		return TRUE;
	}

	return FALSE;
}

void TBossPakkun::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager, 7);
	mMActor       = mMActorKeeper->createMActor("bosspaku_model.bmd", 0);

	BOOL isLight = ((TBossPakkunManager*)mManager)->mIsLight;

	if (isLight == 0) {
		unk180 = mMActorKeeper->createMActor("bosspaku_end.bmd", 0);
		unk180->setBckFromIndex(7);
		unk180->setBrkFromIndex(0);
	}

	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");

	initHitActor(0x800000F, 1, 0x80000000, 2.5f, 100.0f, 2.5f,
	             100.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);

	if (isLight == 0) {
		mHeadHit = new TBPHeadHit(this, "ボスパックン頭部");
		mHeadHit->initHitActor(0x8000010, 5, 0x81000000, 100.0f, 500.0f,
		                       100.0f, 500.0f);
		mHeadHit->offHitFlag(HIT_FLAG_NO_COLLISION);

		mNavel = new TBPNavel(this, "ボスパックンおへそ");
		mNavel->initHitActor(0x8000011, 1, 0x80000000, 200.0f, 100.0f,
		                     200.0f, 100.0f);
		mNavel->offHitFlag(HIT_FLAG_NO_COLLISION);

		group->getChildren().push_back(mHeadHit);
		group->getChildren().push_back(mNavel);

		mMapCollisionManager
		    = new TMapCollisionManager(1, "/scene/bosspakkun", this);
		mMapCollisionManager->init("col_body.col", 1, nullptr);

		TMapCollisionBase* entry = mMapCollisionManager->unk8;
		Mtx mtx;
		MsMtxSetTRS(mtx, mPosition.x, mPosition.y, mPosition.z,
		            mRotation.x, mRotation.y, mRotation.z, mScaling.x,
		            mScaling.y, mScaling.z);
		PSMTXCopy(mtx, entry->unk20);
		entry->setUp();

		mMtxCalc = new TBossPakkunMtxCalc(this);
		mMActor->setCalcForBck(mMtxCalc);
		mMActor->calc();
	}

	if (isLight != 0) {
		mSpine->initWith(&TNerveBPWaitL::theNerve());
	} else if (gpMarDirector->mMap == 0x37) {
		mSpine->initWith(&TNerveBPFall::theNerve());
	} else if (gpMarDirector->unk7D == 4) {
		mSpine->initWith(&TNerveBPSleep::theNerve());
	} else {
		mSpine->initWith(&TNerveBPWait::theNerve());
	}

	mPolDrop        = new TBPPolDrop(this, "<TBPPolDrop>");
	MActor* stamp   = mMActorKeeper->createMActor("pollut_ball_stamp.bmd", 0);
	MActor* ball    = mMActorKeeper->createMActor("pollut_ball.bmd", 0);
	mPolDrop->unk78 = ball;
	mPolDrop->unk7C = stamp;

	ResTIMG* texture = (ResTIMG*)JKRFileLoader::getGlbResource(
	    "/scene/map/pollution/H_ma_rak.bti");
	if (texture != nullptr) {
		SMS_ChangeTextureAll(mPolDrop->unk78->getModel()->getModelData(),
		                     "M_dummy", *texture);
	}

	if (isLight == 0) {
		mVomit        = new TBPVomit(this, "<TBPVomit>");
		MActor* white = mMActorKeeper->createMActor("bosspakuPollut_white.bmd",
		                                            0);
		MActor* black = mMActorKeeper->createMActor("bosspakuPollut.bmd", 0);
		mVomit->unk14 = black;
		mVomit->unk18 = white;

		mTornado = new TBPTornado(this, "<TBPTornado>");
		group->getChildren().push_back(mTornado);

		unk18C = new TWaterEmitInfo("/enemy/bosspakuwater.prm");
	}

	initAnmSound();
	onLiveFlag(LIVE_FLAG_UNK400);
	mScaledBodyRadius = 400.0f;

	unk124->unk0 = gpConductor->getGraphByName("bosspakkun");
	if (unk124->unk0 != nullptr) {
		unk124->mPrevIdx = -1;
		goToShortestNextGraphNode();
	}

	if (getSaveParam())
		mHitPoints = getSaveParam()->mSLHitPointMax.get();
	else
		mHitPoints = 1;
}

void TBossPakkun::setGroundCollision()
{
	const TNerveBPDie& dieNerve = TNerveBPDie::theNerve();
	if (mSpine->getLatestNerve() == &dieNerve)
		return;

	if (getMapCollisionManager() == nullptr)
		return;

	J3DModel* model = getModel();
	TMtx34f mtx;
	mtx.set(model->mNodeMatrices[2]);

	if (getMapCollisionManager()->unk8 != nullptr) {
		getMapCollisionManager()->unk8->moveMtx(mtx);
	}
}

void TBossPakkun::kill()
{
	TLiveActor::kill();
	onHitFlag(HIT_FLAG_NO_COLLISION);
	if (mHeadHit != nullptr)
		mHeadHit->onHitFlag(HIT_FLAG_NO_COLLISION);
	if (mNavel != nullptr)
		mNavel->onHitFlag(HIT_FLAG_NO_COLLISION);
	if (mPolDrop != nullptr)
		mPolDrop->onHitFlag(HIT_FLAG_NO_COLLISION);
}

void TBossPakkun::changeBck(int index)
{
	if (!mMActor->checkCurBckFromIndex(index)
	    || mMActor->curAnmEndsNext(MActor::ANM_TYPE_BCK, nullptr)) {
		int curBck = mMActor->getCurAnmIdx(MActor::ANM_TYPE_BCK);

		TBossPakkunMtxCalc* mtxCalc = mMtxCalc;
		MActorAnmDataEach<J3DAnmTransformKey>* data
		    = mtxCalc->mOwner->mMActorKeeper->getMActorAnmData()->getUnk2C();
		J3DAnmTransform* nextAnm = data->getAnmPtr(index);

		if (mtxCalc->unk54 != nextAnm) {
			mtxCalc->unk58 = mtxCalc->unk54;
			mtxCalc->unk54 = nextAnm;
			mtxCalc->unk50 = 1.0f;
		}

		MActorAnmBck* bck = mMActor->unkC;
		bck->unk0         = index;
		if (index >= 0) {
			bck->unk24 = bck->getData()->getAnmPtr(index);
			bck->unk4.init(bck->unk24->getFrameMax());
			bck->unk4.setAttribute(bck->unk24->getAttribute());
			bck->unk4.setRate(SMSGetAnmFrameRate());
		}

		if (index == 0x15) {
			TBossPakkunParams* params = getBossPakkunSaveParam();
			MActor* actor             = mMActor;
			f32 rate                  = params->mSLVomitAnmRate.value;
			actor->getFrameCtrl(MActor::ANM_TYPE_BCK)->setRate(rate);
		}

		f32 blendFrames = 60.0f;
		if (curBck == 0x19) {
			if (index == 0x15 || index == 0x1A)
				blendFrames
				    = getBossPakkunSaveParam()->mSLAnmBlendTime0.value;
		} else if (curBck == 0x12) {
			if (index == 0x19)
				blendFrames
				    = getBossPakkunSaveParam()->mSLAnmBlendTime0.value;
		} else if (curBck == 0x02) {
			if (index == 0x19)
				blendFrames
				    = getBossPakkunSaveParam()->mSLAnmBlendTime0.value;
		} else if (curBck == 0x14) {
			if (index == 0x19)
				blendFrames
				    = getBossPakkunSaveParam()->mSLAnmBlendTime0.value;
		} else if (curBck == 0x1A) {
			if (index == 0x16)
				blendFrames
				    = getBossPakkunSaveParam()->mSLAnmBlendTime0.value;
		}

		if (blendFrames < 1.0f) {
			J3DFrameCtrl* ctrl = mMActor->getFrameCtrl(MActor::ANM_TYPE_BCK);
			if (ctrl)
				blendFrames = 0.2f * ctrl->getEnd();
		}

		if (blendFrames == 0.0f)
			unk154 = 1.0f;
		else
			unk154 = 1.0f / blendFrames;

		const char** basTable = getBasNameTable();
		const char* basName;
		if (basTable == nullptr)
			basName = nullptr;
		else
			basName = basTable[index];
		setAnmSound(basName);
	}
}

void TBossPakkun::launchPolDrop()
{
	TBPPolDrop* drop = mPolDrop;
	if (drop->unk80 != 0)
		return;

	JGeometry::TVec3<f32> launchPos;
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		launchPos = mPosition;
		launchPos.x += 1.0f;
	} else {
		getJointTransByIndex(0x12, &launchPos);
	}

	f32 marioRotY = gpMarioOriginal->mRotation.y;
	TBossPakkunParams* params = getBossPakkunSaveParam();
	f32 front = params->mSLPollBallFront.value;
	s16 marioAngle = (s16)(marioRotY * (65536.0f / 360.0f));

	JGeometry::TVec3<f32> targetPos = polarXZ(marioAngle, front);
	targetPos.x += gpMarioPos->x;
	targetPos.y += gpMarioPos->y;
	targetPos.z += gpMarioPos->z;

	JGeometry::TVec3<f32> velocity;
	SMSCalcJumpVelocityXZ(targetPos, launchPos,
	                      getBossPakkunSaveParam()->mSLPollBallSpeed.value, 0.1f,
	                      &velocity);

	drop             = mPolDrop;
	drop->mVelocity  = velocity;
	drop->mPosition  = launchPos;
	drop->mScaling.x  = 0.0f;
	drop->mScaling.y  = 0.0f;
	drop->mScaling.z  = 0.0f;
	drop->mRotation.z = 1.0f;
	drop->mRotation.y = 1.0f;
	drop->mRotation.x = 1.0f;
	drop->unk80      = 1;
	drop->unk84      = 0;
	drop->unk78->setBck("pollut_ball");
	drop->unk78->setBtk("pollut_ball_01");
	drop->unk78->setBtk("pollut_ball_02");
	drop->unk88 = launchPos.y;
}

void TBossPakkun::gotHipDropDamage()
{
	if (mHitPoints > 0)
		mHitPoints--;

	unk16C = 0;

	if (mHitPoints == 0) {
		if (&TNerveBPPreDie::theNerve() == mSpine->getLatestNerve())
			return;
		if (&TNerveBPDie::theNerve() == mSpine->getLatestNerve())
			return;

		mSpine->setNext(&TNerveBPPreDie::theNerve());
		if (gpMSound->gateCheck(0x284E))
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x284E, &mPosition, 0, nullptr, 0, 4);
		return;
	}

	const TNerveBase<TLiveActor>* tumbleOut = &TNerveBPTumbleOut::theNerve();
	if (mSpine->getLatestNerve() == tumbleOut)
		return;

	if (gpMSound->gateCheck(0x284D))
		MSoundSESystem::MSoundSE::startSoundActor(0x284D, &mPosition, 0,
		                                          nullptr, 0, 4);

	if (gpMarDirector->unk7D == 4) {
		mSpine->reset();
		mSpine->setNext(&TNerveBPTakeOff::theNerve());
		mSpine->pushNerve(&TNerveBPGetUp::theNerve());
		mSpine->pushNerve(&TNerveBPStompReact::theNerve());
	} else {
		mSpine->reset();
		mSpine->setNext(&TNerveBPWait::theNerve());
		mSpine->pushNerve(&TNerveBPGetUp::theNerve());
		mSpine->pushNerve(&TNerveBPStompReact::theNerve());
	}
}

#pragma dont_inline on
void TBossPakkun::showMessage(u32 message)
{
	u32 bit = message - 0xE0000 == 1 ? 0 : 1 << (message - 0xE0000);

	if ((unk1C0 & bit) == 0)
		gpMarDirector->getConsole()->startAppearBalloon(message, true);
	unk1C0 |= bit;
}
#pragma dont_inline off

void TBossPakkun::rumblePad(int param_1, const JGeometry::TVec3<f32>& pos)
{
	if (!SMS_IsMarioTouchGround4cm())
		return;

	f32 distance;
	JGeometry::TVec3<f32> delta = SMS_GetMarioPos();
	delta -= pos;
	distance     = delta.length();
	f32 strength = (3000.0f - distance) / 1000.0f;

	if (strength < 0.0f)
		return;

	if (strength > 1.0f)
		strength = 1.0f;

	switch (param_1) {
	case 0:
		strength *= 0.4f;
		break;
	case 1:
		strength *= 0.7f;
		break;
	case 2:
		break;
	}

	unk1C8 = strength;
	SMSRumbleMgr->start(8, &unk1C8);
}

BOOL TBossPakkun::checkMarioRiding()
{
	const TBGCheckData* ground = SMS_GetMarioGrPlane();
	if ((s8)unk190 == 0) {
		if (ground != nullptr && ground->getActor() == this
		    && SMS_IsMarioTouchGround4cm()) {
			u32 status = SMS_GetMarioStatus();
			if ((status & 0x200) && !(status & 0x200000)) {
				unk190 = 1;
				return TRUE;
			}
		}
	} else {
		if (ground == nullptr || ground->getActor() != this
		    || !SMS_IsMarioTouchGround4cm())
			unk190 = 0;
	}

	return FALSE;
}

void TBossPakkunMtxCalc::calc(u16 joint_no)
{
	M3UMtxCalcSIAnmBlendQuat::calc(joint_no);
	calcBellyScale(joint_no);
	calcHeadDir(joint_no);
}

void TBossPakkunMtxCalc::calcHeadDir(u16 joint_no)
{
	if (joint_no != 0x12)
		return;

	TBossPakkun* owner = mOwner;
	J3DModel* model    = owner->getModel();
	MtxPtr jointMtx    = model->mNodeMatrices[joint_no];

	JGeometry::TVec3<f32> toMario = *gpMarioPos;
	toMario.x -= jointMtx[0][3];
	toMario.y -= jointMtx[1][3];
	toMario.z -= jointMtx[2][3];

	f32 matrixYaw = calcBossPakkunYaw(jointMtx[0][1], jointMtx[2][0]);
	f32 headYaw   = owner->unk184;

	f32 targetYaw;
	if (owner->mMActor->checkCurBckFromIndex(0x19)) {
		targetYaw = headYaw + calcBossPakkunYaw(toMario.x, toMario.z);
		while (targetYaw >= 360.0f)
			targetYaw -= 360.0f;
		while (targetYaw < 0.0f)
			targetYaw += 360.0f;
	} else {
		targetYaw = matrixYaw;
	}

	f32 wrappedMatrix
	    = callMsWrap(matrixYaw, targetYaw - 180.0f, targetYaw + 180.0f);
	f32 desiredOffset = targetYaw - wrappedMatrix;
	f32 homingLimit = owner->getBossPakkunSaveParam()->mSLHeadHomingLimit.value;
	if (desiredOffset > 0.0f) {
		if (desiredOffset > homingLimit)
			desiredOffset = homingLimit;
	} else {
		if (desiredOffset <= -homingLimit)
			desiredOffset = -homingLimit;
	}

	f32 wrappedHead
	    = callMsWrap(headYaw, desiredOffset - 180.0f, desiredOffset + 180.0f);
	f32 delta = desiredOffset - wrappedHead;
	if (delta > 0.0f) {
		if (delta > 1.0f)
			delta = 1.0f;
	} else {
		if (delta <= -1.0f)
			delta = -1.0f;
	}

	headYaw += delta;
	owner->unk184 = headYaw;

	Mtx headMtx;
	f32 sin = JMASin(headYaw);
	f32 cos = JMACos(headYaw);
	headMtx[0][0] = 25.0f;
	headMtx[0][1] = 0.0f;
	headMtx[0][2] = 0.0f;
	headMtx[0][3] = 0.0f;
	headMtx[1][0] = 0.0f;
	headMtx[1][1] = cos;
	headMtx[1][2] = -sin;
	headMtx[1][3] = 0.0f;
	headMtx[2][0] = 0.0f;
	headMtx[2][1] = sin;
	headMtx[2][2] = cos;
	headMtx[2][3] = 0.0f;

	PSMTXConcat(jointMtx, headMtx, jointMtx);
	PSMTXConcat(J3DSys::mCurrentMtx, headMtx, J3DSys::mCurrentMtx);
}

void TBossPakkunMtxCalc::calcBellyScale(u16 joint_no)
{
	if (joint_no != 4 && joint_no != 0x24)
		return;

	f32 ratio;
	if ((s8)mOwner->unk17C != 0) {
		ratio = (f32)mOwner->unk1B8 / 50.0f;
	} else {
		s32 limit = mOwner->getBossPakkunSaveParam()->mSLWaterMarkLimit.value;
		s32 count = mOwner->unk178;
		if (count > limit)
			count = limit;

		ratio = (f32)count / (f32)limit;
	}

	f32 blend = JMAHermiteInterpolation(ratio, 0.0f, 0.0f, 10.0f, 1.0f,
	                                    1.0f, 0.0f);

	J3DModel* model = mOwner->getModel();
	MtxPtr jointMtx = model->mNodeMatrices[joint_no];
	Mtx scaleMtx;

	if (joint_no == 0x24) {
		static JGeometry::TVec3<f32> targetScale(1.4f, 1.4f, 1.6f);
		static JGeometry::TVec3<f32> startScale(1.0f, 0.8f, 0.8f);

		PSMTXScale(scaleMtx,
		           blend * (targetScale.x - startScale.x) + startScale.x,
		           blend * (targetScale.y - startScale.y) + startScale.y,
		           blend * (targetScale.z - startScale.z) + startScale.z);
	} else {
		static JGeometry::TVec3<f32> targetScale(1.3f, 1.7f, 1.7f);
		static JGeometry::TVec3<f32> startScale(1.0f, 0.9f, 0.9f);

		PSMTXScale(scaleMtx,
		           blend * (targetScale.x - startScale.x) + startScale.x,
		           blend * (targetScale.y - startScale.y) + startScale.y,
		           blend * (targetScale.z - startScale.z) + startScale.z);
	}

	PSMTXConcat(jointMtx, scaleMtx, jointMtx);
	PSMTXCopy(jointMtx, J3DSys::mCurrentMtx);
}

void TBPNavel::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		mOwner->getJointTransByIndex(6, &mPosition);

	THitActor::perform(flags, graphics);
}

BOOL TBPNavel::receiveMessage(THitActor* sender, u32 message)
{
	const TNerveBPSleep& sleepNerve = TNerveBPSleep::theNerve();
	if (mOwner->mSpine->getLatestNerve() == &sleepNerve)
		return mOwner->receiveMessage(sender, message);

	if (sender->getActorType() == 0x1000001)
		return FALSE;

	if ((s8)mOwner->unk16C != 1)
		return TRUE;

	if (sender->getActorType() == 0x80000001 && message == HIT_MESSAGE_HIP_DROP)
		mOwner->gotHipDropDamage();

	return TRUE;
}

void TBPHeadHit::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if ((flags & 1) && (s8)mOwner->unk16C != 1) {
		for (int i = 0; i < mColCount; ++i) {
			THitActor* actor = mCollisions[i];
			bool isMario;
			if (actor->getActorType() == 0x80000001)
				isMario = true;
			else
				isMario = false;

			if (isMario)
				throwActor(actor);
		}
	}

	if (flags & 2)
		mOwner->getJointTransByIndex(1, &mPosition);

	THitActor::perform(flags, graphics);
}

void TBPHeadHit::throwActor(THitActor* actor)
{
	if (actor->getActorType() != 0x80000001)
		return;

	if (!mOwner->mMActor->checkCurBckFromIndex(15))
		return;

	static JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);

	JGeometry::TVec3<f32> dir = mOwner->mPosition;
	dir -= actor->mPosition;

	if (dir.squared() <= JGeometry::TUtil<f32>::epsilon()) {
		dir.x = 0.0f;
		dir.y = 0.0f;
		dir.z = 1.0f;
	} else {
		PSVECNormalize((Vec*)&dir, (Vec*)&dir);
	}

	JGeometry::TVec3<f32> throwVec;
	throwVec.x = up.y * dir.z - up.z * dir.y;
	throwVec.y = up.z * dir.x - up.x * dir.z;
	throwVec.z = up.x * dir.y - up.y * dir.x;

	if (throwVec.squared() <= JGeometry::TUtil<f32>::epsilon()) {
		throwVec.x = 1.0f;
		throwVec.y = 0.0f;
		throwVec.z = 0.0f;
	} else {
		PSVECNormalize((Vec*)&throwVec, (Vec*)&throwVec);
	}

	throwVec *= 2.0f;
	throwVec += up;

	SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
	SMS_SendMessageToMario(this, HIT_MESSAGE_UNK7);
	SMS_ThrowMario(throwVec, 60.0f);
}

BOOL TBPHeadHit::receiveMessage(THitActor* sender, u32 message)
{
	const TNerveBPSleep& sleepNerve = TNerveBPSleep::theNerve();
	if (mOwner->mSpine->getLatestNerve() == &sleepNerve)
		return mOwner->receiveMessage(sender, message);

	TBossPakkun* boss = mOwner;
	s32 mouthState    = (s8)boss->unk16C;

	if (mouthState == 3
	    && (sender->getActorType() == 0x1000000D
	        || sender->getActorType() == 0x1000001)) {
		boss->unk16C = 0;
		boss->mSpine->reset();
		boss->mSpine->setNext(&TNerveBPFall::theNerve());

		if (gpMSound->gateCheck(0x2817)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x2817, &boss->mPosition, 0, nullptr, 0, 4);
		}

		return TRUE;
	}

	if (mouthState != 2) {
		if (gpMarDirector->mMap == 2 && gpMarDirector->unk7D == 4
		    && boss->mSpine->getLatestNerve() == &TNerveBPFly::theNerve())
			boss->showMessage(0xE0002);

		if (sender->getActorType() == 0x1000001)
			return TRUE;

		return FALSE;
	}

	if (sender->getActorType() != 0x1000001)
		return TRUE;

	if (message != HIT_MESSAGE_SPRAYED_BY_WATER)
		return TRUE;

	JGeometry::TVec3<f32> toMario = *gpMarioPos;
	toMario -= mPosition;

	f32 targetYaw;
	if (toMario.z == 0.0f) {
		if (toMario.x >= 0.0f)
			targetYaw = 90.0f;
		else
			targetYaw = -90.0f;
	} else if (toMario.z >= 0.0f) {
		targetYaw = matan(toMario.z, toMario.x) * (360.0f / 65536.0f);
	} else {
		f32 yaw = matan(-toMario.z, toMario.x) * (360.0f / 65536.0f);
		targetYaw = 180.0f - yaw;
	}

	while (targetYaw >= 360.0f)
		targetYaw -= 360.0f;
	while (targetYaw < 0.0f)
		targetYaw += 360.0f;

	f32 wrappedYaw = callMsWrap(boss->mRotation.y, targetYaw - 180.0f,
	                            targetYaw + 180.0f);
	if (fabsf(targetYaw - wrappedYaw)
	    < 0.5f * boss->getBossPakkunSaveParam()->mSLDamageAngle.value) {
		if ((s8)boss->unk17C == 0) {
			boss->unk170++;

			if (boss->unk178
			    < boss->getBossPakkunSaveParam()->mSLWaterMarkLimit.value)
				boss->unk178++;

			boss->unk174
			    = boss->getBossPakkunSaveParam()->mSLWaterHitTimer.value;

			if (boss->mSpine->getLatestNerve()
			    != &TNerveBPSwallow::theNerve()) {
				boss->mSpine->reset();
				boss->mSpine->setNext(&TNerveBPSwallow::theNerve());
			}
		}
	}

	return TRUE;
}

TBPTornado::TBPTornado(TBossPakkun* owner, const char* name)
    : THitActor(name)
    , mOwner(owner)
    , unk94(0.0f)
    , unk98(0)
{
	mActor = mOwner->mMActorKeeper->createMActor("trunade.bmd", 0);
	initHitActor(0x8000010, 5, 0x81000000, 150.0f, 600.0f, 100.0f,
	             600.0f);
	onHitFlag(HIT_FLAG_NO_COLLISION);
	mActor->setBtkFromIndex(2);
	mActor->setBckFromIndex(29);
	mActor->setBrkFromIndex(1);
	mScaling.set(2.0f, 2.0f, 2.0f);
}

void TBPTornado::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (mOwner->mLiveFlag & LIVE_FLAG_DEAD)
		return;

	if (unk98 == 0)
		return;

	if (flags & 1) {
		if (unk98 == 2) {
			mPosition.add(unk88);
			if (mActor->curAnmEndsNext(5, nullptr)) {
				unk98 = 0;
				return;
			}
		} else {
			unk94 += mOwner->getBossPakkunSaveParam()->mSLTornadoMoveInc.value;

			if (unk94
			    > mOwner->getBossPakkunSaveParam()->mSLTornadoMoveLimit.value) {
				onHitFlag(HIT_FLAG_NO_COLLISION);
				unk98 = 2;
				J3DFrameCtrl* ctrl = mActor->getFrameCtrl(5);
				ctrl->setFrame(0.0f);
				ctrl->setRate(SMSGetAnmFrameRate());
				return;
			}

			int phase  = (int)unk94;
			f32 angle  = 360.0f - (phase % 360);
			f32 radius = unk94
			             * mOwner->getBossPakkunSaveParam()
			                   ->mSLTornadoRollSpeed.value;

			JGeometry::TVec3<f32> toTarget = unk70;
			toTarget -= unk7C;
			if (PSVECMag((Vec*)&toTarget) < 60.0f) {
				onHitFlag(HIT_FLAG_NO_COLLISION);
				unk98 = 2;
				J3DFrameCtrl* ctrl = mActor->getFrameCtrl(5);
				ctrl->setFrame(0.0f);
				ctrl->setRate(SMSGetAnmFrameRate());
				return;
			}

			PSVECNormalize((Vec*)&toTarget, (Vec*)&toTarget);
			toTarget.scale(
			    mOwner->getBossPakkunSaveParam()->mSLTornadoSpeed.value);
			unk7C += toTarget;

			JGeometry::TVec3<f32> nextPos;
			nextPos.x = unk7C.x + radius * JMACos(angle);
			nextPos.y = unk7C.y;
			nextPos.z = unk7C.z + radius * JMASin(angle);

			const TBGCheckData* ground = nullptr;
			f32 groundY = gpMap->checkGround(nextPos.x, nextPos.y + 200.0f,
			                                 nextPos.z, &ground);
			if (!ground->checkFlag(BG_CHECK_FLAG_ILLEGAL))
				nextPos.y = groundY;

			if (gpMap->isTouchedOneWallAndMoveXZ(&nextPos.x, nextPos.y,
			                                     &nextPos.z, 80.0f)) {
				onHitFlag(HIT_FLAG_NO_COLLISION);
				unk98 = 2;
				J3DFrameCtrl* ctrl = mActor->getFrameCtrl(5);
				ctrl->setFrame(0.0f);
				ctrl->setRate(SMSGetAnmFrameRate());
			}

			unk88 = nextPos;
			unk88 -= mPosition;
			mPosition = nextPos;

			for (int i = 0; i < getColNum(); ++i) {
				THitActor* actor = getCollision(i);
				if (actor->getActorType() == 0x80000001) {
					static JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
					SMS_SendMessageToMario(this, HIT_MESSAGE_ATTACK);
					SMS_SendMessageToMario(this, HIT_MESSAGE_UNK7);
					SMS_ThrowMario(up, 60.0f);

					onHitFlag(HIT_FLAG_NO_COLLISION);
					unk98 = 2;
					J3DFrameCtrl* ctrl = mActor->getFrameCtrl(5);
					ctrl->setFrame(0.0f);
					ctrl->setRate(SMSGetAnmFrameRate());
				}
			}
		}
	}

	u32 doCalc = flags & 2;
	if (doCalc) {
		J3DModel* model = mActor->getModel();
		PSMTXIdentity(model->unk20);
		model->unk20[0][3] = mPosition.x;
		model->unk20[1][3] = mPosition.y;
		model->unk20[2][3] = mPosition.z;
		model->unk14.x     = mScaling.x;
		model->unk14.y     = mScaling.y;
		model->unk14.z     = mScaling.z;
	}

	if (doCalc) {
		MtxPtr mtx = mActor->getModel()->unk20;
		JPABaseEmitter* emitter
		    = gpMarioParticleManager->emitAndBindToMtxPtr(0x162, mtx, 1, this);
		if (emitter)
			emitter->setScale(mScaling);

		emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x163, mtx, 1, (u8*)this + 1);
		if (emitter)
			emitter->setScale(mScaling);

		emitter = gpMarioParticleManager->emitAndBindToMtxPtr(
		    0x164, mtx, 1, (u8*)this + 2);
		if (emitter)
			emitter->setScale(mScaling);
	}

	if (doCalc) {
		JGeometry::TVec3<f32> soundDist = mPosition;
		soundDist -= *gpMarioPos;
		f32 dist = soundDist.length();
		if (gpMSound->gateCheck(0x210C))
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(
			    0x210C, &mPosition, nullptr, dist, 0, 0, nullptr, 0, 4);
	}

	mActor->perform(flags, graphics);
}

void TBPVomit::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk14->getCurAnmIdx(0) < 0)
		return;

	if ((flags & 2) && unk14->curAnmEndsNext(0, nullptr)) {
		unk14->setBckFromIndex(-1);
		unk18->setBckFromIndex(-1);
		return;
	}

	if (flags & 2)
		unk18->calcAnm();

	if (flags & 0x200)
		gpPollution->stampModel(unk18->getModel());

	unk14->perform(flags, graphics);
}

TBPPolDrop::TBPPolDrop(TBossPakkun* owner, const char* name)
    : THitActor(name)
    , mOwner(owner)
{
	unk78       = nullptr;
	unk7C       = nullptr;
	unk80       = 0;
	unk84       = 0;
	unk88       = 0.0f;
	mVelocity.z = 0.0f;
	mVelocity.y = 0.0f;
	mVelocity.x = 0.0f;
	initHitActor(0x800000F, 1, -0x80000000, 0.0f, 0.0f, 100.0f, 200.0f);
	offHitFlag(HIT_FLAG_NO_COLLISION);
	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	group->getChildren().push_back(this);
}

void TBPPolDrop::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk80 == 0)
		return;

	u32 doMove = flags & 1;
	if (doMove) {
		move();
		unk84++;
	}

	if (doMove) {
		for (int i = 0; i < mColCount; ++i) {
			THitActor* actor = mCollisions[i];
			bool isMario;
			if (actor->getActorType() == 0x80000001)
				isMario = true;
			else
				isMario = false;

			if (isMario) {
				actor->receiveMessage(this, HIT_MESSAGE_ATTACK);
				mOwner->rumblePad(2, mPosition);
				if (SMS_IsMarioTouchGround4cm())
					unk80 = 2;
				else
					unk80 = 0;
			}
		}
	}

	if (flags & 2) {
		MtxPtr modelMtx = unk78->getModel()->unk20;
		PSMTXIdentity(modelMtx);
		modelMtx[0][3] = mPosition.x;
		modelMtx[1][3] = mPosition.y;
		modelMtx[2][3] = mPosition.z;
		unk78->getModel()->unk14 = mScaling;

		if (unk80 == 2) {
			f32 scale = mOwner->getBossPakkunSaveParam()->mSLPollBallStampScale.get();
			JGeometry::TVec3<f32> stampScale(scale);
			unk7C->getModel()->unk14 = stampScale;
			PSMTXCopy(modelMtx, unk7C->getModel()->unk20);
		}
	}

	if (unk80 == 1) {
		unk78->perform(flags, graphics);

		if (flags & 4) {
			TCircleShadowRequest request;
			request.unk0  = mPosition;
			request.unkC  = 400.0f;
			request.unk10 = 400.0f;
			request.unk14 = 0.0f;
			request.unk1C = 0;
			gpBindShadowManager->request(request, 0);
		}
	}

	if (unk80 == 2) {
		if (flags & 2)
			unk7C->calcAnm();

		if (flags & 0x200)
			gpPollution->stampModel(unk7C->getModel());
	}
}

void TBPPolDrop::move()
{
	if (unk80 == 0) {
		onHitFlag(HIT_FLAG_NO_COLLISION);
		return;
	}

	JGeometry::TVec3<f32> nextPos = mPosition;
	nextPos.add(mVelocity);

	if (unk80 == 1) {
		if (unk78->curAnmEndsNext(0, nullptr))
			unk78->setBck("pollut_ball");

		mVelocity.y -= 0.1f;

		bool shouldProcess = true;
		if (unk84 < 60) {
			if (gpMarDirector->mMap == 2 && gpMarDirector->unk7D == 4)
				shouldProcess = true;
			else
				shouldProcess = false;
		}

		if (shouldProcess) {
			const TBGCheckData* ground;
			f32 y = gpMap->checkGround(nextPos.x, mPosition.y, nextPos.z,
			                           &ground)
			        + 1.0f;
			if (ground->checkFlag(BG_CHECK_FLAG_ILLEGAL))
				y = unk88;
			unk88 = y;

			if (nextPos.y < y) {
				unk80       = 2;
				mVelocity.z = 0.0f;
				mVelocity.y = 0.0f;
				mVelocity.x = 0.0f;
				unk7C->setBck("pollut_ball_stamp");
				gpMarioParticleManager->emit(0x52, &mPosition, 0, nullptr);
				if (gpMSound->gateCheck(0x2841))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x2841, &mPosition, 0, nullptr, 0, 4);
				mOwner->rumblePad(2, mPosition);
				nextPos.y = y;
				onHitFlag(HIT_FLAG_NO_COLLISION);
				return;
			}

			offHitFlag(HIT_FLAG_NO_COLLISION);
			if (gpMap->isTouchedOneWallAndMoveXZ(&nextPos.x, nextPos.y,
			                                     &nextPos.z, 80.0f))
				unk80 = 0;

			f32 soundSpeed = -mVelocity.y;
			if (gpMSound->gateCheck(0x2052))
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x2052, &mPosition, nullptr, soundSpeed, 0, 0, nullptr,
				    0, 4);
			soundSpeed = -mVelocity.y;
			if (gpMSound->gateCheck(0x2045))
				MSoundSESystem::MSoundSE::startSoundActorWithInfo(
				    0x2045, &mPosition, nullptr, soundSpeed, 0, 0, nullptr,
				    0, 4);
		}
	} else if (unk80 == 2) {
		if (unk7C->curAnmEndsNext(0, nullptr))
			unk80 = 0;
	}

	mPosition = nextPos;
}
