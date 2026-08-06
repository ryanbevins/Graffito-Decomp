#define JGEOMETRY_SELECTSHINE2_OWNER_HELPERS
#define JMATH_SELECTSHINE2_TRIG_OUT_OF_LINE
#include <GC2D/SelectShine2.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DMaterialAnm.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DDrawBuffer.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DAnmLoader.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JMath.hpp>
#include <JSystem/JParticle/JPAEmitter.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <System/Application.hpp>
#include <dolphin/mtx.h>
#include <stdlib.h>

#undef JGEOMETRY_SELECTSHINE2_OWNER_HELPERS
#undef JMATH_SELECTSHINE2_TRIG_OUT_OF_LINE

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

JGeometry::TVec3<f32> TSelectShineManager::cCenter(300.0f, 160.0f, -9000.0f);

static inline JGeometry::TVec3<f32> makeShinePos(f32 x, f32 y, f32 z)
{
	return JGeometry::TVec3<f32>(x, y, z);
}

TSelectShineManager::TSelectShineManager(const char* name)
    : JDrama::TViewObj(name)
    , mDrawBufOpa(nullptr)
    , mDrawBufXlu(nullptr)
    , unk78(0.0f)
    , unk7C(0.0f)
    , mShineCount(0)
    , unk90(0.0f)
    , unk94(0.0f)
    , unk98(0)
    , unk9C(0)
    , unkA0(0.0f)
    , unkA4(0)
    , unkA5(0)
    , unkA6(0)
    , unkA7(0)
{
}

void TSelectShineManager::initData(u8* shineTypes, u8 count, u8 startIdx,
                                   JPAEmitterManager* emitterMgr)
{
	mDrawBufOpa = new J3DDrawBuffer(0x400);
	j3dSys.setDrawBuffer(mDrawBufOpa, 0);
	mDrawBufXlu = new J3DDrawBuffer(0x400);
	j3dSys.setDrawBuffer(mDrawBufXlu, 1);

	void* bmd = JKRFileLoader::getGlbResource("/select/shine_menu.bmd");
	void* bpk = JKRFileLoader::getGlbResource("/select/shine_menu.bpk");

	J3DModelData* mdShine = J3DModelLoaderDataBase::load(bmd, 0x51040000);
	J3DAnmColor* anmShine = (J3DAnmColor*)J3DAnmLoaderDataBase::load(bpk);
	anmShine->searchUpdateMaterialID(mdShine);

	for (u16 i = 0; i < mdShine->getMaterialNum(); ++i) {
		J3DMaterialAnm* anm = new J3DMaterialAnm;
		mdShine->getMaterialNodePointer(i)->change();
		mdShine->getMaterialNodePointer(i)->setMaterialAnm(anm);
	}
	mdShine->entryMatColorAnimator(anmShine);

	void* bmdE
	    = JKRFileLoader::getGlbResource("/select/shine_menu_empty.bmd");
	void* bpkE
	    = JKRFileLoader::getGlbResource("/select/shine_menu_empty.bpk");

	J3DModelData* mdEmpty = J3DModelLoaderDataBase::load(bmdE, 0x51040000);
	J3DAnmColor* anmEmpty = (J3DAnmColor*)J3DAnmLoaderDataBase::load(bpkE);
	anmEmpty->searchUpdateMaterialID(mdEmpty);

	for (u16 i = 0; i < mdShine->getMaterialNum(); ++i) {
		J3DMaterialAnm* anm = new J3DMaterialAnm;
		mdEmpty->getMaterialNodePointer(i)->change();
		mdEmpty->getMaterialNodePointer(i)->setMaterialAnm(anm);
	}
	mdEmpty->entryMatColorAnimator(anmEmpty);

	mShineCount = (s32)count;
	unk8C       = (s32)startIdx;
	unk9C       = unk8C * -40;

	u8* typePtr = shineTypes;
	s32 offset  = 0;
	for (int i = 0; i < 8; ++i) {
		s16 angle = (s16)(57.295776f * (f32)(s16)(unk9C + offset));
		u16 tblIdx = static_cast<u16>(angle) >> jmaSinShift;
		f32 cosA  = jmaCosTable[tblIdx];
		f32 sinA  = jmaSinTable[tblIdx];
		JGeometry::TVec3<f32> pos
		    = makeShinePos(9000.0f * sinA + cCenter.x,
		                   1500.0f * cosA + cCenter.y, cCenter.z);

		JGeometry::TVec2<f32> diff;
		diff.x = pos.x;
		diff.y = pos.y;
		JGeometry::TVec2<f32> ref;
		ref.x = 1300.0f;
		ref.y = cCenter.y;
		diff.x -= ref.x;
		diff.y -= ref.y;

		f32 a   = diff.x;
		f32 b   = diff.y;
		s16 yaw = (s16)(57.295776f
		                * fabsf(atan2f(a * 1.0f - b * 0.0f,
		                               a * 0.0f + b * 1.0f)));
		if (pos.x > cCenter.x) {
			yaw = -yaw;
		}

		TSelectShine* shine = nullptr;
		if (*typePtr == 3) {
			shine = new TSelectShine(
			    mdShine, anmShine, emitterMgr, pos, yaw, 0,
			    (f32)(s32)((f32)rand() * (1.0f / 32768.0f) * 4000.0f)
			        / 1000.0f,
			    0.01f, 10.0f);
		} else if ((u8)(*typePtr - 1) <= 1) {
			shine = new TSelectShine(
			    mdEmpty, anmEmpty, emitterMgr, pos, yaw, 1,
			    (f32)(s32)((f32)rand() * (1.0f / 32768.0f) * 4000.0f)
			        / 1000.0f,
			    0.01f, 10.0f);
		}
		mShines[i] = shine;
		typePtr++;
		offset += 40;
	}

	TSelectShine* cur = mShines[unk8C];
	if (cur->unk4A != 2) {
		if (cur->unk4A == 0) {
			cur->unk54->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
			cur->unk50->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
		}
		cur->unk58->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
	}
}

void TSelectShineManager::startClose()
{
	TSelectShine* cur = mShines[unk8C];
	cur->unk24        = 0;

	for (int i = 0; i < 8; ++i) {
		TSelectShine* shine = mShines[i];
		if (shine != nullptr && i != unk8C && shine->unk49 == 0) {
			shine->unk49 = 1;
			shine->unk48 = 0;
		}
	}
	unkA7 = 1;
}

void TSelectShineManager::startIncrease(int delta)
{
	TSelectShine* prev = mShines[unk8C];
	if (prev->unk4A != 2) {
		if (prev->unk4A == 0) {
			prev->unk54->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
			prev->unk50->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
		}
		prev->unk58->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
	}

	unkA4 = 1;
	unkA0 = (f32)((delta * -40) / 10);
	unk8C += delta;

	TSelectShine* next = mShines[unk8C];
	if (next->unk4A != 2) {
		if (next->unk4A == 0) {
			next->unk54->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
			next->unk50->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
		}
		next->unk58->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
	}
}

void TSelectShineManager::startDecrease(int delta)
{
	TSelectShine* prev = mShines[unk8C];
	if (prev->unk4A != 2) {
		if (prev->unk4A == 0) {
			prev->unk54->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
			prev->unk50->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
		}
		prev->unk58->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
	}

	unkA5 = 1;
	unkA0 = (f32)((delta * 40) / 10);
	unk8C -= delta;

	TSelectShine* next = mShines[unk8C];
	if (next->unk4A != 2) {
		if (next->unk4A == 0) {
			next->unk54->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
			next->unk50->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
		}
		next->unk58->clearStatus(JPABaseEmitter::STATUS_STOP_EMIT);
	}
}

#pragma dont_inline on
namespace JGeometry {
template <> void TVec2<f32>::sub(const TVec2<f32>& other)
{
	x -= other.x;
	y -= other.y;
}
}

f32 JMASSin(s16 v)
{
	return jmaSinTable[static_cast<u16>(v) >> jmaSinShift];
}

f32 JMASCos(s16 v)
{
	return jmaCosTable[static_cast<u16>(v) >> jmaSinShift];
}
#pragma dont_inline off

void TSelectShineManager::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 1) {
		for (int i = 0; i < mShineCount; ++i) {
			TSelectShine* shine = mShines[i];
			shine->move();
		}

		if (unkA4 != 0 || unkA5 != 0) {
			unk9C = (s32)((f32)unk9C + unkA0);
			if (unkA4 != 0) {
				s32 cap = unk8C * -40;
				if (unk9C < cap) {
					unk9C = cap;
					unkA4 = 0;
				}
			}
			if (unkA5 != 0) {
				s32 cap = unk8C * -40;
				if (unk9C > cap) {
					unk9C = cap;
					unkA5 = 0;
				}
			}
		}

		for (int i = 0; i < mShineCount; ++i) {
			s16 angle = (s16)(57.295776f * (f32)(s16)(unk9C + i * 40));
			f32 cosA  = JMASCos(angle);
			f32 sinA  = JMASSin(angle);
			JGeometry::TVec3<f32> tmp
			    = makeShinePos(9000.0f * sinA + cCenter.x,
			                   1500.0f * cosA + cCenter.y, cCenter.z);

			TSelectShine* shine = mShines[i];

			JGeometry::TVec3<f32> sum;
			sum = tmp;
			sum.add(shine->unk18);

			JGeometry::TVec2<f32> diff;
			diff.x = sum.x;
			diff.y = sum.y;
			JGeometry::TVec2<f32> ref;
			ref.x = 1300.0f;
			ref.y = cCenter.y;
			diff.sub(ref);

			f32 a   = diff.x;
			f32 b   = diff.y;
			s16 yaw = (s16)(57.295776f
			                * fabsf(atan2f(a * 1.0f - b * 0.0f,
			                               a * 0.0f + b * 1.0f)));
			if (sum.x > cCenter.x) {
				yaw = -yaw;
			}

			MtxPtr modelMtx = shine->mModel->unk20;
			Mtx rotMtx;
			PSMTXRotRad(rotMtx, 'y',
			            0.017453292f
			                * (f32)((s16)(yaw - shine->unk3A)));
			PSMTXConcat(modelMtx, rotMtx, modelMtx);
			shine->unk3A = yaw;
		}
	}

	if (flags & 2) {
		for (int i = 0; i < mShineCount; ++i) {
			TSelectShine* shine = mShines[i];
			shine->mAnmColor->mFrame = (f32)shine->unk3C;
			J3DModel* model         = shine->mModel;
			model->entry();
			model->viewCalc();
		}
	}

	if (flags & 8) {
		PSMTXCopy(gfx->mViewMtx.mMtx, j3dSys.mViewMtx);
		j3dSys.drawInit();
		j3dSys.unk4C = 3;
		mDrawBufOpa->draw();
		j3dSys.unk4C = 4;
		mDrawBufXlu->draw();
		mDrawBufOpa->frameInit();
		mDrawBufXlu->frameInit();
	}
}

TSelectShine::TSelectShine(J3DModelData* modelData, J3DAnmColor* anmColor,
                           JPAEmitterManager* emitterMgr,
                           JGeometry::TVec3<f32>& position, s16 angle,
                           u8 type, f32 unk28Init, f32 unk30Init,
                           f32 unk2CInit)
{
	unk24       = 0;
	unk28       = 0.0f;
	unk2C       = 0.0f;
	unk30       = 0.0f;
	unk34       = 0;
	unk38       = 0;
	unk3A       = angle;
	unk3C       = 0;
	unk3E       = 0;
	unk40       = 0.0f;
	unk44       = 0.0f;
	unk48       = 0;
	unk49       = 0;
	unk4A       = 0;
	mEmitterMgr = nullptr;
	unk50       = nullptr;
	unk54       = nullptr;
	unk58       = nullptr;

	mModel    = new J3DModel(modelData, 0, 1);
	mAnmColor = anmColor;
	mPos.set(position);
	unk18.set(0.0f, 0.0f, 0.0f);
	mEmitterMgr = emitterMgr;
	unk30       = unk30Init;
	unk2C       = unk2CInit;
	unk28       = unk28Init;
	unk38       = 3;

	MtxPtr modelMtx = mModel->unk20;
	modelMtx[0][3]  = mPos.x;
	modelMtx[1][3]  = mPos.y;
	modelMtx[2][3]  = mPos.z;

	Mtx rotMtx;
	PSMTXRotRad(rotMtx, 'y', 0.017453292f * (f32)unk3A);
	PSMTXConcat(modelMtx, rotMtx, modelMtx);

	unk4A = type;
	if (unk4A != 2) {
		JGeometry::TVec3<f32> tmp = mPos + unk18;

		if (unk4A == 0) {
			mEmitterMgr->createEmitter(tmp, 0, nullptr, nullptr);
			unk50 = mEmitterMgr->unkC8[0][0];
			mEmitterMgr->createEmitter(tmp, 1, nullptr, nullptr);
			unk54 = mEmitterMgr->unkC8[0][0];
			mEmitterMgr->createEmitter(tmp, 2, nullptr, nullptr);
		} else {
			mEmitterMgr->createEmitter(tmp, 3, nullptr, nullptr);
		}
		unk58 = mEmitterMgr->unkC8[0][0];
	}

	if (unk4A != 2) {
		if (unk4A == 0) {
			unk54->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
			unk50->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
		}
		unk58->setStatus(JPABaseEmitter::STATUS_STOP_EMIT);
	}
}

void TSelectShine::move()
{
	f32 t = unk28;
	f32 splineY;
	if (t < 1.0f) {
		f32 omt  = 1.0f - t;
		f32 amp  = unk2C;
		f32 amp9 = amp * 0.9f;
		splineY  = 0.0f * (omt * omt) + amp9 * (2.0f * omt * t)
		         + amp * (t * t);
	} else if (t < 2.0f) {
		f32 u    = t - 1.0f;
		f32 omu  = 1.0f - u;
		f32 amp  = unk2C;
		f32 amp9 = amp * 0.9f;
		splineY  = amp * (omu * omu) + amp9 * (2.0f * omu * u)
		         + 0.0f * (u * u);
	} else if (t < 3.0f) {
		f32 u    = t - 2.0f;
		f32 omu  = 1.0f - u;
		f32 amp  = -unk2C;
		f32 amp9 = amp * 0.9f;
		splineY  = 0.0f * (omu * omu) + amp9 * (2.0f * omu * u)
		         + amp * (u * u);
	} else if (t < 4.0f) {
		f32 u    = t - 3.0f;
		f32 omu  = 1.0f - u;
		f32 amp  = -unk2C;
		f32 amp9 = amp * 0.9f;
		splineY  = amp * (omu * omu) + amp9 * (2.0f * omu * u)
		         + 0.0f * (u * u);
	} else {
		splineY = unk18.y;
	}
	unk18.y = splineY;

	JGeometry::TVec3<f32> world;
	world = mPos + unk18;
	MtxPtr modelMtx = mModel->unk20;
	modelMtx[0][3]  = world.x;
	modelMtx[1][3]  = world.y;
	modelMtx[2][3]  = world.z;

	s32 capped = (s32)(-0.05f * world.z);
	s16 cap16  = (s16)capped;
	if (cap16 < 30) {
		capped = 30;
	}

	if (unk48 != 0) {
		unk3C = (s16)(unk3C - 8);
		if (unk3C < (s16)capped) {
			unk3C = (s16)capped;
		}
	} else if (unk49 != 0) {
		unk3C        = (s16)(unk3C + 8);
		s16 maxFrame = (s16)(mAnmColor->mMaxFrame - 1);
		if (unk3C > maxFrame) {
			unk3C = maxFrame;
		}
	} else {
		unk3C = (s16)capped;
		if (unk3C < 0) {
			unk3C = 0;
		}
		s16 maxF = mAnmColor->mMaxFrame;
		if ((s16)capped > maxF) {
			unk3C = maxF;
		}
	}

	s32 alpha = (s32)(255.0f
	                  * (1.0f - (f32)(unk3C / mAnmColor->mMaxFrame)));

	if (unk4A != 2) {
		if (unk4A == 0) {
			unk50->setGlobalRTMatrix(modelMtx);
			unk54->setGlobalRTMatrix(modelMtx);
			unk50->unk180.a = (u8)alpha;
			unk54->unk180.a = (u8)alpha;
		}
		unk58->setGlobalRTMatrix(modelMtx);
		unk58->unk180.a = (u8)alpha;
	}

	if (unk24 != 0) {
		f32 rate = SMSGetAnmFrameRate();
		Mtx rotMtx;
		PSMTXRotRad(rotMtx, 'y',
		            0.017453292f * (f32)((s8)unk38) * rate);

		f32 rate2 = SMSGetAnmFrameRate();
		unk34     = (s32)((f32)unk34 + (f32)((s8)unk38) * rate2);
		if (unk34 > 360) {
			unk34 -= 360;
		}
		if (unk34 < 0) {
			unk34 += 360;
		}
		PSMTXConcat(modelMtx, rotMtx, modelMtx);
	} else if (unk34 != 0) {
		f32 rate    = SMSGetAnmFrameRate();
		s16 advance = (s16)((f32)((s8)unk38) * rate);
		unk34 += advance;
		s16 useAngle;
		if (unk34 > 360) {
			useAngle = (s16)(360 - (unk34 - advance));
			unk34    = 0;
		} else {
			useAngle = advance;
		}
		Mtx rotMtx;
		PSMTXRotRad(rotMtx, 'y', 0.017453292f * (f32)useAngle);
		PSMTXConcat(modelMtx, rotMtx, modelMtx);
	}

	unk28 = unk28 + unk30;
	if (unk28 > 4.0f) {
		unk28 = 0.0f;
	}
}
