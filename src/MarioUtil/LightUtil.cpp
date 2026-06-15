#define JGADGET_TLIST_POINTER_END_OUT_OF_LINE
#define JGADGET_TLIST_POINTER_ITERATOR_OUT_OF_LINE
#include <MarioUtil/LightUtil.hpp>
#undef JGADGET_TLIST_POINTER_END_OUT_OF_LINE
#undef JGADGET_TLIST_POINTER_ITERATOR_OUT_OF_LINE
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/ReinitGX.hpp>
#include <Player/MarioAccess.hpp>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <printf.h>
#include <string.h>

TLightWithDBSetManager* gpLightManager;
JDrama::TAmbAry* TLightCommon::mAmbAry;
JDrama::TLightAry* TLightCommon::mLightAry;
Vec* TLightCommon::mLightPos;

namespace {

static const char dummyLightUtilStringValue[]
    = "\0\0\0\0\0\0\0\0\0\0\0";

static inline int findLightIndex(const char* name)
{
	int result = 0;
	while (result < TLightCommon::mLightAry->mLightCount) {
		if (strcmp(name, TLightCommon::mLightAry->mLights[result].getName())
		    == 0)
			return result;
		++result;
	}
	return -1;
}

static inline int findAmbIndex(const char* name)
{
	int result = 0;
	while (result < TLightCommon::mAmbAry->mAmbColorCount) {
		if (strcmp(name, TLightCommon::mAmbAry->mAmbColors[result].getName())
		    == 0)
			return result;
		++result;
	}
	return -1;
}

} // namespace

#pragma dont_inline on
TLightCommon::TLightCommon(const char* name)
    : JDrama::TViewObj(name)
    , unk10(0.0f)
    , unk14(1.0f)
    , unk18(1.0f)
    , unk1C(1.0f)
    , unk20(0)
    , unk24(0)
    , unk28(0)
    , unk41(0)
{
	mAmbAry   = nullptr;
	mLightAry = nullptr;
	mLightPos = nullptr;
	unk10     = 50.0f;
}
#pragma dont_inline off

void TLightCommon::loadAfter()
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	mAmbAry = (JDrama::TAmbAry*)root->search("Ambient Group");
	mLightAry = (JDrama::TLightAry*)root->search("Light Group");
	mLightPos = (Vec*)&mLightAry->mLights[0].mPosition;

	unk10 = 50.0f;
	for (int i = 0; i < 4; ++i) {
		GXGetLightColor(&mLightAry->mLights[unk24 + i].unk24, &unk31[i]);
		unk44[i] = mLightAry->mLights[unk24 + i].mPosition;
	}

	unk29[0] = mAmbAry->mAmbColors[unk20].mColor;
	unk29[1] = mAmbAry->mAmbColors[unk20 + 1].mColor;
}

GXColor TLightCommon::getLightColor(int index) const
{
	if (unk28) {
		if (index >= 4)
			index = 0;
		return unk31[index];
	}

	GXColor lightColor;
	index += unk24;
	GXGetLightColor(&mLightAry->mLights[index].unk24, &lightColor);
	GXColor color = lightColor;
	color.a = (u8)(color.a * unk1C);
	return color;
}

GXColor TLightCommon::getAmbColor(int index) const
{
	if (unk28) {
		if (index >= 2)
			index = 0;
		return unk29[index];
	}

	index += unk20;
	GXColor color = mAmbAry->mAmbColors[index].mColor;
	color.a       = (u8)(color.a * unk18);
	return color;
}

Vec* TLightCommon::getLightPosition(int index)
{
	if (unk41) {
		if (index >= 4)
			index = 0;
		return (Vec*)&unk44[index];
	}
	index += unk24;
	return (Vec*)&mLightAry->mLights[index].mPosition;
}

void TLightCommon::setLight(const JDrama::TGraphics* graphics, int index)
{
	ReInitializeGX();
	SMS_DrawInit();

	int lightIndex = index << 1;
	MtxPtr viewMtx = (MtxPtr)graphics->mViewMtx.mMtx;
	GXLightObj light;
	Vec pos;

	PSMTXMultVec(viewMtx, getLightPosition(lightIndex), &pos);
	GXInitLightPos(&light, pos.x, pos.y, pos.z);
	GXInitLightColor(&light, getLightColor(lightIndex));
	GXInitLightAttn(&light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	GXLoadLightObjImm(&light, GX_LIGHT0);

	TLightWithDBSetManager* manager = gpLightManager;
	if (manager->unk54 && manager->unk55) {
		PSMTXMultVec(viewMtx, &manager->unk1C, &pos);
		GXInitLightPos(&light, pos.x, pos.y, pos.z);
		GXColor color = manager->unk18;
		color.a = (u8)(color.a * manager->unk28);
		GXInitLightColor(&light, color);
		GXInitLightAttnA(&light, 1.0f, 0.0f, 0.0f);
		GXInitLightDistAttn(&light, 1000.0f, 0.5f, GX_DA_STEEP);
		GXLoadLightObjImm(&light, GX_LIGHT1);
	}

	PSMTXMultVec(viewMtx, getLightPosition(lightIndex), &pos);
	PSVECNormalize(&pos, &pos);
	GXInitSpecularDir(&light, -pos.x, -pos.y, -pos.z);
	GXInitLightColor(&light, getLightColor(lightIndex));
	f32 spec = unk10 * 0.5f;
	GXInitLightAttn(&light, 0.0f, 0.0f, 1.0f, spec, 0.0f, 1.0f - spec);
	GXLoadLightObjImm(&light, GX_LIGHT2);

	GXSetChanAmbColor(GX_COLOR0A0, getAmbColor(index));
}

void TLightCommon::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x80) {
		ReInitializeGX();
		SMS_DrawInit();

		GXLightObj light;
		GXInitLightPos(&light, getLightPosition(0)->x, getLightPosition(0)->y,
		               getLightPosition(0)->z);
		GXInitLightColor(&light, getLightColor(0));
		GXInitLightAttn(&light, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
		GXLoadLightObjImm(&light, GX_LIGHT0);
		GXLoadLightObjImm(&light, GX_LIGHT1);
		GXLoadLightObjImm(&light, GX_LIGHT2);
	}

	if (flags & 0x20)
		setLight(graphics, 0);
}

void TLightShadow::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x20)
		setLight(graphics, 1);
}

void TLightMario::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x20)
		setLight(graphics, *gpMarioLightID);
}

void TLightMario::setLight(const JDrama::TGraphics* graphics, int index)
{
	ReInitializeGX();
	SMS_DrawInit();

	int lightIndex = index << 1;
	MtxPtr viewMtx = (MtxPtr)graphics->mViewMtx.mMtx;
	GXLightObj light;
	Vec pos;

	PSMTXMultVec(viewMtx, getLightPosition(lightIndex), &pos);
	GXInitLightPos(&light, pos.x, pos.y, pos.z);
	GXInitLightColor(&light, getLightColor(lightIndex));
	GXInitLightAttn(&light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	GXLoadLightObjImm(&light, GX_LIGHT0);

	TLightWithDBSetManager* manager = gpLightManager;
	if (manager->unk54 && manager->unk55) {
		PSMTXMultVec(viewMtx, &manager->unk1C, &pos);
		GXInitLightPos(&light, pos.x, pos.y, pos.z);
		GXColor color = manager->unk18;
		color.a = (u8)(color.a * manager->unk28);
		GXInitLightColor(&light, color);
		GXInitLightAttnA(&light, 1.0f, 0.0f, 0.0f);
		GXInitLightDistAttn(&light, 1000.0f, 0.5f, GX_DA_STEEP);
		GXLoadLightObjImm(&light, GX_LIGHT1);
	}

	PSMTXMultVec(viewMtx, getLightPosition(lightIndex), &pos);
	PSVECNormalize(&pos, &pos);
	GXInitSpecularDir(&light, -pos.x, -pos.y, -pos.z);
	GXInitLightColor(&light, getLightColor(lightIndex));
	f32 spec = unk10 * 0.5f;
	GXInitLightAttn(&light, 0.0f, 0.0f, 1.0f, spec, 0.0f, 1.0f - spec);
	GXLoadLightObjImm(&light, GX_LIGHT2);

	GXSetChanAmbColor(GX_COLOR0A0, getAmbColor(index));
}

GXColor TLightMario::getLightColor(int index) const
{
	int lightIndex = index + unk24;
	GXColor color;
	if (unk28) {
		if (lightIndex >= 4)
			lightIndex = 0;
		color = unk31[lightIndex];
	} else {
		GXColor lightColor;
		lightIndex += unk24;
		GXGetLightColor(&mLightAry->mLights[lightIndex].unk24, &lightColor);
		lightColor.a = (u8)(lightColor.a * unk1C);
		color        = lightColor;
	}

	GXColor result = color;
	result.a       = (u8)(result.a * unk14);
	return result;
}

GXColor TLightMario::getAmbColor(int index) const
{
	int ambIndex = index + unk24;
	GXColor color;
	if (unk28) {
		if (ambIndex >= 2)
			ambIndex = 0;
		color = unk29[ambIndex];
	} else {
		ambIndex += unk20;
		GXColor ambColor = mAmbAry->mAmbColors[ambIndex].mColor;
		ambColor.a       = (u8)(ambColor.a * unk18);
		color            = ambColor;
	}

	GXColor result = color;
	result.a       = (u8)(result.a * unk14);
	return result;
}

#pragma dont_inline on
TLightDrawBuffer::TLightDrawBuffer(int index, u32 size, const char* name)
    : JDrama::TViewObj(name)
    , unk10(nullptr)
    , unk14(nullptr)
    , unk18(nullptr)
    , unk80(index)
{
	snprintf(unk1C, 0x32, "%s%s", name, "opa");
	unk14 = new JDrama::TDrawBufObj(3, size, unk1C);
	snprintf(unk1C + 0x32, 0x32, "%s%s", name, "xlu");
	unk18 = new JDrama::TDrawBufObj(4, size, unk1C + 0x32);
}
#pragma dont_inline off

void TLightDrawBuffer::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x20)
		unk10->setLight(graphics, unk80);
}

void TLightDrawBuffer::setLight(TLightCommon* light)
{
	unk10 = light;
	unk10->loadAfter();
}

void TLightWithDBSet::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x20) {
		for (int i = 0; i < unk1C; ++i) {
			unk10[i]->perform(0x20, graphics);
			if (flags & 0x10000)
				unk10[i]->unk14->perform(8, graphics);
			if (flags & 0x20000)
				unk10[i]->unk18->perform(8, graphics);
		}
	}

	if (flags & 0x400) {
		for (int i = 0; i < unk1C; ++i) {
			unk10[i]->unk14->perform(0x480, graphics);
			unk10[i]->unk18->perform(0x480, graphics);
		}
	}
}

void TLightWithDBSet::changeLightDrawBuffer(int param_1)
{
	unk14 = nullptr;
	unk18 = nullptr;
	if (param_1 > unk1C)
		param_1 = 0;

	unk14 = j3dSys.getDrawBuffer(0);
	unk18 = j3dSys.getDrawBuffer(1);

	j3dSys.setDrawBuffer(unk10[param_1]->unk14->mDrawBuffer, 0);
	j3dSys.setDrawBuffer(unk10[param_1]->unk18->mDrawBuffer, 1);
}

void TLightWithDBSet::resetLightDrawBuffer()
{
	if (!unk14)
		return;
	if (!unk18)
		return;

	j3dSys.setDrawBuffer(unk14, 0);
	j3dSys.setDrawBuffer(unk18, 1);
	unk14 = nullptr;
	unk18 = nullptr;
}

void TPlayerLightWithDBSet::makeDrawBuffer()
{
	static const char lightName[] = "太陽（プレイヤー）";
	static const char ambName[]   = "太陽アンビエント（プレイヤー）";

	int lightIndex = findLightIndex(lightName);
	int ambIndex   = findAmbIndex(ambName);

	unk10 = new TLightDrawBuffer*[unk1C];
	for (int i = 0; i < unk1C; ++i) {
		unk10[i] = new TLightDrawBuffer(
		    i, 0x80, TLightCommon::mAmbAry->mAmbColors[ambIndex + i].getName());
		TLightMario* light = new TLightMario();
		unk10[i]->setLight(light);
		unk10[i]->unk10->unk20 = ambIndex;
		unk10[i]->unk10->unk24 = lightIndex;
		unk10[i]->unk10->loadAfter();
	}
}

void TObjectLightWithDBSet::makeDrawBuffer()
{
	static const char lightName[] = "太陽（オブジェクト）";
	static const char ambName[]   = "太陽アンビエント（オブジェクト）";

	int lightIndex = findLightIndex(lightName);
	int ambIndex   = findAmbIndex(ambName);

	unk10 = new TLightDrawBuffer*[unk1C];
	for (int i = 0; i < unk1C; ++i) {
		unk10[i] = new TLightDrawBuffer(
		    i, 0x100, TLightCommon::mAmbAry->mAmbColors[ambIndex + i].getName());
		TLightCommon* light = new TLightCommon();
		unk10[i]->setLight(light);
		unk10[i]->unk10->unk20 = ambIndex;
		unk10[i]->unk10->unk24 = lightIndex;
		unk10[i]->unk10->loadAfter();
	}
}

void TMapObjectLightWithDBSet::makeDrawBuffer()
{
	static const char lightName[] = "太陽（オブジェクト）";
	static const char ambName[]   = "太陽アンビエント（オブジェクト）";
	static const char* className[] = { "マップオブジェ太陽",
		"マップオブジェ影" };

	int lightIndex = findLightIndex(lightName);
	int ambIndex   = findAmbIndex(ambName);

	unk10 = new TLightDrawBuffer*[unk1C];
	for (int i = 0; i < unk1C; ++i) {
		unk10[i] = new TLightDrawBuffer(i, 0x100, className[i]);
		TLightCommon* light = new TLightCommon();
		unk10[i]->setLight(light);
		unk10[i]->unk10->unk20 = ambIndex;
		unk10[i]->unk10->unk24 = lightIndex;
		unk10[i]->unk10->loadAfter();
	}
}

void TIndirectLightWithDBSet::makeDrawBuffer()
{
	static const char lightName[] = "太陽（オブジェクト）";
	static const char ambName[]   = "太陽アンビエント（オブジェクト）";
	static const char* className[]
	    = { "インダイレクト太陽", "インダイレクト影" };

	int lightIndex = findLightIndex(lightName);
	int ambIndex   = findAmbIndex(ambName);

	unk10 = new TLightDrawBuffer*[unk1C];
	for (int i = 0; i < unk1C; ++i) {
		unk10[i] = new TLightDrawBuffer(i, 0x100, className[i]);
		TLightCommon* light = new TLightCommon();
		unk10[i]->setLight(light);
		unk10[i]->unk10->unk20 = ambIndex;
		unk10[i]->unk10->unk24 = lightIndex;
		unk10[i]->unk10->loadAfter();
	}
}

TLightWithDBSetManager::TLightWithDBSetManager(const char* name)
    : JDrama::TViewObj(name)
    , unk10(nullptr)
    , unk14(nullptr)
    , unk54(false)
    , unk55(true)
{
	unk14    = new TLightWithDBSet*[4];
	unk14[0] = new TPlayerLightWithDBSet();
	unk14[1] = new TObjectLightWithDBSet();
	unk14[2] = new TMapObjectLightWithDBSet();
	unk14[3] = new TIndirectLightWithDBSet();

	gpLightManager = this;
	unk48.x        = 0.0f;
	unk48.y        = 0.0f;
	unk48.z        = 0.0f;
	unk28          = 1.0f;
	unk2C          = 100.0f;
	unk30          = 400.0f;
	unk34          = 1000.0f;
	unk38          = 1.80535f;
	unk3C          = -0.012058f;
	unk40          = 0.00003f;
	unk44          = 90.0f;

	f32 points[3] = { 0.9f, 0.5f, 0.05f };
	f32 distance[3];
	distance[0] = unk2C;
	distance[1] = unk30;
	distance[2] = unk34;
	f32 firstA       = points[1] * (points[0]
	                          * (distance[0] * distance[0]
	                             - distance[1] * distance[1]));
	f32 firstB       = points[1] * (points[0] * (distance[0] - distance[1]));
	f32 firstC       = points[1] - points[0];
	f32 secondA      = points[2] * (points[1]
	                           * (distance[1] * distance[1]
	                              - distance[2] * distance[2]));
	f32 secondB      = points[2] * (points[1] * (distance[1] - distance[2]));
	f32 secondC      = points[2] - points[1];
	unk40 = (firstC * secondB - secondC * firstB)
	        / (firstA * secondB - secondA * firstB);
	unk3C = (firstC - firstA * unk40) / firstB;
	unk38 = points[0] - (unk40 * distance[0] * distance[0]
	                     + distance[0] * unk3C);
}

void TLightWithDBSetManager::loadAfter()
{
	JDrama::TNameRef* root
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef();
	JDrama::TLightAry* lightAry
	    = (JDrama::TLightAry*)root->search("Light Group");
	GXColor color;
	GXGetLightColor(&lightAry->mLights[0].unk24, &color);
	unk18 = color;
	unk1C = lightAry->mLights[0].mPosition;
}

void TLightWithDBSetManager::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x20) {
		int begin;
		int end;
		if (flags & 0x80000) {
			begin = 3;
			end   = 4;
		} else if (flags & 0x40000) {
			begin = 2;
			end   = 3;
		} else {
			begin = 0;
			end   = 2;
		}

		for (int i = begin; i < end; ++i)
			if (unk14[i]->unk20)
				unk14[i]->perform(flags, graphics);
	}

	if (flags & 0x400) {
		for (int i = 0; i < 4; ++i)
			if (unk14[i]->unk20)
				unk14[i]->perform(flags, graphics);
	}
}

#pragma dont_inline on
namespace JGadget {
template <>
TList_pointer<JDrama::TViewObj*>::iterator::iterator(
    TList<void*, TAllocator<void*> >::iterator it)
    : TList<void*, TAllocator<void*> >::iterator(it)
{
}

template <>
TList_pointer<JDrama::TViewObj*>::iterator
TList_pointer<JDrama::TViewObj*>::end()
{
	return iterator(Base::end());
}
} // namespace JGadget
#pragma dont_inline off

void TLightWithDBSetManager::addChildGroupObj(
    JDrama::TViewObjPtrListT<JDrama::TViewObj, JDrama::TViewObj>* group)
{
	JGadget::TList_pointer<JDrama::TViewObj*>& children = group->getChildren();
	for (int i = 0; i < 4; ++i) {
		TLightWithDBSet* set = unk14[i];
		if (!set->unk20)
			continue;

		for (int j = 0; j < set->unk1C; ++j) {
			JDrama::TViewObj* drawBuffer = set->unk10[j]->unk14;
			children.insert(children.end(), drawBuffer);
			drawBuffer = set->unk10[j]->unk18;
			children.insert(children.end(), drawBuffer);
		}
	}
}

void TLightWithDBSetManager::makeDrawBuffer()
{
	for (int i = 0; i < 4; ++i)
		if (unk14[i]->unk20)
			unk14[i]->makeDrawBuffer();
}

Vec* TLightWithDBSetManager::getLightPos() const
{
	return TLightCommon::mLightPos;
}
