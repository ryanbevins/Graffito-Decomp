#include <Map/PollutionLayer.hpp>
#include <Map/PollutionManager.hpp>
#include <MoveBG/MapObjWave.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/ReinitGX.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <JSystem/ResTIMG.hpp>
#include <dolphin/gx.h>
#include <dolphin/os/OSCache.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

class JAISound;
class MSound {
public:
	JAISound* startSoundSet(u32, const Vec*, u32, f32, u32, u32, u8);
};
extern MSound* gpMSound;

f32 TPollutionLayerWave::mInterval = 300.0f;
u8 TPollutionLayerWave::mAlpha     = 0xE6;
u32 TPollutionLayer::mEffectTime   = 15;

void TPollutionLayerWave::initGX() const
{
	ReInitializeGX();

	GXLoadPosMtxImm(j3dSys.mViewMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);

	GXColor color = { 0, 0, 0, mAlpha };
	GXSetChanMatColor(GX_COLOR0A0, color);

	JUTTexture texture(unk58);
	texture.load(GX_TEXMAP0);

	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_RASC, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetCullMode(GX_CULL_NONE);
	GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
	               GX_LO_NOOP);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetZCompLoc(GX_FALSE);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
}

void TPollutionLayerWave::draw() const
{
	f32 width  = unk3C - unk38;
	f32 height = unk44 - unk40;
	int count  = (u16)(int)(width / mInterval) * 2;
	f32 invX   = 1.0f / width;
	f32 invZ   = 1.0f / height;

	for (f32 z = unk40; z < unk44 - mInterval; z += mInterval) {
		GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, count);

		for (f32 x = unk38; x < unk3C - mInterval; x += mInterval) {
			f32 nextZ = z + mInterval;
			f32 y     = gpMapObjWave->getWaveHeight(x, z) - 10.0f;

			GXPosition3f32(x, y, z);
			GXTexCoord2f32((x - unk38) * invX, (z - unk40) * invZ);

			y = gpMapObjWave->getWaveHeight(x, nextZ) - 10.0f;

			GXPosition3f32(x, y, nextZ);
			GXTexCoord2f32((x - unk38) * invX, (nextZ - unk40) * invZ);
		}
	}
}

void TPollutionLayerWave::perform(u32 flags, JDrama::TGraphics*)
{
	if (flags & 8) {
		initGX();
		draw();
	}
}

ResTIMG* TPollutionLayerWave::getTexResource(const char* name)
{
	char fullPath[256];
	snprintf(fullPath, 256, "/scene/map/pollution/%s.bti", name);
	return (ResTIMG*)JKRGetResource(fullPath);
}

void TPollutionLayerWave::initJointModel(TJointModelManager* manager,
                                         const char* name, MActorAnmData*)
{
	mManager = manager;

	const TPollutionLayerInfo* info
	    = &((TPollutionManager*)mManager)->unk6C[mIndexInParent];
	initLayerInfo(info);
	unk5C.init(this, info->unk8, info->unkC, info->unk28, info->unk20,
	           info->unk22);
	unk88 = info->unk24;

	unk58               = getTexResource(name);
	unk58->alphaEnabled = 2;

	unk54 = (u8*)unk58 + unk58->imageDataOffset;
	initTexImage(name);
	if ((int)unk30 == 4)
		SMS_LoadParticle("/scene/map/pollution/ms_thunder_s.jpa", 0x6F);
}

void TPollutionLayerWallPlusZ::stamp(u16 type, f32 x, f32 y, f32 z, f32 size)
{
	if (!isInAreaSize(x, y, z, size))
		return;
	TPollutionPos* pos = &unk5C;
	gpPollution->getCounterLayer().pushStampTask(
	    type, mIndexInParent, pos->worldToTexSize(size), getTexPosS(x),
	    getTexPosT(y), pos->worldToDepth(z));
}

void TPollutionLayerWallPlusZ::initLayerInfo(
    const TPollutionLayerInfo* param_1)
{
	TPollutionLayer::initLayerInfo(param_1);
	unkAC = unk40;
	unkB0 = unk44;
	unk48 = 20;
}

void TPollutionLayerWallPlusX::stamp(u16 type, f32 x, f32 y, f32 z, f32 size)
{
	if (!isInAreaSize(x, y, z, size))
		return;
	TPollutionPos* pos = &unk5C;
	gpPollution->getCounterLayer().pushStampTask(
	    type, mIndexInParent, pos->worldToTexSize(size), getTexPosS(z),
	    getTexPosT(y), pos->worldToDepth(x));
}

void TPollutionLayerWallPlusX::initLayerInfo(
    const TPollutionLayerInfo* param_1)
{
	TPollutionLayer::initLayerInfo(param_1);
	unkAC = unk40;
	unkB0 = unk44;
	unk40 = unk38;
	unk44 = unk3C;
	unk48 = 20;
}

TPollutionLayerWallBase::TPollutionLayerWallBase()
{
	unkAC = 0.0f;
	unkB0 = 0.0f;
}

void TPollutionLayer::stampModel(J3DModel* model)
{
	f32 x = *(f32*)((u8*)model + 0x2C);
	f32 z = *(f32*)((u8*)model + 0x4C);

	if (x < unk38 || z < unk40 || x >= unk3C || z >= unk44)
		return;

	gpPollution->unk70.pushModelStampTask((u8)mIndexInParent, model);
}

void TPollutionLayer::appearItem(f32, f32, f32) { }

void TPollutionLayer::cleaned(f32 x, f32 y, f32 z, f32 size)
{
	static int effect_counter = 1;
	effect_counter++;
	if (effect_counter > 5) {
		effect_counter = 0;

		static JGeometry::TVec3<f32> pos[10];
		static int now_pos_no = 0;
		pos[now_pos_no].x    = x;
		pos[now_pos_no].y    = y;
		pos[now_pos_no].z    = z;

		static int x_offset_table[10]
		    = { -1, 0, 2, 4, 1, -1, -2, 0, 3, -3 };
		static int z_offset_table[10]
		    = { -1, -1, 0, 2, -2, -3, 0, 3, 0, 1 };
		static int counter_x = 0;
		static int counter_z = 0;

		pos[now_pos_no].x += x_offset_table[counter_x] * 32.0f;
		pos[now_pos_no].z += z_offset_table[counter_z] * 32.0f;

		counter_x++;
		counter_z += 3;
		if (counter_x >= 10)
			counter_x -= 10;
		if (counter_z >= 10)
			counter_z -= 10;

		if (isPolluted(pos[now_pos_no].x, pos[now_pos_no].y,
		               pos[now_pos_no].z)) {
			int t = getTexPosT(pos[now_pos_no].z);
			int s = getTexPosS(pos[now_pos_no].x);
			if (unk54[unk5C.index(s, t)] > 100) {
				if ((int)unkA8 <= 0)
					unkA8 = TPollutionManager::mFlushTime;

				gpMSound->startSoundSet(0x6809, (Vec*)&pos[now_pos_no],
				                        0, size, 0, 0, 4);

				static int effect_timer = 0;
				if (effect_timer == 0) {
					if (gpMarDirector->mMap == 5) {
						gpMarioParticleManager->emit(0x6A, &pos[now_pos_no],
						                             0, this);
					} else {
						gpMarioParticleManager->emit(0x1DB, &pos[now_pos_no],
						                             2, this);
					}
				}

				effect_timer++;
				if ((int)effect_timer > (int)mEffectTime)
					effect_timer = 0;

				now_pos_no++;
				if (now_pos_no > 10)
					now_pos_no = 0;
			}
		}
	}

	appearItem(x, y, z);
}

void TPollutionLayer::stamp(u16 stamp_type, f32 x, f32 y, f32 z, f32 size)
{
	if (!isInAreaSize(x, y, z, size))
		return;

	gpPollution->getCounterLayer().pushStampTask(
	    stamp_type, mIndexInParent, getPos().worldToTexSize(size), getTexPosS(x),
	    getTexPosT(z), unk5C.worldToDepth(y));

	if (getPlaneType() != 6
	    && gpPollution->getCounterLayer().stampIsCleanType(stamp_type))
		cleaned(x, y, z, size);
}

bool TPollutionLayer::isPolluted(f32 x, f32 y, f32 z) const
{
	if (!isInArea(x, y, z))
		return false;

	if (getPlaneType() == 6 && y > 0.0f)
		return false;

	int s = getTexPosS(x);
	int t = getTexPosT(z);
	if (!unk5C.isSame(s, t, y))
		return false;

	if (unk54[unk5C.index(s, t)] > unk50)
		return true;
	return false;
}

void TPollutionLayer::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1)
		action();

	TJointModel::perform(flags, graphics);
}

void TPollutionLayer::initTexImage(const char* param_1)
{
	char fullPath[256];
	snprintf(fullPath, 256, "/scene/map/pollution/%s.bmp", param_1);
	unk80 = (u8*)JKRGetResource(fullPath);

	bool cVar1 = false;
	if (gpMarDirector->mMap == 9)
		cVar1 = true;

	for (int y = 0; y < unk5C.mHeight; ++y) {
		for (int x = 0; x < unk5C.mWidth; ++x) {
			u8 depth = unk80[0x436 + x + unk5C.mWidth * (unk5C.mHeight - 1 - y)];
			bool shouldClear = false;
			if (cVar1
			    && (x <= 0 || x >= unk5C.mWidth - 1 || y <= 0
			        || y >= unk5C.mHeight - 1))
				shouldClear = true;
			if (depth == 0 || unk5C.isProhibit(x, y))
				shouldClear = true;

			if (!shouldClear) {
				int degree = unk5C.getEdgeDegree(x, y);
				if (degree != 0) {
					unk54[unk5C.index(x, y)]
					    = depth - degree * TPollutionManager::mEdgeAlpha;
				} else {
					unk54[unk5C.index(x, y)] = depth;
				}
			} else {
				unk54[unk5C.index(x, y)] = 0;
			}
		}
	}
	DCStoreRange(unk54, unk5C.mWidth * unk5C.mHeight);
}

void TPollutionLayer::initLayerInfo(const TPollutionLayerInfo* param_1)
{
	unk30 = param_1->unk0;
	unk32 = param_1->unk2;
	unk38 = param_1->unk10;
	unk40 = param_1->unk14;
	unk3C = param_1->unk18;
	unk44 = param_1->unk1C;
	unk48 = 2;

	if ((int)unk30 == 7) {
		unk50 = 200;
		unk85 = 0xA0;
	} else if ((int)unk30 == 1) {
		unk50 = 0x80;
		unk85 = 0x80;
	} else {
		unk50 = 30;
		unk85 = 50;
	}

	unk94 = 0x1E;
	unk98 = new JGeometry::TVec3<f32>[unk94];
	memset(unk98, 0, unk94 * sizeof(unk98[0]));
}

void TPollutionLayer::initJointModel(TJointModelManager* param_1,
                                     const char* param_2,
                                     MActorAnmData* param_3)
{
	TJointModel::initJointModel(param_1, param_2, param_3);
	const TPollutionLayerInfo* info
	    = &((TPollutionManager*)mManager)->unk6C[mIndexInParent];
	initLayerInfo(info);
	unk5C.init(this, info->unk8, info->unkC, info->unk28, info->unk20,
	           info->unk22);
	unk88 = info->unk24;

	unk58               = getTexResource(param_2);
	unk58->alphaEnabled = 2;

	unk54 = (u8*)unk58 + unk58->imageDataOffset;
	initTexImage(param_2);
	if ((int)unk30 == 4)
		SMS_LoadParticle("/scene/map/pollution/ms_thunder_s.jpa", 0x6F);

	if (mActor->checkAnmFileExist(param_2, 4))
		mActor->setBtk(param_2);

	if (mActor->checkAnmFileExist(param_2, 2))
		mActor->setBpk(param_2);

	if (mActor->checkAnmFileExist(param_2, 5))
		mActor->setBrk(param_2);

	for (int i = 0; i < mChildrenNum; ++i)
		((TPollutionObj*)mChildren[i])->initAreaInfo(this);
}

TPollutionLayer::TPollutionLayer()
    : unk30(0)
    , unk32(0)
    , unk34(0)
    , unk38(0.0f)
    , unk3C(0.0f)
    , unk40(0.0f)
    , unk44(0.0f)
    , unk48(0)
    , unk4C(0)
    , unk50(0)
    , unk54(nullptr)
    , unk58(nullptr)
    , unk80(nullptr)
    , unk84(8)
    , unk85(0)
    , unk8C(0)
    , unk90(0)
    , unk94(0)
    , unk98(nullptr)
    , unk9C(1)
    , unkA0(1000)
    , unkA4(100)
    , unkA8(0)
{
}
