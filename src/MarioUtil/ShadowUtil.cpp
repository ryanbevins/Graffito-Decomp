#include <MarioUtil/ShadowUtil.hpp>
#include <MarioUtil/GDUtil.hpp>
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
#include <dolphin/gd/GDTransform.h>
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
	switch ((s32)actor_type) {
	case (s32)ACTOR_TYPE_SHADOW_MARIO:
	case (s32)ACTOR_TYPE_SHADOW_B:
		return true;
	case (s32)ACTOR_TYPE_SHADOW_A:
		if (joint_no == 0x17)
			return false;
		return true;
	default:
		return true;
	}
}

static inline bool isCircleBindJoint(u32 actor_type, int joint_no)
{
	switch ((s32)actor_type) {
	case (s32)ACTOR_TYPE_SHADOW_MARIO:
	case (s32)ACTOR_TYPE_SHADOW_B:
		if (joint_no != 0x1A)
			return false;
		return true;
	case (s32)ACTOR_TYPE_SHADOW_A:
		if (joint_no != 0x13) {
			if (joint_no != 0x17)
				return false;
		}
		return true;
	default:
		return false;
	}
}

static inline bool isBodyBindJoint(u32 actor_type, int joint_no)
{
	switch ((s32)actor_type) {
	case (s32)ACTOR_TYPE_SHADOW_MARIO:
	case (s32)ACTOR_TYPE_SHADOW_B:
		if (joint_no != 2) {
			if (joint_no != 0xE)
				return false;
		}
		return true;
	default:
		return false;
	}
}

#pragma dont_inline on
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
	unk8 = model->getModelData()->getJointName()->getName(joint_no);
	unkC = model->getAnmMtx(joint_no);

	J3DJoint* joint      = model->getModelData()->getJointNodePointer(joint_no);
	J3DJoint* childJoint = (J3DJoint*)joint->getChild();
	unk10               = model->getAnmMtx(childJoint->getJntNo());
	unk0                = scale;
}
#pragma dont_inline off

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
	switch ((s32)actorType) {
	case (s32)ACTOR_TYPE_SHADOW_MARIO:
	case (s32)ACTOR_TYPE_SHADOW_B:
		unk10 = 38.0f;
		unk14 = 18.0f;
		unk18 = 25.0f;
		break;
	case (s32)ACTOR_TYPE_SHADOW_A:
		unk10 = 280.0f;
		unk14 = 50.0f;
		break;
	default:
		unk10 = 50.0f;
		unk14 = 50.0f;
		break;
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

	gpBindShadowManager->unk4C.push_back(this);
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
    , unk14(0)
    , unk20(0)
    , unk2C(0)
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

	void* circle = JKRFileLoader::getGlbResource("/common/shadowCircle.bmd");
	unk3C[0] = new SDLModelData(
	    J3DModelLoaderDataBase::load(circle, 0x10210000));
	void* lowCircle
	    = JKRFileLoader::getGlbResource("/common/shadowCircleLow.bmd");
	unk3C[1] = new SDLModelData(
	    J3DModelLoaderDataBase::load(lowCircle, 0x10210000));
	void* cube = JKRFileLoader::getGlbResource("/common/shadowCube.bmd");
	unk3C[2]
	    = new SDLModelData(J3DModelLoaderDataBase::load(cube, 0x10210000));
	void* ship = JKRFileLoader::getGlbResource("/common/ShipShadow.bmd");
	unk3C[3]
	    = new SDLModelData(J3DModelLoaderDataBase::load(ship, 0x10210000));

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
	if (request.unkC < 0.01f || request.unk10 < 0.01f)
		return;
	if (!gpMap->isInArea(request.unk0.x, request.unk0.z))
		return;
	if (isnan(request.unk0.x) || isnan(request.unk0.z))
		return;
	int requestCount = unk14;
	if (requestCount >= 0x200)
		return;

	TCircleShadowRequest& dst = unk10[requestCount];
	dst                       = request;
	unk10[unk14].unk20       = actor_type;
	unk10[unk14].unk18       = distSq;

	if (request.unk1C == 2) {
		if (unk40 >= 1)
			return;

		TModelShadowInfo& info = unk70[unk40];
		info.unk0              = request.unk0;
		unk70[unk40].unkC      = 0;
		unk70[unk40].unkD      = 1;
		if (distSq > 200000000.0f)
			unk70[unk40].unkC = 1;
		++unk40;
	} else {
		++unk14;
	}
}

void TMBindShadowManager::forceRequest(const TCircleShadowRequest& request,
                                       u32 actor_type)
{
	JGeometry::TVec3<f32> pos      = request.unk0;
	JGeometry::TVec3<f32> toCamera = pos;
	const JGeometry::TVec3<f32>& cameraPos = gpCamera->unk124;
	toCamera.x -= cameraPos.x;
	toCamera.y -= cameraPos.y;
	toCamera.z -= cameraPos.z;

	f32 distSq = toCamera.x * toCamera.x + toCamera.y * toCamera.y
	             + toCamera.z * toCamera.z;

	int requestCount = unk14;
	if (requestCount >= 0x200)
		return;

	TCircleShadowRequest& dst = unk10[requestCount];
	dst.unk0                  = pos;
	dst.unkC                  = request.unkC;
	dst.unk10                 = request.unk10;
	dst.unk14                 = request.unk14;
	dst.unk18                 = request.unk18;
	dst.unk1C                 = request.unk1C;
	dst.unk1D                 = request.unk1D;
	dst.unk20                 = request.unk20;
	unk10[unk14].unk20       = actor_type;
	unk10[unk14].unk18       = distSq;
	++unk14;
}

void TMBindShadowManager::drawShadow(u32 flags, JDrama::TGraphics* graphics)
{
	if (!mTestSw) {
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
		MtxPtr viewMtx = graphics->mViewMtx.mMtx;
		MtxPtr cubeViewMtx = graphics->mViewMtx.mMtx;
		GXLoadNrmMtxImm(viewMtx, GX_PNMTX0);

		for (int i = 0; i < (int)unk20; ++i) {
			if (unk1C[i].unk4 == nullptr || unk1C[i].unkC == nullptr)
				continue;
			if (!(flags & unk1C[i].unk0))
				continue;

			GXSetCullMode(GX_CULL_NONE);
			GXLoadPosMtxImm(viewMtx, GX_PNMTX0);
			GXSetColorUpdate(GX_FALSE);
			GXSetDstAlpha(GX_TRUE, 0);
			GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
			GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE,
			               GX_LO_NOOP);

			JGeometry::TVec3<f32> min;
			JGeometry::TVec3<f32> max;
			TAlphaShadowBlendQuad* blend = unk1C[i].unkC;
			min.set(blend->unk0.x, blend->unk0.y - blend->unkC.y,
			        blend->unk0.z);
			max.set(blend->unkC.x, blend->unk0.y + blend->unkC.y,
			        blend->unkC.z);
			SMS_DrawCube(min, max);

			TAlphaShadowQuad* first = unk1C[i].unk4;
			u8 highQuality          = false;
			if (first->unk68->unk18 < 20000000.0f) {
				SMS_SettingDrawShape(unk3C[0]->getModelData(), 0);
				highQuality = 1;
			} else {
				SMS_SettingDrawShape(unk3C[1]->getModelData(), 0);
			}

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

			for (TAlphaShadowQuad* quad = unk1C[i].unk4; quad != nullptr;
			     quad = quad->unk6C) {
				GXLoadPosMtxImm(quad->unk4, GX_PNMTX0);
				drawShadowVolume(highQuality, quad);
			}

			GXSetCullMode(GX_CULL_BACK);
			GXSetColorUpdate(GX_FALSE);
			GXSetDstAlpha(GX_TRUE, 0);
			GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);

			for (TAlphaShadowQuad* quad = unk1C[i].unk4; quad != nullptr;
			     quad = quad->unk6C) {
				if (quad->unk68->unk1C != 3)
					continue;

				GXLoadPosMtxImm(quad->unk4, GX_PNMTX0);
				SMS_SettingDrawShape(unk3C[3]->getModelData(), 0);
				SMS_DrawShape(unk3C[3]->getModelData(), 0);
			}

			GXClearVtxDesc();
			GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
			GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
			GXLoadPosMtxImm(cubeViewMtx, GX_PNMTX0);
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

		Mtx identity;
		PSMTXIdentity(identity);
		GXSetCurrentMtx(GX_PNMTX0);
		GXLoadPosMtxImm(identity, GX_PNMTX0);
		GXLoadNrmMtxImm(identity, GX_PNMTX0);

		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetNumChans(1);
		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanMatColor(GX_COLOR0A0,
		                  (GXColor) { 0xff, 0xff, 0xff, 0x80 });
		GXSetNumTexGens(0);
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
		              GX_COLOR0A0);
		GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
		GXSetColorUpdate(GX_FALSE);
		GXSetAlphaUpdate(GX_TRUE);
		GXSetDstAlpha(GX_FALSE, 0);
		SMS_SettingDrawShape(unk3C[0]->getModelData(), 0);

		u32 drawFlagged = flags & 0x40000000;
		for (int i = 0; i < (int)unk14; ++i) {
			if (drawFlagged) {
				if (!(unk18[i].unk68->unk20 & 0x40000000))
					continue;
			} else if (unk18[i].unk68->unk20 & 0x40000000) {
				continue;
			}

			GXLoadPosMtxImm(unk18[i].unk4, GX_PNMTX0);
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
		GXSetNumChans(1);
		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanMatColor(GX_COLOR0A0,
		                  (GXColor) { 0xff, 0xff, 0xff, 0xff });
		GXSetNumTevStages(1);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
		              GX_COLOR0A0);
		GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_ONE,
		               GX_LO_NOOP);
		GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

		GXBegin(GX_QUADS, GX_VTXFMT0, 4);
		GXPosition3f32(-1000.0f, 1000.0f, -200.0f);
		GXPosition3f32(1000.0f, 1000.0f, -200.0f);
		GXPosition3f32(1000.0f, -1000.0f, -200.0f);
		GXPosition3f32(-1000.0f, -1000.0f, -200.0f);

		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetCullMode(GX_CULL_FRONT);
		GXSetChanMatColor(GX_COLOR0A0,
		                  (GXColor) { 0xff, 0xff, 0xff, 0x5a });
		GXSetBlendMode(GX_BM_BLEND, GX_BL_DSTALPHA, GX_BL_ZERO,
		               GX_LO_NOOP);
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

static inline void GDLoadPosMtxImm(MtxPtr mtx)
{
	GXWGFifo.u8  = GX_CMD_LOAD_XF_REG;
	GXWGFifo.u16 = 0x000B;
	GXWGFifo.u16 = 0x0000;
	GXWGFifo.f32 = mtx[0][0];
	GXWGFifo.f32 = mtx[0][1];
	GXWGFifo.f32 = mtx[0][2];
	GXWGFifo.f32 = mtx[0][3];
	GXWGFifo.f32 = mtx[1][0];
	GXWGFifo.f32 = mtx[1][1];
	GXWGFifo.f32 = mtx[1][2];
	GXWGFifo.f32 = mtx[1][3];
	GXWGFifo.f32 = mtx[2][0];
	GXWGFifo.f32 = mtx[2][1];
	GXWGFifo.f32 = mtx[2][2];
	GXWGFifo.f32 = mtx[2][3];
}

static inline void GDDrawCube(const JGeometry::TVec3<f32>& min,
                              const JGeometry::TVec3<f32>& max)
{
	GXWGFifo.u8  = GX_QUADS;
	GXWGFifo.u16 = 24;

	GXWGFifo.f32 = min.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = min.z;

	GXWGFifo.f32 = min.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = min.z;

	GXWGFifo.f32 = min.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = max.z;

	GXWGFifo.f32 = max.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = max.z;

	GXWGFifo.f32 = max.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = min.y; GXWGFifo.f32 = min.z;

	GXWGFifo.f32 = max.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = max.z;
	GXWGFifo.f32 = min.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = min.z;
	GXWGFifo.f32 = max.x; GXWGFifo.f32 = max.y; GXWGFifo.f32 = min.z;
}

void TMBindShadowManager::drawShadowGD(u32 flags, JDrama::TGraphics* graphics)
{
	GXSetZCompLoc(GX_TRUE);

	// Top-level GX state, baked into a display list once.
	static class TSetup1 : public TGDLStatic {
	public:
		TSetup1() { alloc(0x100); }
		void makeDL()
		{
			GDSetGenMode2(0, 1, 1, 0, GX_CULL_BACK);
			GDSetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
			              GX_DF_NONE, GX_AF_NONE);
			GDSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
			              GX_DF_NONE, GX_AF_NONE);
			GDSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
			GDSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
			              GX_COLOR0A0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
			              GX_COLOR0A0);
			GXColor color = { 0x1E, 0x32, 0x73, 0xB4 };
			GDSetChanMatColor(GX_COLOR0A0, color);
			GDSetCurrentMtx(GX_PNMTX0, 60, 60, 60, 60, 60, 60, 60, 60);
			static GXVtxDescList vl[] = {
				{ GX_VA_POS, GX_DIRECT }, { GX_VA_NULL, GX_NONE }
			};
			GDSetVtxDescv(vl);
			static GXVtxAttrFmtList fl[] = {
				{ GX_VA_POS, GX_POS_XYZ, GX_F32, 0 },
				{ GX_VA_NULL, GX_POS_XY, GX_U8, 0 }
			};
			GDSetVtxAttrFmtv(GX_VTXFMT0, fl);
		}
	} setup1;
	if (!setup1.unk10)
		setup1.make();
	GXCallDisplayList(setup1.mDispListObj.start,
	                  setup1.mDispListObj.ptr - setup1.mDispListObj.start);

	// Cylinder helper list (allocated here, built lazily on demand).
	static class TCylinder : public TGDLStatic {
	public:
		TCylinder() { alloc(0x400); }
		void makeDL()
		{
			f32 vCos[11];
			f32 vSin[11];
			for (int i = 0; i <= 10; ++i) {
				f32 angle = (f32)i * 2.0f * 3.1415927f / 10.0f;
				vCos[i]   = cosf(angle);
				vSin[i]   = sinf(angle);
			}

			GXWGFifo.u8  = GX_TRIANGLESTRIP;
			GXWGFifo.u16 = 22;
			for (int i = 0; i <= 10; ++i) {
				GDWrite_f32(vCos[i]);
				GDWrite_f32(1.0f);
				GDWrite_f32(vSin[i]);
				GDWrite_f32(vCos[i]);
				GDWrite_f32(-1.0f);
				GDWrite_f32(vSin[i]);
			}

			GXWGFifo.u8  = GX_TRIANGLEFAN;
			GXWGFifo.u16 = 12;
			GDWrite_f32(0.0f);
			GDWrite_f32(0.0f);
			GDWrite_f32(0.0f);
			for (int i = 0; i <= 10; ++i) {
				GDWrite_f32(vCos[i]);
				GDWrite_f32(-1.0f);
				GDWrite_f32(vSin[i]);
			}
		}
	} cylinder;
	(void)cylinder;

	for (int i = 0; i < (int)unk20; ++i) {
		TAlphaShadowQuadAry& group = unk1C[i];
		if (group.unk4 == nullptr || group.unkC == nullptr)
			continue;
		if (!(flags & group.unk0))
			continue;

		// First state group + cube.
		static class TSetup2 : public TGDLStatic {
		public:
			TSetup2() { alloc(0x80); }
			void makeDL()
			{
				GDSetCullMode(GX_CULL_NONE);
				GDSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
				GDSetBlendModeEtc(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE,
				                  GX_LO_NOOP, GX_FALSE, GX_TRUE, GX_FALSE);
				GDSetDstAlpha(GX_TRUE, 0);
			}
		} setup2;
		if (!setup2.unk10)
			setup2.make();
		GXCallDisplayList(setup2.mDispListObj.start,
		                  setup2.mDispListObj.ptr - setup2.mDispListObj.start);

		JGeometry::TVec3<f32> min;
		JGeometry::TVec3<f32> max;
		min.set(group.unkC->unk0.x, group.unkC->unk0.y - group.unkC->unkC.y,
		        group.unkC->unk0.z);
		max.set(group.unkC->unkC.x, group.unkC->unk0.y + group.unkC->unkC.y,
		        group.unkC->unkC.z);
		GDLoadPosMtxImm(graphics->mViewMtx.mMtx);
		GDDrawCube(min, max);

		TAlphaShadowQuad* first = group.unk4;

		// Second state group.
		static class TSetup3 : public TGDLStatic {
		public:
			TSetup3() { alloc(0x80); }
			void makeDL()
			{
				GDSetDstAlpha(GX_FALSE, 0);
				GDSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
				GDSetCullMode(GX_CULL_BACK);
				GDSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO,
				               GX_LO_NOOP);
			}
		} setup3;
		if (!setup3.unk10)
			setup3.make();
		GXCallDisplayList(setup3.mDispListObj.start,
		                  setup3.mDispListObj.ptr - setup3.mDispListObj.start);

		bool highQuality;
		if (first->unk68->unk18 < 20000000.0f) {
			SMS_SettingDrawShape(unk3C[0]->getModelData(), 0);
			highQuality = true;
		} else {
			SMS_SettingDrawShape(unk3C[1]->getModelData(), 0);
			highQuality = false;
		}

		for (TAlphaShadowQuad* quad = first; quad != nullptr;
		     quad = quad->unk6C) {
			GDLoadPosMtxImm(quad->unk4);
			drawShadowVolume(highQuality, quad);
		}

		// Third state group.
		static class TSetup4 : public TGDLStatic {
		public:
			TSetup4() { alloc(0x80); }
			void makeDL()
			{
				GDSetDstAlpha(GX_TRUE, 0);
				GDSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
				GDSetCullMode(GX_CULL_FRONT);
				GDSetBlendModeEtc(GX_BM_BLEND, GX_BL_DSTALPHA,
				                  GX_BL_INVDSTALPHA, GX_LO_NOOP, GX_TRUE,
				                  GX_TRUE, GX_FALSE);
			}
		} setup4;
		if (!setup4.unk10)
			setup4.make();
		GXCallDisplayList(setup4.mDispListObj.start,
		                  setup4.mDispListObj.ptr - setup4.mDispListObj.start);

		for (TAlphaShadowQuad* quad = first; quad != nullptr;
		     quad = quad->unk6C) {
			GDLoadPosMtxImm(quad->unk4);
			drawShadowVolume(highQuality, quad);
		}

		// Fourth state group + final cube.
		static class TSetup5 : public TGDLStatic {
		public:
			TSetup5() { alloc(0x100); }
			void makeDL()
			{
				GDSetDstAlpha(GX_TRUE, 0);
				GDSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
				GDSetCullMode(GX_CULL_BACK);
				GDSetBlendModeEtc(GX_BM_BLEND, GX_BL_DSTALPHA,
				                  GX_BL_INVDSTALPHA, GX_LO_NOOP, GX_FALSE,
				                  GX_TRUE, GX_FALSE);
			}
		} setup5;
		if (!setup5.unk10)
			setup5.make();
		GXCallDisplayList(setup5.mDispListObj.start,
		                  setup5.mDispListObj.ptr - setup5.mDispListObj.start);

		GDLoadPosMtxImm(graphics->mViewMtx.mMtx);
		GDDrawCube(min, max);
	}
}

void TMBindShadowManager::drawShadowVolume(bool high_quality,
                                           TAlphaShadowQuad* quad)
{
	TCircleShadowRequest* request = quad->unk68;
	u8 kind                       = request->unk1C;

	if (kind == 1) {
		if (quad->unk64 == nullptr) {
			SMS_SettingDrawShape(unk3C[2]->getModelData(), 0);
			SMS_DrawShape(unk3C[2]->getModelData(), 0);
		} else {
			const s32 topIndices[] = { 2, 1, 0, 3, 2, 0, 4, 3, 0 };
			const s32 bottomIndices[] = { 0, 1, 2, 0, 2, 3, 0, 3, 4 };

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
			TSquareShadowInfo* info = quad->unk64;
#define DRAW_SHADOW_SIDE(a, b)                                                \
	do {                                                                       \
		GXPosition3f32(info->unk0[(a)].x, info->unk0[(a)].y + 50.0f,           \
		               info->unk0[(a)].z);                                    \
		GXPosition3f32(info->unk0[(b)].x, info->unk0[(b)].y + 50.0f,           \
		               info->unk0[(b)].z);                                    \
		GXPosition3f32(info->unk0[(b)].x, info->unk0[(b)].y - 50.0f,           \
		               info->unk0[(b)].z);                                    \
		GXPosition3f32(info->unk0[(b)].x, info->unk0[(b)].y - 50.0f,           \
		               info->unk0[(b)].z);                                    \
		GXPosition3f32(info->unk0[(a)].x, info->unk0[(a)].y - 50.0f,           \
		               info->unk0[(a)].z);                                    \
		GXPosition3f32(info->unk0[(a)].x, info->unk0[(a)].y + 50.0f,           \
		               info->unk0[(a)].z);                                    \
		GXPosition3f32(info->unk0[(b)].x, info->unk0[(b)].y + 50.0f,           \
		               info->unk0[(b)].z);                                    \
		GXPosition3f32(info->unk0[(a)].x, info->unk0[(a)].y + 50.0f,           \
		               info->unk0[(a)].z);                                    \
		GXPosition3f32(info->unk0[(b)].x, info->unk0[(b)].y - 50.0f,           \
		               info->unk0[(b)].z);                                    \
		GXPosition3f32(info->unk0[(b)].x, info->unk0[(b)].y - 50.0f,           \
		               info->unk0[(b)].z);                                    \
		GXPosition3f32(info->unk0[(a)].x, info->unk0[(a)].y + 50.0f,           \
		               info->unk0[(a)].z);                                    \
		GXPosition3f32(info->unk0[(a)].x, info->unk0[(a)].y - 50.0f,           \
		               info->unk0[(a)].z);                                    \
	} while (0)
			DRAW_SHADOW_SIDE(0, 1);
			DRAW_SHADOW_SIDE(1, 2);
			DRAW_SHADOW_SIDE(2, 3);
			DRAW_SHADOW_SIDE(3, 4);
			DRAW_SHADOW_SIDE(4, 0);
#undef DRAW_SHADOW_SIDE
		}

		goto set_next_shape;
	}

	if (kind == 3) {
		SMS_SettingDrawShape(unk3C[3]->getModelData(), 0);
		SMS_DrawShape(unk3C[3]->getModelData(), 0);
		goto set_next_shape;
	}

	if (high_quality)
		SMS_DrawShape(unk3C[0]->getModelData(), 0);
	else
		SMS_DrawShape(unk3C[1]->getModelData(), 0);
	return;

set_next_shape:
	if (high_quality)
		SMS_SettingDrawShape(unk3C[0]->getModelData(), 0);
	else
		SMS_SettingDrawShape(unk3C[1]->getModelData(), 0);
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

	if (a->unk18 != b->unk18 || a->unk18 == 0 || b->unk18 == 0
	    || (a->unk18 & 0x40000000) || (b->unk18 & 0x40000000))
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
	for (int i = 0; i < (int)unk14; ++i, ++quad) {
		TCircleShadowRequest* request = &unk10[i];
		JGeometry::TVec3<f32> original = request->unk0;

		if (request->unk1C == 1) {
			JGeometry::TVec3<f32> base = request->unk0;
			JGeometry::TVec3<f32> top  = base;
			top.y += mSquareShadowHeight;

			f32 baseDY = base.y - base.y;
			f32 topDY  = top.y - base.y;
			JGeometry::TVec3<f32>* dir = &gpBindShadowManager->unk30;
			f32 baseX = base.x - dir->x * baseDY;
			f32 topX  = top.x - dir->x * topDY;
			f32 baseZ = base.z - dir->z * baseDY;
			f32 topZ  = top.z - dir->z * topDY;
			f32 projectedX = (topX + baseX) * 0.5f;
			f32 projectedY = (base.y + base.y) * 0.5f;
			f32 projectedZ = (topZ + baseZ) * 0.5f;
			request->unk0.x = projectedX;
			request->unk0.y = projectedY;
			request->unk0.z = projectedZ;
		}

		JGeometry::TVec3<f32> groundPos = request->unk0;
		f32 checkY  = groundPos.y;
		f32 groundY = checkY;
		const TBGCheckData* checkData;
		if (request->unk1D) {
			f32 groundZ = groundPos.z;
			groundY = gpMap->checkGround(groundPos.x,
			                             checkY + gpBindShadowManager->unk60,
			                             groundZ, &checkData);
			if (checkData != nullptr && checkData->isWaterSurface()) {
				groundY = gpMap->checkGroundIgnoreWaterSurface(
				    groundPos.x, checkY, groundZ, &checkData);
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

		f32 typeScale = 1.0f;
		f32 rotX   = 90.0f;
		f32 rotY   = request->unk14;
		f32 scaleX = request->unkC * 0.08f * typeScale;
		f32 scaleY = request->unk10 * 0.08f * typeScale;
		f32 scaleZ = maxRadius * farScale * 0.08f * typeScale;

		if (request->unk1C == 3) {
			rotX   = 0.0f;
			scaleY = 0.2f;
			scaleX = request->unk10 * 0.08f * mTreeScale;
			scaleZ = request->unkC * 0.08f * mTreeScale;
			request->unk10 = 1.0f;
			request->unkC  = 1.0f;
		}

		JGeometry::TVec3<f32> mtxPos = request->unk0;
		request->unkC *= 0.8f;
		request->unk10 *= 0.8f;

		quad->unk0  = 0.01f;
		quad->unk64 = nullptr;
		quad->unk68 = nullptr;
		quad->unk6C = nullptr;
		quad->unk68 = request;
		quad->unk0  = request->unkC;
		if (quad->unk0 < request->unk10)
			quad->unk0 = request->unk10;
		if (quad->unk0 > 200.0f)
			quad->unk0 = 200.0f;
		quad->unk0 *= 1.1f;

		if (request->unk1C == 1 && (int)unk2C < 0x1D
		    && __fabsf(groundY - request->unk0.y) < 1.0f) {
			scaleX       = 1.0f;
			scaleY       = 1.0f;
			scaleZ       = 1.0f;
			mtxPos       = original;
			u8 madeShape = false;

			if (original.x >= groundPos.x
			    && !(original.z < groundPos.z)) {
				JGeometry::TVec3<f32> shapeBase = original;
				f32 dx                    = groundPos.x - shapeBase.x;
				f32 dz                    = groundPos.z - shapeBase.z;
				madeShape                = true;
				unk28[unk2C].unk0[0].x = request->unkC;
				unk28[unk2C].unk0[0].z = -request->unk10;
				unk28[unk2C].unk0[1].x = dx + request->unkC;
				unk28[unk2C].unk0[1].z = dz - request->unk10;
				unk28[unk2C].unk0[2].x = dx - request->unkC;
				unk28[unk2C].unk0[2].z = dz - request->unk10;
				unk28[unk2C].unk0[3].x = dx - request->unkC;
				unk28[unk2C].unk0[3].z = dz + request->unk10;
				unk28[unk2C].unk0[4].x = -request->unkC;
				unk28[unk2C].unk0[4].z = request->unk10;
				unk28[unk2C].unk0[0].y = 0.0f;
				unk28[unk2C].unk0[1].y = 0.0f;
				unk28[unk2C].unk0[2].y = 0.0f;
				unk28[unk2C].unk0[3].y = 0.0f;
				unk28[unk2C].unk0[4].y = 0.0f;
			}

			if (!madeShape) {
				JGeometry::TVec3<f32> shapeBase = original;
				f32 dx                    = groundPos.x - shapeBase.x;
				f32 dz                    = groundPos.z - shapeBase.z;
				unk28[unk2C].unk0[0].x = 1.0f;
				unk28[unk2C].unk0[0].z = 1.0f;
				unk28[unk2C].unk0[1].x = dx + 1.0f;
				unk28[unk2C].unk0[1].z = dz - 1.0f;
				unk28[unk2C].unk0[2].x = dx - 1.0f;
				unk28[unk2C].unk0[2].z = dz - 1.0f;
				unk28[unk2C].unk0[3].x = dx - 1.0f;
				unk28[unk2C].unk0[3].z = dz + 1.0f;
				unk28[unk2C].unk0[4].x = -1.0f;
				unk28[unk2C].unk0[4].z = 1.0f;
				unk28[unk2C].unk0[0].y = 0.0f;
				unk28[unk2C].unk0[1].y = 0.0f;
				unk28[unk2C].unk0[2].y = 0.0f;
				unk28[unk2C].unk0[3].y = 0.0f;
				unk28[unk2C].unk0[4].y = 0.0f;
			}

			quad->unk64 = unk28;
			++unk2C;
		}

		f32 shadowScale = 1.0f;
		if (request->unk20 == ACTOR_TYPE_SHADOW_MARIO)
			shadowScale = 1.5f;
		scaleZ *= shadowScale;

		MsMtxSetTRS(quad->unk4, mtxPos.x, mtxPos.y, mtxPos.z, rotX, rotY,
		            0.0f, scaleX, scaleY, scaleZ);
		PSMTXConcat(j3dSys.getViewMtx(), quad->unk4, quad->unk4);

		for (int j = 0; j < 4; ++j) {
			quad->unk34[j].x
			    = request->unk0.x + request->unkC * calctablex[j];
			quad->unk34[j].z
			    = request->unk0.z + request->unk10 * calctablez[j];
			quad->unk34[j].y = groundY;
		}

		if ((int)unk14 >= 0x200)
			return;
	}

	if ((int)unk14 == 0)
		return;

	if (mTestSw)
		return;

	TAlphaShadowQuad* shadowQuads      = unk18;
	TAlphaShadowBlendQuad* blendQuads  = unk24;
	TAlphaShadowQuadAry* shadowGroups  = unk1C;

	for (int i = 0; i < (int)unk20; ++i) {
		shadowGroups[i].unk4  = nullptr;
		shadowGroups[i].unkC  = nullptr;
		shadowGroups[i].unk8  = nullptr;
		shadowGroups[i].unk10 = nullptr;
		shadowGroups[i].unk0  = 0x20000000;
	}

	unk20 = 0;
	for (int i = 0; i < (int)unk14; ++i) {
		TAlphaShadowQuad* shadowQuad = &shadowQuads[i];
		shadowQuad->unk6C           = nullptr;

		TAlphaShadowBlendQuad* blend = &blendQuads[i];
		blend->unk1C                 = nullptr;
		blend->unk0.x                = shadowQuad->unk34[0].x;
		blend->unk0.z                = shadowQuad->unk34[0].z;
		blend->unkC.x                = shadowQuad->unk34[2].x;
		blend->unkC.z                = shadowQuad->unk34[2].z;
		blend->unk0.y                = shadowQuad->unk34[0].y;
		blend->unkC.y                = mYScalePlus + shadowQuad->unk0;
		blend->unk18                 = 0;

		u8 connected = false;
		for (int j = 0; j < (int)unk20; ++j) {
			if (conectCubeDiffer(shadowGroups[j].unkC, blend)) {
				connected        = true;
				shadowGroups[j].unk8->unk6C = shadowQuad;
				shadowGroups[j].unk8        = shadowQuad;
				shadowGroups[j].unk10->unk1C = blend;
				shadowGroups[j].unk10       = blend;
				break;
			}
		}

		if (connected)
			continue;

		if ((int)unk20 >= 0x100) {
			unk20 = 0x100;
			continue;
		}

		shadowGroups[unk20].unk4  = shadowQuad;
		shadowGroups[unk20].unk8  = shadowQuad;
		shadowGroups[unk20].unkC  = blend;
		shadowGroups[unk20].unk10 = blend;
		shadowGroups[unk20].unk0  = 0x20000000;
		if (shadowQuad->unk68->unk20 & 0x40000000)
			shadowGroups[unk20].unk0 = 0x40000000;
		++unk20;
	}

	for (int i = 0; i < (int)unk20; ++i) {
		for (int j = 0; j < (int)unk20; ++j) {
			if (i == j)
				continue;
			if (shadowGroups[i].unk4 == nullptr || shadowGroups[j].unk4 == nullptr)
				continue;
			if (!conectCubeSame(shadowGroups[i].unkC, shadowGroups[j].unkC))
				continue;

			shadowGroups[i].unk8->unk6C  = shadowGroups[j].unk4;
			shadowGroups[i].unk8         = shadowGroups[j].unk8;
			shadowGroups[i].unk10->unk1C = shadowGroups[j].unkC;
			shadowGroups[i].unk10        = shadowGroups[j].unk10;
			if ((shadowGroups[i].unk4->unk68->unk20 & 0x40000000)
			    || (shadowGroups[j].unk4->unk68->unk20 & 0x40000000)) {
				shadowGroups[i].unk0 = 0x40000000;
			}
			shadowGroups[j].unk4 = nullptr;
			shadowGroups[j].unkC = nullptr;
		}
	}
}
