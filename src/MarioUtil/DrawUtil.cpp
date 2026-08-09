#define JGEOMETRY_DRAWUTIL_OWNER_HELPERS
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Camera/Camera.hpp>
#include <Camera/SunMgr.hpp>
#include <Map/JointModel.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/PollutionLayer.hpp>
#include <System/MarDirector.hpp>
#include <System/TexCache.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphBase/Blocks/J3DPEBlocks.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <dolphin/os/OSCache.h>
#include <stdlib.h>

#undef JGEOMETRY_DRAWUTIL_OWNER_HELPERS

TSilhouette* gpSilhouetteManager;

void TSilhouette::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);
	unk12               = (GXColor) { 0, 0x27, 0x77, 0xff };
	unk16               = (GXColor) { 0xff, 0xff, 0xff, 0xff };
	unk1C               = 100.0f;
	unk20               = 0.9f;
	gpSilhouetteManager = this;
}

void TSilhouette::loadAfter()
{
	// TODO: ewwww floats
	unk10     = 1;
	unk12     = gpSunMgr->unk18;
	unk12.a   = 0;
	unk24     = 30.0f;
	unk28     = 650.0f;
	unk2C     = 1500.0f;
	f32 m[3][2];
	f32 dist[3];
	f32 atten[3] = { 0.9f, 0.5f, 0.05f };

	dist[0] = unk24;
	dist[1] = unk28;
	dist[2] = unk2C;

	for (int i = 0; i < 2; ++i) {
		m[0][i]
		    = atten[i + 1]
		      * (atten[i] * (dist[i] * dist[i] - dist[i + 1] * dist[i + 1]));
		m[1][i] = atten[i + 1] * (atten[i] * (dist[i] - dist[i + 1]));
		m[2][i] = atten[i + 1] - atten[i];
	}

	unk38 = (m[2][0] * m[1][1] - m[2][1] * m[1][0])
	        / (m[0][0] * m[1][1] - m[0][1] * m[1][0]);
	unk34 = (m[2][0] - m[0][0] * unk38) / m[1][0];
	unk30 = atten[0] - (dist[0] * dist[0] * unk38 + dist[0] * unk34);
	unk3C = 8e-05f;

	if (gpPollution->getJointModelNum() > 0) {
		TPollutionLayer* pJVar9
		    = (TPollutionLayer*)gpPollution->getJointModel(0);
		ResTIMG* img = pJVar9->unk58;
		unk40        = new JUTTexture(img);
	}
	ResTIMG* pRVar8 = (ResTIMG*)JKRFileLoader::getGlbResource(
	    "/common/timg/H_marukage_xlu_i8.bti");

	unk44 = new JUTTexture(pRVar8);
	unk48 = 0.0f;
	unk4C = 0.01f;
	unk50 = 128.0f;
}

void TSilhouette::setting(MtxPtr param_1)
{
	GXSetChanAmbColor(GX_COLOR0A0, (GXColor) { unk12.r, unk12.g, unk12.b, 0 });
	Vec local_6C = *gpMarioPos;
	Vec local_60;
	PSMTXMultVec(param_1, &local_6C, &local_60);
	GXLightObj GStack_54;
	GXInitLightPos(&GStack_54, local_60.x, local_60.y, local_60.z);
	GXInitLightAttnK(&GStack_54, unk30, unk34, unk38);
	GXInitLightAttnA(&GStack_54, 1.0f, 0.0f, 0.0f);
	unk16   = unk12;
	unk16.a = unk48;
	GXInitLightColor(&GStack_54, unk16);
	GXLoadLightObjImm(&GStack_54, GX_LIGHT0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, 1, GX_DF_NONE,
	              GX_AF_SPOT);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
	GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
}

void TSilhouette::perform(u32 param_1, JDrama::TGraphics* param_2)
{

	if ((param_1 & 1) != 0) {
		f32 fVar1 = SMS_CheckMarioFlag(1) ? unk50 : 0.0f;
		unk48 += unk4C * (fVar1 - unk48);
		unk12.a = unk48;
	}

	if ((param_1 & 8) != 0) {
		GXSetNumChans(1);
		GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 1,
		              GX_DF_NONE, GX_AF_SPOT);
		GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
		              GX_DF_NONE, GX_AF_NONE);
		GXSetChanMatColor(GX_COLOR0A0, unk12);
		setting(param_2->getUnkB4());
	}
	if ((param_1 & 0x80) != 0) {
		GXColor color = unk12;
		color.a       = gpSunMgr->unk18.a;
		GXSetChanMatColor(GX_COLOR0A0, color);
		setting(param_2->getUnkB4());
	}
	if (((param_1 & 0x10) != 0) && gpPollution->getJointModelNum()) {
		Mtx afStack_80;
		C_MTXLightFrustum(afStack_80, -1.0f, 1.0f, -1.0f, 1.0f, 10.0f, 0.5f,
		                  0.5f, 0.5f, 0.5f);
		Mtx afStack_b0;
		PSMTXRotRad(afStack_b0, 0x58, 1.570796f);
		Mtx afStack_50;
		PSMTXConcat(afStack_80, afStack_b0, afStack_50);
		Mtx afStack_e0;
		PSMTXScale(afStack_e0, unk3C, unk3C, unk3C);
		Mtx afStack_110;
		PSMTXTrans(afStack_110, -gpMarioPos->x, 0.0f, -gpMarioPos->z);
		Mtx afStack_140;
		PSMTXTrans(afStack_140, 1.75f, 1.75f, 0.0f);
		PSMTXConcat(afStack_e0, afStack_110, afStack_e0);
		PSMTXConcat(afStack_50, afStack_e0, afStack_50);
		PSMTXConcat(afStack_140, afStack_50, afStack_50);
		GXLoadTexMtxImm(afStack_50, 0x1e, GX_MTX3x4);
		GXSetNumTexGens(2);
		GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3c, 0,
		                  0x7d);
		GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_POS, 0x1e, 0, 0x7d);
		unk40->load(GX_TEXMAP0);
		unk44->load(GX_TEXMAP1);
		GXSetNumChans(1);
		GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_REG, 1, GX_DF_NONE,
		              GX_AF_SPOT);
		GXSetChanCtrl(GX_COLOR1A1, 0, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
		              GX_AF_NONE);
		GXColor color = unk12;
		GXSetChanMatColor(GX_COLOR0A0, color);
		color.a = 0x40;
		GXSetTevColor(GX_TEVREG0, color);
		GXSetNumTevStages(2);
		GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC,
		                GX_CC_C0);
		GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
		                GX_TEVPREV);
		GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA,
		                GX_CA_A0);
		GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, 1,
		                GX_TEVPREV);
		GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
		GXSetTevOp(GX_TEVSTAGE1, GX_MODULATE);
		GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
		GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
		               GX_LO_NOOP);
		GXSetZCompLoc(GX_TRUE);
		GXSetZMode(GX_TRUE, GX_GEQUAL, GX_FALSE);
	}
}

void TTrembleModelEffect::init(J3DModel* model)
{
	int found = 0;
	unk0      = model;
	unk10     = 0;

	GXVtxAttrFmtList* fmt
	    = model->getModelData()->getVertexData().getVtxAttrFmtList();
	while (fmt->attr != GX_VA_NULL) {
		if (fmt->attr == GX_VA_POS) {
			if (fmt->type == GX_S16) {
				unk8  = 0;
				found = 1;
				unkA  = fmt->frac;
				unkC  = 1 << fmt->frac;
			} else if (fmt->type == GX_F32) {
				unk8  = 0;
				found = 1;
				unk8 |= 2;
				unkA = 0;
				unkC = 1;
			}
			break;
		}
	}

	if (found == 1) {
		unk4  = unk0->getModelData()->getVertexData().getVtxPosArray();
		u32 n = unk0->getModelData()->getVertexData().getVtxNum();
		unk9  = 0;

		switch (unk8 & 2) {
		case 0: {
			unk14     = new JGeometry::TVec3<s16>[n];
			unk18[0]  = new JGeometry::TVec3<s16>[n];
			unk18[1]  = new JGeometry::TVec3<s16>[n];
			unk20     = new JGeometry::TVec3<s16>[n];
			unk24     = 0;
			unk26     = 0;
			void* src = model->getModelData()->getVertexData().getVtxPosArray();
			for (u32 i = 0; i < n; ++i) {
				unk14[i]    = ((JGeometry::TVec3<s16>*)src)[i];
				unk18[0][i] = ((JGeometry::TVec3<s16>*)src)[i];
				unk18[1][i] = ((JGeometry::TVec3<s16>*)src)[i];
				unk20[i].set(0, 0, 0);
			}
			break;
		}
		case 2: {
			unk28     = new JGeometry::TVec3<f32>[n];
			unk2C[0]  = new JGeometry::TVec3<f32>[n];
			unk2C[1]  = new JGeometry::TVec3<f32>[n];
			unk34     = new JGeometry::TVec3<f32>[n];
			unk38     = 0.0f;
			unk3C     = 0.0f;
			void* src = model->getModelData()->getVertexData().getVtxPosArray();
			for (u32 i = 0; i < n; ++i) {
				unk28[i]    = ((JGeometry::TVec3<f32>*)src)[i];
				unk2C[0][i] = ((JGeometry::TVec3<f32>*)src)[i];
				unk2C[1][i] = ((JGeometry::TVec3<f32>*)src)[i];
				unk34[i].set(0.0f, 0.0f, 0.0f);
			}
			break;
		}
		}
	}
}

void TTrembleModelEffect::tremble(f32 power, f32 spring, f32 damping,
                                  int frames)
{
	unk8 |= 1;

	switch (unk8 & 2) {
	case 0: {
		unk26 = (s16)(spring * (f32)unkC);
		unk24 = (s16)(damping * (f32)unkC);

		JGeometry::TVec3<s16>* original
		    = (JGeometry::TVec3<s16>*)unk4;

		for (u32 i = 0; i < unk0->mModelData->getVtxNum(); ++i) {
			unk20[i].x = (s16)((f32)unkC
			                   * (power
			                      * (2.0f
			                               * ((f32)rand()
			                                  * 0.000030517578f)
			                         - 1.0f)));
			unk20[i].y = (s16)((f32)unkC
			                   * (power
			                      * (2.0f
			                               * ((f32)rand()
			                                  * 0.000030517578f)
			                         - 1.0f)));
			unk20[i].z = (s16)((f32)unkC
			                   * (power
			                      * (2.0f
			                               * ((f32)rand()
			                                  * 0.000030517578f)
			                         - 1.0f)));

			unk14[i] = original[i];
			unk18[0][i] = original[i];
			unk18[1][i] = original[i];
		}
		break;
	}
	case 2: {
		unk3C = spring;
		unk38 = damping;

		JGeometry::TVec3<f32>* original
		    = (JGeometry::TVec3<f32>*)unk4;

		for (u32 i = 0; i < unk0->mModelData->getVtxNum(); ++i) {
			unk34[i].x = power
			             * (2.0f * ((f32)rand() * 0.000030517578f)
			                - 1.0f);
			unk34[i].y = power
			             * (2.0f * ((f32)rand() * 0.000030517578f)
			                - 1.0f);
			unk34[i].z = power
			             * (2.0f * ((f32)rand() * 0.000030517578f)
			                - 1.0f);

			unk28[i] = original[i];
			unk2C[0][i] = original[i];
			unk2C[1][i] = original[i];
		}
		break;
	}
	}

	unk10 = frames;
	unk8 &= ~4;
}

#pragma dont_inline on
namespace JGeometry {
void TVec3<s16>::add(const TVec3<s16>& operand)
{
	x += operand.x;
	y += operand.y;
	z += operand.z;
}

void TVec3<f32>::add(const TVec3<f32>& operand)
{
	x += operand.x;
	y += operand.y;
	z += operand.z;
}
}
#pragma dont_inline off

void TTrembleModelEffect::clash(f32 power)
{
	tremble(power, 0.0f, 0.0f, 0);

	switch (unk8 & 2) {
	case 0:
		for (u32 i = 0; i < unk0->mModelData->getVtxNum(); ++i) {
			JGeometry::TVec3<s16> position = unk14[i];
			position.add(unk20[i]);

			unk14[i] = position;
			unk18[0][i] = position;
			unk18[1][i] = position;
		}
		break;
	case 2:
		for (u32 i = 0; i < unk0->mModelData->getVtxNum(); ++i) {
			JGeometry::TVec3<f32> position = unk28[i];
			position.add(unk34[i]);

			unk28[i] = position;
			unk2C[0][i] = position;
			unk2C[1][i] = position;
		}
		break;
	}

	unk8 |= 4;
}

#pragma dont_inline on
namespace JGeometry {
void TVec3<s16>::sub(const TVec3<s16>& operand)
{
	x -= operand.x;
	y -= operand.y;
	z -= operand.z;
}

void TVec3<f32>::sub(const TVec3<f32>& operand)
{
	x -= operand.x;
	y -= operand.y;
	z -= operand.z;
}
}
#pragma dont_inline off

void TTrembleModelEffect::movement()
{
	if ((unk8 & 1) == 0)
		return;

	if ((unk8 & 4) != 4) {
		unk10--;
		if (unk10 <= 0) {
			reset();
			return;
		}
	}

	switch (unk8 & 2) {
	case 0: {
		JGeometry::TVec3<s16>* original
		    = (JGeometry::TVec3<s16>*)unk4;

		u32 vtxNum = unk0->mModelData->getVtxNum();
		for (u32 i = 0; i < vtxNum; ++i) {
			JGeometry::TVec3<s16> displacement = original[i];
			displacement.sub(unk14[i]);

			unk20[i].x += (s16)((displacement.x * unk26) >> unkA);
			unk20[i].y += (s16)((displacement.y * unk26) >> unkA);
			unk20[i].z += (s16)((displacement.z * unk26) >> unkA);

			unk20[i].x = (s16)((unk24 * unk20[i].x) >> unkA);
			unk20[i].y = (s16)((unk24 * unk20[i].y) >> unkA);
			unk20[i].z = (s16)((unk24 * unk20[i].z) >> unkA);

			unk14[i].x += unk20[i].x;
			unk14[i].y += unk20[i].y;
			unk14[i].z += unk20[i].z;

			unk18[unk9][i] = unk14[i];
		}

		JGeometry::TVec3<s16>* current = unk18[unk9];
		DCFlushRange(current, vtxNum * sizeof(JGeometry::TVec3<s16>));
		unk0->mVertexBuffer->unk4[0] = current;

		for (int i = 0; i < unk0->mModelData->getShapeNum(); ++i)
			unk0->mShapePackets->unk24 = current;
		break;
	}
	case 2: {
		JGeometry::TVec3<f32>* original
		    = (JGeometry::TVec3<f32>*)unk4;

		u32 vtxNum = unk0->mModelData->getVtxNum();
		for (u32 i = 0; i < vtxNum; ++i) {
			JGeometry::TVec3<f32> displacement = original[i];
			displacement.sub(unk28[i]);

			unk34[i].x += displacement.x * unk3C;
			unk34[i].y += displacement.y * unk3C;
			unk34[i].z += displacement.z * unk3C;

			unk34[i].x *= unk38;
			unk34[i].y *= unk38;
			unk34[i].z *= unk38;

			unk28[i].x += unk34[i].x;
			unk28[i].y += unk34[i].y;
			unk28[i].z += unk34[i].z;

			unk2C[unk9][i] = unk28[i];
		}

		JGeometry::TVec3<f32>* current = unk2C[unk9];
		DCFlushRange(current, vtxNum * sizeof(JGeometry::TVec3<f32>));
		unk0->mVertexBuffer->unk4[0] = current;

		for (int i = 0; i < unk0->mModelData->getShapeNum(); ++i)
			unk0->mShapePackets->unk24 = current;
		break;
	}
	}

	unk9 = 1 - unk9;
}

void TTrembleModelEffect::reset()
{
	switch (unk8 & 2) {
	case 0: {
		JGeometry::TVec3<s16>* original
		    = (JGeometry::TVec3<s16>*)unk4;

		for (u32 i = 0; i < unk0->mModelData->getVtxNum(); ++i) {
			unk20[i].zero();
			unk14[i] = original[i];
			unk18[0][i] = original[i];
			unk18[1][i] = original[i];
		}
		break;
	}
	case 2: {
		JGeometry::TVec3<f32>* original
		    = (JGeometry::TVec3<f32>*)unk4;

		for (u32 i = 0; i < unk0->mModelData->getVtxNum(); ++i) {
			unk34[i].zero();
			unk28[i] = original[i];
			unk2C[0][i] = original[i];
			unk2C[1][i] = original[i];
		}
		break;
	}
	}

	unk8 &= ~1;
	GXInvalidateVtxCache();

	unk0->mModelData->mVertexData.mVtxPosArray = unk4;
	unk0->mVertexBuffer->unk4[0]               = unk4;
	unk0->mVertexBuffer->unk4[1]               = unk4;
	unk0->mVertexBuffer->unk2C                 = unk4;
}

void SMS_AddDamageFogEffect(J3DModelData* modelData,
                            const JGeometry::TVec3<f32>& position,
                            JDrama::TGraphics* graphics)
{
	Vec viewPos;
	MTXMultVec(graphics->mViewMtx, (Vec*)&position, &viewPos);

	f32 startZ = -700.0f;
	f32 endZ   = 500.0f;
	f32 sin    = jmaSinTable[((u16)(gpMarDirector->unk58 * 0x888))
	                       >> jmaSinShift];
	f32 startOffset = (-400.0f - startZ) * sin;
	f32 endOffset   = (800.0f - endZ) * sin;

	for (u16 i = 0; i < modelData->getMaterialNum(); ++i) {
		J3DFog* fog
		    = modelData->getMaterialNodePointer(i)->getPEBlock()->getFog();
		fog->mStartZ = -viewPos.z + startZ + startOffset;
		fog->mEndZ   = -viewPos.z + endZ + endOffset;
		fog->mNearZ  = gpCamera->mNear;
		fog->mFarZ   = gpCamera->mFar;
	}
}

void SMS_ResetDamageFogEffect(J3DModelData* modelData)
{
	for (u16 i = 0; i < modelData->getMaterialNum(); ++i) {
		J3DFog* fog
		    = modelData->getMaterialNodePointer(i)->getPEBlock()->getFog();
		fog->mNearZ = gpCamera->mNear;
		fog->mFarZ  = gpCamera->mFar;
		fog->mEndZ  = fog->mFarZ;
		fog->mStartZ = fog->mEndZ - 1.0f;
	}
}

// fabricated
struct Plane {
	f32 nx;
	f32 ny;
	f32 nz;
	f32 offset; // from origin

	// fabricated
	f32 sdf(Vec* local)
	{
		return local->x * nx + local->y * ny + local->z * nz + offset;
	}

	// fabricated
	void set(Vec* normal, Vec* point)
	{
		nx = normal->x;
		ny = normal->y;
		nz = normal->z;
		// project any point on a plane onto it's normal and you get the
		// distance from the origin
		offset = -(normal->x * point->x + normal->y * point->y
		           + normal->z * point->z);
	}

	// fabricated
	void set(Vec* p1, Vec* p2, Vec* p3)
	{
		Vec v1;
		Vec v2;
		Vec normal;

		VECSubtract(p1, p3, &v1);
		VECSubtract(p2, p3, &v2);
		VECCrossProduct(&v1, &v2, &normal);
		MsVECNormalize(&normal, &normal);

		nx = normal.x;
		ny = normal.y;
		nz = normal.z;
		// project any point on a plane onto it's normal and you get the
		// distance from the origin
		offset = -(normal.x * p3->x + normal.y * p3->y + normal.z * p3->z);
	}
};

Plane sViewPlane[6];

static void SetViewFrustumClipCheck(f32 top, f32 bottom, f32 left, f32 right,
                                    f32 near, f32 far)
{
	f32 farTop    = top * (far / near);
	f32 farBottom = bottom * (far / near);
	f32 farLeft   = left * (far / near);
	f32 farRight  = right * (far / near);

	Vec local_98;
	Vec local_8c;
	Vec local_80;
	Vec local_74;
	Vec local_68;
	Vec local_5c;
	Vec local_50;
	Vec local_44;

	local_44.x = left;
	local_44.y = top;
	local_44.z = -near;

	local_50.x = farLeft;
	local_50.y = farTop;
	local_50.z = -far;

	local_5c.x = right;
	local_5c.y = top;
	local_5c.z = -near;

	local_68.x = farRight;
	local_68.y = farTop;
	local_68.z = -far;

	local_74.x = left;
	local_74.y = bottom;
	local_74.z = -near;

	local_80.x = farLeft;
	local_80.y = farBottom;
	local_80.z = -far;

	local_8c.x = right;
	local_8c.y = bottom;
	local_8c.z = -near;

	local_98.x = farRight;
	local_98.y = farBottom;
	local_98.z = -far;

	Vec v1;
	Vec v2;
	Vec normal;

	VECSubtract(&local_50, &local_44, &v1);
	VECSubtract(&local_5c, &local_44, &v2);
	VECCrossProduct(&v1, &v2, &normal);
	MsVECNormalize(&normal, &normal);
	sViewPlane[0].set(&normal, &local_44);

	VECSubtract(&local_8c, &local_74, &v1);
	VECSubtract(&local_80, &local_74, &v2);
	VECCrossProduct(&v1, &v2, &normal);
	MsVECNormalize(&normal, &normal);
	sViewPlane[1].set(&normal, &local_74);

	VECSubtract(&local_74, &local_44, &v1);
	VECSubtract(&local_50, &local_44, &v2);
	VECCrossProduct(&v1, &v2, &normal);
	MsVECNormalize(&normal, &normal);
	sViewPlane[2].set(&normal, &local_44);

	VECSubtract(&local_68, &local_5c, &v1);
	VECSubtract(&local_8c, &local_5c, &v2);
	VECCrossProduct(&v1, &v2, &normal);
	MsVECNormalize(&normal, &normal);
	sViewPlane[3].set(&normal, &local_5c);

	VECSubtract(&local_5c, &local_44, &v1);
	VECSubtract(&local_74, &local_44, &v2);
	VECCrossProduct(&v1, &v2, &normal);
	MsVECNormalize(&normal, &normal);
	sViewPlane[4].set(&normal, &local_44);

	VECSubtract(&local_68, &local_98, &v1);
	VECSubtract(&local_80, &local_98, &v2);
	VECCrossProduct(&v1, &v2, &normal);
	MsVECNormalize(&normal, &normal);
	sViewPlane[5].set(&normal, &local_98);
}

static f32 sKeepViewClipFovy;
static f32 sKeepViewClipAspect;
static f32 sKeepViewClipNear;
static f32 sKeepViewClipFar;

void SetViewFrustumClipCheckPerspective(f32 fovy, f32 aspect, f32 clip_near,
                                        f32 clip_far)
{
	if (fovy != sKeepViewClipFovy || aspect != sKeepViewClipAspect
	    || clip_near != sKeepViewClipNear || clip_far != sKeepViewClipFar) {
		sKeepViewClipFovy   = fovy;
		sKeepViewClipAspect = aspect;
		sKeepViewClipNear   = clip_near;
		sKeepViewClipFar    = clip_far;

		//       E
		//      /|\
		//     / | \
		//    /  |  \
		//   /   |   \
		//  /____|____\
		// L     C     R
		//
		// Vertical slice of camera frustsum, E is the eye, LR is the near
		// plane. LER then is the fovy. LEC is half of it. Tan of LEC is
		// by definition LC/EC, but EC is the near plane dist, so we get
		// what we want -- LC, half the vertical span of the near plane.
		f32 tan          = tanf(fovy * (3.141593f / 180.0f / 2.0f));
		f32 vertHalfSize = clip_near * tan;
		f32 horHalfSize  = vertHalfSize * aspect;
		SetViewFrustumClipCheck(vertHalfSize, -vertHalfSize, -horHalfSize,
		                        horHalfSize, clip_near, clip_far);
	}
}

BOOL ViewFrustumClipCheck(JDrama::TGraphics* gfx, Vec* position, f32 radius)
{
	Vec local_18;
	MTXMultVec(gfx->mViewMtx, position, &local_18);

	for (int i = 0; i < 6; ++i)
		if (-radius > sViewPlane[i].sdf(&local_18))
			return false;

	return true;
}

int SMS_CountPolygonNumInShape(J3DShape* shape)
{
	int sizeTable[4] = { 0, 1, 1, 2 };
	int polyNum      = 0;
	int vtxSize      = 0;

	for (GXVtxDescList* desc = shape->getVtxDesc(); desc->attr != GX_VA_NULL;
	     desc++) {
		vtxSize += sizeTable[desc->type];
	}

	for (u16 i = 0; i < shape->getMtxGroupNum(); i++) {
		u8* dl = shape->getShapeDraw(i)->getDisplayList();
		u8* p  = dl;
		while (p - dl < shape->getShapeDraw(i)->getDisplayListSize()) {
			u8 op = *p;
			if (op == GX_TRIANGLEFAN || op == GX_TRIANGLESTRIP) {
				u16 n = *(u16*)(p + 1);
				polyNum = n + polyNum;
				p += vtxSize * n;
				polyNum -= 2;
				p += 3;
			} else {
				break;
			}
		}
	}

	return polyNum;
}

void SMS_DrawCube(const JGeometry::TVec3<f32>& min,
                  const JGeometry::TVec3<f32>& max)
{
	GXBegin(GX_QUADS, GX_VTXFMT0, 24);

	GXPosition3f32(min.x, min.y, min.z);
	GXPosition3f32(min.x, min.y, max.z);
	GXPosition3f32(max.x, min.y, max.z);
	GXPosition3f32(max.x, min.y, min.z);

	GXPosition3f32(min.x, min.y, min.z);
	GXPosition3f32(max.x, min.y, min.z);
	GXPosition3f32(max.x, max.y, min.z);
	GXPosition3f32(min.x, max.y, min.z);

	GXPosition3f32(min.x, min.y, min.z);
	GXPosition3f32(min.x, max.y, min.z);
	GXPosition3f32(min.x, max.y, max.z);
	GXPosition3f32(min.x, min.y, max.z);

	GXPosition3f32(max.x, max.y, max.z);
	GXPosition3f32(min.x, max.y, max.z);
	GXPosition3f32(min.x, max.y, min.z);
	GXPosition3f32(max.x, max.y, min.z);

	GXPosition3f32(max.x, max.y, max.z);
	GXPosition3f32(max.x, min.y, max.z);
	GXPosition3f32(min.x, min.y, max.z);
	GXPosition3f32(min.x, max.y, max.z);

	GXPosition3f32(max.x, max.y, max.z);
	GXPosition3f32(max.x, max.y, min.z);
	GXPosition3f32(max.x, min.y, min.z);
	GXPosition3f32(max.x, min.y, max.z);
}

void SMS_SettingDrawShape(J3DModelData* param_1, u16 param_2)
{
	J3DShape* shape = param_1->getShapeNodePointer(param_2);
	GXCallDisplayList(shape->getDrawList(), 0xC0);
	J3DVertexData& data = param_1->getVertexData();
	j3dSys.unk10C       = data.getVtxPosArray();
	j3dSys.unk110       = data.getVtxNormArray();
	shape->loadVtxArray();
}

void SMS_DrawShape(J3DModelData* param_1, u16 param_2)
{
	J3DShape* shape = param_1->getShapeNodePointer(param_2);
	for (u16 i = 0; i < shape->getMtxGroupNum(); ++i)
		shape->getShapeDraw(i)->draw();
}

void SMS_MakeDLAndLock(J3DModel* param_1)
{
	param_1->prepareShapePackets();
	for (u16 i = 0; i < param_1->getModelData()->getMaterialNum(); ++i) {
		param_1->getModelData()->getMaterialNodePointer(i)->calc(
		    (MtxPtr)j3dDefaultMtx);
	}
	param_1->makeDL();
	param_1->lock();
}

void SMS_DrawInit()
{
	j3dSys.drawInit();
	SMS_ResetTexCacheRegion();
}

void SMS_ShowJoint(J3DMaterial* param_1, bool param_2)
{
	if (param_2) {
		for (; param_1 != nullptr; param_1 = param_1->getNext())
			param_1->getShape()->offFlag(1);
	} else {
		for (; param_1 != nullptr; param_1 = param_1->getNext())
			param_1->getShape()->onFlag(1);
	}
}

#pragma dont_inline on
namespace JGeometry {
void TRotation3<TMatrix34<SMatrix34C<f32> > >::identity33()
{
	this->ref(0, 0) = 1.0f;
	this->ref(1, 0) = 0.0f;
	this->ref(2, 0) = 0.0f;

	this->ref(0, 1) = 0.0f;
	this->ref(1, 1) = 1.0f;
	this->ref(2, 1) = 0.0f;

	this->ref(0, 2) = 0.0f;
	this->ref(1, 2) = 0.0f;
	this->ref(2, 2) = 1.0f;
}
}
#pragma dont_inline off

void SMS_CalcMatAnmAndMakeDL(J3DModel* param_1, u16 param_2)
{
	J3DMaterial* mat = param_1->getModelData()->getMaterialNodePointer(param_2);
	param_1->getModelData()
	    ->getMaterialNodePointer(param_2)
	    ->getMaterialAnm()
	    ->calc(mat);
	j3dSys.setMatPacket(param_1->getMatPacket(param_2));
	mat->makeDisplayList();
}

void SMS_UnifyMaterial(J3DModel* param_1)
{
	J3DModelData* modelData = param_1->getModelData();
	J3DMaterial* unifier    = modelData->getMaterialNodePointer(0);
	for (u16 i = 0; i < modelData->getMaterialNum(); i = i + 1) {
		J3DMaterial* mat = param_1->getModelData()->getMaterialNodePointer(i);
		u32 thing        = unifier->unk18 & 0x7fffffff;
		mat->unk18       = thing;
		param_1->getMatPacket(i)->unk3C = thing;

		u16 texNo = unifier->getTevBlock()->getTexNo(0);
		mat->getTevBlock()->setTexNo(0, texNo);
	}
}
