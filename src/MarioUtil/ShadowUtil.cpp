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
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/LightUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <MarioUtil/ReinitGX.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/HitActor.hpp>
#include <System/Application.hpp>
#include <dolphin/gx.h>
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

static bool isShadowWaterSurface(const TBGCheckData* data)
{
	if (data == nullptr)
		return false;

	u16 type = data->getBGType();
	return type == BG_TYPE_WATER || type == BG_TYPE_DAMAGING_WATER
	       || (u16)(type - BG_TYPE_SEA_WATER) <= 3
	       || type == BG_TYPE_SHADED_POOL;
}

void TMBindShadowManager::drawShadow(u32 flags, JDrama::TGraphics* graphics)
{
	ReInitializeGX();

	GXSetZCompLoc(GX_TRUE);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
	              GX_COLOR0A0);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetChanMatColor(GX_COLOR0A0, unk5C);
	GXSetCurrentMtx(GX_PNMTX0);
	GXLoadNrmMtxImm(graphics->mViewMtx.mMtx, GX_PNMTX0);

	if (!mTestSw) {
		for (int i = 0; i < unk20; ++i) {
			TAlphaShadowQuadAry& group = unk1C[i];
			if (group.unk4 == nullptr || group.unkC == nullptr)
				continue;
			if (!(flags & group.unk0))
				continue;

			GXSetCullMode(GX_CULL_NONE);
			GXLoadPosMtxImm(graphics->mViewMtx.mMtx, GX_PNMTX0);
			GXSetColorUpdate(GX_FALSE);
			GXSetDstAlpha(GX_TRUE, 0);
			GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE,
			               GX_LO_NOOP);

			JGeometry::TVec3<f32> min;
			JGeometry::TVec3<f32> max;
			min.set(group.unkC->unk0.x, group.unkC->unk0.y - group.unkC->unkC.y,
			        group.unkC->unk0.z);
			max.set(group.unkC->unkC.x, group.unkC->unk0.y + group.unkC->unkC.y,
			        group.unkC->unkC.z);
			SMS_DrawCube(min, max);

			TAlphaShadowQuad* first = group.unk4;
			bool highQuality = first->unk68->unk18 < 20000000.0f;
			J3DModelData* modelData
			    = highQuality ? unk3C[0]->getModelData()
			                  : unk3C[1]->getModelData();
			SMS_SettingDrawShape(modelData, 0);

			GXSetDstAlpha(GX_FALSE, 0);
			GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
			GXSetCullMode(GX_CULL_BACK);
			GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO,
			               GX_LO_NOOP);

			for (TAlphaShadowQuad* quad = first; quad != nullptr;
			     quad = quad->unk6C) {
				GXLoadPosMtxImm(quad->unk4, GX_PNMTX0);
				drawShadowVolume(highQuality, quad);
			}

			GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA,
			               GX_LO_NOOP);
			GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
			GXSetCullMode(GX_CULL_FRONT);
			GXSetDstAlpha(GX_TRUE, 0);
			GXSetColorUpdate(GX_TRUE);

			for (TAlphaShadowQuad* quad = first; quad != nullptr;
			     quad = quad->unk6C) {
				GXLoadPosMtxImm(quad->unk4, GX_PNMTX0);
				drawShadowVolume(highQuality, quad);
			}

			GXSetCullMode(GX_CULL_BACK);
			GXSetColorUpdate(GX_FALSE);
			GXSetDstAlpha(GX_TRUE, 0);
			GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);

			for (TAlphaShadowQuad* quad = first; quad != nullptr;
			     quad = quad->unk6C) {
				if (quad->unk68->unk1C != 3)
					continue;

				J3DModelData* shipModel = unk3C[3]->getModelData();
				GXLoadPosMtxImm(quad->unk4, GX_PNMTX0);
				SMS_SettingDrawShape(shipModel, 0);
				SMS_DrawShape(shipModel, 0);
			}

			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
			GXLoadPosMtxImm(graphics->mViewMtx.mMtx, GX_PNMTX0);
			SMS_DrawCube(min, max);

			if (unk64) {
				GXSetColorUpdate(GX_TRUE);
				GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA,
				               GX_BL_INVSRCALPHA, GX_LO_NOOP);
				GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
				SMS_DrawCube(min, max);
			}
		}
	} else {
		Mtx identity;
		PSMTXIdentity(identity);
		GXSetCurrentMtx(GX_PNMTX0);
		GXLoadPosMtxImm(identity, GX_PNMTX0);
		GXLoadNrmMtxImm(identity, GX_PNMTX0);

		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanMatColor(GX_COLOR0A0, unk5C);
		GXSetColorUpdate(GX_FALSE);
		GXSetAlphaUpdate(GX_TRUE);
		GXSetDstAlpha(GX_FALSE, 0);
		SMS_SettingDrawShape(unk3C[0]->getModelData(), 0);

		bool drawFlagged = (flags & 0x40000000) != 0;
		for (int i = 0; i < unk14; ++i) {
			TAlphaShadowQuad& quad = unk18[i];
			bool flagged = (quad.unk68->unk20 & 0x40000000) != 0;
			if (drawFlagged != flagged)
				continue;

			GXLoadPosMtxImm(quad.unk4, GX_PNMTX0);
			GXSetCullMode(GX_CULL_BACK);
			GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE,
			               GX_LO_NOOP);
			SMS_DrawShape(unk3C[0]->getModelData(), 0);
			GXSetCullMode(GX_CULL_FRONT);
			GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_ONE, GX_BL_ONE,
			               GX_LO_NOOP);
			SMS_DrawShape(unk3C[0]->getModelData(), 0);
		}

		GXSetCurrentMtx(GX_PNMTX0);
		GXLoadPosMtxImm(identity, GX_PNMTX0);
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetCullMode(GX_CULL_FRONT);
		GXSetColorUpdate(GX_TRUE);
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA,
		               GX_LO_NOOP);
		GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

		GXBegin(GX_QUADS, GX_VTXFMT0, 4);
		GXPosition3f32(-1000.0f, 1000.0f, -200.0f);
		GXPosition3f32(1000.0f, 1000.0f, -200.0f);
		GXPosition3f32(1000.0f, -1000.0f, -200.0f);
		GXPosition3f32(-1000.0f, -1000.0f, -200.0f);

		GXSetColorUpdate(GX_TRUE);
		GXSetAlphaUpdate(GX_TRUE);
		GXSetDstAlpha(GX_TRUE, 0);
		GXSetChanMatColor(GX_COLOR0A0, unk5C);
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_INVDSTALPHA,
		               GX_LO_NOOP);
		GXBegin(GX_QUADS, GX_VTXFMT0, 4);
		GXPosition3f32(-1000.0f, 1000.0f, -200.0f);
		GXPosition3f32(1000.0f, 1000.0f, -200.0f);
		GXPosition3f32(1000.0f, -1000.0f, -200.0f);
		GXPosition3f32(-1000.0f, -1000.0f, -200.0f);
	}

	GXSetZCompLoc(GX_FALSE);
	GXSetColorUpdate(GX_FALSE);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetDstAlpha(GX_TRUE, 0);
}

void TMBindShadowManager::drawShadowVolume(bool high_quality,
                                           TAlphaShadowQuad* quad)
{
	TCircleShadowRequest* request = quad->unk68;
	u8 kind                       = request->unk1C;

	if (kind == 1) {
		if (quad->unk64 == nullptr) {
			J3DModelData* modelData = unk3C[2]->getModelData();
			SMS_SettingDrawShape(modelData, 0);
			SMS_DrawShape(modelData, 0);
		} else {
			static const s32 topIndices[] = { 2, 1, 0, 3, 2,
				                              0, 4, 3, 0 };
			static const s32 bottomIndices[] = { 0, 1, 2, 0, 2,
				                                 3, 0, 3, 4 };

			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);

			GXBegin(GX_TRIANGLES, GX_VTXFMT0, 18);
			for (int i = 0; i < 9; ++i) {
				Vec& v = quad->unk64->unk0[topIndices[i]];
				GXPosition3f32(v.x, v.y + 50.0f, v.z);
			}
			for (int i = 0; i < 9; ++i) {
				Vec& v = quad->unk64->unk0[bottomIndices[i]];
				GXPosition3f32(v.x, v.y - 50.0f, v.z);
			}

			GXBegin(GX_TRIANGLES, GX_VTXFMT0, 60);
			for (int i = 0; i < 5; ++i) {
				Vec& a = quad->unk64->unk0[i];
				Vec& b = quad->unk64->unk0[(i + 1) % 5];

				GXPosition3f32(a.x, a.y + 50.0f, a.z);
				GXPosition3f32(b.x, b.y + 50.0f, b.z);
				GXPosition3f32(b.x, b.y - 50.0f, b.z);

				GXPosition3f32(a.x, a.y + 50.0f, a.z);
				GXPosition3f32(b.x, b.y - 50.0f, b.z);
				GXPosition3f32(a.x, a.y - 50.0f, a.z);
			}
		}

		J3DModelData* modelData
		    = high_quality ? unk3C[0]->getModelData() : unk3C[1]->getModelData();
		SMS_SettingDrawShape(modelData, 0);
		return;
	}

	if (kind == 3) {
		J3DModelData* modelData = unk3C[3]->getModelData();
		SMS_SettingDrawShape(modelData, 0);
		SMS_DrawShape(modelData, 0);
		return;
	}

	J3DModelData* modelData
	    = high_quality ? unk3C[0]->getModelData() : unk3C[1]->getModelData();
	SMS_DrawShape(modelData, 0);
}

static bool conectCubeSame(TAlphaShadowBlendQuad* a,
                           TAlphaShadowBlendQuad* b)
{
	if (a == nullptr || b == nullptr)
		return false;

	f32 joinDist = TMBindShadowManager::mJoinDist;
	if (__fabsf(a->unk0.y - b->unk0.y) > 50.0f)
		return false;

	if (a->unk0.x <= b->unkC.x - joinDist
	    && a->unkC.x >= b->unk0.x + joinDist
	    && a->unk0.z <= b->unkC.z - joinDist
	    && a->unkC.z >= b->unk0.z + joinDist) {
		if (a->unkC.x <= b->unkC.x - joinDist)
			a->unkC.x = b->unkC.x;
		if (a->unk0.x >= b->unk0.x + joinDist)
			a->unk0.x = b->unk0.x;
		if (a->unkC.z <= b->unkC.z - joinDist)
			a->unkC.z = b->unkC.z;
		if (a->unk0.z >= b->unk0.z + joinDist)
			a->unk0.z = b->unk0.z;
		if (a->unk0.y >= b->unk0.y)
			a->unk0.y = b->unk0.y;
		if (a->unkC.y <= b->unkC.y)
			a->unkC.y = b->unkC.y;
		return true;
	}

	return false;
}

static bool conectCubeDiffer(TAlphaShadowBlendQuad* a,
                             TAlphaShadowBlendQuad* b)
{
	if (a == nullptr || b == nullptr)
		return false;

	if (a->unk18 != b->unk18 || a->unk18 == 0 || b->unk18 == 0)
		return false;
	if ((a->unk18 & 0x40000000) || (b->unk18 & 0x40000000))
		return false;

	if (__fabsf(a->unk0.y - b->unk0.y) > 50.0f)
		return false;

	if (a->unk0.x <= b->unkC.x && a->unkC.x >= b->unk0.x
	    && a->unk0.z <= b->unkC.z && a->unkC.z >= b->unk0.z) {
		if (a->unkC.x <= b->unkC.x)
			a->unkC.x = b->unkC.x;
		if (a->unk0.x >= b->unk0.x)
			a->unk0.x = b->unk0.x;
		if (a->unkC.z <= b->unkC.z)
			a->unkC.z = b->unkC.z;
		if (a->unk0.z >= b->unk0.z)
			a->unk0.z = b->unk0.z;
		if (a->unk0.y >= b->unk0.y)
			a->unk0.y = b->unk0.y;
		if (a->unkC.y <= b->unkC.y)
			a->unkC.y = b->unkC.y;
		return true;
	}

	return false;
}

void TMBindShadowManager::calcVtx()
{
	static const f32 calctablex[] = { -1.0f, 1.0f, 1.0f, -1.0f };
	static const f32 calctablez[] = { -1.0f, -1.0f, 1.0f, 1.0f };

	unk2C = 0;

	TAlphaShadowQuad* quad = unk18;
	for (int i = 0; i < unk14; ++i, ++quad) {
		TCircleShadowRequest* request = &unk10[i];
		JGeometry::TVec3<f32> original = request->unk0;

		if (request->unk1C == 1) {
			JGeometry::TVec3<f32> top = request->unk0;
			top.y += mSquareShadowHeight;

			f32 dy = top.y - request->unk0.y;
			request->unk0.x
			    = (top.x - unk30.x * dy + request->unk0.x) * 0.5f;
			request->unk0.z
			    = (top.z - unk30.z * dy + request->unk0.z) * 0.5f;
		}

		f32 groundY = request->unk0.y;
		const TBGCheckData* checkData = nullptr;
		if (request->unk1D) {
			groundY = gpMap->checkGround(request->unk0.x,
			                             request->unk0.y + unk60,
			                             request->unk0.z, &checkData);
			if (isShadowWaterSurface(checkData)) {
				groundY = gpMap->checkGroundIgnoreWaterSurface(
				    request->unk0.x, request->unk0.y, request->unk0.z,
				    &checkData);
			}
		}

		if (request->unk1C != 1)
			request->unk0.y = groundY;

		f32 farScale = 1.0f;
		if (request->unk18 > 20000000.0f || request->unk14 != 0.0f)
			farScale = 0.2f;

		f32 maxRadius = request->unkC;
		if (maxRadius < request->unk10)
			maxRadius = request->unk10;

		f32 rotX   = 90.0f;
		f32 rotY   = request->unk14;
		f32 scaleX = request->unkC * 0.08f;
		f32 scaleY = request->unk10 * 0.08f;
		f32 scaleZ = maxRadius * farScale * 0.08f;

		if (request->unk1C == 3) {
			rotX   = 0.0f;
			scaleY = 0.2f;
			scaleX = request->unk10 * 0.08f * mTreeScale;
			scaleZ = request->unkC * 0.08f * mTreeScale;
			request->unk10 = 1.0f;
			request->unkC  = 1.0f;
		}

		request->unkC *= 0.8f;
		request->unk10 *= 0.8f;

		quad->unk0  = 0.01f;
		quad->unk64 = nullptr;
		quad->unk68 = request;
		quad->unk6C = nullptr;
		quad->unk0  = request->unkC;
		if (quad->unk0 < request->unk10)
			quad->unk0 = request->unk10;
		if (quad->unk0 > 200.0f)
			quad->unk0 = 200.0f;
		quad->unk0 *= 1.1f;

		JGeometry::TVec3<f32> mtxPos = request->unk0;
		if (request->unk1C == 1 && unk2C < 0x1D
		    && __fabsf(groundY - request->unk0.y) < 1.0f) {
			TSquareShadowInfo& square = unk28[unk2C];
			f32 dx                    = request->unk0.x - original.x;
			f32 dz                    = request->unk0.z - original.z;

			if (original.x >= request->unk0.x
			    && original.z >= request->unk0.z) {
				square.unk0[0].x = request->unkC;
				square.unk0[0].z = -request->unk10;
				square.unk0[1].x = dx + request->unkC;
				square.unk0[1].z = dz - request->unk10;
				square.unk0[2].x = dx - request->unkC;
				square.unk0[2].z = dz - request->unk10;
				square.unk0[3].x = dx - request->unkC;
				square.unk0[3].z = dz + request->unk10;
				square.unk0[4].x = -request->unkC;
				square.unk0[4].z = request->unk10;
			} else {
				square.unk0[0].x = 1.0f;
				square.unk0[0].z = 1.0f;
				square.unk0[1].x = dx + 1.0f;
				square.unk0[1].z = dz - 1.0f;
				square.unk0[2].x = dx - 1.0f;
				square.unk0[2].z = dz - 1.0f;
				square.unk0[3].x = dx - 1.0f;
				square.unk0[3].z = dz + 1.0f;
				square.unk0[4].x = -1.0f;
				square.unk0[4].z = 1.0f;
			}

			for (int j = 0; j < 5; ++j)
				square.unk0[j].y = 0.0f;

			quad->unk64 = unk28;
			++unk2C;
		}

		if (request->unk20 == ACTOR_TYPE_SHADOW_MARIO)
			scaleZ *= 1.5f;

		MsMtxSetTRS(quad->unk4, mtxPos.x, mtxPos.y, mtxPos.z, rotX, rotY,
		            0.0f, scaleX, scaleY, scaleZ);
		PSMTXConcat(J3DSys::mCurrentMtx, quad->unk4, quad->unk4);

		for (int j = 0; j < 4; ++j) {
			quad->unk34[j].x
			    = request->unk0.x + request->unkC * calctablex[j];
			quad->unk34[j].y = groundY;
			quad->unk34[j].z
			    = request->unk0.z + request->unk10 * calctablez[j];
		}

		if (unk14 >= 0x200)
			return;
	}

	if (unk14 == 0 || mTestSw)
		return;

	for (int i = 0; i < unk20; ++i) {
		unk1C[i].unk0  = 0x20000000;
		unk1C[i].unk4  = nullptr;
		unk1C[i].unk8  = nullptr;
		unk1C[i].unkC  = nullptr;
		unk1C[i].unk10 = nullptr;
	}

	unk20 = 0;
	for (int i = 0; i < unk14; ++i) {
		TAlphaShadowQuad* shadowQuad = &unk18[i];
		shadowQuad->unk6C           = nullptr;

		TAlphaShadowBlendQuad* blend = &unk24[i];
		blend->unk1C                 = nullptr;
		blend->unk0.x                = shadowQuad->unk34[0].x;
		blend->unk0.z                = shadowQuad->unk34[0].z;
		blend->unkC.x                = shadowQuad->unk34[2].x;
		blend->unkC.z                = shadowQuad->unk34[2].z;
		blend->unk0.y                = shadowQuad->unk34[0].y;
		blend->unkC.y                = shadowQuad->unk0 + mYScalePlus;
		blend->unk18                 = 0;

		bool connected = false;
		for (int j = 0; j < unk20; ++j) {
			if (conectCubeDiffer(unk1C[j].unkC, blend)) {
				connected        = true;
				unk1C[j].unk8->unk6C = shadowQuad;
				unk1C[j].unk8        = shadowQuad;
				unk1C[j].unk10->unk1C = blend;
				unk1C[j].unk10       = blend;
				break;
			}
		}

		if (connected)
			continue;

		if (unk20 >= 0x100) {
			unk20 = 0x100;
			continue;
		}

		TAlphaShadowQuadAry& group = unk1C[unk20];
		group.unk4                 = shadowQuad;
		group.unk8                 = shadowQuad;
		group.unkC                 = blend;
		group.unk10                = blend;
		group.unk0                 = 0x20000000;
		if (shadowQuad->unk68->unk20 & 0x40000000)
			group.unk0 = 0x40000000;
		++unk20;
	}

	for (int i = 0; i < unk20; ++i) {
		for (int j = 0; j < unk20; ++j) {
			if (i == j)
				continue;
			if (unk1C[i].unk4 == nullptr || unk1C[j].unk4 == nullptr)
				continue;
			if (!conectCubeSame(unk1C[i].unkC, unk1C[j].unkC))
				continue;

			unk1C[i].unk8->unk6C  = unk1C[j].unk4;
			unk1C[i].unk8         = unk1C[j].unk8;
			unk1C[i].unk10->unk1C = unk1C[j].unkC;
			unk1C[i].unk10        = unk1C[j].unk10;
			if ((unk1C[i].unk4->unk68->unk20 & 0x40000000)
			    || (unk1C[j].unk4->unk68->unk20 & 0x40000000)) {
				unk1C[i].unk0 = 0x40000000;
			}
			unk1C[j].unk4 = nullptr;
			unk1C[j].unkC = nullptr;
		}
	}
}
