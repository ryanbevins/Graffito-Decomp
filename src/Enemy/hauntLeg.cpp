#define JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#define JGEOMETRY_DRAWUTIL_OWNER_HELPERS
#include <Enemy/HauntLeg.hpp>
#undef JGEOMETRY_TVEC3_SUB_OUT_OF_LINE
#include <Enemy/Conductor.hpp>
#include <Enemy/Graph.hpp>
#include <Enemy/Spider.hpp>
#include <Enemy/Walker.hpp>
#include <Strategic/Spine.hpp>
#include <Strategic/Strategy.hpp>
#include <Map/MapData.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <Strategic/ObjModel.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>

// rogue includes needed for matching sinit + infectious strings/rodata
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static THauntLeg* gpCurHauntLeg;

BOOL HauntLegCallback(J3DNode* node, int when);

static const char* hauntleg_bastable[] = {
	nullptr,
	nullptr,
	nullptr,
};

DEFINE_NERVE(TNerveHauntLegHaunt, TLiveActor)
{
	THauntLeg* self = (THauntLeg*)spine->getBody();

	if (spine->getTime() == 0) {
		f32 speed = 10.0f;
		self->mJumpVelocity = self->calcVelocityToJumpToY(
		    *(const JGeometry::TVec3<f32>*)((u8*)self->mTarget + 0x10),
		    speed, self->getGravityY());
		self->setVelocity(self->mJumpVelocity);
		self->mPosition.y += speed;
		self->onLiveFlag(LIVE_FLAG_AIRBORNE);
		self->unk199 = true;
	} else {
		if (!self->isAirborne()) {
			if (self->unk199) {
				self->setVelocity(self->mJumpVelocity);
				self->mPosition.y += 10.0f;
				self->unk199 = false;

				JGeometry::TVec3<f32> diff = self->mPosition;
				diff.sub(*(const JGeometry::TVec3<f32>*)((u8*)self->mTarget
				                                        + 0x10));
				f32 distSq = diff.squared();
				f32 dist;
				if (distSq <= 0.0f) {
					dist = distSq;
				} else {
					dist = JGeometry::TUtil<f32>::sqrt(distSq);
				}

				if (dist < 200.0f) {
					if (((TTakeActor*)self->mTarget)->getHolder() == nullptr) {
						if (self->mTarget->receiveMessage(self, 4)) {
							self->mHeldObject
							    = (TTakeActor*)self->mTarget;
							self->unk198 = true;
						}
					}
				}
			} else {
				self->unk1AC = 0.0f;
				spine->pushAfterCurrent(
				    &TNerveWalkerGraphWander::theNerve());
				return TRUE;
			}
		}
	}

	if (self->isAirborne()) {
		if (self->unk199) {
			self->unk1AC
			    = MsClamp(self->unk1AC + 2.0f, 0.0f, 180.0f);
		} else {
			self->unk1AC
			    = MsClamp(self->unk1AC + 2.0f, 0.0f, 360.0f);
		}
	}

	return FALSE;
}

MtxPtr THauntLeg::getTakingMtx()
{
	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		TRotation3f local;
		f32 px;
		f32 py;
		f32 pz;
		pz = mPosition.z;
		py = mPosition.y;
		px = mPosition.x;
		local.identity33();
		local.mMtx[0][3] = px;
		local.mMtx[1][3] = py;
		local.mMtx[2][3] = pz;
		PSMTXCopy(local.mMtx, mMActor->getModel()->getBaseTRMtx());
		return mMActor->getModel()->getBaseTRMtx();
	}
	return (MtxPtr)((u8*)mMActor->getModel()->getAnmMtx(0) + 0x60);
}

const char** THauntLeg::getBasNameTable() const { return hauntleg_bastable; }

bool THauntLeg::isCollidMove(THitActor* other)
{
	if (mSpine->getCurrentNerve() != &TNerveHauntLegHaunt::theNerve()
	    && !unk198 && !checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		u32 type = other->getActorType() & 0xFFFF0000;
		if (type == 0x20000000 || type == 0x40000000) {
			TTakeActor* otherTake = (TTakeActor*)other;
			if (otherTake->getHolder() == nullptr || other != mTarget) {
				mTarget = other;
				mSpine->setNext(&TNerveHauntLegHaunt::theNerve());
			}
			return false;
		}
	}
	return false;
}

void THauntLeg::attackToMario()
{
	JGeometry::TVec3<f32> tmp;
	updateSquareToMario();
	if (mDistToMarioSquared < 10000.0f) {
		sendAttackMsgToMario();
	}
}

void THauntLeg::setDeadAnm()
{
	if (mTarget != nullptr) {
		mTarget->receiveMessage(this, HIT_MESSAGE_UNK6);
		mHolder     = nullptr;
		mHeldObject = nullptr;
	}
	mHauntedObject->onHitFlag(HIT_FLAG_NO_COLLISION);
}

void THauntLeg::setRunAnm() { setBckAnm(1); }

void THauntLeg::setWalkAnm() { setBckAnm(1); }

void THauntLeg::setWaitAnm() { setBckAnm(2); }

void THauntLeg::setGenerateAnm() { setBckAnm(0); }

void THauntLeg::calcRootMatrix()
{
	gpCurHauntLeg = this;

	if (isEaten())
		return;

	{
		MtxPtr m = getModel()->getBaseTRMtx();
		// Copy mScaling into model base TR matrix slot at 0x14
		// (compiler emits stw of three words, overwriting m[1][1..3])
		m[1][1] = mScaling.x; // 0x14
		m[1][2] = mScaling.y; // 0x18
		m[1][3] = mScaling.z; // 0x1C
	}

	MtxPtr anmMtx = (MtxPtr)((u8*)getModel() + 0x20);

	if (getWalker()->unk2C->unk10 > 0.0f && unk138 != nullptr) {
		JGeometry::TVec3<f32> dir(0.0f, 1.0f, 0.0f);
		JGeometry::TVec3<f32> normal = unk138->getNormal();
		JGeometry::TVec3<f32> sideways;
		sideways.cross(normal, dir);
		MsVECNormalize(&sideways, &sideways);

		dir.cross(sideways, normal);
		MsVECNormalize(&dir, &dir);

		anmMtx[0][0] = sideways.x;
		anmMtx[1][0] = sideways.y;
		anmMtx[2][0] = sideways.z;

		anmMtx[0][1] = normal.x;
		anmMtx[1][1] = normal.y;
		anmMtx[2][1] = normal.z;

		anmMtx[0][2] = dir.x;
		anmMtx[1][2] = dir.y;
		anmMtx[2][2] = dir.z;

		anmMtx[0][3] = 0.0f;
		anmMtx[1][3] = 0.0f;
		anmMtx[2][3] = 0.0f;

		f32 angle = (1.0f - getWalker()->unk2C->unk10) * 90.0f;
		f32 s     = JMASin(angle);
		f32 c     = JMACos(angle);

		Mtx tilt;
		tilt[0][0] = 1.0f;
		tilt[0][1] = 0.0f;
		tilt[0][2] = 0.0f;
		tilt[0][3] = 0.0f;
		tilt[1][0] = 0.0f;
		tilt[1][1] = c;
		tilt[1][2] = -s;
		tilt[1][3] = 0.0f;
		tilt[2][0] = 0.0f;
		tilt[2][1] = s;
		tilt[2][2] = c;
		tilt[2][3] = 0.0f;

		PSMTXConcat(anmMtx, tilt, anmMtx);
	} else {
		JGeometry::TVec3<f32> dir(JMASin(mRotation.y), 0.0f,
		                          JMACos(mRotation.y));
		JGeometry::TVec3<f32> normal = mGroundPlane->getNormal();

		JGeometry::TVec3<f32> sideways;
		sideways.cross(normal, dir);
		MsVECNormalize(&sideways, &sideways);

		dir.cross(sideways, normal);
		MsVECNormalize(&dir, &dir);

		anmMtx[0][0] = sideways.x;
		anmMtx[1][0] = sideways.y;
		anmMtx[2][0] = sideways.z;

		anmMtx[0][1] = normal.x;
		anmMtx[1][1] = normal.y;
		anmMtx[2][1] = normal.z;

		anmMtx[0][2] = dir.x;
		anmMtx[1][2] = dir.y;
		anmMtx[2][2] = dir.z;
	}

	anmMtx[0][3] = mPosition.x;
	anmMtx[1][3] = mPosition.y;
	anmMtx[2][3] = mPosition.z;

	if (checkLiveFlag(LIVE_FLAG_CLIPPED_OUT)) {
		*(Vec*)&mHauntedObject->mPosition = *(Vec*)&mPosition;
	} else {
		MtxPtr m = (MtxPtr)((u8*)getModel()->getAnmMtx(0) + 0x60);
		mHauntedObject->mPosition.x = m[0][3];
		mHauntedObject->mPosition.y = m[1][3];
		mHauntedObject->mPosition.z = m[2][3];
	}

	for (u16 i = 0; i < mHauntedObject->mColCount; ++i) {
		// loop body intentionally empty (matches asm bdnz with empty body)
	}
}

void THauntLeg::reset()
{
	mTarget = nullptr;
	unk198  = false;
	unk199  = true;
	TWalkerEnemy::reset();
}

void THauntLeg::setMActorAndKeeper()
{
	mMActorKeeper = new TMActorKeeper(mManager, 1);
	mMActor       = mMActorKeeper->createMActor("hauntleg.bmd", 3);
}

void THauntLeg::init(TLiveManager* manager)
{
	TWalkerEnemy::init(manager);
	mActorType = 0x10000025;
	unk150     = 0x3A;
	unk64 |= 0x60000000;
	((TWalker*)mBinder)->setMode(1);
	unk130 = 2;
	getMActor()->setJointCallback(1, &HauntLegCallback);

	mHauntedObject = new THauntedObject("ハントオブジェクト");

	JDrama::TNameRefGen::search<TIdxGroupObj>("敵グループ")
	    ->getChildren()
	    .push_back(mHauntedObject);

	mHauntedObject->initHitActor(0x10000025, 2, 0x80000000, 30.0f * mBodyScale,
	                             30.0f * mBodyScale, 30.0f * mBodyScale,
	                             30.0f * mBodyScale);
	mHauntedObject->mOwner = this;
}

BOOL THauntedObject::receiveMessage(THitActor* sender, u32 message)
{
	if (message <= 1) {
		mOwner->kill();
		return TRUE;
	}
	if (message == 0xF) {
		return TRUE;
	}
	return FALSE;
}

void THauntLegManager::createModelData()
{
	static TModelDataLoadEntry entry[] = {
		{ "hauntleg.bmd", 0x10220000, 0 },
		{ nullptr, 0, 0 },
	};
	createModelDataArray(entry);
}

void THauntLegManager::initSetEnemies()
{
	static const GXColorS10 tevColorData1[8] = {
		{ 0x0000, 0x0000, 0x0078, 0x00FF },
		{ 0x0078, 0x0000, 0x0000, 0x00FF },
		{ 0x0000, 0x0078, 0x0000, 0x00FF },
		{ 0x0078, 0x0078, 0x0000, 0x00FF },
		{ 0x0078, 0x0000, 0x0078, 0x00FF },
		{ 0x0064, 0x00C8, 0x0000, 0x00FF },
		{ 0x0000, 0x0064, 0x00C8, 0x00FF },
		{ 0x00C8, 0x0064, 0x0096, 0x00FF },
	};
	static const GXColorS10 tevColorData2[8] = {
		{ 0x0000, 0x0000, 0x00FA, 0x00FF },
		{ 0x00FA, 0x0000, 0x0000, 0x00FF },
		{ 0x0000, 0x00FA, 0x0000, 0x00FF },
		{ 0x00FA, 0x00FA, 0x0000, 0x00FF },
		{ 0x00FA, 0x0000, 0x00FA, 0x00FF },
		{ 0x0096, 0x00FA, 0x0000, 0x00FF },
		{ 0x0000, 0x0096, 0x00FA, 0x00FF },
		{ 0x00FA, 0x0096, 0x00C8, 0x00FF },
	};

	int tevIdx = 0;
	for (int i = 0; i < mObjNum; ++i) {
		TGraphWeb* graph = gpConductor->getGraphByName("main");
		THauntLeg* enemy = (THauntLeg*)unk18[i];

		Vec pt;
		TMsRange<s32> nodeRange(0, graph->getNodeNum());
		int nodeIdx = nodeRange.rand();
		graph->getGraphNode(nodeIdx).getPoint(&pt);

		*(Vec*)&enemy->mPosition = pt;
		enemy->mPosition.y += 5.0f;
		enemy->onLiveFlag(LIVE_FLAG_AIRBORNE);
		enemy->reset();

		for (u16 j = 0;
		     j
		     < enemy->getMActor()->getModel()->getModelData()->getMaterialNum();
		     ++j) {
			SMS_InitPacket_TwoTevColor(
			    ((THauntLeg*)unk18[i])->getMActor()->getModel(), j, GX_TEVREG0,
			    &tevColorData1[tevIdx], GX_TEVREG1, &tevColorData2[tevIdx]);
		}

		++tevIdx;
		if (tevIdx >= 8)
			tevIdx = 0;
	}
}

TSpineEnemy* THauntLegManager::createEnemyInstance() { return new THauntLeg; }

void THauntLegManager::load(JSUMemoryInputStream& stream)
{
	TSmallEnemyManager::load(stream);
	unk38 = new TWalkerEnemyParams("/enemy/hauntLeg.prm");
}

THauntLegManager::THauntLegManager(const char* name)
    : TSmallEnemyManager(name)
{
	gpCurHauntLeg = nullptr;
}

BOOL HauntLegCallback(J3DNode* node, int when)
{
	if (when == 0) {
		THauntLeg* self = gpCurHauntLeg;
		if (self == nullptr
		    || !((self->mSpine->getCurrentNerve()
		          == &TNerveHauntLegHaunt::theNerve())
		             ? true
		             : false))
			return TRUE;

		f32 angle      = self->unk1AC * 182.04445f;
		s16 angleFixed = (s16)(s32)angle;
		f32 s          = JMASSin(angleFixed);
		f32 c          = JMASCos(angleFixed);

		MtxPtr base = self->getMActor()->getModel()->getAnmMtx(
		    ((J3DJoint*)node)->getJntNo());

		Mtx tilt;
		tilt[0][0] = c;
		tilt[0][1] = -s;
		tilt[0][2] = 0.0f;
		tilt[0][3] = 0.0f;
		tilt[1][0] = s;
		tilt[1][1] = c;
		tilt[1][2] = 0.0f;
		tilt[1][3] = 0.0f;
		tilt[2][0] = 0.0f;
		tilt[2][1] = 0.0f;
		tilt[2][2] = 1.0f;
		tilt[2][3] = 0.0f;

		PSMTXConcat(base, tilt, base);
		PSMTXConcat(J3DSys::mCurrentMtx, tilt, J3DSys::mCurrentMtx);
	}
	return TRUE;
}
