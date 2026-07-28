#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossHanachanSaveParams.hpp>
#include <Enemy/BossHanachanSub.hpp>
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Camera/CameraShake.hpp>
#include <Camera/cameralib.hpp>
#include <GC2D/GCConsole2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAttach.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTexture.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MSound/MSModBgm.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <Map/Map.hpp>
#include <Map/MapCollisionEntry.hpp>
#define MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapCollisionEntry.hpp>
#undef MAP_COLLISION_ENTRY_DEFINE_SET_UP_TRANS
#include <Map/MapData.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <MoveBG/ItemManager.hpp>
#include <NPC/NpcInbetween.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/Binder.hpp>
#include <Strategic/ObjModel.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <System/MarDirector.hpp>
#include <System/TargetArrow.hpp>
#include <dolphin/mtx.h>

const char* cSandPillarModelName = "sunabashira.bmd";
const char* cHitPoint1_RailName  = "bosshanachan2";
const char* cHitPoint2_RailName  = "bosshanachan1";
const char* cSandTextureName     = "suna";
const char* cDummyTextureName    = "M_dummy";

static f32 getRotFromXZ(f32 x, f32 z)
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

static inline f32 callMsWrap(f32 t, f32 l, f32 r)
{
	return MsWrap<f32>(t, l, r);
}

static inline JGeometry::TVec3<f32> makeVec3(f32 x, f32 y, f32 z)
{
	return JGeometry::TVec3<f32>(x, y, z);
}

static inline bool isBossHanachanDirectorBlocked()
{
	bool result = true;
	bool isTalk = result;
	if (gpMarDirector->unk124 != 1 && gpMarDirector->unk124 != 2)
		isTalk = false;
	if (!isTalk) {
		if (gpMarDirector->unk124 != 4)
			result = false;
	}
	return result;
}

static void CalcRevisionPosByRotateZ(const JGeometry::TVec3<f32>&, f32,
                                     Vec*);

BOOL TBossHanachan::hasMapCollision() const { return TRUE; }

void TBossHanachan::removeAllMapCollision()
{
	mHead->mMapCollision->remove();
	for (int i = 0; i < 8; ++i)
		mBody[i]->mMapCollision->remove();
}

void TBossHanachan::execDamage()
{
	mSpine->reset();

	if (mHitPoints != 0)
		--mHitPoints;

	if (mHitPoints == 0) {
		((TBossHanachanPartsHead*)mHead)->mWaterHit->onHitFlag(1);
		for (int i = 0; i < 8; ++i) {
			TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
			body->mWaterHit->onHitFlag(1);
			body->mFeet[0]->onHitFlag(1);
			body->mFeet[1]->onHitFlag(1);
		}

		mSpine->setNext(&TNerveBossHanachanDead::theNerve());
		setAnmTimerWhenDead();
		unk1AC = *gpMarioPos;
		if (gpMSound->gateCheck(0x28E6))
			MSoundSESystem::MSoundSE::startSoundActor(0x28E6, &unk1AC, 0,
			                                          nullptr, 0, 4);
	} else {
		mSpine->setNext(&TNerveBossHanachanDamage::theNerve());
		setAnmTimerWhenDamage();

		TBossHanachanManager* manager = (TBossHanachanManager*)mManager;
		mChangeParams                 = manager->mChangeParams[3 - mHitPoints];

		const char* railName = cHitPoint1_RailName;
		if (mHitPoints == 2)
			railName = cHitPoint2_RailName;

		unk124->setGraph(gpConductor->getGraphByName(railName));
		mLiveFlag |= 0x20000;

		Vec* pos = (Vec*)&((TBossHanachanPartsBody*)mBody[unk174])->unk154;
		if (gpMSound->gateCheck(0x280F))
			MSoundSESystem::MSoundSE::startSoundActor(0x280F, pos, 0,
			                                          nullptr, 0, 4);
	}
}

void TBossHanachan::goToInitialRecoverGraphNode()
{
	unk124->mPrevIdx = -1;
	unk124->mCurrIdx = -1;

	int index = unk124->getGraph()->findNearestVisibleIndex(
	    mPosition, mRotation.y, mParams->mSLRecoverSearchDist.value,
	    mParams->mSLRecoverSearchDegree.get(), 0xffffffff);
	if (index < 0) {
		goToShortestNextGraphNode();
	} else {
		unk124->setTo(index);
		setGoalPathFromGraph();
		unk128 = 0;
		unk12C = 0.0f;
	}
}

void TBossHanachan::execSlip()
{
	CLBChaseGeneralConstantSpecifySpeed<f32>(
	    &mMarchSpeed, 0.0f, mChangeParams->mSLMarchDecrease.value);
	mTurnSpeed = 0.1f;

	if (mMarchSpeed <= 0.001f)
		return;

	JGeometry::TVec3<f32> dir = unk188;
	if (mMarchSpeed > 4.0f) {
		f32 maxRot = getBodyMaxRotateZ();
		f32 yForce = 0.0f;
		f32 xSide  = 1.0f;
		if (maxRot > 0.0f) {
			yForce = -0.0f;
			xSide  = -1.0f;
		}

		s16 angle = CLBRoundf<s16>(mRotation.y * (65536.0f / 360.0f));
		f32 sinV = jmaSinTable[(u16)angle >> jmaSinShift];
		f32 cosV = jmaCosTable[(u16)angle >> jmaSinShift];
		f32 speed = 0.005f * mMarchSpeed;
		dir.x += (xSide * cosV + yForce * sinV) * speed;
		dir.y += yForce * speed;
		dir.z += (-xSide * sinV + yForce * cosV) * speed;
	}

	if (dir.squared() > 0.0000038146973f) {
		MsVECNormalize(&dir, &dir);
		dir.scale(500.0f);

		JGeometry::TVec3<f32> target = mPosition;
		target.add(dir);
		setGoalPath(TPathNode(target));
		walkToCurPathNode(mMarchSpeed, mTurnSpeed, 0.0f);
	}

	gpCameraShake->keepShake((EnumCamShakeMode)9, 1.0f);
	if (SMS_IsMarioTouchGround4cm() && mSpine->getTime() < 120)
		SMSRumbleMgr->start(0x16, (f32*)nullptr);
}

void TBossHanachan::execWalk(bool walk)
{
	if (walk) {
		CLBChaseGeneralConstantSpecifySpeed<f32>(
		    &mMarchSpeed, mChangeParams->mSLMaxMarchSpeed.value,
		    mChangeParams->mSLMarchAccel.value);
	} else {
		CLBChaseGeneralConstantSpecifySpeed<f32>(
		    &mMarchSpeed, 0.0f, mChangeParams->mSLMarchDecrease.value);
	}
	mTurnSpeed = mChangeParams->mSLWalkTurnSpeed.value;

	bool shouldWalk = true;
	const JGeometry::TVec3<f32>& point = unkF4.getPoint();
	JGeometry::TVec3<f32> diff(point);
	diff.sub(mPosition);

	if (diff.squared() < CLBSquared<f32>(10.0f))
		shouldWalk = false;
	if (shouldWalk)
		walkToCurPathNode(mMarchSpeed, mTurnSpeed, 0.0f);

	const JGeometry::TVec3<f32>& curPoint = unkF4.getPoint();
	JGeometry::TVec3<f32> curDiff(curPoint);
	curDiff.x -= mPosition.x;
	curDiff.y = 0.0f;
	curDiff.z -= mPosition.z;

	if (curDiff.squared() < CLBSquared<f32>(100.0f)) {
		if (!unk114.empty()) {
			unkF4 = unk114.pop();
		} else {
			goToDirLimitedNextGraphNode(90.0f);
		}
	}
}

f32 TBossHanachan::getBodyMaxRotateZ() const
{
	f32 maxRot = 0.0f;
	TBossHanachanPartsBase* const* body = mBody;
	for (int i = 0; i < 8; ++i) {
		f32 maxAbs = __fabsf(maxRot);
		f32 rot    = (*body)->mRotation.z;
		if (__fabsf(rot) > maxAbs)
			maxRot = rot;
		++body;
	}
	return maxRot;
}

bool TBossHanachan::checkFallDecideAndSetup()
{
	bool result = false;
	for (int i = 0; i < 8; ++i) {
		TBossHanachanChangeSaveParams* params = mChangeParams;
		f32* fallDecideRotateZ = &params->mSLFallDecideRotateZ.value;
		TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
		f32 absRot = body->mRotation.z;
		absRot     = absRot >= 0.0f ? absRot : -absRot;

		if (absRot > *fallDecideRotateZ) {
			emitOneTimeSandPillar_(body);
			if (body->mRotation.z > 0.0f)
				unk194 = 179.0f;
			else
				unk194 = -179.0f;

			f32 diff = body->unk13C - body->mRotation.z;
			if (!(diff >= 0.0f))
				diff = -diff;
			unk198 = mChangeParams->mSLWaveFallDownSpeed.value * diff;
			if (unk198 < mChangeParams->mSLFallDecideMinSpeed.value)
				unk198 = mChangeParams->mSLFallDecideMinSpeed.value;
			result = true;
			break;
		}
	}
	return result;
}

bool TBossHanachan::isTumbleCompletelyAllBody() const
{
	TBossHanachanPartsBase* firstBody = mBody[0];
	bool result                       = true;
	bool isTumble                     = true;
	f32 rot                           = firstBody->mRotation.z;
	if (rot != -179.0f && rot != 179.0f)
		isTumble = false;

	if (!(isTumble ? true : false)) {
		result = false;
	} else {
		for (int i = 1; i < 8; ++i) {
			if (mBody[i]->mRotation.z != rot) {
				result = false;
				break;
			}
		}
	}

	return result;
}

void TBossHanachan::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (mLiveFlag & (LIVE_FLAG_DEAD | LIVE_FLAG_UNK200))
		return;

	bool graphicsDrawn = (graphics->unk0 & 2) != 0;
	if (mLiveFlag & LIVE_FLAG_UNK40000) {
		if ((flags & 1) && graphicsDrawn) {
			if (gpMSound->gateCheck(0x6010))
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x6010, &mPosition, 0, nullptr, 0, 4);

			if (!isBossHanachanDirectorBlocked()) {
				bool appear = true;
				if (!(mLiveFlag & LIVE_FLAG_UNK100000)) {
					if (((MSModBgm*)gpMSound->unk98)->modBgm(1, 1))
						appear = false;
				}
				if (appear) {
					mLiveFlag |= LIVE_FLAG_DEAD | LIVE_FLAG_UNK40;
					gpItemManager->makeShineAppearWithDemo(
					    "シャイン（ボス用）", "ボスシャインカメラ", unk17C.x,
					    unk17C.y + mParams->mSLShineAppearOffsetY.value,
					    unk17C.z);
				}
			}
		}
		return;
	}

	const TNerveBase<TLiveActor>* latestNerve = nullptr;

	if (flags & 1) {
		if (isBossHanachanDirectorBlocked()) {
			mLinearVelocity.set(0.0f, 0.0f, 0.0f);
			mAngularVelocity.set(0.0f, 0.0f, 0.0f);

			if (graphicsDrawn
			    && mSpine->getLatestNerve()
			           == &TNerveBossHanachanDead::theNerve()
			    && !(mLiveFlag & LIVE_FLAG_UNK100000)) {
				mLiveFlag |= LIVE_FLAG_UNK100000;
				MSBgm::stopTrackBGM(1, 30);
			}
		} else {
			if (!(mLiveFlag & 0x80000) && mHitPoints == 3
			    && mMarchSpeed != 0.0f
			    && mSpine->getLatestNerve()
			           != &TNerveBossHanachanTumble::theNerve()) {
				if (unk1B8 == -1) {
					if (gpMarDirector->mState == 4)
						unk1B8 = 7200;
				} else if (unk1B8 > 0) {
					--unk1B8;
					if (unk1B8 == 0) {
						unk1B8 = 7200;
						gpMarDirector->getConsole()->startAppearBalloon(
						    0xE0006, true);
					}
				}
			}

			moveObject();
			latestNerve = mSpine->getLatestNerve();

			for (int i = 0; i < 8; ++i) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				body->unk130 = body->unk124;
				body->unk134 = body->unk128;
				body->unk138 = body->unk12C;
				body->unk124 = body->mPosition.x;
				body->unk128 = body->mPosition.y;
				body->unk12C = body->mPosition.z;
				body->unk140 = body->unk13C;
				body->unk13C = body->mRotation.z;
				body->unk148 = body->unk144;
			}

			s16 bodyRotY = CLBRoundf<s16>(
			    mBody[0]->mRotation.y * (65536.0f / 360.0f));
			CLBChaseAngleDecrease(
			    &bodyRotY,
			    CLBRoundf<s16>(mRotation.y * (65536.0f / 360.0f)), 20);
			mBody[0]->mRotation.y = bodyRotY * (360.0f / 65536.0f);

			for (int i = 1; i < 8; ++i) {
				JGeometry::TVec3<f32> diff = mSphereLink->mPoints[i - 1].mPos;
				diff.sub(mSphereLink->mPoints[i].mPos);
				mBody[i]->mRotation.y
				    = callMsWrap(getRotFromXZ(diff.x, diff.z), 0.0f, 360.0f);
			}

			mSphereLink->mAngleOffset = mBody[0]->mRotation.y;
			for (int i = 0; i < 8; ++i)
				mSphereLink->setDegreeZAndRevisionPosXZ(
				    i, mBody[i]->mRotation.z);

			JGeometry::TVec3<f32> headTarget = mPosition;
			s16 headAngle = CLBRoundf<s16>(
			    mRotation.y * (65536.0f / 360.0f));
			f32 sinHead = jmaSinTable[(u16)headAngle >> jmaSinShift];
			f32 cosHead = jmaCosTable[(u16)headAngle >> jmaSinShift];
			headTarget.x -= sinHead * mParams->mSLHeadLength.value;
			headTarget.z -= cosHead * mParams->mSLHeadLength.value;

			f32 revX;
			f32 revZ;
			BHSCalcRevisionDistXZByRotateZ(mRotation.y, mSphereLink->m14,
			                               mRotation.z, &revX, &revZ);
			headTarget.x += revX;
			headTarget.z += revZ;
			mSphereLink->moveHead(headTarget);

			for (int i = 0; i < 8; ++i) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				BHSCalcRevisionDistXZByRotateZ(
				    body->mRotation.y, mSphereLink->m14, body->mRotation.z,
				    &revX, &revZ);
				body->mPosition = mSphereLink->mPoints[i].mPos;
				body->mPosition.x -= revX;
				body->mPosition.z -= revZ;
			}

			bool isTumble
			    = latestNerve == &TNerveBossHanachanTumble::theNerve();
			f32 waveTarget = 0.0f;
			if (isTumble)
				waveTarget = getBodyMaxRotateZ();

			for (int i = 0; i < 8; ++i) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				if (!isTumble) {
					waveTarget
					    = BHSCalcCentrifugalForce(
					          body->mPosition,
					          *(JGeometry::TVec3<f32>*)&body->unk124,
					          *(JGeometry::TVec3<f32>*)&body->unk130,
					          body->mRotation.y)
					      * mChangeParams->mSLCentrifugalForce.value;
				}
				CLBChaseGeneralConstantSpecifySpeed<f32>(
				    &body->unk144, waveTarget,
				    mChangeParams->mSLCentrifugalSpeed.value);
				if (body->unk144 > 179.0f)
					body->unk144 = 179.0f;
				else if (body->unk144 < -179.0f)
					body->unk144 = -179.0f;
			}

			if (latestNerve != &TNerveBossHanachanDown::theNerve()) {
				for (int i = 0; i < 8; ++i) {
					TBossHanachanPartsBody* body
					    = (TBossHanachanPartsBody*)mBody[i];
					f32 centerGround = gpMap->checkGroundIgnoreWaterSurface(
					    body->mPosition.x, body->mPosition.y + 500.0f,
					    body->mPosition.z, &body->mGroundPlane);

					const TLiveActor* sand = body->getSandActor_();
					if (sand != nullptr) {
						JGeometry::TVec3<f32> toSand = sand->mPosition;
						toSand.sub(mPosition);
						f32 dist2 = toSand.x * toSand.x + toSand.z * toSand.z;
						if (dist2 <= CLBSquared<f32>(50.0f)) {
							body->unk120 = 0.0f;
						} else {
							f32 sandAngle = callMsWrap(
							    getRotFromXZ(toSand.x, toSand.z), -180.0f,
							    180.0f);
							f32 bossAngle
							    = callMsWrap(mRotation.y, -180.0f, 180.0f);
							f32 diff
							    = callMsWrap(sandAngle - bossAngle, -180.0f,
							             180.0f);
							f32 absDiff = __fabsf(diff);
							if (absDiff <= 15.0f || absDiff >= 165.0f) {
								body->unk120 = 0.0f;
							} else {
								f32 ratio = SMS_GetSandRiseUpRatio(sand);
								if (diff > 15.0f)
									body->unk120 = 70.0f * ratio;
								else
									body->unk120 = -70.0f * ratio;
							}
						}
					} else {
						s16 angle = CLBRoundf<s16>(
						    body->mRotation.y * (65536.0f / 360.0f));
						f32 sinV
						    = jmaSinTable[(u16)angle >> jmaSinShift];
						f32 cosV
						    = jmaCosTable[(u16)angle >> jmaSinShift];
						f32 sideX = 200.0f * cosV;
						f32 sideZ = -200.0f * sinV;

						const TBGCheckData* ground;
						f32 groundA = gpMap->checkGroundIgnoreWaterSurface(
						    body->mPosition.x + sideX,
						    body->mPosition.y + 500.0f,
						    body->mPosition.z + sideZ, &ground);
						f32 groundB = gpMap->checkGroundIgnoreWaterSurface(
						    body->mPosition.x - sideX,
						    body->mPosition.y + 500.0f,
						    body->mPosition.z - sideZ, &ground);
						f32 diffA = groundA - centerGround;
						f32 diffB = groundB - centerGround;
						if (__fabsf(diffA) < 0.001f
						    && __fabsf(diffB) < 0.001f) {
							body->unk120 = 0.0f;
						} else if (__fabsf(diffA) > __fabsf(diffB)) {
							body->unk120
							    = matan(200.0f, diffA)
							      * (360.0f / 65536.0f);
						} else {
							body->unk120
							    = -matan(200.0f, diffB)
							      * (360.0f / 65536.0f);
						}
					}
				}

				if (latestNerve == &TNerveBossHanachanGetUp::theNerve()) {
					mHead->calcRotateZWhenGetUp_();
					mRotation.z = mHead->mRotation.z;
					for (int i = 0; i < 8; ++i)
						mBody[i]->calcRotateZWhenGetUp_();
				} else {
					f32 maxRot = getBodyMaxRotateZ();
					const TNerveBase<TLiveActor>* curNerve
					    = mSpine->getLatestNerve();
					f32 waveDecay
					    = mChangeParams->mSLWaveDecrease.value
					      * 0.008333334f;
					f32 bodyLength = mParams->mSLBodyLength.value;
					f32 invLenSq   = 1.0f / (bodyLength * bodyLength);
					f32 decayScale = 1.0f / (1.0f + waveDecay);
					f32 oldScale   = 1.0f - waveDecay;
					f32 velocityTerm
					    = 0.008333334f
					      * (mChangeParams->mSLWaveVelocity.value
					         * mChangeParams->mSLWaveVelocity.value);

					for (int i = 0; i < 8; ++i) {
						TBossHanachanPartsBody* body
						    = (TBossHanachanPartsBody*)mBody[i];
						if ((body->mRotation.z == 179.0f
						     || body->mRotation.z == -179.0f)
						    && body->mRotation.z == maxRot)
							continue;

						f32 prevRot;
						if (i == 0)
							prevRot
							    = ((TBossHanachanPartsBody*)mBody[i + 1])
							          ->unk13C;
						else
							prevRot
							    = ((TBossHanachanPartsBody*)mBody[i - 1])
							          ->unk13C;

						f32 nextRot;
						if (i == 7)
							nextRot
							    = ((TBossHanachanPartsBody*)mBody[i - 1])
							          ->unk13C;
						else
							nextRot
							    = ((TBossHanachanPartsBody*)mBody[i + 1])
							          ->unk13C;

						f32 curRot      = body->unk13C;
						f32 neighborAcc = prevRot + nextRot - 2.0f * curRot;
						f32 target
						    = decayScale
						      * (2.0f * curRot - body->unk140 * oldScale
						         + velocityTerm
						               * (invLenSq * neighborAcc
						                  + body->unk148));
						CLBChaseGeneralConstantSpecifySpeed<f32>(
						    &body->mRotation.z, target,
						    mChangeParams->mSLRotateZLeanSpeed.value);

						bool sandActorFound = false;
						if (curNerve
						    == &TNerveBossHanachanGraphWander::theNerve()) {
							if (body->getSandActor_() != nullptr) {
								sandActorFound = true;
								if (body->unk120 != 0.0f) {
									f32 targetRot = 179.0f;
									f32 speed
									    = body->unk120 * mMarchSpeed
									      * mChangeParams->mSLSandSlopeForce
									            .value;
									if (body->unk120 < 0.0f)
										targetRot = -179.0f;
									CLBChaseGeneralConstantSpecifySpeed<f32>(
									    &body->mRotation.z, targetRot, speed);
								}
							} else {
								CLBChaseGeneralConstantSpecifySpeed<f32>(
								    &body->mRotation.z, body->unk120,
								    mChangeParams->mSLRotateZRestorationSpeed
								        .value);
							}
						} else if (curNerve
						           == &TNerveBossHanachanTumble::theNerve()) {
							CLBChaseGeneralConstantSpecifySpeed<f32>(
							    &body->mRotation.z, unk194, unk198);
						}

						if (body->mRotation.z > 179.0f)
							body->mRotation.z = 179.0f;
						else if (body->mRotation.z < -179.0f)
							body->mRotation.z = -179.0f;

						if (curNerve
						    == &TNerveBossHanachanGraphWander::theNerve()) {
							if (mSpine->getTime()
							        >= mChangeParams->mSLNotFallDownFrames
							               .value
							    && !sandActorFound) {
								f32 maxNotSand
								    = mChangeParams->mSLMaxRotateZNotSand.value;
								if (body->mRotation.z < -maxNotSand) {
									CLBChaseGeneralConstantSpecifySpeed<f32>(
									    &body->mRotation.z, -maxNotSand, 15.0f);
								} else if (body->mRotation.z > maxNotSand) {
									CLBChaseGeneralConstantSpecifySpeed<f32>(
									    &body->mRotation.z, maxNotSand, 15.0f);
								}
							}
						}
					}

					f32 diffMax = mChangeParams->mSLDiffMaxRotateZ.value;
					for (int i = 1; i < 8; ++i) {
						TBossHanachanPartsBody* prev
						    = (TBossHanachanPartsBody*)mBody[i - 1];
						TBossHanachanPartsBody* body
						    = (TBossHanachanPartsBody*)mBody[i];
						f32 diff = __fabsf(prev->mRotation.z - body->mRotation.z);
						if (diff > diffMax) {
							if (body->mRotation.z < prev->mRotation.z)
								body->mRotation.z = prev->mRotation.z - diffMax;
							else
								body->mRotation.z = prev->mRotation.z + diffMax;
						}
					}
				}
			}

			bool threwMario = false;
			TWaterHitActor* headHit
			    = ((TBossHanachanPartsHead*)mHead)->mWaterHit;
			if (headHit->unk68 >= 1)
				--headHit->unk68;
			for (int i = 0; i < headHit->mColCount; ++i) {
				if (headHit->mCollisions[i]->mActorType == 0x80000001) {
					throwMario_(headHit);
					threwMario = true;
					break;
				}
			}

			for (int i = 0; i < 8; ++i) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				TWaterHitActor* bodyHit = body->mWaterHit;
				if (bodyHit->unk68 >= 1)
					--bodyHit->unk68;
				if (!threwMario) {
					for (int j = 0; j < bodyHit->mColCount; ++j) {
						if (bodyHit->mCollisions[j]->mActorType
						    == 0x80000001) {
							throwMario_(bodyHit);
							threwMario = true;
							break;
						}
					}
				}

				for (int j = 0; j < 2; ++j) {
					TFootHitActor* foot = body->mFeet[j];
					if (foot->unk68 >= 1)
						--foot->unk68;
					if (!threwMario) {
						for (int k = 0; k < foot->mColCount; ++k) {
							if (foot->mCollisions[k]->mActorType
							    == 0x80000001) {
								throwMario_(foot);
								threwMario = true;
								break;
							}
						}
					}
				}
			}

			if (graphicsDrawn
			    && latestNerve == &TNerveBossHanachanDead::theNerve()) {
				if (gpMSound->gateCheck(0x6010))
					MSoundSESystem::MSoundSE::startSoundActor(
					    0x6010, &mPosition, 0, nullptr, 0, 4);
				if (!(mLiveFlag & LIVE_FLAG_UNK100000))
					((MSModBgm*)gpMSound->unk98)->modBgm(1, 1);
			}
		}

		mHead->moveMapCollision_();
		for (int i = 0; i < 8; ++i)
			mBody[i]->moveMapCollision_();
	}

	bool doCalc = (flags & 2) != 0;
	if (doCalc) {
		gpTargetArrow->unk14 = 0;
		if (!isBossHanachanDirectorBlocked()) {
			bool attackHits = false;
			const TNerveBase<TLiveActor>* curNerve = mSpine->getLatestNerve();
			if (curNerve == &TNerveBossHanachanGraphWander::theNerve()
			    && mMarchSpeed > 0.001f)
				attackHits = true;

			{
				TBossHanachanPartsHead* head
				    = (TBossHanachanPartsHead*)mHead;
				MtxPtr mtx = head->mCenterJointMtx;
				JGeometry::TVec3<f32> hitPos = makeVec3(
				    mtx[0][3],
				    mtx[1][3] - mParams->mSLHeadHitOffsetY.value,
				    mtx[2][3]);
				head->mWaterHit->mPosition = hitPos;
				if (attackHits) {
					if (!head->mWaterHit->checkHitFlag(0x80000000)) {
						head->mWaterHit->onHitFlag(0x80000000);
						head->mMapCollision->remove();
					}
				} else if (head->mWaterHit->checkHitFlag(0x80000000)) {
					head->mWaterHit->offHitFlag(0x80000000);
					head->mMapCollision->setUpTrans(hitPos);
				}
			}

			for (int i = 0; i < 8; ++i) {
				TBossHanachanPartsBody* body
				    = (TBossHanachanPartsBody*)mBody[i];
				MtxPtr mtx = body->mCenterJointMtx;
				body->unk154 = mtx[0][3];
				body->unk158 = mtx[1][3];
				body->unk15C = mtx[2][3];
				body->mWaterHit->mPosition.x = body->unk154;
				body->mWaterHit->mPosition.y
				    = body->unk158 - mParams->mSLBodyHitOffsetY.value;
				body->mWaterHit->mPosition.z = body->unk15C;

				if (attackHits) {
					if (!body->mWaterHit->checkHitFlag(0x80000000)) {
						body->mWaterHit->onHitFlag(0x80000000);
						body->mMapCollision->remove();
					}
				} else if (body->mWaterHit->checkHitFlag(0x80000000)) {
					body->mWaterHit->offHitFlag(0x80000000);
					body->mMapCollision->setUpTrans(body->mWaterHit->mPosition);
				}

				for (int j = 0; j < 2; ++j) {
					TFootHitActor* foot = body->mFeet[j];
					MtxPtr footMtx      = foot->unk6C;
					foot->mPosition.x   = footMtx[0][3];
					foot->mPosition.y
					    = footMtx[1][3] - mParams->mSLFootHitOffsetY.value;
					foot->mPosition.z = footMtx[2][3];
					if (attackHits)
						foot->onHitFlag(0x80000000);
					else
						foot->offHitFlag(0x80000000);
				}
			}

			emitParticle_();
			emitCamShake_();
		}
	}

	if (doCalc) {
		if (mLiveFlag & LIVE_FLAG_UNK10000) {
			if (mEffectActor->curAnmEndsNext(0, nullptr))
				offLiveFlag(LIVE_FLAG_UNK10000);
		}

		if (!isBossHanachanDirectorBlocked())
			changeAnmRateAndFrameUpdate_();

		((TNpcInbetween*)mHead->mPalFrame)->execMotionBlend(mHead->mMActor);
		for (int i = 0; i < 8; ++i)
			((TNpcInbetween*)mBody[i]->mPalFrame)
			    ->execMotionBlend(mBody[i]->mMActor);

		JGeometry::TVec3<f32> headPos = mPosition;
		CalcRevisionPosByRotateZ(mRotation, mParams->mSLHeadPlusYByRotateZ.value,
		                         &headPos);
		CLBCalcRotateZXYTranslateMatrix(
		    mHead->mMActor->getModel()->getBaseTRMtx(), mRotation, headPos);
		mHead->mMActor->calc();

		for (int i = 0; i < 8; ++i) {
			TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
			JGeometry::TVec3<f32> pos   = body->mPosition;
			CalcRevisionPosByRotateZ(body->mRotation,
			                         mParams->mSLBodyPlusYByRotateZ.value, &pos);
			Mtx mtx;
			CLBCalcRotateZXYTranslateMatrix(mtx, body->mRotation, pos);
			PSMTXCopy(mtx, body->mMActor->getModel()->getBaseTRMtx());
			body->mMActor->calc();
		}

		if (!isBossHanachanDirectorBlocked()) {
			const TNerveBase<TLiveActor>* curNerve = mSpine->getLatestNerve();
			if (curNerve == &TNerveBossHanachanTumble::theNerve()
			    || curNerve == &TNerveBossHanachanDown::theNerve()) {
				MtxPtr mtx = ((TBossHanachanPartsBody*)mBody[unk174])
				                 ->mCenterJointMtx;
				JGeometry::TVec3<f32> arrowPos
				    = makeVec3(mtx[0][3], mtx[1][3] + 400.0f, mtx[2][3]);
				gpTargetArrow->setPos(arrowPos);
				gpTargetArrow->unk14 = 1;
			}
		}
	}

	if (flags & 0x200) {
		mHead->entryCircleShadow_();
		mHead->setDamageFog_(graphics);
		mHead->drawObject(graphics);
		for (int i = 0; i < 8; ++i) {
			TBossHanachanPartsBase* body = mBody[i];
			body->entryCircleShadow_();
			body->setDamageFog_(graphics);
			body->drawObject(graphics);
		}
	}

	if (flags & 4) {
		mHead->mMActor->viewCalc();
		for (int i = 0; i < 8; ++i)
			mBody[i]->mMActor->viewCalc();
	}

	if (mLiveFlag & LIVE_FLAG_UNK10000)
		mEffectActor->perform(flags, graphics);
}

void TBossHanachan::moveObject()
{
	updateSquareToMario();
	unk188.set(mLinearVelocity);
	TLiveActor::moveObject();

	if (mSpine->getLatestNerve() != &TNerveBossHanachanGetUp::theNerve()) {
		CLBChaseDecrease(&mRotation.z, mBody[0]->mRotation.z, 0.04f, 0.0f);
	}

	mHead->mPosition    = mPosition;
	mHead->mRotation    = mRotation;
	mHead->mGroundPlane = mGroundPlane;
}

void TBossHanachan::bind()
{
	if (checkLiveFlag(LIVE_FLAG_UNK10))
		return;

	if (mBinder != nullptr) {
		mBinder->bind(this);
		return;
	}

	JGeometry::TVec3<f32> oldPosition = mPosition;
	JGeometry::TVec3<f32> next        = mPosition;
	next.add(mLinearVelocity);
	next.add(mVelocity);

	mVelocity.y -= getGravityY();
	if (mVelocity.y < TLiveActor::mVelocityMinY)
		mVelocity.y = TLiveActor::mVelocityMinY;

	unk17C = next;
	f32 revX;
	f32 revZ;
	BHSCalcRevisionDistXZByRotateZ(mRotation.y, mSphereLink->m14,
	                               mRotation.z, &revX, &revZ);
	unk17C.x += revX;
	unk17C.z += revZ;

	mGroundHeight = gpMap->checkGroundIgnoreWaterSurface(
	    unk17C.x, unk17C.y + mHeadHeight, unk17C.z, &mGroundPlane);
	mGroundHeight += 1.0f;

	if (unk17C.y <= mGroundHeight + 0.05f) {
		if (mGroundPlane && !mGroundPlane->isIllegalData()) {
			offLiveFlag(LIVE_FLAG_AIRBORNE);
			mVelocity.set(0.0f, 0.0f, 0.0f);
			unk17C.y = mGroundHeight;
		}
	} else {
		onLiveFlag(LIVE_FLAG_AIRBORNE);
	}

	gpMap->isTouchedOneWallAndMoveXZ(&unk17C.x, unk17C.y + mHeadHeight,
	                                 &unk17C.z, mBodyRadius);

	JGeometry::TVec3<f32> moved = unk17C;
	moved.sub(oldPosition);
	JGeometry::TVec3<f32> oldToCur = oldPosition;
	oldToCur.sub(mPosition);
	mLinearVelocity = oldToCur;
	mLinearVelocity.add(moved);
}

void TBossHanachan::kill() { }

static void CalcRevisionPosByRotateZ(const JGeometry::TVec3<f32>& rot,
                                     f32 scale, Vec* pos)
{
	f32 absZ = __fabsf(rot.z);
	pos->y += scale * absZ;
	if (absZ > 90.0f) {
		f32 mag = 7.0f * (absZ - 90.0f);
		if (rot.z > 0.0f)
			mag = -mag;

		s16 angle = CLBRoundf<s16>((65536.0f / 360.0f) * rot.y);
		f32 sinV = jmaSinTable[(u16)angle >> jmaSinShift];
		f32 cosV = jmaCosTable[(u16)angle >> jmaSinShift];
		f32 zero = 0.0f;
		pos->x += mag * cosV + zero * sinV;
		pos->z += -mag * sinV + zero * cosV;
	}
}

void TBossHanachan::throwMario_(THitActor* hit_actor)
{
	JGeometry::TVec3<f32> throwVec = *gpMarioPos - hit_actor->mPosition;

	f32 throwPower;
	if (throwVec.squared() <= 0.0000038146973f) {
		throwVec.set(0.0f, 1.0f, 0.0f);
		throwPower = mMarchSpeed * mChangeParams->mSLThrowTotalPower.value;
	} else {
		s16 oldShort = CLBRoundf<s16>(
		    getRotFromXZ(unk188.x, unk188.z) * (65536.0f / 360.0f));
		s16 newShort = CLBRoundf<s16>(
		    getRotFromXZ(throwVec.x, throwVec.z) * (65536.0f / 360.0f));
		s16 diff     = oldShort - newShort;
		if (diff < 0)
			diff = -diff;

		throwPower = mMarchSpeed * mChangeParams->mSLThrowTotalPower.value;
		f32 ratio  = 1.0f - (diff * (1.0f / 32768.0f));
		f32 scale  = ratio * mChangeParams->mSLThrowMoveDirPower.value;
		JGeometry::TVec3<f32> add = unk188;
		add.scale(scale);
		throwVec.add(add);
		throwVec.y = mChangeParams->mSLThrowVecY.value;
	}

	if (throwPower > mChangeParams->mSLThrowSpeedMax.value)
		throwPower = mChangeParams->mSLThrowSpeedMax.value;
	else if (throwPower < mChangeParams->mSLThrowSpeedMin.value)
		throwPower = mChangeParams->mSLThrowSpeedMin.value;

	SMS_SendMessageToMario(mHead, HIT_MESSAGE_ATTACK);
	SMS_SendMessageToMario(mHead, HIT_MESSAGE_UNK7);
	SMS_ThrowMario(throwVec, throwPower);
	((TBossHanachanPartsHead*)mHead)->mWaterHit->onWaterHitCounter();
}

void TBossHanachan::init(TLiveManager* manager)
{
	mManager = manager;
	manager->manageActor(this);

	mMActorKeeper = new TMActorKeeper(manager, 10);
	mEffectActor  = mMActorKeeper->createMActor(cSandPillarModelName, 0);

	TBossHanachanManager* bossManager = (TBossHanachanManager*)manager;
	mParams                       = bossManager->mCommonParams;
	mChangeParams                 = bossManager->mChangeParams[0];
	mBodyScale                    = 1.0f;
	mBodyRadius                   = 350.0f;
	mWallRadius                   = mBodyRadius;
	mHeadHeight                   = 500.0f;
	mMarchSpeed                   = 0.0f;
	mGravity                      = 2.0f;
	mHitPoints                    = 3;
	mScaledBodyRadius             = 0.0f;
	mLiveFlag |= 0x1008;

	mSpine->initWith(&TNerveBossHanachanGraphWander::theNerve());
	unk124->mPrevIdx = -1;
	goToShortestNextGraphNode();

	initHitActor(0x08000014, 0, 0, 0.0f, 0.0f, 0.0f, 0.0f);
	onHitFlag(1);

	for (int i = 0; i < 8; ++i) {
		mBody[i] = new TBossHanachanPartsBody(this, "ボスハナチャンの体");
		((TBossHanachanPartsBody*)mBody[i])->unk114 = i;
	}
	mHead = new TBossHanachanPartsHead(this, "ボスハナチャンの頭");
	mMActor = mHead->mMActor;

	unk17C = mPosition;
	JGeometry::TVec3<f32> linkPos = unk17C;
	s16 angle = CLBRoundf<s16>(mRotation.y * (65536.0f / 360.0f));
	f32 sinV = jmaSinTable[(u16)angle >> jmaSinShift];
	f32 cosV = jmaCosTable[(u16)angle >> jmaSinShift];
	linkPos.x -= sinV * mParams->mSLBodyLength.value;
	linkPos.z -= cosV * mParams->mSLBodyLength.value;

	mSphereLink = new TSphereLink(8, linkPos, mParams->mSLBodyLength.value,
	                              mParams->mSLBodyAttackRadius.value, 0.2f,
	                              -2.0f, -3.5f, mRotation.y);

	mHead->mPosition    = mPosition;
	mHead->mRotation    = mRotation;
	mHead->mGroundPlane = mGroundPlane;

	for (int i = 0; i < 8; ++i) {
		TSpherePoint& point = mSphereLink->mPoints[i + 1];
		TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
		body->mPosition              = point.mPos;
		body->unk124                 = point.mPos.x;
		body->unk128                 = point.mPos.y;
		body->unk12C                 = point.mPos.z;
		body->unk130                 = body->unk124;
		body->unk134                 = body->unk128;
		body->unk138                 = body->unk12C;
		body->mRotation              = mRotation;
	}

	setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_OFF);

	JGeometry::TVec3<f32> headPos = mPosition;
	CalcRevisionPosByRotateZ(mRotation, mParams->mSLHeadPlusYByRotateZ.value,
	                         &headPos);
	CLBCalcRotateZXYTranslateMatrix(
	    mHead->mMActor->getModel()->getBaseTRMtx(), mRotation, headPos);
	mHead->mMActor->calc();

	for (int i = 0; i < 8; ++i) {
		TBossHanachanPartsBody* body = (TBossHanachanPartsBody*)mBody[i];
		JGeometry::TVec3<f32> pos   = body->mPosition;
		CalcRevisionPosByRotateZ(body->mRotation,
		                         mParams->mSLBodyPlusYByRotateZ.value, &pos);
		Mtx mtx;
		CLBCalcRotateZXYTranslateMatrix(mtx, body->mRotation, pos);
		PSMTXCopy(mtx, body->mMActor->getModel()->getBaseTRMtx());
		body->mMActor->calc();
	}

	TIdxGroupObj* group
	    = JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ");
	mHead->initMapCollisionAndHitActor_(group);
	for (int i = 0; i < 8; ++i) {
		((TBossHanachanPartsBody*)mBody[i])
		    ->initMapCollisionAndHitActor_(group);
		((TBossHanachanPartsBody*)mBody[i])->initFootHitActor_(group);
	}
}

void TBossHanachan::setRandomWeakBodyIndex()
{
	unk174 = MsRandF() * 8.0f;
}

TBossHanachan::TBossHanachan(const char* name)
    : TSpineEnemy(name)
    , unk174(0)
    , mSphereLink(nullptr)
    , unk17C(0.0f, 0.0f, 0.0f)
    , unk188(0.0f, 0.0f, 0.0f)
    , unk194(0.0f)
    , unk198(0.0f)
    , mEffectActor(nullptr)
    , mEffectPos(0.0f, 0.0f, 0.0f)
    , unk1AC(0.0f, 0.0f, 0.0f)
    , unk1B8(-1)
    , mParams(nullptr)
    , mChangeParams(nullptr)
{
	setRandomWeakBodyIndex();
}

BOOL TBossHanachanManager::hasMapCollision() const { return TRUE; }

void TBossHanachanManager::clipEnemies(JDrama::TGraphics* graphics)
{
	TBossHanachanCommonSaveParams* params = mCommonParams;
	f32 radius                            = params->mSLViewClipRadius.value;
	f32 far                               = params->mSLViewClipFar.value;
	clipActorsAux(graphics, far, radius);
}

void TBossHanachanManager::loadAfter()
{
	J3DMaterialTable* table = gpMapObjManager->unkC0;
	int index              = table->getTextureName()->getIndex(cSandTextureName);
	ResTIMG* texture       = table->getTexture()->getResTIMG(index);

	for (int i = 0; i < 2; ++i) {
		SDLModelData* data = getModelDataKeeper()->getNthData(i);
		SMS_ChangeTextureAll(data->getModelData(), cDummyTextureName, *texture);
	}
}

void TBossHanachanManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "hanabody_model.bmd", 0x10300000, 0 },
		{ "hanahead_model.bmd", 0x10100000, 0 },
		{ cSandPillarModelName, 0x10010000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

TBossHanachanManager::TBossHanachanManager(const char* name)
    : TEnemyManager(name)
{
	static const char* sCommonSaveFileName = "/enemy/bosshanachanCommon.prm";
	static const char* sChangeSaveFileName[] = {
		"/enemy/bosshanachan0.prm",
		"/enemy/bosshanachan1.prm",
		"/enemy/bosshanachan2.prm",
	};

	mCommonParams = new TBossHanachanCommonSaveParams(sCommonSaveFileName);
	for (int i = 0; i < 3; ++i)
		mChangeParams[i]
		    = new TBossHanachanChangeSaveParams(sChangeSaveFileName[i]);
}
