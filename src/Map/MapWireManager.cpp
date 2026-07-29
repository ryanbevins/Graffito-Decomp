#define JDRAMA_NAMEREF_SEARCH_OUT_OF_LINE
#define JDRAMA_NAMEREFGEN_ACCESSORS_OUT_OF_LINE
#define JDRAMA_VIEWOBJ_PTRLIST_GETCHILDREN_OUT_OF_LINE
#define JGADGET_TLIST_POINTER_END_INSERT_OUT_OF_LINE
#include <Map/MapWireManager.hpp>
#include <Player/MarioMain.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <Strategic/Strategy.hpp>
#include <Map/MapWire.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JDrama/JDRViewObjPtrList.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <dolphin/gx.h>
#undef JDRAMA_NAMEREF_SEARCH_OUT_OF_LINE
#undef JDRAMA_NAMEREFGEN_ACCESSORS_OUT_OF_LINE
#undef JDRAMA_VIEWOBJ_PTRLIST_GETCHILDREN_OUT_OF_LINE
#undef JGADGET_TLIST_POINTER_END_INSERT_OUT_OF_LINE

TMapWireManager* gpMapWireManager;

void TMapWireActor::getTipPoints(JGeometry::TVec3<f32>* out_start,
                                 JGeometry::TVec3<f32>* out_end) const
{
	TMapWire* wire = unk74->unk7C;
	*out_start     = wire->mStartPoint;
	*out_end       = wire->mEndPoint;
}

f32 TMapWireActor::getPosInWire() const
{
	TMapWire* wire = unk74->unk7C;

	JGeometry::TVec3<f32> flatStart = wire->mStartPoint;
	JGeometry::TVec3<f32> flatEnd   = wire->mEndPoint;
	flatStart.y                    = 0.0f;
	flatEnd.y                      = 0.0f;

	JGeometry::TVec3<f32> foot
	    = MsPerpendicFootToLineR(flatStart, flatEnd, mPosition);

	f32 totalLength = (flatEnd - flatStart).length();
	f32 partLength  = (foot - flatStart).length();
	return partLength / totalLength;
}

BOOL TMapWireActor::receiveMessage(THitActor* sender, u32 message)
{
	if (message == HIT_MESSAGE_UNK8 && sender == mHeldObject) {
		mHeldObject = nullptr;
		unk70       = 1;
		return true;
	}

	return false;
}

#pragma dont_inline on
TMapWireActor::TMapWireActor(const char* name)
    : TTakeActor(name)
    , unk70(0)
    , unk74(nullptr)
{
}
#pragma dont_inline off

static void initDraw()
{
	GXSetColorUpdate(GX_TRUE);
	GXLoadPosMtxImm(j3dSys.getViewMtx(), 0);
	GXSetCurrentMtx(0);
	GXClearVtxDesc();
	GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
	GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
	GXSetNumChans(1);
	GXSetChanMatColor(GX_COLOR0A0, (GXColor) { 0, 0, 0, 0xff });
	GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0, GX_DF_NONE,
	              GX_AF_NONE);
	GXSetNumTexGens(0);
	GXSetNumTevStages(1);
	GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
	GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
	GXSetLineWidth(24, GX_TO_ZERO);
	GXSetCullMode(GX_CULL_BACK);
	GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0xff);
	GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
	GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_SET);
}

void TMapWireActorManager::doActorToWire()
{
	TMapWire* oldWire = unk7C;
	s32       wireNo  = gpCubeWire->getInCubeNo(unk0->mPosition);
	if (wireNo != -1) {
		unk7C = gpMapWireManager->unk18[(u16)wireNo];
	} else {
		unk7C = nullptr;
	}

	if (unk4.mHeldObject != nullptr && unk4.mHeldObject->mHolder != &unk4) {
		unk4.mHeldObject = nullptr;
		unk4.unk70       = 1;
	}

	if (unk4.unk74->unk7C != nullptr) {
		for (int i = 0; i < unk4.mColCount; ++i) {
			THitActor* hit = unk4.mCollisions[i];
			bool       isMario = hit->mActorType == 0x80000001 ? true : false;
			if (isMario && hit->receiveMessage(&unk4, HIT_MESSAGE_TAKE)) {
				unk4.mHeldObject = (TTakeActor*)unk4.mCollisions[i];
			}
		}
	}

	if (oldWire != nullptr && unk7C != nullptr && unk7C != oldWire) {
		unk4.unk70 = 1;
	}

	if (unk4.unk70 != 0) {
		if (oldWire != nullptr) {
			oldWire->release();
		}
		unk4.unk70 = 0;
	} else {
		if (unk4.mHeldObject != nullptr) {
			if (unk7C != nullptr) {
				unk7C->setFootPointsAtHanged(gpMarioOriginal->getTakenMtx());
			}
			if (oldWire != nullptr) {
				oldWire->setFootPointsAtHanged(
				    gpMarioOriginal->getTakenMtx());
				unk7C = oldWire;
			}
		}

		if (unk7C == nullptr && oldWire != nullptr) {
			oldWire->release();
		}
	}
}

inline TMapWireActorManager::TMapWireActorManager(TTakeActor* param_1)
    : unk0(param_1)
    , unk4("アクター補助")
    , unk7C(0)
{
	unk4.unk74 = this;
	unk4.initHitActor(0x40000098, 1, -0x80000000,
	                  TMapWireActor::mCommonAttackRadius,
	                  TMapWireActor::mCommonAttackHeight, 0.0f, 0.0f);

	// TODO: inlines are messed up =(
	JDrama::TNameRefGen* gen   = JDrama::TNameRefGen::getInstance();
	JDrama::TNameRef*    root  = gen->getRootNameRef();
	TIdxGroupObj*        group = (TIdxGroupObj*)root->search("アイテムグループ");
	JGadget::TList_pointer<THitActor*>& children = group->getChildren();
	children.insert(children.end(), &unk4);
}

JUtility::TColor TMapWireManager::mUpperSurface;
JUtility::TColor TMapWireManager::mLowerSurface;

f32 TMapWireActor::mCommonAttackRadius = 200.0f;
f32 TMapWireActor::mCommonAttackHeight = 200.0f;

u32 TMapWireManager::getWireNo(const JGeometry::TVec3<f32>& param_1) const
{
	return gpCubeWire->getInCubeNo(param_1);
}

void TMapWireManager::getPointPosInNthWire(int param_1,
                                           const JGeometry::TVec3<f32>& param_2,
                                           JGeometry::TVec3<f32>* param_3) const
{
	unk18[param_1]->getPointPosOnWire(unk18[param_1]->getPosInWire(param_2),
	                                  param_3);
}

void TMapWireManager::perform(u32 flags, JDrama::TGraphics*)
{
	if (flags & 1) {
		for (int i = 0; i < unk1C; ++i) {
			unk24[i]->doActorToWire();
		}

		for (int i = 0; i < unk10; ++i) {
			unk18[i]->move();
		}

		TMapWireActorManager* manager;
		for (int i = 0; i < unk1C; ++i) {
			manager = unk24[i];
			manager->unk4.onHitFlag(1);
			if (manager->unk7C != nullptr) {
				MtxPtr mtx = gpMarioOriginal->getTakenMtx();
				manager->unk4.offHitFlag(1);
				manager->unk4.mPosition.set(mtx[0][3], mtx[1][3],
				                            mtx[2][3]);
			}
		}
	}

	if (flags & 8) {
		initDraw();

		GXColor upper = mUpperSurface;
		GXSetChanMatColor(GX_COLOR0A0, upper);
		for (int i = 0; i < unk10; ++i) {
			unk18[i]->drawUpper();
		}

		GXColor lower = mLowerSurface;
		GXSetChanMatColor(GX_COLOR0A0, lower);
		for (int i = 0; i < unk10; ++i) {
			unk18[i]->drawLower();
		}
	}

	if (flags & 0x200) {
		for (int i = 0; i < unk10; ++i) {
			unk18[i]->calcViewAndDBEntry();
		}
	}
}

#pragma dont_inline on
namespace JDrama {
TNameRefGen* TNameRefGen::getInstance() { return instance; }

TNameRef* TNameRefGen::getRootNameRef() { return mRootNameRef; }

TNameRef* TNameRef::search(const char* name)
{
	return searchF(calcKeyCode(name), name);
}

template <>
JGadget::TList_pointer<THitActor*>&
TViewObjPtrListT<THitActor, TViewObj>::getChildren()
{
	return *this;
}
} // namespace JDrama

namespace JGadget {
template <>
TList_pointer<THitActor*>::iterator TList_pointer<THitActor*>::end()
{
	return iterator(Base::end());
}

template <>
TList_pointer<THitActor*>::iterator
TList_pointer<THitActor*>::insert(iterator where, THitActor* const& what)
{
	return Base::insert(where, what);
}
} // namespace JGadget
#pragma dont_inline off

void TMapWireManager::loadAfter()
{
	JDrama::TViewObj::loadAfter();

	TTakeActor* mario = gpMarioOriginal;
	unk24[unk1C]      = new TMapWireActorManager(mario);
	++unk1C;
}

void TMapWireManager::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);
	stream.readString();
	stream.read(&unk14, 4);
	stream.read(&unk20, 4);
	stream.read(&TMapWire::mDrawWidth, 4);
	stream.read(&TMapWire::mDrawHeight, 4);
	int val;
	stream.read(&val, 4);
	mUpperSurface.r = val;
	stream.read(&val, 4);
	mUpperSurface.g = val;
	stream.read(&val, 4);
	mUpperSurface.b = val;

	stream.read(&val, 4);
	mLowerSurface.r = val;
	stream.read(&val, 4);
	mLowerSurface.g = val;
	stream.read(&val, 4);
	mLowerSurface.b = val;

	unk18 = new TMapWire*[unk14];
	unk24 = new TMapWireActorManager*[unk20];

	unk10 = gpCubeWire->unk10;

	for (int i = 0; i < unk10; ++i) {
		unk18[i] = new TMapWire;
		unk18[i]->init(&(*gpCubeWire->unk14)[i]);
	}
}

TMapWireManager::TMapWireManager(const char* name)
    : JDrama::TViewObj(name)
{
	unk10            = 0;
	unk18            = nullptr;
	unk1C            = 0;
	unk24            = nullptr;
	unk28            = 0;
	gpMapWireManager = this;
	mUpperSurface.r  = 0x78;
	mUpperSurface.g  = 0x78;
	mUpperSurface.b  = 0x78;
	mUpperSurface.a  = 0xff;
	mLowerSurface.r  = 0x32;
	mLowerSurface.g  = 0x32;
	mLowerSurface.b  = 0x32;
	mLowerSurface.a  = 0xff;
}
