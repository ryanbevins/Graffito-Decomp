#include <MarioUtil/ShadowUtil.hpp>
#include <Camera/Camera.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <MarioUtil/LightUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/HitActor.hpp>
#include <System/Application.hpp>
#include <dolphin/mtx.h>
#include <math.h>

TMBindShadowManager* gpBindShadowManager;

f32 TMBindShadowManager::mSquareShadowHeight = 200.0f;
f32 TMBindShadowManager::mTreeScale          = 0.02f;
f32 TMBindShadowManager::mYScalePlus         = 20.0f;
f32 TMBindShadowManager::mJoinDist;
u8 TMBindShadowManager::mTestSw;
u8 TMBindShadowManager::mDLSw;

enum {
	ACTOR_TYPE_SHADOW_MARIO = 0x80000001,
	ACTOR_TYPE_SHADOW_A     = 0x08000001,
	ACTOR_TYPE_SHADOW_B     = 0x08000002,
};

static inline bool isUseThisBindJoint(u32 actor_type, int joint_no)
{
	return actor_type != ACTOR_TYPE_SHADOW_A || joint_no != 0x17;
}

static inline bool isCircleBindJoint(u32 actor_type, int joint_no)
{
	if (actor_type == ACTOR_TYPE_SHADOW_A)
		return joint_no == 0x13 || joint_no == 0x17;

	if (actor_type == ACTOR_TYPE_SHADOW_MARIO
	    || actor_type == ACTOR_TYPE_SHADOW_B)
		return joint_no == 0x1A;

	return false;
}

static inline bool isBodyBindJoint(u32 actor_type, int joint_no)
{
	if (actor_type == ACTOR_TYPE_SHADOW_MARIO
	    || actor_type == ACTOR_TYPE_SHADOW_B)
		return joint_no == 2 || joint_no == 0xE;

	return false;
}

TMBindShadowParts::TMBindShadowParts(J3DModel* model, u8 joint_no,
                                     TMBindShadowBody* body, f32 scale)
    : unk0(0.01f)
    , unk4(body)
    , unk8(nullptr)
    , unkC(nullptr)
    , unk10(nullptr)
    , unk14(1)
    , unk15(0)
    , unk16(0)
{
	J3DModelData* data = model->getModelData();
	unk8               = data->getJointName()->getName(joint_no);
	unkC               = model->getAnmMtx(joint_no);

	J3DJoint* joint      = data->getJointNodePointer(joint_no);
	J3DJoint* childJoint = (J3DJoint*)joint->getChild();
	unk10               = model->getAnmMtx(childJoint->getJntNo());
	unk0                = scale;
}

void TMBindShadowParts::calc(f32 ground_y)
{
	if (!unk14)
		return;

	TMBindShadowBody* body = unk4;
	THitActor* actor       = body->unk4;
	f32 heightOffset       = __fabsf(actor->mPosition.y - ground_y);

	f32 jointY = unkC[1][3] - heightOffset - ground_y;
	f32 childY = unk10[1][3] - heightOffset - ground_y;

	f32 jointX
	    = unkC[0][3] - gpBindShadowManager->unk30.x * jointY;
	f32 childX
	    = unk10[0][3] - gpBindShadowManager->unk30.x * childY;
	f32 childZ
	    = unk10[2][3] - gpBindShadowManager->unk30.z * childY;
	f32 jointZ
	    = unkC[2][3] - gpBindShadowManager->unk30.z * jointY;

	f32 centerX = (jointX + childX) * 0.5f;
	f32 centerZ = (jointZ + childZ) * 0.5f;
	f32 extentX = __fabsf(centerX - childX);
	f32 extentZ = __fabsf(centerZ - childZ);

	if (unk16) {
		unk0 = body->unk18;
	} else if (unk15) {
		unk0 = body->unk10;
	} else {
		unk0 = body->unk14;
	}

	if (extentX < unk0)
		extentX = unk0;
	if (extentZ < unk0)
		extentZ = unk0;

	if (!unk15) {
		f32 ratio = gpBindShadowManager->unk68;
		f32 scale = gpBindShadowManager->unk6C;
		if (extentZ > extentX) {
			if (extentZ > ratio * extentX)
				extentZ *= scale;
			if (extentX > unk0)
				extentX = unk0;
		} else if (extentX > extentZ) {
			if (extentX > ratio * extentZ)
				extentX *= scale;
			if (extentZ > unk0)
				extentZ = unk0;
		}
	}

	TCircleShadowRequest request;
	request.unk0.set(centerX, ground_y, centerZ);
	request.unkC  = extentX;
	request.unk10 = extentZ;

	if (!unk15) {
		u32 actorType = actor->mActorType;
		if (actorType != ACTOR_TYPE_SHADOW_MARIO
		    && actorType != ACTOR_TYPE_SHADOW_B) {
			f32 angle = matan(childZ - jointZ, childX - jointX)
			            * (360.0f / 65536.0f);
			if (extentX > extentZ)
				angle -= 90.0f;
			request.unk14 = angle;
		} else {
			request.unk14 = 0.0f;
		}
	} else {
		request.unk14 = 0.0f;
	}

	gpBindShadowManager->request(request, actor->mActorType);
}

TMBindShadowBody::TMBindShadowBody(THitActor* actor, J3DModel* model, f32 scale)
    : unk0(nullptr)
    , unk4(actor)
    , unk8(0)
    , unkC(actor->getName())
    , unk10(0.01f)
    , unk14(0.01f)
    , unk18(50.0f)
{
	u32 actorType = actor->mActorType;
	if (actorType == ACTOR_TYPE_SHADOW_MARIO
	    || actorType == ACTOR_TYPE_SHADOW_B) {
		unk10 = 38.0f;
		unk14 = 18.0f;
		unk18 = 25.0f;
	} else if (actorType == ACTOR_TYPE_SHADOW_A) {
		unk10 = 280.0f;
		unk14 = 50.0f;
	} else {
		unk10 = 50.0f;
		unk14 = 50.0f;
	}

	unk10 *= scale;
	unk14 *= scale;

	J3DModelData* data = model->getModelData();
	for (int i = 0; i < data->getJointNum(); ++i) {
		J3DJoint* joint = data->getJointNodePointer(i);
		if ((joint->mKind & 0xF) == 1
		    && isUseThisBindJoint(actorType, i)) {
			++unk8;
		}
	}

	unk0 = new TMBindShadowParts*[unk8];

	int entry = 0;
	for (int i = 0; i < data->getJointNum(); ++i) {
		J3DJoint* joint = data->getJointNodePointer(i);
		if ((joint->mKind & 0xF) != 1)
			continue;
		if (!isUseThisBindJoint(actorType, i))
			continue;

		if (isCircleBindJoint(actorType, i)) {
			unk0[entry]       = new TMBindShadowParts(model, i, this, unk10);
			unk0[entry]->unk15 = 1;
		} else if (isBodyBindJoint(actorType, i)) {
			unk0[entry]       = new TMBindShadowParts(model, i, this, unk18);
			unk0[entry]->unk16 = 1;
		} else {
			unk0[entry] = new TMBindShadowParts(model, i, this, unk14);
		}

		++entry;
	}

	gpBindShadowManager->unk4C.insert(gpBindShadowManager->unk4C.end(), this);
}

void TMBindShadowBody::entryDrawShadow()
{
	const JGeometry::TVec3<f32>& pos = unk4->mPosition;

	f32 dx = pos.x - gpMarioPos->x;
	bool nearMario = -0.0000038146973f <= dx && dx <= 0.0000038146973f;
	if (nearMario) {
		f32 dy = pos.y - gpMarioPos->y;
		nearMario = -0.0000038146973f <= dy && dy <= 0.0000038146973f;
	}
	if (nearMario) {
		f32 dz = pos.z - gpMarioPos->z;
		nearMario = -0.0000038146973f <= dz && dz <= 0.0000038146973f;
	}

	if (nearMario) {
		if (gpBindShadowManager->unk65)
			return;
		gpBindShadowManager->unk65 = 1;
	}

	const TBGCheckData* checkData;
	f32 groundY = gpMap->checkGround(pos.x, pos.y + gpBindShadowManager->unk60,
	                                 pos.z, &checkData);
	if (checkData->isWaterSurface()) {
		groundY = gpMap->checkGround(pos.x, pos.y - 50.0f, pos.z, &checkData);
	}

	if (checkData->checkFlag(BG_CHECK_FLAG_ILLEGAL))
		return;

	for (int i = 0; i < unk8; ++i)
		unk0[i]->calc(groundY);
}

TSquareShadowInfo::TSquareShadowInfo()
{
	for (Vec* v = unk0; v != &unk0[5]; ++v) {
		v->x = 0.0f;
		v->y = 0.0f;
		v->y = 0.0f;
	}
}

TModelShadowInfo::TModelShadowInfo()
    : unk0(0.0f, 0.0f, 0.0f)
    , unkC(0)
    , unkD(1)
    , unk10(0.01f)
{
}

TMBindShadowManager::TMBindShadowManager(const char* name)
    : JDrama::TViewObj(name)
    , unk10(nullptr)
    , unk14(0)
    , unk18(nullptr)
    , unk1C(nullptr)
    , unk20(0)
    , unk24(nullptr)
    , unk28(nullptr)
    , unk2C(0)
    , unk3C(nullptr)
    , unk40(0)
    , unk44(0)
    , unk48(0)
    , unk49(0)
    , unk60(30.0f)
    , unk64(0)
    , unk65(0)
    , unk68(0.5f)
    , unk6C(1.55f)
    , unk70(nullptr)
{
	unk5C.r = 0x1E;
	unk5C.g = 0x32;
	unk5C.b = 0x73;
	unk5C.a = 0xB4;

	switch (gpApplication.mCurrArea.unk0) {
	case 6:
		unk5C.r = 0x09;
		unk5C.g = 0x09;
		unk5C.b = 0x1C;
		unk5C.a = 0x74;
		break;
	case 7:
		unk5C.r = 0x2D;
		unk5C.g = 0x28;
		unk5C.b = 0x3C;
		unk5C.a = 0x5A;
		break;
	}

	gpBindShadowManager = this;

	unk10 = new TCircleShadowRequest[0x200];
	unk18 = new TAlphaShadowQuad[0x200];
	unk1C = new TAlphaShadowQuadAry[0x100];
	unk24 = new TAlphaShadowBlendQuad[0x200];
	unk28 = new TSquareShadowInfo[0x1E];
	unk70 = new TModelShadowInfo[1];
	unk3C = new SDLModelData*[5];
}

void TMBindShadowManager::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);

	unk3C[0] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource("/common/shadowCircle.bmd"),
	    0x10210000));
	unk3C[1] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource("/common/shadowCircleLow.bmd"),
	    0x10210000));
	unk3C[2] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource("/common/shadowCube.bmd"), 0x10210000));
	unk3C[3] = new SDLModelData(J3DModelLoaderDataBase::load(
	    JKRFileLoader::getGlbResource("/common/ShipShadow.bmd"), 0x10210000));

	unk49 = 1;
	unk14 = 0;
	unk20 = 0;
	unk65 = 0;
	unk2C = 0;
	unk40 = 0;
}

void TMBindShadowManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x4) {
		unk49 = 0;
		PSVECNormalize(gpLightManager->getLightPos(), &unk30);
		calcVtx();
	}

	if (flags & 0x8) {
		if (mDLSw)
			drawShadowGD(flags, graphics);
		else
			drawShadow(flags, graphics);

		if (flags & 0x20000000) {
			unk49 = 1;
			unk14 = 0;
			unk20 = 0;
			unk65 = 0;
			unk2C = 0;
			unk40 = 0;
		}
	}
}

void TMBindShadowManager::request(const TCircleShadowRequest& request,
                                  u32 actor_type)
{
	JGeometry::TVec3<f32> toCamera = request.unk0;
	toCamera.x -= gpCamera->unk124.x;
	toCamera.y -= gpCamera->unk124.y;
	toCamera.z -= gpCamera->unk124.z;

	f32 distSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y
	             + toCamera.z * toCamera.z;

	f32 distScale = 6.0f;
	if (request.unk1C == 2)
		distScale = 10.0f;
	if (request.unk1C == 1)
		distScale = 1.0f;

	if (distSq > 20000000.0f * distScale)
		return;
	if (request.unkC < 0.01f)
		return;
	if (request.unk10 < 0.01f)
		return;
	if (!gpMap->isInArea(request.unk0.x, request.unk0.z))
		return;
	if (isnan(request.unk0.x))
		return;
	if (isnan(request.unk0.z))
		return;
	if (unk14 >= 0x200)
		return;

	TCircleShadowRequest& dst = unk10[unk14];
	dst                       = request;
	dst.unk20                 = actor_type;
	dst.unk18                 = distSq;

	if (request.unk1C == 2) {
		if (unk40 >= 1)
			return;

		TModelShadowInfo& info = unk70[unk40];
		info.unk0              = request.unk0;
		info.unkC              = 0;
		info.unkD              = 1;
		if (distSq > 200000000.0f)
			info.unkC = 1;
		++unk40;
	} else {
		++unk14;
	}
}

void TMBindShadowManager::forceRequest(const TCircleShadowRequest& request,
                                       u32 actor_type)
{
	JGeometry::TVec3<f32> toCamera = request.unk0;
	toCamera.x -= gpCamera->unk124.x;
	toCamera.y -= gpCamera->unk124.y;
	toCamera.z -= gpCamera->unk124.z;

	f32 distSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y
	             + toCamera.z * toCamera.z;

	if (unk14 >= 0x200)
		return;

	TCircleShadowRequest& dst = unk10[unk14];
	dst                       = request;
	dst.unk20                 = actor_type;
	dst.unk18                 = distSq;
	++unk14;
}
