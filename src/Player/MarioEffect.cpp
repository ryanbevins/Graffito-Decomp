#include <Player/MarioEffect.hpp>
#include <Enemy/Conductor.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphLoader/J3DModelLoader.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static const char cDirtyFileName[] = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]  = "H_ma_rak_dummy";

enum { MARIO_FLAG_FLUDD_EMITTING = 1 << 14 };

class TWaterGun {
public:
	MtxPtr getEmitMtx(int);
};

void TMarioEffect::init(TMario* mario)
{
	unk68    = mario;
	unk6C[0] = 0;
	unk6C[1] = 0;

	MActorAnmData* jumpAnm = new MActorAnmData;
	jumpAnm->init("mario/04_tobikomi", nullptr);

	for (int i = 0; i < 2; ++i)
		unk74[i] = new MActor(jumpAnm);

	void* jumpRes
	    = JKRFileLoader::getGlbResource("/mario/04_tobikomi/04_tobikomi.bmd");
	u32 jumpModelFlags = 0x10040000;
	for (int i = 0; i < 2; ++i) {
		J3DModel* model = new J3DModel(
		    J3DModelLoaderDataBase::load(jumpRes, jumpModelFlags), 0, 1);
		unk74[i]->setModel(model, 0);
	}

	MActorAnmData* boostAnm = new MActorAnmData;
	boostAnm->init("mario/01_waterboost", nullptr);
	unk7C = 0;
	unk80 = new MActor(boostAnm);

	void* boostRes = JKRFileLoader::getGlbResource(
	    "/mario/01_waterboost/01_waterboost.bmd");
	unk80->setModel(new J3DModel(
	                    J3DModelLoaderDataBase::load(boostRes, 0x10040000), 0,
	                    1),
	                0);
	unk80->setBck("01_waterboost_in");
	unk80->setBtk("01_waterboost");
	unk80->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
	unk80->getFrameCtrl(4)->setRate(SMSGetAnmFrameRate());

	gpConductor->registerOtherObj(this);
}

void TMarioEffect::setJumpIntoWaterEffect()
{
	f32 speed    = *(f32*)((u8*)unk68 + 0xA8);
	f32 absSpeed = speed;
	if (speed < 0.0f)
		absSpeed = -speed;

	if (absSpeed < *(f32*)((u8*)unk68 + 0x22C4))
		return;

	if (*(f32*)((u8*)unk68 + 0xF0) - *(f32*)((u8*)unk68 + 0xEC) < 50.0f)
		return;

	int idx = getThing();
	if (idx < 0)
		return;
	MActor* (&actors)[2] = unk74;

	Mtx mtx;
	PSMTXCopy((MtxPtr)((u8*)unk68 + 0x220), mtx);

	f32 lo = *(f32*)((u8*)unk68 + 0x22D8);
	f32 hi = *(f32*)((u8*)unk68 + 0x22EC);
	f32 t;
	if (absSpeed < lo)
		t = 0.0f;
	if (absSpeed > hi)
		t = 1.0f;
	if (lo <= absSpeed && absSpeed <= hi)
		t = (absSpeed - lo) / (hi - lo);

	f32 scale = t * *(f32*)((u8*)unk68 + 0x2314)
	            + *(f32*)((u8*)unk68 + 0x2300);
	mtx[0][0] = scale;
	mtx[1][1] = scale;
	mtx[2][2] = scale;

	actors[idx]->setBck("04_tobikomi");
	actors[idx]->setBpk("04_tobikomi");
	actors[idx]->setBtk("04_tobikomi");
	actors[idx]->setBrk("04_tobikomi");
	actors[idx]->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
	actors[idx]->getFrameCtrl(2)->setRate(SMSGetAnmFrameRate());
	actors[idx]->getFrameCtrl(4)->setRate(SMSGetAnmFrameRate());
	actors[idx]->getFrameCtrl(5)->setRate(SMSGetAnmFrameRate());
	PSMTXCopy(mtx, actors[idx]->getModel()->unk20);

	actors[idx]->getModel()->mShapePackets[0].show();
	actors[idx]->getModel()->mShapePackets[1].show();
	actors[idx]->getModel()->mShapePackets[2].show();
	actors[idx]->getModel()->mShapePackets[3].show();
	actors[idx]->getModel()->mShapePackets[4].show();
	unk6C[idx] = 1;
}

void TMarioEffect::setJumpIntoWaterEffectSmall()
{
	int idx = getThing();
	if (idx < 0)
		return;

	Mtx base;
	Mtx scale;
	PSMTXCopy((MtxPtr)((u8*)unk68 + 0x220), base);
	PSMTXScale(scale, 0.8f, 0.4f, 0.8f);
	PSMTXConcat(base, scale, base);

	MActor** actor = &unk74[idx];
	(*actor)->setBck("04_tobikomi");
	(*actor)->setBpk("04_tobikomi");
	(*actor)->setBtk("04_tobikomi");
	(*actor)->setBrk("04_tobikomi");
	(*actor)->getFrameCtrl(0)->setRate(SMSGetAnmFrameRate());
	(*actor)->getFrameCtrl(2)->setRate(SMSGetAnmFrameRate());
	(*actor)->getFrameCtrl(4)->setRate(SMSGetAnmFrameRate());
	(*actor)->getFrameCtrl(5)->setRate(SMSGetAnmFrameRate());
	PSMTXCopy(base, (*actor)->getModel()->unk20);

	(*actor)->getModel()->mShapePackets[1].hide();
	(*actor)->getModel()->mShapePackets[2].hide();
	(*actor)->getModel()->mShapePackets[4].hide();
	unk6C[idx] = 1;
}

void TMarioEffect::perform(u32 flags, JDrama::TGraphics* gfx)
{
	if (flags & 1) {
		switch (unk7C) {
		case 0:
			if ((*(u32*)((u8*)unk68 + 0x118) & MARIO_FLAG_FLUDD_EMITTING)
			    ? true
			    : false) {
				unk80->setBck("01_waterboost_in");
				unk80->setBtk("01_waterboost");
				unk80->getFrameCtrl(0)->setFrame(0.0f);
				unk80->getFrameCtrl(4)->setFrame(0.0f);
				unk7C = 1;
			}
			break;
		case 1:
			if (((*(u32*)((u8*)unk68 + 0x118) & MARIO_FLAG_FLUDD_EMITTING)
			        ? true
			        : false)
			    == true) {
				if ((*(TWaterGun**)((u8*)unk68 + 0x3E4))->getEmitMtx(0)
				    != nullptr) {
					gpMarioParticleManager->emitAndBindToMtxPtr(
					    0xFE,
					    (*(TWaterGun**)((u8*)unk68 + 0x3E4))->getEmitMtx(0),
					    1, this);
					gpMarioParticleManager->emitAndBindToMtxPtr(
					    0xFF,
					    (*(TWaterGun**)((u8*)unk68 + 0x3E4))->getEmitMtx(0),
					    1, this);
				}
			} else {
				unk80->setBck("01_waterboost_out");
				unk80->getFrameCtrl(0)->setFrame(0.0f);
				unk7C = 2;
			}
			break;
		case 2:
			if (unk80->getFrameCtrl(0)->checkState(3))
				unk7C = 0;
			break;
		}
	}

	if ((flags & 2) && unk7C != 0) {
		if ((*(TWaterGun**)((u8*)unk68 + 0x3E4))->getEmitMtx(0) != nullptr) {
			MtxPtr emitMtx
			    = (*(TWaterGun**)((u8*)unk68 + 0x3E4))->getEmitMtx(0);
			PSMTXCopy(emitMtx, unk80->getModel()->unk20);
			unk80->perform(2, gfx);
		}
	}

	if ((flags & 4) && unk7C != 0)
		unk80->perform(4, gfx);

	if ((flags & 0x200) && unk7C != 0)
		unk80->perform(0x200, gfx);

	for (int i = 0; i < 2; ++i) {
		if (unk6C[i] == 1) {
			unk74[i]->perform(flags, gfx);
			if (unk74[i]->getFrameCtrl(0)->checkState(3))
				unk6C[i] = 0;
		}
	}
}
