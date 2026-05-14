#include <MoveBG/MapObjSirena.hpp>
#include <Map/Map.hpp>
#include <Enemy/Conductor.hpp>
#include <Player/MarioAccess.hpp>
#include <MarioUtil/ModelUtil.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <M3DUtil/MActorData.hpp>
#include <Strategic/ObjModel.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>

// rogue includes needed for matching sinit
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

TWarpAreaActor::TWarpAreaActor(const char* name)
    : THitActor(name)
{
	unk68 = -1;
	unk6A = -1;
}

void TWarpAreaActor::load(JSUMemoryInputStream& stream)
{
	JDrama::TActor::load(stream);

	s32 tmp;
	stream.read(&tmp, 4);
	unk68 = (s16)tmp;
	stream.read(&tmp, 4);
	unk6A = (s16)tmp;

	initHitActor(0x4000019d, 1, 0x80000000, mScaling.x * 100.0f,
	             mScaling.y * 100.0f, 0.0f, 0.0f);
	unk64 &= ~1;
	gpConductor->registerOtherObj(this);
}

void TWarpAreaActor::perform(u32 flags, JDrama::TGraphics* graphics)
{
	THitActor::perform(flags, graphics);

	if (!(flags & 1))
		return;
	if (mColCount == 0)
		return;

	if (*gpMarioSpeedY > 0.0f && unk68 != -1)
		gpMap->changeModel(unk68);
	if (*gpMarioSpeedY < 0.0f && unk6A != -1)
		gpMap->changeModel(unk6A);
}

u32 TSirenaCasinoRoof::getSDLModelFlag() const { return 0; }

void TSirenaCasinoRoof::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		unk138->update();
	TMapObjBase::perform(flags, graphics);
}

void TSirenaCasinoRoof::initMapObj()
{
	TMapObjBase::initMapObj();
	mMActor->offMakeDL();

	MActorAnmData* anmData = mMActorKeeper->getMActorAnmData();
	unk138 = new TMultiBtk(3, getModel()->getModelData());
	for (int i = 0; i <= 2; ++i) {
		MActorAnmDataEach<J3DAnmTextureSRTKey>* btk = anmData->getUnk38();
		J3DAnmTextureSRTKey* anm;
		if (i < btk->unk0)
			anm = (J3DAnmTextureSRTKey*)btk->unkC[i];
		else
			anm = nullptr;
		unk138->setNthData(i, anm);
	}
	mMActor->setBrk("casino_lighting");
}

u32 TSirenabossWall::getSDLModelFlag() const { return 0; }

void TSirenabossWall::drawObject(JDrama::TGraphics* gfx)
{
	mMActor->getModel()->entry();
}

void TSirenabossWall::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 2)
		unk138->update();
	TMapObjBase::perform(flags, graphics);
}

void TSirenabossWall::initMapObj()
{
	TMapObjBase::initMapObj();
	mMActor->offMakeDL();

	MActorAnmData* anmData = mMActorKeeper->getMActorAnmData();
	unk138 = new TMultiBtk(3, getModel()->getModelData());
	for (int i = 0; i <= 2; ++i) {
		MActorAnmDataEach<J3DAnmTextureSRTKey>* btk = anmData->getUnk38();
		J3DAnmTextureSRTKey* anm;
		if (i < btk->unk0)
			anm = (J3DAnmTextureSRTKey*)btk->unkC[i];
		else
			anm = nullptr;
		unk138->setNthData(i, anm);
	}
}
