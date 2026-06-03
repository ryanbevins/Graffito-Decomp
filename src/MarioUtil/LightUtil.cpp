#include <MarioUtil/LightUtil.hpp>
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
	GXColor color;
	if (unk28) {
		if (index >= 4)
			index = 0;
		color = unk31[index];
	} else {
		GXGetLightColor(&mLightAry->mLights[unk24 + index].unk24, &color);
		color.a = (u8)(color.a * unk1C);
	}
	return color;
}

GXColor TLightCommon::getAmbColor(int index) const
{
	GXColor color;
	if (unk28) {
		if (index >= 2)
			index = 0;
		color = unk29[index];
	} else {
		color = mAmbAry->mAmbColors[unk20 + index].mColor;
		color.a = (u8)(color.a * unk18);
	}
	return color;
}

Vec* TLightCommon::getLightPosition(int index)
{
	if (unk41) {
		if (index >= 4)
			index = 0;
		return (Vec*)&unk44[index];
	}
	return (Vec*)&mLightAry->mLights[unk24 + index].mPosition;
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
	GXColor color = getLightColor(lightIndex);
	GXInitLightColor(&light, color);
	GXInitLightAttn(&light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	GXLoadLightObjImm(&light, GX_LIGHT0);

	TLightWithDBSetManager* manager = gpLightManager;
	if (manager->unk54 && manager->unk55) {
		PSMTXMultVec(viewMtx, &manager->unk48, &pos);
		GXInitLightPos(&light, pos.x, pos.y, pos.z);
		color   = manager->unk18;
		color.a = (u8)(color.a * manager->unk28);
		GXInitLightColor(&light, color);
		GXInitLightAttnA(&light, 1.0f, 0.0f, 0.0f);
		GXInitLightDistAttn(&light, 1000.0f, 0.5f, GX_DA_STEEP);
		GXLoadLightObjImm(&light, GX_LIGHT1);
	}

	PSMTXMultVec(viewMtx, getLightPosition(lightIndex), &pos);
	PSVECNormalize(&pos, &pos);
	GXInitSpecularDir(&light, -pos.x, -pos.y, -pos.z);
	color = getLightColor(lightIndex);
	GXInitLightColor(&light, color);
	f32 spec = unk10 * 0.5f;
	GXInitLightAttn(&light, 0.0f, 0.0f, 1.0f, spec, 0.0f, 1.0f - spec);
	GXLoadLightObjImm(&light, GX_LIGHT2);

	color = getAmbColor(index);
	GXSetChanAmbColor(GX_COLOR0A0, color);
}

void TLightCommon::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 0x80) {
		ReInitializeGX();
		SMS_DrawInit();

		GXLightObj light;
		Vec pos;
		Vec* lightPos = getLightPosition(0);
		GXInitLightPos(&light, lightPos->x, lightPos->y, lightPos->z);
		GXColor color = getLightColor(0);
		GXInitLightColor(&light, color);
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
	GXColor color = getLightColor(lightIndex);
	GXInitLightColor(&light, color);
	GXInitLightAttn(&light, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	GXLoadLightObjImm(&light, GX_LIGHT0);

	TLightWithDBSetManager* manager = gpLightManager;
	if (manager->unk54 && manager->unk55) {
		PSMTXMultVec(viewMtx, &manager->unk48, &pos);
		GXInitLightPos(&light, pos.x, pos.y, pos.z);
		color   = manager->unk18;
		color.a = (u8)(color.a * manager->unk28);
		GXInitLightColor(&light, color);
		GXInitLightAttnA(&light, 1.0f, 0.0f, 0.0f);
		GXInitLightDistAttn(&light, 1000.0f, 0.5f, GX_DA_STEEP);
		GXLoadLightObjImm(&light, GX_LIGHT1);
	}

	PSMTXMultVec(viewMtx, getLightPosition(lightIndex), &pos);
	PSVECNormalize(&pos, &pos);
	GXInitSpecularDir(&light, -pos.x, -pos.y, -pos.z);
	color = getLightColor(lightIndex);
	GXInitLightColor(&light, color);
	f32 spec = unk10 * 0.5f;
	GXInitLightAttn(&light, 0.0f, 0.0f, 1.0f, spec, 0.0f, 1.0f - spec);
	GXLoadLightObjImm(&light, GX_LIGHT2);

	color = getAmbColor(index);
	GXSetChanAmbColor(GX_COLOR0A0, color);
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
		GXGetLightColor(&mLightAry->mLights[lightIndex + unk24].unk24, &color);
		color.a = (u8)(color.a * unk1C);
	}

	color.a = (u8)(color.a * unk14);
	return color;
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
		color = mAmbAry->mAmbColors[ambIndex + unk20].mColor;
		color.a = (u8)(color.a * unk18);
	}

	color.a = (u8)(color.a * unk14);
	return color;
}

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

void TPlayerLightWithDBSet::makeDrawBuffer() { }

void TObjectLightWithDBSet::makeDrawBuffer() { }

void TMapObjectLightWithDBSet::makeDrawBuffer() { }

void TIndirectLightWithDBSet::makeDrawBuffer() { }

TLightWithDBSetManager::TLightWithDBSetManager(const char* name)
    : JDrama::TViewObj(name)
{
}

void TLightWithDBSetManager::loadAfter() { }

void TLightWithDBSetManager::perform(u32, JDrama::TGraphics*) { }

void TLightWithDBSetManager::addChildGroupObj(
    JDrama::TViewObjPtrListT<JDrama::TViewObj, JDrama::TViewObj>*)
{
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
