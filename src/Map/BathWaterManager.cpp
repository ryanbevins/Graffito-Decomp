#include <Map/BathWaterManager.hpp>

#include <dolphin/mtx.h>
#include <dolphin/gx.h>
#include <dolphin/gd/GDBase.h>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTevs.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTexture.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JKernel/JKRHeap.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <Camera/Camera.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/ScreenUtil.hpp>
#include <MoveBG/MapObjCorona.hpp>
#include <MSound/MSound.hpp>
#include <Player/MarioAccess.hpp>
#include <System/Resolution.hpp>
#include <dolphin/os/OSCache.h>
#include <math.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

// Infectious strings from the MActor headers used by the original TU. Their
// pointer tables are not emitted in this object; only the literals survive.
static const char SMS_NO_MEMORY_MESSAGE[] = "メモリが足りません\n";
static const char dummyMactorStringValue1[] = "\0\0\0\0\0\0\0\0\0\0\0";
static const char MtxCalcTypeNameBasic[]
    = "MActorMtxCalcType_Basic クラシックスケールＯＮ";
static const char MtxCalcTypeNameSoftimage[]
    = "MActorMtxCalcType_Softimage クラシックスケールＯＦＦ";
static const char MtxCalcTypeNameMotionBlend[]
    = "MActorMtxCalcType_MotionBlend モーションブレンド";
static const char MtxCalcTypeNameUser[]
    = "MActorMtxCalcType_User ユーザー定義";

TBathWaterParams::TBathWaterParams(const char* path)
    : TParams(path)
    , PARAM_INIT(suppliesDrops, 1)
    , PARAM_INIT(bathtubGravity, 1)
    , PARAM_INIT(intersects, 1)
    , PARAM_INIT(isVisible, 1)
    , PARAM_INIT(checksMario, 1)
    , PARAM_INIT(numDrops, 120)
    , PARAM_INIT(dropRadius, 300.0f)
    , PARAM_INIT(texScale, 3.0f)
    , PARAM_INIT(hitScale, 5.0f)
    , PARAM_INIT(modelScale, 1.5f)
    , PARAM_INIT(modelScale2, 1.0f)
    , PARAM_INIT(modelScaleY, 1.0f)
    , PARAM_INIT(gravity, 18.0f)
    , PARAM_INIT(bounceY, 0.05f)
    , PARAM_INIT(bounceXZ, 0.5f)
    , PARAM_INIT(damp, 0.985f)
    , PARAM_INIT(jump, 65.0f)
    , PARAM_INIT(overGravity, 0.0f)
    , PARAM_INIT(emitVel, 20.0f)
    , PARAM_INIT(lifeTime, 0)
{
	TParams::load(mPrmPath);
}

TBathWaterGlobalParams::TBathWaterGlobalParams()
    : TParams("/MapObj/bathwaterglobal.prm")
    , PARAM_INIT(regR, 0)
    , PARAM_INIT(regG, 0)
    , PARAM_INIT(regB, 0)
    , PARAM_INIT(regA, 0xff)
    , PARAM_INIT(kRegR, 0x90)
    , PARAM_INIT(kRegG, 0x18)
    , PARAM_INIT(kRegB, 0)
    , PARAM_INIT(kRegA, 0xff)
    , PARAM_INIT(polygonR, 0xff)
    , PARAM_INIT(polygonG, 0xff)
    , PARAM_INIT(polygonB, 0x97)
    , PARAM_INIT(indTexScale, 1.5f)
    , PARAM_INIT(showsCap, 1)
    , PARAM_INIT(bendsNormal, 0)
    , PARAM_INIT(showsMist, 0)
    , PARAM_INIT(clearsAlpha, 1)
    , PARAM_INIT(alpha, 0xc8)
    , PARAM_INIT(scrolls, 1)
    , PARAM_INIT(displaysMesh, 0)
    , PARAM_INIT(mode, 0)
    , PARAM_INIT(mask, 1)
    , PARAM_INIT(indirectScale, -3)
    , PARAM_INIT(scrollSpan, 60)
    , PARAM_INIT(meshTexWidth, 80)
    , PARAM_INIT(envMapScale, 0.6f)
    , PARAM_INIT(capHeight, 150.0f)
    , PARAM_INIT(meshWidth, 7000.0f)
{
	TParams::load(mPrmPath);
}

extern void OSReport(const char*, ...);

class TBathWater : public THitActor {
public:
	class TDrop {
	public:
		TDrop() { }
		void reset(const JGeometry::TVec3<f32>& position, f32 rand)
		{
			unk48 = rand;
			unk0.set(position);
			unkC.zero();
			unk18.i.zero();
			unk18.f.zero();
			unk30.i.zero();
			unk30.f.zero();
			unk4C = 0;
		}
		void doThing(f32 damp)
		{
			unk0.add(unk18.i);
			unk0.add(unk18.f);
			unkC.scale(damp);
			unkC.add(unk30.i);
			unkC.add(unk30.f);
		}

		void calcBathtub(const TBathtubData& data, f32 radius,
		                 const JGeometry::TVec3<f32>& minClamp,
		                 const JGeometry::TVec3<f32>& maxClamp, int& count,
		                 JGeometry::TVec3<f32>& average)
		{
			JGeometry::TVec3<f32> wall(data.unk24.x, data.unk24.y,
			                                data.unk24.z);
			JGeometry::TVec3<f32> delta;
			delta.sub(unk0, data.unk0);
			f32 outer   = data.unk40 + radius;
			f32 inner   = data.unk3C - radius;
			f32 distSq  = delta.squared();
			f32 outerSq = outer * outer;
			f32 proj    = wall.dot(delta);

			if (distSq <= outerSq) {
				if (proj < 0.0f) {
					if (distSq >= inner * inner) {
						f32 dist  = JGeometry::TUtil<f32>::sqrt(distSq);
						f32 depth = dist - inner;
						f32 inv   = -1.0f / dist;
						JGeometry::TVec3<f32> normal;
						normal.scale(inv, delta);
						JGeometry::TVec3<f32> push;
						push.scale(depth, normal);
						unk18.extend(push);

						f32 speed = -normal.dot(unkC);
						if (speed < 0.0f)
							speed = 0.0f;

						JGeometry::TVec3<f32> bounce;
						bounce.scale(speed, normal);
						bounce += data.unk58;
						unk30.extend(bounce);
					} else {
						f32 floor = radius + (data.unk0.y - data.unk44);
						if (unk0.y < floor) {
							f32 pushY = floor - unk0.y;
							JGeometry::TVec3<f32> push(0.0f, pushY, 0.0f);
							unk18.extend(push);

							JGeometry::TVec3<f32> bounce(0.0f,
							                                  -1.0f * unkC.y,
							                                  0.0f);
							bounce += data.unk58;
							unk30.extend(bounce);
						} else {
							count++;
							average.add(unk0);
						}
					}
				} else {
					count++;
					average.add(unk0);
				}

				unk30.extend(minClamp);
			} else {
				if (proj > 0.0f && proj < radius + data.unk48
				    && distSq > inner * inner && distSq < outerSq) {
					JGeometry::TVec3<f32> push;
					push.scale((radius + data.unk48) - proj, wall);
					unk18.extend(push);

					JGeometry::TVec3<f32> bounce;
					bounce.scale(1.5f * -data.unk24.dot(unkC), data.unk24);
					JGeometry::TVec3<f32> away;
					away.set(delta);
					away.setLength(0.01f * radius);
					bounce.add(away);
					unk30.extend(bounce);
					unk30.extend(maxClamp);
				} else {
					unk30.extend(maxClamp);
					count++;
					average.add(unk0);
				}
			}
		}
		static void calcWaterModel(TBathWater* water,
		                           const TBathtubData& data)
		{
			f32 gravity = water->unk8C->gravity.get();
			JGeometry::TVec3<f32> gravityForce;
			gravityForce.scale(
			    -gravity,
			    data.getGravityDir(water->unk8C->overGravity.get()));
			JGeometry::TVec3<f32> downGravity(0.0f, -gravity, 0.0f);
			TBathWater::TDrop* end = water->unk88 + water->unk74;
			f32 radius             = water->unk8C->dropRadius.get();
			int active             = 0;
			JGeometry::TVec3<f32> average(0.0f, 0.0f, 0.0f);

			if (data.unk24.y > 0.0f) {
				for (TBathWater::TDrop* drop = water->unk88; drop < end;
				     ++drop) {
					drop->unk0 += drop->unkC;
					drop->unk18.i.zero();
					drop->unk18.f.zero();
					drop->unk30.i.zero();
					drop->unk30.f.zero();
					drop->calcBathtub(data, radius, gravityForce, downGravity,
					                   active, average);
				}
			} else {
				for (TBathWater::TDrop* drop = water->unk88; drop < end;
				     ++drop) {
					drop->unk0 += drop->unkC;
					drop->unk18.i.zero();
					drop->unk18.f.zero();
					drop->unk30.i.zero();
					drop->unk30.f.zero();
					drop->unk30.extend(downGravity);
					average.x += drop->unk0.x;
					average.y += drop->unk0.y;
					average.z += drop->unk0.z;
					active += 1;
				}
			}

			f32 volume;
			if (active * 30 > water->unk74) {
				f32 inv = 1.0f / (f32)active;
				average.scale(inv);
				volume = JGeometry::TUtil<f32>::sqrt(
				    (3.0f * (f32)active) / (f32)water->unk74);
				if (volume > 1.0f)
					volume = 1.0f;
			} else {
				volume = 0.0f;
			}
			water->unk78 = average;
			water->unk84 = volume;

			if (water->unk8C->intersects.get()) {
				f32 twoR = 2.0f * radius;
				f32 sep2 = 4.0f * (radius * radius);
				for (TBathWater::TDrop* drop = water->unk88; drop < end;
				     ++drop) {
					TBathWater::TDrop* other
					    = drop + water->unk8C->intersects.get();
					for (; other < end;
					     other += water->unk8C->intersects.get()) {
						JGeometry::TVec3<f32> diff;
						diff.sub(other->unk0, drop->unk0);
						f32 distSq = diff.squared();
						if (!(distSq > sep2)) {
							f32 dist = diff.length();
							JGeometry::TVec3<f32> normal;
							normal.scale(1.0f / dist, diff);

							f32 half = (twoR - dist) / 2.0f;

							diff.x = normal.x * half;
							diff.z = normal.z * half;

							f32 mag = half * normal.y;

							diff.set(normal.x * half, (normal.y + 1.0f) * mag,
							         normal.z * half);
							other->unk18.extend(diff);

							diff.x = -diff.x;
							diff.z = -diff.z;
							diff.y = (normal.y - 1.0f) * mag;
							drop->unk18.extend(diff);

							if (mag < twoR - dist)
								mag = twoR - dist;

							normal.x *= mag * water->unk8C->bounceXZ.get();
							normal.y *= mag * water->unk8C->bounceY.get();
							normal.z *= mag * water->unk8C->bounceXZ.get();
							other->unk30.extend(normal);
							normal.negate();
							drop->unk30.extend(normal);
						}
					}
				}
			}

			f32 floorY = data.unk0.y - data.unk3C;
			if (data.unk65 != 0)
				floorY = data.unk0.y - 8.0f * data.unk3C;

			int respawnIndex = 0;
			for (TBathWater::TDrop* drop = water->unk88; drop < end; ++drop) {
				if (drop->unk0.y < floorY) {
					if (water->unk8C->suppliesDrops.get() && data.unk65 == 0) {
						drop->reset(
						    data.getPos(respawnIndex++, water->unk70, radius),
						    water->unk68.get_float01());
					} else if (water->eraseDrop(drop)) {
						end -= 1;
						drop->doThing(water->unk8C->damp.get());
					}
				} else {
					drop->doThing(water->unk8C->damp.get());
				}
			}

			if (water->unk8C->lifeTime.get() > 0) {
				for (TBathWater::TDrop* drop = water->unk88; drop < end;
				     --end, ++drop) {
					drop->unk4C += 1;
					if (drop->unk4C > water->unk8C->lifeTime.get())
						water->eraseDrop(drop);
				}
			}

			if (water->unk74 < water->unk8C->numDrops.get()) {
				if (water->unk8C->suppliesDrops.get() && data.unk65 == 0) {
					water->unk88[water->unk74++].reset(
					    data.getPos(respawnIndex++, water->unk70, radius),
					    water->unk68.get_float01());
				}
			} else if (water->unk74 > water->unk8C->numDrops.get()) {
				water->unk74 = water->unk8C->numDrops.get();
			}
		}

	public:
		/* 0x00 */ JGeometry::TVec3<f32> unk0;
		/* 0x0C */ JGeometry::TVec3<f32> unkC;
		/* 0x18 */ JGeometry::TBox3<f32> unk18;
		/* 0x30 */ JGeometry::TBox3<f32> unk30;
		/* 0x48 */ f32 unk48;
		/* 0x4C */ s32 unk4C;
	};

	TBathWater()
	    : THitActor("HitActor")
	    , unk68(0)
	{
		unk70 = 500;
		unk88 = new TDrop[unk70];
		unk8C = 0;
	}
	virtual ~TBathWater() { }
	void initialize(TBathWaterParams* params, const TBathtubData& data)
	{
		unk8C = params;
		unk74 = params->numDrops.get();

		int i = 0;
		for (TBathWater::TDrop *drop = unk88, *end = unk88 + unk70;
		     drop < end; ++drop) {
			drop->reset(data.getPos(i++, unk70, unk8C->dropRadius.get()),
			            unk68.get_float01());
		}

		initHitActor(0x4000025b, 1, 0x80000000,
		             unk8C->dropRadius.get(),
		             unk8C->dropRadius.get() * 2.0f, 0.0f, 0.0f);
		onHitFlag(HIT_FLAG_NO_COLLISION);
		onHitFlag(HIT_FLAG_UNK4);
		unk78.zero();
		unk84 = 0.0f;
	}
	void addDrop(const JGeometry::TVec3<f32>& position, f32 velY)
	{
		if (unk74 < unk8C->numDrops.get()) {
			unk88[unk74].reset(position, unk68.get_float01());
			unk88[unk74].unkC.y = velY;

			OSReport("BathWaterManager.cpp(%d): ...\n", 0x28f, unk74++);
		}
	}
	bool eraseDrop(TDrop* drop)
	{
		int index = drop - unk88;
		if (index >= unk74)
			return false;

		unk74 -= 1;
		if (index < unk74)
			*drop = unk88[unk74];
		return true;
	}
	bool tryHitMario(THitActor* mario)
	{
		TBathWaterParams* params = unk8C;
		f32 radius = params->dropRadius.get() * params->hitScale.get();
		f32 maxDown = mario->mDamageHeight + radius;
		f32 maxUp = mario->mDamageRadius + radius;
		f32 maxUpSq = maxUp * maxUp;
		f32 marioX = mario->mPosition.x;
		f32 marioY = mario->mPosition.y;
		f32 marioZ = mario->mPosition.z;

		for (TBathWater::TDrop* drop = unk88; drop < unk88 + unk74; ++drop) {
			f32 dy = drop->unk0.y - marioY;
			if (!(dy > maxDown) && !(dy < -radius)) {
				f32 dx = marioX - drop->unk0.x;
				f32 dz = marioZ - drop->unk0.z;
				if (dx * dx + dz * dz < maxUpSq) {
					setAttackRadius(unk8C->dropRadius.get());
					setAttackRadius(unk8C->dropRadius.get() * 2.0f);
					mPosition.set(drop->unk0);
					mPosition.y -= unk8C->dropRadius.get();
					mario->receiveMessage(this, HIT_MESSAGE_UNKA);
					return true;
				}
			}
		}

		return false;
	}
	bool tryHitMario2(THitActor* mario, const TBathtubData& data)
	{
		f32 marioX = mario->mPosition.x;
		f32 marioY = mario->mPosition.y;
		f32 marioZ = mario->mPosition.z;
		f32 marioHeight = mario->mDamageHeight;
		JGeometry::TVec3<f32> center = data.getThing();
		f32 radius = JGeometry::TUtil<f32>::sqrt(
		    data.unk3C * data.unk3C - data.unk44 * data.unk44);

		if (center.y < marioY)
			return false;

		f32 waterDepth = data.unk3C - data.unk44;
		if (marioY + marioHeight < center.y - waterDepth)
			return false;

		f32 dx = marioX - center.x;
		f32 dz = marioZ - center.z;
		if (dx * dx + dz * dz >= radius * radius)
			return false;

		setAttackRadius(radius);
		mPosition.set(data.getThing());
		mPosition.y -= waterDepth;
		setAttackHeight(waterDepth);
		mario->receiveMessage(this, HIT_MESSAGE_UNKA);
		return true;
	}

public:
	/* 0x68 */ JMath::TRandomFast unk68;
	/* 0x6C */ u32 unk6C;
	/* 0x70 */ s32 unk70;
	/* 0x74 */ s32 unk74;
	/* 0x78 */ JGeometry::TVec3<f32> unk78;
	/* 0x84 */ f32 unk84;
	/* 0x88 */ TDrop* unk88;
	/* 0x8C */ TBathWaterParams* unk8C;
};

static inline void doSetEffectMtx(J3DTexMtxInfo* info, MtxPtr mtx)
{
	info->setEffectMtx(mtx);
}

#pragma dont_inline on
static void init_tobj_resource(GXTexObj* obj, void* resource)
{
	ResTIMG* timg = (ResTIMG*)resource;
	GXTexFmt format = (GXTexFmt)timg->format;
	u16 width    = timg->width;
	u16 height   = timg->height;
	u8 wrapS     = timg->wrapS;
	u8 wrapT     = timg->wrapT;
	u8 minFilter = timg->minFilter;
	u8 magFilter = timg->magFilter;
	void* image  = (u8*)resource + timg->imageDataOffset;
	DCStoreRange(image, GXGetTexBufferSize(width, height, format, 0, 0));
	GXInitTexObj(obj, image, width, height, format, (GXTexWrapMode)wrapS,
	             (GXTexWrapMode)wrapT, GX_FALSE);
	GXInitTexObjLOD(obj, (GXTexFilter)minFilter, (GXTexFilter)magFilter, 0.0f,
	                0.0f, 0.0f, GX_FALSE, GX_FALSE, GX_ANISO_1);
}
#pragma dont_inline off

static void draw_mist(u16 x, u16 y, u16 wd, u16 ht, void* buffer)
{
	Mtx e_m;
	Mtx44 m;
	GXTexObj tex_obj;

	GXColor tev_color = { 0x03, 0x03, 0x03, 0x00 };
	u8 vFilter[7]     = { 0x15, 0x00, 0x00, 0x16, 0x00, 0x00, 0x15 };

	f32 f_left   = x;
	f32 f_wd     = wd;
	f32 f_top    = y;
	f32 f_ht     = ht;
	f32 f_right  = f_left + f_wd;
	f32 f_bottom = f_top + f_ht;
	f32 offset_x = (4.0f / f_wd);
	f32 offset_y = (2.0f / f_ht);

	C_MTXOrtho(m, f_top, f_bottom, f_left, f_right, 0.0f, 1.0f);
	PSMTXIdentity(e_m);
	GXSetTexCopySrc(x, y, wd, ht);
	GXSetCopyFilter(GX_FALSE, 0, GX_TRUE, vFilter);
	GXSetTexCopyDst(wd >> 1, ht >> 1, GX_TF_RGB565, GX_TRUE);
	GXCopyTex(buffer, GX_FALSE);
	GXPixModeSync();
	GXInitTexObj(&tex_obj, buffer, wd >> 1, ht >> 1, GX_TF_RGB565, GX_CLAMP,
	             GX_CLAMP, 0);
	GXInitTexObjLOD(&tex_obj, GX_LINEAR, GX_LINEAR, 0.0, 0.0, 0.0, GX_FALSE,
	                GX_FALSE, GX_ANISO_1);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX1, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX2, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX3, GX_DIRECT);

	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX1, GX_TEX_ST, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX2, GX_TEX_ST, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX3, GX_TEX_ST, GX_F32, 0);

	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);

	GXSetNumTexGens(4);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, 0x3c, GX_FALSE,
	                  0x7d);
	GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_TEX1, 0x3c, GX_FALSE,
	                  0x7d);
	GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX2, 0x3c, GX_FALSE,
	                  0x7d);
	GXSetTexCoordGen2(GX_TEXCOORD3, GX_TG_MTX2x4, GX_TG_TEX3, 0x3c, GX_FALSE,
	                  0x7d);

	GXLoadTexObj(&tex_obj, GX_TEXMAP0);

	GXSetNumTevStages(4);

	// Stage 0
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_HALF, GX_CC_C0);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_FALSE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevDirect(GX_TEVSTAGE0);

	// Stage 1
	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_HALF,
	                GX_CC_CPREV);
	GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_FALSE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevDirect(GX_TEVSTAGE1);

	// Stage 2
	GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD2, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_TEXC, GX_CC_HALF,
	                GX_CC_CPREV);
	GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_FALSE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevDirect(GX_TEVSTAGE2);

	// Stage 3
	GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD3, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE3, GX_CC_ZERO, GX_CC_TEXC, GX_CC_HALF,
	                GX_CC_CPREV);
	GXSetTevColorOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_DIVIDE_2,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE3, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevDirect(GX_TEVSTAGE3);

	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);

	GXSetTevColor(GX_TEVREG0, tev_color);
	GXSetProjection(m, GX_ORTHOGRAPHIC);
	GXSetViewport(f_left, f_top, f_wd, f_ht, 0.0, 1.0);
	GXSetScissor(x, y, wd, ht);

	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetAlphaUpdate(GX_FALSE);
	GXSetColorUpdate(GX_TRUE);
	GXLoadPosMtxImm(e_m, 0);
	GXSetCurrentMtx(0);
	GXSetCullMode(GX_CULL_NONE);
	GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_ZERO, GX_BL_ZERO, GX_LO_NOOP);

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition2u16(x, y);
	GXTexCoord2f32(-offset_x, 0.0f);
	GXTexCoord2f32(offset_x, 0.0f);
	GXTexCoord2f32(0.0f, -offset_y);
	GXTexCoord2f32(0.0f, offset_y);
	GXPosition2u16(x + wd, y);
	GXTexCoord2f32(1.0f - offset_x, 0.0f);
	GXTexCoord2f32(1.0f + offset_x, 0.0f);
	GXTexCoord2f32(1.0f, -offset_y);
	GXTexCoord2f32(1.0f, offset_y);
	GXPosition2u16(x + wd, y + ht);
	GXTexCoord2f32(1.0f - offset_x, 1.0f);
	GXTexCoord2f32(1.0f + offset_x, 1.0f);
	GXTexCoord2f32(1.0f, 1.0f - offset_y);
	GXTexCoord2f32(1.0f, 1.0f + offset_y);
	GXPosition2u16(x, y + ht);
	GXTexCoord2f32(-offset_x, 1.0f);
	GXTexCoord2f32(+offset_x, 1.0f);
	GXTexCoord2f32(0.0f, 1.0f - offset_y);
	GXTexCoord2f32(0.0f, 1.0f + offset_y);
	GXEnd();

	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition2u16(x, y);
	GXTexCoord2f32(-offset_x, 0.0f);
	GXTexCoord2f32(offset_x, 0.0f);
	GXTexCoord2f32(0.0f, -offset_y);
	GXTexCoord2f32(0.0f, offset_y);
	GXPosition2u16(x + wd, y);
	GXTexCoord2f32(1.0f - offset_x, 0.0f);
	GXTexCoord2f32(1.0f + offset_x, 0.0f);
	GXTexCoord2f32(1.0f, -offset_y);
	GXTexCoord2f32(1.0f, offset_y);
	GXPosition2u16(x + wd, y + ht);
	GXTexCoord2f32(1.0f - offset_x, 1.0f);
	GXTexCoord2f32(1.0f + offset_x, 1.0f);
	GXTexCoord2f32(1.0f, 1.0f - offset_y);
	GXTexCoord2f32(1.0f, 1.0f + offset_y);
	GXPosition2u16(x, y + ht);
	GXTexCoord2f32(-offset_x, 1.0f);
	GXTexCoord2f32(+offset_x, 1.0f);
	GXTexCoord2f32(0.0f, 1.0f - offset_y);
	GXTexCoord2f32(0.0f, 1.0f + offset_y);
	GXEnd();
}

namespace {
void clearEFB_alpha(s16 x, s16 y, s16 width, s16 height, u8 alpha)
{
	Mtx matrix;
	Mtx44 projection;

	if (width <= 0) {
		s16 renderWidth = SMSGetGameRenderWidth();
		width           = renderWidth;
	}
	if (height <= 0) {
		s16 renderHeight = SMSGetGameRenderHeight();
		height           = renderHeight;
	}

	f32 left   = x;
	f32 wd     = width;
	f32 top    = y;
	f32 ht     = height;
	f32 right  = left + wd;
	f32 bottom = top + ht;

	C_MTXOrtho(projection, top, bottom, left, right, 0.0f, 1.0f);
	PSMTXIdentity(matrix);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_U16, 0);
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL,
	              GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetProjection(projection, GX_ORTHOGRAPHIC);
	GXSetViewport(left, top, wd, ht, 0.0f, 1.0f);
	GXSetScissor(x, y, width, height);
	GXSetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_NOOP);
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetColorUpdate(GX_FALSE);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetDstAlpha(GX_TRUE, alpha);
	GXLoadPosMtxImm(matrix, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetCullMode(GX_CULL_NONE);

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition2s16(x, y);
	GXPosition2s16(x + width, y);
	GXPosition2s16(x + width, y + height);
	GXPosition2s16(x, y + height);
	GXEnd();

	GXSetColorUpdate(GX_TRUE);
	GXSetDstAlpha(GX_FALSE, alpha);
}
} // namespace

class TBathWaterMeshRenderer : public TBathWaterRenderer {
public:
	TBathWaterMeshRenderer(TBathWaterGlobalParams* params,
	                       JUTTexture* screen_texture)
	    : unk80134(params)
	    , unk80138(screen_texture)
	{
		static u32 clear_z_TX[16] __attribute__((aligned(0x20))) = {
			0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
			0x00FF00FF, 0x00FF00FF, 0x00FF00FF, 0x00FF00FF,
			0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
			0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
		};

		unk800AE = 0;
		unk80150 = new (0x20) u8[0x55668];
		unk80154 = new (0x20) u8[0x30023];
		unk800A4 = new (0x20) u8[0x10000];
		unk800A8 = new (0x20) u8[0x55668];

		unk80144 = J3DModelLoaderDataBase::load(
		    JKRGetResource("/scene/map/map/ball.bmd"), 0x240000);
		unk80148 = J3DModelLoaderDataBase::load(
		    JKRGetResource("/scene/map/map/water.bmd"), 0x240000);
		unk8013C = new JUTTexture(
		    (ResTIMG*)JKRGetResource("/scene/map/map/water_ball.bti"));
		unk80140 = new JUTTexture(
		    (ResTIMG*)JKRGetResource("/scene/map/map/water_warp.bti"));

		init_tobj_resource(&unk800B4,
		                   JKRGetResource("/scene/map/map/ball.bti"));
		init_tobj_resource(&unk800F4,
		                   JKRGetResource("/scene/map/map/mesh.bti"));

		TScreenTexture* screen = JDrama::TNameRefGen::search<TScreenTexture>(
		    "\x83\x58\x83\x4E\x83\x8A\x81\x5B\x83\x93\x83\x65\x83\x4E\x83\x58\x83\x60\x83\x83");
		unk80148->getTexture()->setResTIMG(
		    1, *screen->getTexture()->getTexInfo());

		unk80148->getMaterialNodePointer(0)->makeDisplayList();
		unk8014C = new J3DModel(unk80148, 0, 1);

		GXInitTexObj(&unk80114, clear_z_TX, 4, 4, GX_TF_Z24X8, GX_REPEAT,
		             GX_REPEAT, GX_FALSE);
		GXInitTexObjLOD(&unk80114, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f,
		                GX_FALSE, GX_FALSE, GX_ANISO_1);
		clearHeightMap();
	}

	void makeHeightMap(f32);
	void makeNormalMap();
	void calcCoord();
	void clearHeightMap();
	void tmpFake(const JGeometry::TVec3<f32>&, const JGeometry::TVec3<f32>&);

	virtual void prerender(JDrama::TGraphics*, const TBathtubData&,
	                       TBathWater**, TBathWaterParams**, int);
	virtual void render(JDrama::TGraphics*, const TBathtubData&, TBathWater**,
	                    TBathWaterParams**, int);
	virtual f32 getHeight(f32, f32) const;

public:
	/* 0x00004 */ char unk4[0x1C];
	/* 0x00020 */ JGeometry::TVec3<f32> unk20[0x80][0x80];
	/* 0x30020 */ JGeometry::TVec3<f32> unk30020[0x80][0x80];
	/* 0x60020 */ JGeometry::TVec2<f32> unk60020[0x80][0x80];
	/* 0x80020 */ TPosition3f unk80020;
	/* 0x80050 */ TPosition3f unk80050;
	/* 0x80080 */ JGeometry::TVec3<f32> unk80080;
	/* 0x8008C */ char unk8008C[0x18];
	/* 0x800A4 */ void* unk800A4;
	/* 0x800A8 */ void* unk800A8;
	/* 0x800AC */ s16 unk800AC;
	/* 0x800AE */ s16 unk800AE;
	/* 0x800B0 */ f32 unk800B0;
	/* 0x800B4 */ GXTexObj unk800B4;
	/* 0x800D4 */ GXTexObj unk800D4;
	/* 0x800F4 */ GXTexObj unk800F4;
	/* 0x80114 */ GXTexObj unk80114;
	/* 0x80134 */ TBathWaterGlobalParams* unk80134;
	/* 0x80138 */ JUTTexture* unk80138;
	/* 0x8013C */ JUTTexture* unk8013C;
	/* 0x80140 */ JUTTexture* unk80140;
	/* 0x80144 */ J3DModelData* unk80144;
	/* 0x80148 */ J3DModelData* unk80148;
	/* 0x8014C */ J3DModel* unk8014C;
	/* 0x80150 */ void* unk80150;
	/* 0x80154 */ void* unk80154;
	/* 0x80158 */ u32 unk80158;
	/* 0x8015C */ char unk8015C[4];
};

static void drawCap(const JGeometry::TVec3<f32>& center, f32 radius)
{
	static f32 delta = 0.20943952f;

	f32 angle;
	f32 drawRadius;

	drawRadius = radius / cosf(0.5f * delta);
	angle      = 0.0f;
	GXBegin(GX_TRIANGLEFAN, GX_VTXFMT0, 0x1e);
	for (int i = 0; i < 0x1e; ++i) {
		GXPosition3f32(drawRadius * cosf(angle) + center.x, center.y,
		               drawRadius * sinf(angle) + center.z);
		GXTexCoord2u8(0x40, 0x40);
		angle += delta;
	}
	GXEnd();
}

JGeometry::TVec3<f32> TBathtubData::getPos(int index, int count,
                                           f32 height) const
{
	f32 t     = (f32)index / (f32)count;
	f32 angle = (f32)index * 0.31415927f;
	f32 amp   = t * (unk3C - height);

	JGeometry::TVec3<f32> result;
	result = unk0;

	f32 s = amp * sinf(angle);

	result.x += unk18.x * s;
	result.y += unk18.y * s;
	result.z += unk18.z * s;

	f32 c = amp * cosf(angle);
	result.x += unk30.x * c;
	result.y += unk30.y * c;
	result.z += unk30.z * c;

	f32 yScale = (1.0f - t) * -(unk44 - height);
	result.x += unk24.x * yScale;
	result.y += unk24.y * yScale;
	result.z += unk24.z * yScale;

	return result;
}

JGeometry::TVec3<f32> TBathtubData::getGravityDir(f32 rate) const
{
	(void)0;
	if (!unk65) {
		JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
		(void)&up;
		JGeometry::TQuat4<f32> quat;
		quat.setRotate(unkC, up, rate);
		JGeometry::TVec3<f32> result;
		quat.rotate(up, result);
		return result;
	} else {
		return JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f);
	}
}

static inline const TBathtubData& bathData(TBathtub* bathtub)
{
	return bathtub->getBathtubData();
}

inline TBathWaterPreprocessor::TBathWaterPreprocessor(TBathWaterManager* manager)
    : JDrama::TViewObj()
    , unk10(manager)
{
}

void TBathWaterPreprocessor::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (!(flags & 8))
		return;

	TBathWaterManager* manager = unk10;
	if (manager->unk24 && manager->unk30)
		manager->unk30->prerender(graphics, manager->unk24->getBathtubData(),
		                           manager->unk20, manager->unk14, 2);
}

TBathWaterManager::TBathWaterManager()
    : JDrama::TViewObj("\x83\x6F\x83\x58\x83\x5E\x83\x75\x82\xCC\x90\x85")
    , unk10(0)
    , unk34(this)
{
	unk14 = new TBathWaterParams*[2];
	unk18 = 0;
	unk1C = 0;
	unk20 = new TBathWater*[2];
	for (int i = 0; i < 2; ++i)
		unk20[i] = new TBathWater;
	unk24 = 0;
	unk30 = 0;
}

const char* TBathWaterManager::fileNames[] = {
	"MapObj/bathwater_wave.prm",
	"MapObj/bathwater_overflow.prm",
};

void TBathWaterManager::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);
	unk18 = new TBathWaterGlobalParams;
	for (int i = 0; i < 2; ++i)
		unk14[i] = new TBathWaterParams(fileNames[i]);
}

inline TBathWaterFlatRenderer::TBathWaterFlatRenderer(
    TBathWaterGlobalParams* params)
    : unk2C(params)
{
	unk28 = new (0x20) u8[0x14a000];
	unk24 = JKRGetResource("/scene/map/map/ball.bti");
	init_tobj_resource(&unk4, unk24);
}

void TBathWaterManager::loadAfter()
{
	TScreenTexture* screen = JDrama::TNameRefGen::search<TScreenTexture>(
	    "\x83\x58\x83\x4E\x83\x8A\x81\x5B\x83\x93\x83\x65\x83\x4E\x83\x58\x83\x60\x83\x83");
	unk28[0] = new TBathWaterFlatRenderer(unk18);
	unk28[1] = new TBathWaterMeshRenderer(unk18, screen->getTexture());
	unk30    = unk28[1];
}

void TBathWaterManager::initializeIfYet_()
{
	static const char bathtubName[] = "\x83\x6F\x83\x58\x83\x5E\x83\x75";

	if (unk24 == 0) {
		TBathtub* bathtub
		    = JDrama::TNameRefGen::search<TBathtub>(bathtubName);
		if (bathtub != 0 && bathtub->unk298 != 0) {
			const TBathtubData& data = bathData(bathtub);
			for (int i = 0; i < 2; ++i) {
				unk20[i]->initialize(unk14[i], data);

				for (int step = 0; step < 200; ++step)
					TBathWater::TDrop::calcWaterModel(unk20[i], data);
			}

			unk24 = bathtub;
		}
	}
}

void TBathWaterManager::throwMario(f32 jump)
{
	TBathtub* bathtub         = unk24;
	const TBathtubData& data  = bathData(bathtub);
	JGeometry::TVec3<f32> diff;
	diff.sub(SMS_GetMarioPos(), data.unk0);

	JGeometry::TVec3<f32> local(
	    data.unk18.x * diff.x + data.unk18.y * diff.y
	        + data.unk18.z * diff.z,
	    data.unk24.x * diff.x + data.unk24.y * diff.y
	        + data.unk24.z * diff.z,
	    data.unk30.x * diff.x + data.unk30.y * diff.y
	        + data.unk30.z * diff.z);

	JGeometry::TVec3<f32> horiz = local;
	horiz.y                     = 0.0f;

	JGeometry::TVec3<f32> throwSpeed;
	if (horiz.length() < 4500.0) {
		horiz.setLength(4150.0f);

		JGeometry::TVec3<f32> target(
		    data.unk18.x * horiz.x + data.unk30.x * horiz.z,
		    data.unk18.y * horiz.x + data.unk30.y * horiz.z,
		    data.unk18.z * horiz.x + data.unk30.z * horiz.z);
		target.add(data.unk0);
		target.y += 120.0f;

		f32 gravity = SMS_GetMarioGravity();
		int frames  = 1;
		f32 velY    = 100.0f;
		f32 y       = gpMarioPos->y;
		while (true) {
			y += velY;
			if (velY < 0.0f && y <= target.y)
				break;
			velY -= gravity;
			if (velY < -75.0f)
				velY = -75.0f;
			frames += 1;
		}

		throwSpeed.x = (target.x - gpMarioPos->x) / (f32)frames;
		throwSpeed.y = 100.0f;
		throwSpeed.z = (target.z - gpMarioPos->z) / (f32)frames;
	} else {
		throwSpeed.x = 0.0f;
		throwSpeed.y = jump;
		throwSpeed.z = 0.0f;
	}

	SMS_ThrowMario(throwSpeed, throwSpeed.length());
}

static inline bool fakeCalcPos(const TBathtubData& data, f32 radius, f32 rand,
                               JGeometry::TVec3<f32>* out)
{
	JGeometry::TVec3<f32> axis(data.unk24.x, 0.0f, data.unk24.z);
	if (axis.isZero())
		return false;

	JGeometry::TVec3<f32> dir;
	dir.normalize(axis);

	JGeometry::TVec3<f32> cross;
	cross.cross(dir, JGeometry::TVec3<f32>(0.0f, 1.0f, 0.0f));
	cross.normalize();

	f32 perturb = 0.18f * rand;
	dir.x += cross.x * perturb;
	dir.y += cross.y * perturb;
	dir.z += cross.z * perturb;

	f32 h = 0.9f * (data.unk3C - radius);
	dir.setLength(h);

	JGeometry::TVec3<f32> up(0.0f, 1.0f, 0.0f);
	JGeometry::TVec3<f32> center;
	center.set(data.getThing());
	out->set(up.x * radius + dir.x + center.x,
	         up.y * radius + dir.y + center.y,
	         up.z * radius + dir.z + center.z);
	return true;
}

void TBathWaterManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (unk24 == 0) {
		initializeIfYet_();
		return;
	}

	if (flags & 1) {
		unk30 = unk28[unk18->displaysMesh.get()];
		unk1C += 1;

		if ((unk1C & 3) == 0) {
			for (int i = 0; i < 2; ++i) {
				TBathWater* water       = unk20[i];
				const TBathtubData& data = unk24->getBathtubData();
				TBathWater::TDrop::calcWaterModel(water, data);
			}

			if (unk24->unk29A == 0) {
				for (int i = 0; i < 2; ++i) {
					if (!SMS_CheckMarioFlag(0x400) && unk14[i]->checksMario.get()) {
						if (unk20[i]->tryHitMario(SMS_GetMarioHitActor()))
							throwMario(unk14[i]->jump.get());
					}
				}

				if (!SMS_CheckMarioFlag(0x400)) {
					if (unk20[0]->tryHitMario2(SMS_GetMarioHitActor(),
					                           unk24->getBathtubData()))
						throwMario(unk14[0]->jump.get());
				}
			}

			if ((unk1C & 7) == 4 && unk24->getBathtubData().unk64) {
				const TBathtubData& data = unk24->getBathtubData();
				JGeometry::TVec3<f32> vel;
				if (fakeCalcPos(data, unk14[1]->dropRadius.get(),
				                unk10.get_float(-1.0f, 1.0f), &vel))
					unk20[1]->addDrop(vel, unk10.get_float01() * 10.0f);
			}

			TBathWater* soundWater = unk20[0];
			JGeometry::TVec3<f32> soundPos;
			soundPos.set(soundWater->unk78);
			f32 volume = soundWater->unk84;
			if (volume > 0.0f)
				SMSGetMSound()->startSoundActorWithInfo(
				    0x819d, (Vec*)&soundPos, 0, volume, 0, 0, 0, 0, 4);
		}
	}

	if (flags & 8)
		unk30->render(graphics, unk24->getBathtubData(), unk20, unk14, 2);
}

void TBathWaterFlatRenderer::prerender(JDrama::TGraphics*,
                                       const TBathtubData&, TBathWater**,
                                       TBathWaterParams**, int)
{
}

f32 TBathWaterFlatRenderer::getHeight(f32, f32) const { return 0.0f; }

void TBathWaterFlatRenderer::render(JDrama::TGraphics* graphics,
                                    const TBathtubData& data,
                                    TBathWater** waters,
                                    TBathWaterParams** params, int count)
{
	s16 renderWidth  = SMSGetGameRenderWidth();
	s16 renderHeight = SMSGetGameRenderHeight();

	if (unk2C->clearsAlpha.get())
		clearEFB_alpha(0, 0, 0, 0, 0);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);
	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXLoadTexObj(&unk4, GX_TEXMAP0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_TEXC);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_TEXA);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LESS, GX_FALSE);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
	GXSetColorUpdate(GX_FALSE);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetCullMode(GX_CULL_NONE);
	GXSetProjection(graphics->mProjMtx.mMtx, GX_PERSPECTIVE);
	GXLoadPosMtxImm(graphics->mViewMtx.mMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);

	f32 xAxisX = graphics->mViewMtx.mMtx[0][0];
	f32 xAxisY = graphics->mViewMtx.mMtx[0][1];
	f32 xAxisZ = graphics->mViewMtx.mMtx[0][2];
	f32 yAxisX = graphics->mViewMtx.mMtx[1][0];
	f32 yAxisY = graphics->mViewMtx.mMtx[1][1];
	f32 yAxisZ = graphics->mViewMtx.mMtx[1][2];

	for (int i = 0; i < count; ++i) {
		if (!params[i]->isVisible.get())
			continue;

		f32 scale         = params[i]->texScale.get() * params[i]->dropRadius.get();
		f32 x0            = xAxisX * scale;
		f32 y0            = xAxisY * scale;
		f32 z0            = xAxisZ * scale;
		f32 x1            = yAxisX * scale;
		f32 y1            = yAxisY * scale;
		f32 z1            = yAxisZ * scale;

		GXBegin(GX_QUADS, GX_VTXFMT0, (waters[i]->unk74 * 4) & 0xfffc);
		for (TBathWater::TDrop* drop = waters[i]->unk88;
		     drop < waters[i]->unk88 + waters[i]->unk74; ++drop) {
			f32 x = drop->unk0.x;
			f32 y = drop->unk0.y;
			f32 z = drop->unk0.z;
			GXPosition3f32(x + x0, y + y0, z + z0);
			GXTexCoord2u8(0, 0);
			GXPosition3f32(x + x1, y + y1, z + z1);
			GXTexCoord2u8(0, 0x80);
			GXPosition3f32(x - x0, y - y0, z - z0);
			GXTexCoord2u8(0x80, 0x80);
			GXPosition3f32(x - x1, y - y1, z - z1);
			GXTexCoord2u8(0x80, 0);
		}
		GXEnd();
	}

	if (unk2C->showsCap.get() && data.unk65 == 0) {
		drawCap(data.getThing(),
		        JGeometry::TUtil<f32>::sqrt(data.unk3C * data.unk3C
		                                    - data.unk44 * data.unk44));
	}

	GXTexObj texObj;
	GXColor color = { 0x78, 0xfa, 0x14, unk2C->alpha.get() };
	GXInitTexObj(&texObj, unk28, renderWidth, renderHeight, GX_TF_I8, GX_CLAMP,
	             GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE,
	                GX_FALSE, GX_ANISO_1);
	GXSetTexCopySrc(0, 0, renderWidth, renderHeight);
	GXSetTexCopyDst(renderWidth, renderHeight, GX_TF_A8, GX_FALSE);
	GXSetCopyFilter(GX_FALSE, 0, GX_FALSE, 0);
	GXCopyTex(unk28, GX_FALSE);
	GXPixModeSync();

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
	              GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXLoadTexObj(&texObj, GX_TEXMAP0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_C0);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_TEXA, GX_CA_A0, GX_CA_KONST,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_COMP_A8_GT, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_5_8);
	GXSetTevColor(GX_TEVREG0, color);

	s16 overlayWidth  = SMSGetGameRenderWidth();
	s16 overlayHeight = SMSGetGameRenderHeight();
	TPosition3f matrix;
	matrix.identity();
	Mtx44 projection;
	C_MTXOrtho(projection, 0.0f, (f32)overlayHeight, 0.0f, (f32)overlayWidth,
	           -1.0f, 1.0f);
	GXSetProjection(projection, GX_ORTHOGRAPHIC);
	GXSetViewport(0.0f, 0.0f, (f32)overlayWidth, (f32)overlayHeight, 0.0f, 1.0f);
	GXSetScissor(0, 0, overlayWidth, overlayHeight);
	GXLoadPosMtxImm(matrix.mMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetCullMode(GX_CULL_BACK);
	GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
	               GX_LO_NOOP);
	GXSetColorUpdate(GX_TRUE);
	GXSetAlphaUpdate(GX_FALSE);

	GXBegin(GX_QUADS, GX_VTXFMT0, 4);
	GXPosition2s16(0, 0);
	GXTexCoord2f32(0.0f, 0.0f);
	GXPosition2s16(overlayWidth, 0);
	GXTexCoord2f32(1.0f, 0.0f);
	GXPosition2s16(overlayWidth, overlayHeight);
	GXTexCoord2f32(1.0f, 1.0f);
	GXPosition2s16(0, overlayHeight);
	GXTexCoord2f32(0.0f, 1.0f);
	GXEnd();

	if (unk2C->showsMist.get())
		draw_mist(0, 0, renderWidth, renderHeight, unk28);

	GXSetProjection(graphics->mProjMtx.mMtx, GX_PERSPECTIVE);
}

void TBathWaterMeshRenderer::makeHeightMap(f32 scale)
{
	s16 blockCount = unk800AC >> 2;

	for (s16 bx = 0; bx < blockCount; ++bx) {
		for (s16 bz = 0; bz < blockCount; ++bz) {
			u16* block = (u16*)unk800A4 + ((bx + bz * blockCount) << 5);
			for (s16 x = 0; x < 4; ++x) {
				for (s16 z = 0; z < 4; ++z) {
					s32 sample = (((block + x + z * 4)[0] << 16) & 0xff0000)
					             | (block + x + z * 4)[0x10];
					unk20[x + bx * 4][z + bz * 4].y
					    = scale * (5.9604645e-8f * (f32)sample);
				}
			}
		}
	}
}

void TBathWaterMeshRenderer::makeNormalMap()
{
	f32 scale = -unk80134->meshWidth.get() / (f32)unk800AC;
	if (unk80134->bendsNormal.get() == 0)
		scale *= 2.0f;

	for (s32 r = 0; r < unk800AC; ++r) {
		for (s32 c = 0; c < unk800AC; ++c) {
			f32 a  = unk20[r > 0 ? r - 1 : 0][c].y;
			f32 b  = unk20[r < unk800AC - 1 ? r + 1 : r][c].y;
			f32 a2 = unk20[r][c > 0 ? c - 1 : 0].y;
			f32 b2 = unk20[r][c < unk800AC - 1 ? c + 1 : c].y;

			unk30020[r][c].x = scale * (b - a);
			unk30020[r][c].y = scale * scale;
			unk30020[r][c].z = scale * (b2 - a2);
			unk30020[r][c].normalize();
		}
	}
}

void TBathWaterMeshRenderer::calcCoord()
{
	if (unk800AE == unk800AC)
		return;

	unk800AE = unk800AC;

	for (s32 x = 0; x < unk800AE; ++x) {
		for (s32 z = 0; z < unk800AE; ++z) {
			unk60020[x][z].set((f32)x * unk800B0, (f32)z * unk800B0);
		}
	}
	DCStoreRange(&unk60020, 0x20000);

	for (s32 x = 0; x < unk800AE; ++x) {
		for (s32 z = 0; z < unk800AE; ++z) {
			unk20[x][z].x = unk80080.x * (f32)x;
			unk20[x][z].z = unk80080.z * (f32)z;
		}
	}

	GDLObj dl;
	GDInitGDLObj(&dl, unk80154, 0x30040);
	GDSetCurrent(&dl);
	u16 count = (unk800AC * (unk800AC - 1) * 2) & 0xfffe;
	GDWrite_u8(GX_TRIANGLESTRIP);
	GDWrite_u16(count);
	for (s32 z = unk800AC - 1; z > 0; --z) {
		s32 zm = z - 1;
		for (s32 x = 0; x < unk800AC; ++x) {
			u16 index0 = z + x * 0x80;
			u16 index1 = zm + x * 0x80;
			GDWrite_u16(index0);
			GDWrite_u16(index0);
			GDWrite_u16(index0);
			GDWrite_u16(index1);
			GDWrite_u16(index1);
			GDWrite_u16(index1);
		}
	}
	GDPadCurr32();
	GDFlushCurrToMem();
	unk80158 = GDGetCurrOffset();
	GDSetCurrent(0);
}

void TBathWaterMeshRenderer::render(JDrama::TGraphics* graphics,
                                    const TBathtubData& data,
                                    TBathWater** waters,
                                    TBathWaterParams** params, int count)
{
	CPolarSubCamera* camera = gpCamera;
	MtxPtr viewMtx          = camera->getUnk1EC();
	MtxPtr projMtx          = camera->getUnk16C();
	s16 renderWidth         = SMSGetGameRenderWidth();
	s16 renderHeight        = SMSGetGameRenderHeight();

	clearEFB_alpha(0, 0, 0, 0, 0);

	MtxPtr viewMtx2 = gpCamera->unk1EC;
	s16 copyWidth   = SMSGetGameRenderWidth();
	s16 copyHeight  = SMSGetGameRenderHeight();
	GXSetProjection(projMtx, GX_PERSPECTIVE);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);
	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXLoadTexObj(&unk800B4, GX_TEXMAP0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_TEXA);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LESS, GX_FALSE);
	GXSetZCompLoc(GX_TRUE);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ONE, GX_LO_NOOP);
	GXSetColorUpdate(GX_FALSE);
	GXSetAlphaUpdate(GX_TRUE);
	GXSetCullMode(GX_CULL_NONE);
	GXLoadPosMtxImm(viewMtx2, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);

	f32 xAxisX = viewMtx2[0][0];
	f32 xAxisY = viewMtx2[0][1];
	f32 xAxisZ = viewMtx2[0][2];
	f32 yAxisX = viewMtx2[1][0];
	f32 yAxisY = viewMtx2[1][1];
	f32 yAxisZ = viewMtx2[1][2];

	for (int i = 0; i < count; ++i) {
		TBathWater*& water = waters[i];
		f32 scale         = params[i]->texScale.get() * params[i]->dropRadius.get();
		f32 x1            = yAxisX * scale;
		f32 y1            = yAxisY * scale;
		f32 z1            = yAxisZ * scale;
		f32 x0            = xAxisX * scale;
		f32 y0            = xAxisY * scale;
		f32 z0            = xAxisZ * scale;

		GXBegin(GX_QUADS, GX_VTXFMT0, (water->unk74 * 4) & 0xfffc);
		for (TBathWater::TDrop* drop = water->unk88;
		     drop < water->unk88 + water->unk74; ++drop) {
			f32 x = drop->unk0.x;
			f32 y = drop->unk0.y;
			f32 z = drop->unk0.z;
			GXPosition3f32(x + x0, y + y0, z + z0);
			GXTexCoord2u8(0, 0);
			GXPosition3f32(x + x1, y + y1, z + z1);
			GXTexCoord2u8(0, 0x80);
			GXPosition3f32(x - x0, y - y0, z - z0);
			GXTexCoord2u8(0x80, 0x80);
			GXPosition3f32(x - x1, y - y1, z - z1);
			GXTexCoord2u8(0x80, 0);
		}
		GXEnd();
	}

	if (unk80134->showsCap.get() && data.unk65 == 0) {
		f32 radiusSq = data.unk3C * data.unk3C - data.unk44 * data.unk44;
		f32 capRadius = JGeometry::TUtil<f32>::sqrt(radiusSq);
		JGeometry::TVec3<f32> center(data.unk0.x, data.unk0.y - data.unk44,
		                              data.unk0.z);
		drawCap(center, capRadius);
	}

	GXInitTexObj(&unk800D4, unk800A8, copyWidth, copyHeight, GX_TF_I8,
	             GX_CLAMP, GX_CLAMP, GX_FALSE);
	GXInitTexObjLOD(&unk800D4, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_FALSE,
	                GX_FALSE, GX_ANISO_1);
	GXSetCopyFilter(GX_FALSE, 0, GX_FALSE, 0);
	GXSetTexCopySrc(0, 0, copyWidth, copyHeight);
	GXSetTexCopyDst(copyWidth, copyHeight, GX_TF_A8, GX_FALSE);
	GXCopyTex(unk800A8, GX_FALSE);
	GXPixModeSync();
	GXSetProjection(projMtx, GX_PERSPECTIVE);

	GXColor amb = { 0x00, 0x00, 0x00, 0xff };
	GXColor mat = { unk80134->polygonR.get(), unk80134->polygonG.get(),
	                unk80134->polygonB.get(), 0xff };
	GXSetNumChans(1);
	GXSetChanCtrl(GX_COLOR0, GX_TRUE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT0,
	              GX_DF_CLAMP, GX_AF_NONE);
	GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetChanMatColor(GX_COLOR0A0, mat);
	GXSetChanAmbColor(GX_COLOR0A0, amb);
	GXSetNumTexGens(4);
	unk80138->load(GX_TEXMAP0);
	unk8013C->load(GX_TEXMAP1);
	unk80140->load(GX_TEXMAP2);
	GXLoadTexObj(&unk800D4, GX_TEXMAP3);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0,
	                  GX_FALSE, GX_PTIDENTITY);
	GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX1,
	                  GX_FALSE, GX_PTIDENTITY);
	GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX3x4, GX_TG_TEX0, GX_TEXMTX2,
	                  GX_FALSE, GX_PTIDENTITY);
	GXSetTexCoordGen2(GX_TEXCOORD3, GX_TG_MTX3x4, GX_TG_POS, GX_TEXMTX0,
	                  GX_FALSE, GX_PTIDENTITY);

	f32 cell = unk800B0 * unk80134->meshWidth.get();
	unk80080.set(cell, -4.0f * data.unk3C, cell);
	unk80050.identity33();
	unk80050.mMtx[0][3] = data.unk0.x - 0.5f * unk80134->meshWidth.get();
	unk80050.mMtx[1][3] = data.unk0.y - data.unk44 + 3.0f * data.unk3C;
	unk80050.mMtx[2][3] = data.unk0.z - 0.5f * unk80134->meshWidth.get();

	j3dSys.setViewMtx(viewMtx);
	J3DTexGenBlock* texGenBlock
	    = unk80148->getMaterialNodePointer(0)->getTexGenBlock();
	J3DTexMtx* texMtx0 = texGenBlock->getTexMtx(0);
	texMtx0->mSRT.mScaleX = 1.0f;
	texMtx0->mSRT.mScaleY = 1.0f;
	texMtx0->mSRT.mTranslationX = 0.0f;
	texMtx0->mCenter.x = 0.5f;
	texMtx0->mCenter.y = 0.5f;
	J3DTexMtx* texMtx1 = texGenBlock->getTexMtx(1);
	texMtx1->mSRT.mScaleX = unk80134->envMapScale.get();
	texMtx1->mSRT.mScaleY = unk80134->envMapScale.get();
	texMtx1->mSRT.mTranslationX = 0.0f;
	texMtx1->mSRT.mTranslationY = 0.0f;
	texMtx1->mCenter.x = 0.5f;
	texMtx1->mCenter.y = 0.5f;
	J3DTexMtx* texMtx2 = texGenBlock->getTexMtx(2);
	texMtx2->mSRT.mScaleX = unk80134->indTexScale.get();
	texMtx2->mSRT.mScaleY = unk80134->indTexScale.get();
	u32 frame = waters[0]->unk6C % (u32)unk80134->scrollSpan.get();
	texMtx2->mSRT.mTranslationY
	    = (f32)frame / (f32)unk80134->scrollSpan.get();
	texMtx2->mCenter.x = 0.5f;
	texMtx2->mCenter.y = 0.5f;

	Mtx effectMtx;
	SMS_GetLightPerspectiveForEffectMtx(effectMtx);
	doSetEffectMtx(texMtx0, effectMtx);
	texGenBlock->calc(unk80050.mMtx);
	GXLoadTexMtxImm(texMtx0->mTotalMtx, GX_TEXMTX0, GX_MTX3x4);
	GXLoadTexMtxImm(texMtx1->mTotalMtx, GX_TEXMTX1, GX_MTX3x4);
	GXLoadTexMtxImm(texMtx2->mTotalMtx, GX_TEXMTX2, GX_MTX3x4);

	GXSetNumIndStages(1);
	GXSetIndTexCoordScale(GX_INDTEXSTAGE0, GX_ITS_1, GX_ITS_1);
	GXSetIndTexOrder(GX_INDTEXSTAGE0, GX_TEXCOORD2, GX_TEXMAP2);
	f32 indMtx[2][3];
	indMtx[0][0] = 0.6f;
	indMtx[0][1] = 0.0f;
	indMtx[0][2] = 0.0f;
	indMtx[1][0] = 0.0f;
	indMtx[1][1] = 0.6f;
	indMtx[1][2] = 0.0f;
	GXSetIndTexMtx(GX_ITM_0, indMtx, (s8)unk80134->indirectScale.get());

	if (unk80134->mode.get() == 2)
		GXSetNumTevStages(1);
	else
		GXSetNumTevStages(3);

	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0);
	switch (unk80134->mode.get()) {
	case 0:
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_C0,
		                GX_CC_KONST);
		break;
	case 1:
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
		                GX_CC_ZERO);
		break;
	case 2:
		GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_TEXC, GX_CC_ZERO, GX_CC_ZERO,
		                GX_CC_ZERO);
		break;
	}
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevIndWarp(GX_TEVSTAGE0, GX_INDTEXSTAGE0, 1, 0, GX_ITM_0);
	GXSetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
	GXColor kColor = { unk80134->kRegR.get(), unk80134->kRegG.get(),
		               unk80134->kRegB.get(), unk80134->kRegA.get() };
	GXSetTevKColor(GX_KCOLOR0, kColor);
	GXColor regColor = { unk80134->regR.get(), unk80134->regG.get(),
		                 unk80134->regB.get(), unk80134->regA.get() };
	GXSetTevColor(GX_TEVREG0, regColor);

	GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC,
	                GX_CC_CPREV);
	GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevIndWarp(GX_TEVSTAGE1, GX_INDTEXSTAGE0, 1, 0, GX_ITM_0);

	GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD3, GX_TEXMAP3, GX_COLOR0A0);
	GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_CPREV, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_ZERO);
	GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevDirect(GX_TEVSTAGE2);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZCompLoc(GX_FALSE);
	GXSetZMode(GX_TRUE, GX_LESS, GX_TRUE);
	GXSetColorUpdate(GX_TRUE);
	GXSetAlphaUpdate(GX_FALSE);
	GXSetCullMode(GX_CULL_NONE);

	Mtx drawMtx;
	PSMTXConcat(viewMtx, unk80050.mMtx, drawMtx);
	DCInvalidateRange(unk800A4, unk800AC * (unk800AC * 4));
	GXLoadPosMtxImm(drawMtx, GX_PNMTX0);
	GXLoadNrmMtxImm(viewMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);
	GXSetCullMode(GX_CULL_BACK);
	GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
	if (unk80134->mask.get())
		GXSetAlphaCompare(GX_GEQUAL, unk80134->alpha.get(), GX_AOP_AND,
		                  GX_ALWAYS, 0);
	else
		GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);

	makeHeightMap(unk80080.y);
	makeNormalMap();
	calcCoord();
	DCStoreRange(unk20, 0x30000);
	DCStoreRange(unk30020, 0x30000);

	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_INDEX16);
	GXSetVtxDesc(GX_VA_NRM, GX_INDEX16);
	GXSetVtxDesc(GX_VA_TEX0, GX_INDEX16);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
	GXSetArray(GX_VA_POS, unk20, 0xC);
	GXSetArray(GX_VA_NRM, unk30020, 0xC);
	GXSetArray(GX_VA_TEX0, &unk60020, 8);
	GXCallDisplayList(unk80154, unk80158);

	if (unk80134->showsMist.get())
		draw_mist(0, 0, renderWidth, renderHeight, unk80150);

	GXSetProjection(graphics->mProjMtx.mMtx, GX_PERSPECTIVE);
}

inline void TBathWaterMeshRenderer::tmpFake(const JGeometry::TVec3<f32>& dir,
                                            const JGeometry::TVec3<f32>& up)
{
	unk80020.setLookDir(dir, up);
}

void TBathWaterMeshRenderer::prerender(JDrama::TGraphics* graphics,
                                        const TBathtubData& data,
                                        TBathWater** waters,
                                        TBathWaterParams** params, int count)
{
	unk800AC = unk80134->meshTexWidth.get() & ~3;
	unk800B0 = 1.0f / (f32)unk800AC;

	f32 negR = -data.unk3C;
	f32 R3   = 3.0f * data.unk3C;

	JGeometry::TVec3<f32> v1;
	v1.set(data.getThing());
	if (data.unk3C * data.unk3C - data.unk44 * data.unk44 <= 0)
		(void)(data.unk3C * data.unk3C - data.unk44 * data.unk44);

	JGeometry::TVec3<f32> v2;
	v2.set(data.getThing());

	f32 ey = v2.y + R3;
	JGeometry::TVec3<f32> v3;
	v3.set(data.getThing());

	JGeometry::TVec3<f32> dir(v3.x - v2.x, (v3.y + negR) - ey,
	                          v3.z - v2.z);
	JGeometry::TVec3<f32> up(0.0f, 0.0f, -1.0f);
	tmpFake(dir, up);
	unk80020.mMtx[0][3] = -unk80020.mMtx[0][0] * v2.x
	                       - unk80020.mMtx[0][1] * ey
	                       - unk80020.mMtx[0][2] * v2.z;
	unk80020.mMtx[1][3] = -unk80020.mMtx[1][0] * v2.x
	                       - unk80020.mMtx[1][1] * ey
	                       - unk80020.mMtx[1][2] * v2.z;
	unk80020.mMtx[2][3] = -unk80020.mMtx[2][0] * v2.x
	                       - unk80020.mMtx[2][1] * ey
	                       - unk80020.mMtx[2][2] * v2.z;

	MtxPtr oldProjection = (MtxPtr)((u8*)gpCamera + 0x16C);
	j3dSys.drawInit();
	s16 renderWidth  = SMSGetGameRenderWidth();
	s16 renderHeight = SMSGetGameRenderHeight();
	GXSetViewport(0.0f, 0.0f, (f32)renderWidth, (f32)renderHeight, 0.0f,
	              1.0f);

	TProjection3f projection;
	f32 halfWidth = 0.5f * unk80134->meshWidth.get();
	projection.orthographic(
	    halfWidth,
	    halfWidth
	        - unk800B0 * (unk80134->meshWidth.get() * (f32)renderHeight),
	    -halfWidth,
	    unk800B0 * (unk80134->meshWidth.get() * (f32)renderWidth) - halfWidth,
	    0.0f, R3 - negR);

	GXSetProjection(projection.mMtx, GX_ORTHOGRAPHIC);
	GXSetScissor(0, 0, unk800AC, unk800AC);
	GXSetNumChans(0);
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG,
	              GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
	GXSetNumTexGens(1);
	GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY,
	                  GX_FALSE, GX_PTIDENTITY);
	GXLoadTexObj(&unk80114, GX_TEXMAP0);
	GXSetNumTevStages(1);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
	GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO,
	                GX_CC_TEXC);
	GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO,
	                GX_CA_ZERO);
	GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1,
	                GX_TRUE, GX_TEVPREV);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_OR, GX_ALWAYS, 0);
	GXSetZMode(GX_TRUE, GX_LESS, GX_TRUE);
	GXSetBlendMode(GX_BM_NONE, GX_BL_ZERO, GX_BL_ZERO, GX_LO_NOOP);
	GXSetColorUpdate(GX_FALSE);
	GXSetAlphaUpdate(GX_FALSE);
	GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
	GXSetCullMode(GX_CULL_BACK);
	J3DVertexData& vertexData = unk80144->getVertexData();
	J3DLoadArrayBasePtr(GX_VA_POS, vertexData.getVtxPosArray());
	J3DLoadArrayBasePtr(GX_VA_NRM, vertexData.getVtxNormArray());
	J3DLoadArrayBasePtr(GX_VA_CLR0, vertexData.getVtxColorArray(0));
	GXCallDisplayList(unk80144->getShapeNodePointer(0)->getDrawList(),
	                  J3DShape::kVcdVatDLSize);
	GXLoadNrmMtxImm(unk80020.mMtx, GX_PNMTX0);
	GXSetCurrentMtx(GX_PNMTX0);

	for (int i = 0; i < count; ++i) {
		if (!params[i]->isVisible.get())
			continue;

		TBathWater* water = waters[i];

		for (TBathWater::TDrop* drop = water->unk88;
		     drop < water->unk88 + water->unk74; ++drop) {
			TBathWaterParams* param = params[i];
			f32 dropRadius          = param->dropRadius.get();
			f32 modelScale
			    = dropRadius
			      * (drop->unk48 * param->modelScale.get()
			         + (1.0f - drop->unk48) * param->modelScale2.get());
			JGeometry::TVec3<f32> pos = drop->unk0;
			pos.y += param->modelScaleY.get() * dropRadius - modelScale;

			TPosition3f local;
			local.set(modelScale, 0.0f, 0.0f, pos.x, 0.0f, modelScale,
			          0.0f, pos.y, 0.0f, 0.0f, modelScale, pos.z);
			TPosition3f dropMtx;
			dropMtx.concat(unk80020, local);
			GXLoadPosMtxImm(dropMtx.mMtx, GX_PNMTX0);

			for (u16 shapeIndex = 0; shapeIndex < unk80144->getShapeNum();
			     ++shapeIndex) {
				J3DShape* shape = unk80144->getShapeNodePointer(shapeIndex);
				for (u16 drawIndex = 0; drawIndex < shape->getMtxGroupNum();
				     ++drawIndex) {
					J3DShapeDraw* draw = shape->getShapeDraw(drawIndex);
					if (draw)
						draw->draw();
				}
			}
		}
	}

	if (unk80134->showsCap.get() && data.unk65 == 0) {
		GXLoadPosMtxImm(unk80020.mMtx, GX_PNMTX0);
		GXClearVtxDesc();
		GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
		GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
		GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_U8, 7);

		f32 radiusSq = data.unk3C * data.unk3C - data.unk44 * data.unk44;
		f32 capRadius = JGeometry::TUtil<f32>::sqrt(radiusSq);
		JGeometry::TVec3<f32> capCenter = data.getThing();
		drawCap(capCenter, capRadius);
		capCenter.y += 0.5f * negR;
		drawCap(capCenter, capRadius + unk800B0 * unk80134->meshWidth.get());
	}

	GXSetCopyFilter(GX_FALSE, 0, GX_FALSE, 0);
	GXSetTexCopySrc(0, 0, unk800AC, unk800AC);
	GXSetTexCopyDst(unk800AC, unk800AC, GX_TF_Z24X8, GX_FALSE);
	GXCopyTex(unk800A4, GX_FALSE);
	GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
	GXSetProjection(oldProjection, GX_PERSPECTIVE);
	GXSetScissor(0, 0, renderWidth, renderHeight);
}

f32 TBathWaterMeshRenderer::getHeight(f32 x, f32 z) const
{
	s16 n = unk800AC;
	if (n < 1)
		return 0.0f;

	int i = (int)((x - unk80050.at(0, 3)) / unk80080.x);
	if (i < 0)
		i = 0;
	else if (i >= n)
		i = n - 1;

	int j = (int)((z - unk80050.at(2, 3)) / unk80080.z);
	if (j < 0)
		j = 0;
	else if (j >= n)
		j = n - 1;

	return unk20[i][j].y + unk80050.at(1, 3);
}

void TBathWaterMeshRenderer::clearHeightMap()
{
	for (int x = 0; x < 0x80; ++x)
		for (int z = 0; z < 0x80; ++z)
			unk20[x][z].y = 0.0f;

	unk80080.set(1.0f, 1.0f, 1.0f);
	unk80050.identity();
}

f32 TBathWaterManager::getWaterHeight(f32 x, f32 z) const
{
	if (unk24 && unk30)
		return unk30->getHeight(x, z);
	return 0.0f;
}
