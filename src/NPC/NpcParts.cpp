#include <NPC/NpcParts.hpp>

#include <NPC/NpcBase.hpp>
#include <NPC/NpcInitData.hpp>
#include <NPC/NpcManager.hpp>
#include <NPC/NpcSave.hpp>
#include <Strategic/SharedParts.hpp>
#include <Strategic/ObjModel.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorAnm.hpp>
#include <M3DUtil/SDLModel.hpp>
#include <M3DUtil/LodAnm.hpp>
#include <MarioUtil/MtxUtil.hpp>
#include <MarioUtil/TexUtil.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/PacketUtil.hpp>
#include <MarioUtil/RandomUtil.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTexture.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DStruct.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DMaterial.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DShape.hpp>
#include <M3DUtil/InfectiousStrings.hpp>
#include <string.h>

extern const char* cNpcPartsNameRootJoint;

const char* cPeachPartsTextureName              = "H_peach_main_dummy";
const char* cPeachHostTextureName               = "H_peach_main_s3tc";
static const char* cNpcPartsNameRootJointStringInfectious
    = "__ROOT_JOINT__";

void SMS_InitChangeNpcColor(const MActor*, const TColorChangeInfo*, s16,
                            const GXColor*);

static inline void setEffectMtxOnTex0(J3DMaterial* mat, MtxPtr mtx)
{
	mat->getTexGenBlock()->getTexMtx(0)->setEffectMtx(mtx);
}

TNpcParts::TNpcParts(u32 mask, const J3DGXColorS10* color_info,
                     TBaseNPC* base_npc)
{
	unk60                        = base_npc;
	const TNpcInitInfo* initData = SMSGetNpcInitData(
	    unk60->getActorType() - 0x04000001);

	memset(unk0, 0, sizeof(unk0));

	for (int level = 0; level < 12; level++) {
		if (initData->unk4[level] == nullptr)
			continue;
		if ((mask & (1 << level)) == 0)
			continue;

		const GXColor* pollutionColor = nullptr;
		s16 unifyIdx = ((s16*)color_info)[initData->unk4[level]->unk28];
		if (initData->unk4[level]->unk2A) {
			pollutionColor = unk60->getPtrInitPollutionColor();
		}

		for (int subIdx = 0; subIdx < 2; subIdx++) {
			if (subIdx >= unk60->mManager->unk28)
				break;

			const char* jointName = initData->unk4[level]->unk0[subIdx];
			const char* partsName = initData->unk4[level]->unk8[subIdx];
			if (partsName == nullptr)
				continue;

			int index;
			if (strcmp(jointName, cNpcPartsNameRootJoint) == 0) {
				index = -1;
			} else {
				MActor* keeperActor = unk60->mMActorKeeper
				    ->mActors[subIdx];
				index = keeperActor->getModel()
				    ->getModelData()->getJointName()
				    ->getIndex(jointName);
			}

			SDLModelData* sdlData =
			    ((TNPCManager*)unk60->mManager)
			        ->getPartsSDLModelData(partsName);

			unk0[subIdx][level] = new TSharedParts(
			    unk60, index, sdlData, 3, "<TSharedParts>");

			if (initData->unk4[level]->unk2B) {
				SMS_UnifyMaterial(
				    unk0[subIdx][level]->unk18->getModel());
			}

			u32 actorType = unk60->getActorType();
			switch (actorType) {
			case 0x04000018: // Peach
				if (subIdx != 0
				    || (level != 3 && level != 4)) {
					J3DModelData* mdataPart = unk0[subIdx][level]->unk18
					    ->getModel()->getModelData();
					J3DModelData* mdataBase = unk60->getModel()
					    ->getModelData();
					int peachHostIdx
					    = mdataBase->getTextureName()->getIndex(
					        cPeachHostTextureName);
					SMS_ChangeTextureAll(mdataPart,
					    cPeachPartsTextureName,
					    mdataBase->getTexture()
					        ->mResources[peachHostIdx]);
					unk0[subIdx][level]->unk18->initDL();
				}
				if (subIdx == 0
				    && (level == 0
				        || (level >= 3 && level < 5))) {
					int bckIdx = -1;
					if (bckIdx == -1) {
						bckIdx = TBaseNPC::mPtrSaveNormal
						    ->mMotionBlendFrame.get();
					}
					if (unk0[subIdx][level]->unk18->getAnmBck()
					    != nullptr) {
						unk0[subIdx][level]->unk18->getAnmBck()
						    ->initSimpleMotionBlend(bckIdx);
					}
				}
				break;
			case 0x04000010: // Sunflower
				if (subIdx == 0 && level == 9) {
					int bckIdx = 0x14;
					if (unk0[subIdx][level]->unk18->getAnmBck()
					    != nullptr) {
						unk0[subIdx][level]->unk18->getAnmBck()
						    ->initSimpleMotionBlend(bckIdx);
					}
				}
				break;
			case 0x04000015:
				if (subIdx == 0 && level == 10) {
					int bckIdx = -1;
					if (bckIdx == -1) {
						bckIdx = TBaseNPC::mPtrSaveNormal
						    ->mMotionBlendFrame.get();
					}
					if (unk0[subIdx][level]->unk18->getAnmBck()
					    != nullptr) {
						unk0[subIdx][level]->unk18->getAnmBck()
						    ->initSimpleMotionBlend(bckIdx);
					}
				}
				break;
			}

			for (int ci = 0; ci < 3; ci++) {
				const TColorChangeInfo* colorInfo
				    = initData->unk4[level]->unk10[ci].unk0;
				if (colorInfo != nullptr) {
					SMS_InitChangeNpcColor(unk0[subIdx][level]->unk18,
					    colorInfo, unifyIdx, pollutionColor);
				}
			}

			if (pollutionColor != nullptr) {
				J3DModelData* mdata = unk0[subIdx][level]->unk18->getModel()
				    ->getModelData();
				J3DModel* model
				    = unk0[subIdx][level]->unk18->getModel();
				u16 numMaterials = mdata->getMaterialNum();
				for (u16 ki = 0; ki < numMaterials; ki++) {
					J3DMaterial* mat
					    = mdata->getMaterialNodePointer(ki);
					u16 shapeIdx = mat->mShape->unk4;
					J3DShapePacket* sp = model->getShapePacket(shapeIdx);
					if (((u32*)sp)[3] == 0) {
						SMS_InitPacket_OneTevKColor(
						    model, ki, (GXTevKColorID)0,
						    pollutionColor);
					}
				}
			}
			unk0[subIdx][level]->unk18->setLightType(1);
		}
	}
}

void TNpcParts::addJellyFishParts(f32 frame)
{
	TSharedParts** dest = &unk0[0][11];
	int numData = gpMareJellyFishManager->getModelDataKeeper()
	    ->getModelDataNum();
	int randIdx = (int)(MsRandF() * (f32)numData);
	SDLModelData* sdlData = gpMareJellyFishManager->getModelDataKeeper()
	    ->getNthData(randIdx);
	SDLModel* model = new SDLModel(sdlData, 0, 1);
	MActor* mactor = new MActor(gpMareJellyFishManager->getMActorAnmData());
	mactor->setModel(model, 0);
	*dest = new TSharedParts(unk60, -1, mactor, "<TSharedParts>");
	mactor->setBckFromIndex(0);
	mactor->setBrkFromIndex(randIdx);
	mactor->getFrameCtrl(0)->setFrame(frame);
	mactor->getFrameCtrl(5)->setFrame(frame);
	mactor->setLightType(3);
}

void TNpcParts::setPartsAnmFrame(f32 frame)
{
	MActor* m;
	J3DFrameCtrl* fc;

	switch (unk60->getActorType()) {
	case 0x04000010: { // Sunflower
		TSharedParts* parts = unk0[0][9];
		m                   = nullptr;
		if (parts != nullptr)
			m = parts->unk18;
		if (m != nullptr) {
			fc = m->getFrameCtrl(0);
			if (fc != nullptr)
				fc->setFrame(frame);
		}
		break;
	}
	case 0x04000015:
		m = getPartsMActor(10, 0);
		if (m == nullptr)
			break;
		fc = m->getFrameCtrl(0);
		if (fc != nullptr)
			fc->setFrame(frame);
		fc = m->getFrameCtrl(3);
		if (fc != nullptr)
			fc->setFrame(frame);
		break;
	case 0x04000018: // Peach
		m = getPartsMActor(0, 0);
		if (m != nullptr) {
			fc = m->getFrameCtrl(0);
			if (fc != nullptr)
				fc->setFrame(frame);
		}
		m = getPartsMActor(3, 0);
		if (m != nullptr) {
			fc = m->getFrameCtrl(0);
			if (fc != nullptr)
				fc->setFrame(frame);
		}
		m = getPartsMActor(4, 0);
		if (m != nullptr) {
			fc = m->getFrameCtrl(0);
			if (fc != nullptr)
				fc->setFrame(frame);
		}
		break;
	}
}

MActor* TNpcParts::getPartsMActor(int joint, int layer)
{
	TSharedParts* parts = unk0[layer][joint];
	MActor* result      = nullptr;
	if (parts != nullptr) {
		result = parts->unk18;
	}
	return result;
}

void TNpcParts::partsFrameUpdate()
{
	int i                = 0;
	TSharedParts** parts = &unk0[unk60->unkD0->unk8][0];
	for (; i < 12; i++) {
		if (*parts != nullptr) {
			(*parts)->unk18->frameUpdate();
		}
		parts++;
	}
}

void TNpcParts::partsPerform(u32 flag, JDrama::TGraphics* graphics)
{
	u32 doTexMtx         = flag & 2;
	TSharedParts** parts = &unk0[unk60->unkD0->unk8][0];

	for (int i = 0; i < 12; i++) {
		if (parts[i] == nullptr)
			continue;

		if (unk60->getActorType() == 0x04000018) {
			bool show = true;
			u8 state  = unk60->unk1D8;
			if (state & 0x4) {
				switch (i) {
				case 1:
				case 2:
				case 4:
					show = false;
					break;
				}
			} else if (state & 0x1) {
				switch (i) {
				case 1:
				case 2:
					show = false;
					break;
				}
			} else {
				switch (i) {
				case 4:
				case 5:
				case 6:
					show = false;
					break;
				}
			}
			if (!show)
				continue;
		}

		if (doTexMtx) {
			if (unk60->isJellyFishMare() && i == 11) {
				Mtx effectMtx;
				MActor* mactor = parts[i]->unk18;
				SMS_GetLightPerspectiveForEffectMtx(effectMtx);
				J3DModelData* mdata = mactor->getModel()
				    ->getModelData();
				int starglowIdx
				    = mdata->getMaterialName()->getIndex("_starglow1");
				u16 numTexMtx = mdata->getMaterialNum();
				for (u16 j = 0; j < numTexMtx; j++) {
					if ((int)j == starglowIdx)
						continue;
					setEffectMtxOnTex0(
					    mdata->getMaterialNodePointer(j),
					    effectMtx);
				}
			}
		}

		parts[i]->perform(flag, graphics);
	}
}
