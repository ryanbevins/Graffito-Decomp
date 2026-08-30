#include <MSound/MSound.hpp>
#include <MSound/MSRandVol.hpp>
#include <MSound/MSHandle.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSModBgm.hpp>
#include <System/MSoundMainSide.hpp>
#include <JSystem/JAudio/JADebug/JADHioNode.hpp>
#include <JSystem/JAudio/JAInterface/JAIDebug.hpp>
#include <JSystem/JAudio/JAInterface/JAIGlobalParameter.hpp>
#include <JSystem/JAudio/JAInterface/JAIAsnData.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>
#include <JSystem/JAudio/JASystem/JASSystemHeap.hpp>
#include <JSystem/JAudio/JASystem/JASWaveBankMgr.hpp>
#include <JSystem/JAudio/JASystem/JASWaveBank.hpp>
#include <JSystem/JAudio/JASystem/JASBasicWaveBank.hpp>
#include <JSystem/JAudio/JASystem/JASSimpleWaveBank.hpp>
#include <JSystem/JAudio/JASystem/JASAudioThread.hpp>
#include <JSystem/JAudio/JASystem/JASDriverIF.hpp>
#include <JSystem/JAudio/JASystem/JASDvdThread.hpp>
#include <JSystem/JAudio/JALibrary/JALSystem.hpp>
#include <math.h>

MSound* MSGMSound  = 0;
JAIBasic* MSGBasic = 0;

template <class T> static inline T min(T a, T b) { return a < b ? a : b; }
template <class T> static inline T max(T a, T b) { return a < b ? b : a; }

u16 MSSeCallBack::smTrackCategory[32]
    = { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
	    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF };
u8 MSSeCallBack::smPolifonic[16] = { 0 };
u16 MSSeCallBack::smWaterFilter;

bool MSLoadWave::loadWaveBackword(int param_1, int param_2)
{
	JASystem::TWaveBank* bank = JASystem::WaveBankMgr::getWaveBank(param_1);
	if (!bank)
		return false;

	if (bank->getType() == 'BSIC') {
		JASystem::TBasicWaveBank::TWaveGroup* grp
		    = ((JASystem::TBasicWaveBank*)bank)->getWaveGroup(param_2);

		if (!grp)
			return false;

		if (!loadWaveBackword(grp))
			return false;

		((JASystem::TBasicWaveBank*)bank)->incWaveTable(grp);
		return true;
	} else if (bank->getType() == 'SMPL') {
		JASystem::TSimpleWaveBank* obj = (JASystem::TSimpleWaveBank*)bank;
		return loadWaveBackword(obj);
	} else {
		return false;
	}
}

bool MSLoadWave::loadWaveBackword(JASystem::WaveArcLoader::TObject* obj)
{
	JASystem::Kernel::THeap* heap = obj->getHeap();
	if (!heap)
		return false;

	if (heap->unk8 != 0)
		return false;

	char filePath[256];
	strcpy(filePath, JASystem::WaveArcLoader::getCurrentDir());
	strcat(filePath, obj->getWaveArcFileName());
	u32 extent = JASystem::Dvd::checkFileExtend(filePath);
	if (!extent)
		return false;

	JASystem::Kernel::THeap* root = JASystem::WaveArcLoader::getRootHeap();
	if (!heap->selfAlloc(root, extent, (u32)root->unk8 + root->unk10 - extent))
		return false;

	u32* ptr = obj->getLoadFlagPtr();
	*ptr     = 0;
	if (JASystem::Dvd::loadToAramDvdT(0, filePath, heap->unk8, 0, extent, ptr,
	                                  nullptr)
	    == -1) {
		heap->free();
		return false;
	}

	return true;
}

void MSSeCallBack::setWaterCameraFir(bool enabled)
{
	if (enabled)
		smWaterFilter = 0x78;
	else
		smWaterFilter = 0;
}

u16 MSSeCallBack::setParameterSeqSync(JASystem::TTrack* track, u16 param)
{
	switch (param) {
	case 0: {
		u16 result = JAIBasic::setParameterSeqSync(track, param);
		track->unk3C1 = 0x4A;
		if (track->unk2C0->unk2C4[15] == track)
			track->unk3C2 = 1;
		return result;
	}
	case 0xC: {
		u16 parentValue;
		u16 targetValue;
		u16 step;
		u16 current;

		track->unk2C0->readPortAppDirect(8, &parentValue);
		track->readPortAppDirect(0xF, &targetValue);

		u16 limit = parentValue > smWaterFilter ? parentValue : smWaterFilter;
		if (limit != targetValue) {
			track->readPortAppDirect(0xE, &current);
			if (current > limit)
				step = 0xFFEC;
			else
				step = 20;

			track->writePortAppDirect(0xD, step);
			track->writePortAppDirect(0xF, limit);
			targetValue = limit;
		} else {
			track->readPortAppDirect(0xD, &step);
		}

		if (step != 0) {
			track->readPortAppDirect(0xE, &current);
			current += step;
			if (current > 0x7FFF || current == 0) {
				step    = 0;
				current = 0;
				track->writePortAppDirect(0xD, step);
			} else if (targetValue != 0 && current > targetValue) {
				current = targetValue;
				step    = 0;
				track->writePortAppDirect(0xD, step);
			}

			track->writePortAppDirect(0xE, current);
			return 0x7F - current;
		}

		return 0xFF;
	}
	case 0xD: {
		u16 parentValue;
		track->unk2C0->readPortAppDirect(8, &parentValue);
		u16 value = parentValue > smWaterFilter ? parentValue : smWaterFilter;
		track->writePortAppDirect(0xE, value);
		track->writePortAppDirect(0xF, value);
		return 0x7F - value;
	}
	case 0xF:
		return MSGMSound->unk94;
	case 0x14: {
		for (u16 i = 0; i < 2; ++i) {
			JASystem::TTrack* parent = track->unk2C4[i];
			for (u16 j = 0; j < 16; ++j) {
				JASystem::TTrack* child = parent->unk2C4[j];
				if (child == nullptr)
					break;

				u16* category = &smTrackCategory[i * 16 + j];
				child->readPortAppDirect(9, category);
				++smPolifonic[*category];
			}
		}

		for (u16 i = 0; i < JAIGlobalParameter::getParamSeCategoryMax(); ++i) {
		}

		return 0;
	}
	case 0x1E: {
		u16 result = JAIBasic::setParameterSeqSync(track, 0);
		track->setPanSwitchParent(1, 0);
		track->setPanSwitchParent(1, 1);
		track->setPanSwitchParent(1, 2);
		track->writeRegDirect(8, 0);
		track->writeRegDirect(0xA, 0);
		track->writeRegDirect(0xB, 0);
		track->writeRegDirect(0xC, 0x7FFF);

		JASystem::TTrack* parent = track->unk2C0;
		if (parent != nullptr) {
			parent->setPanSwitchExt(1, 0);
			parent->setPanSwitchExt(1, 1);
			parent->setPanSwitchExt(1, 2);
			parent->writeRegDirect(8, 0);
			parent->writeRegDirect(0xA, 0);
			parent->writeRegDirect(0xB, 0x7FFF);
			parent->writeRegDirect(0xC, 0);

			if (parent->mOuterParam != nullptr) {
				parent->mOuterParam->onSwitch(8);
				parent->mOuterParam->onSwitch(4);
				parent->mOuterParam->onSwitch(0x10);
			}
		}

		*(u16*)((u8*)track + 0x13A) = 0xFFFF;
		return result;
	}
	case 0x28:
		MSGMSound->unkD1 = 1;
		return 0;
	case 0x6E:
		u8 area = MSGMSound->unkCD;
		u8 episode = MSGMSound->unkCE;
		if (area == 8 && episode == 6)
			return 0xFFFF;
		return area;
	case 0x78:
		static bool ukuleleFlag = 0;
		ukuleleFlag ^= true;
		return ukuleleFlag;
	case 0x79: {
		return ukuleleFlag;
	}
	default:
		return JAIBasic::setParameterSeqSync(track, param);
	}
}

JAISound* MSound::makeSound(u32 count)
{
	if (unkC != 0)
		return new (unkC, 0) MSHandle[count];
	else
		return new (JASDram, 0) MSHandle[count];
}

void MSound::setRegisterTrackCallback()
{
	JASystem::TrackMgr::registerTrackCallback(
	    &MSSeCallBack::setParameterSeqSync);
}

void MSound::loadGroupWave(s32 param_1, s32 param_2)
{
	if (param_1 == 2 && param_2 != 0x10) {
		MSLoadWave::loadWaveBackword(param_1, param_2);
		setSceneSetFinishCallback(param_1, param_2);
		unk60[param_1] = param_2;
	} else if (param_1 == 2 && param_2 == 0x10) {
		JASystem::WaveBankMgr::loadWave(param_1, param_2);
	} else {
		JASystem::WaveBankMgr::loadWave(param_1, param_2);
		setSceneSetFinishCallback(param_1, param_2);
		unk60[param_1] = param_2;
	}
}

void MSound::loadWave(MS_SCENE_WAVE wave)
{
	if (wave == -1)
		return;

	u8 hi = wave >> 8;
	u8 lo = wave & 0xff;
	if (wave == 0x210)
		JASystem::WaveBankMgr::loadWave(hi, lo);
	else
		loadSceneWave(hi, lo);
}

void MSound::enterStage(MS_SCENE_WAVE wave, u8 param2, u8 param3)
{
	unkCD = param2;
	unkCE = param3;

	if (wave == -1)
		return;

	loadWave(wave);
}

void MSound::exitStage()
{
	for (u8 i = 0; i < JAIGlobalParameter::getParamSeCategoryMax(); ++i)
		if (unk0->unk88.unk2[i])
			stopAllSe(i);

	MSBgm::stopTrackBGMs(7, 0);
	if (unkC4)
		unkC4->stop(0);

	unk8[0] = JAInullCamera;

	unkAC[0] = JAICamera();
	unkAC[0] = JAInullCamera;
	unkAC[1] = JAICamera();
	unkAC[1] = JAInullCamera;

	unkCD = 0xff;
	unkCE = 0xff;
	unkC8[0] = 0;
}

bool MSound::checkWaveOnAram(MS_SCENE_WAVE wave)
{
	u8 hi = wave >> 8;
	u8 lo = wave & 0xff;
	if (wave == 0x210) {
		JASystem::TWaveBank* bank = JASystem::WaveBankMgr::getWaveBank(hi);
		if (!bank)
			return false;

		if (bank->getType() == 'BSIC') {
			JASystem::TBasicWaveBank::TWaveGroup* grp
			    = ((JASystem::TBasicWaveBank*)bank)->getWaveGroup(lo);

			if (!grp)
				return false;

			if (*grp->getLoadFlagPtr())
				return true;
			else
				return false;
		} else if (bank->getType() == 'SMPL') {
			JASystem::TSimpleWaveBank* obj = (JASystem::TSimpleWaveBank*)bank;
			if (*obj->getLoadFlagPtr())
				return true;
			else
				return false;
		} else {
			return false;
		}
	} else {
		return checkSceneWaveOnMemory(hi, lo);
	}
}

void MSound::setCameraInfo(Vec* param_1, Vec* param_2, MtxPtr param_3,
                           u32 param_4)
{
	if (param_1 == nullptr) {
		unk8[param_4] = JAInullCamera;
	} else {
		JAIBasic::setCameraInfo(param_1, param_2, param_3, param_4);
	}
}

void MSound::setPlayerInfo(Vec* param_1, Vec* param_2, MtxPtr param_3,
                           bool param_4)
{
	u8 i = param_4 == 1 ? 0 : 1;
	if (param_1 == 0) {
		unkAC[i] = JAICamera();
		unkAC[i] = JAInullCamera;
	} else {
		unkAC[i].unk0 = param_1;
		unkAC[i].unk4 = param_2;
		unkAC[i].unk8 = param_3;
		MSoundSESystem::MSRandPlay::createRandPlayVec(0x7865, 1);
		MSoundSESystem::MSRandPlay::registerTrans(0x7865, param_1);
		MSoundSESystem::MSRandPlay::createRandPlayVec(0x7094, 1);
		MSoundSESystem::MSRandPlay::registerTrans(0x7094, param_1);
		MSoundSESystem::MSRandPlay::createRandPlayVec(0x1950, 1);
		MSoundSESystem::MSRandPlay::registerTrans(0x1950, param_1);
	}
}

f32 MSound::getDistFromCamera(Vec* pos)
{
	return JALCalc::getDist(pos, unk8->unk0);
}

MSound::MSound(JKRHeap* param_1, JKRHeap* param_2, u32 param_3, u8* param_4,
               u8* param_5, u32 param_6)
{
	JKRSolidHeap* heap = JKRSolidHeap::create(0x151800, param_1, false);
	if (param_2 != nullptr) {
		JAInter::TDebugHeap::currentHeap = param_2;
		JADHioNode::smCurrentHeap        = param_2;
	}
	JAIGlobalParameter::setParamInitDataFileName("/AudioRes/mSound.aaf");
	JAInter::TAsnData::asnFileName = "/AudioRes/mSound.asn";
	JAIGlobalParameter::setParamWavePath("/AudioRes/Banks/");
	JAIGlobalParameter::setParamSequenceArchivesPath("/AudioRes/Seqs/");
	JAIGlobalParameter::setParamStreamPath("/AudioRes/Streams/");
	JAIGlobalParameter::setParamSystemTrackMax(0xb9);
	JAIGlobalParameter::setParamSeqPlayTrackMax(4);
	JAIGlobalParameter::setParamSeqControlBufferMax(8);
	JAIGlobalParameter::setParamSystemRootTrackMax(8);
	JAIGlobalParameter::setParamStayHeapSize(0xf000);
	JAIGlobalParameter::setParamAutoHeapMax(3);
	JAIGlobalParameter::setParamAutoHeapRoomSize(0xa2ff);
	JAIGlobalParameter::setParamStayHeapMax(1);
	JAIGlobalParameter::setParamStreamInsideBufferCut(true);
	JAIGlobalParameter::setParamInputGainDown(0.802);
	JAIGlobalParameter::setParamOutputGainUp(5.0);
	setInitFileLoadSwitch(2);

	if (param_4 != nullptr)
		JAIGlobalParameter::setParamInitDataPointer(param_4);
	if (param_5 != nullptr)
		JAInter::TAsnData::asnData = param_5;

	MSSeCallBack::smWaterFilter = nullptr;
	initDriver(heap, param_3, 1);
	initInterface(1);
	f32 fVar1 = 0.0f;
	for (u8 i = 0; i < 16; ++i) {
		if (unk0->unk88.unk2[i] != 0) {
			fVar1 = max<f32>(fVar1, MSHandle::smSeCategory[i].unk4);
			setSeCategoryVolume(
			    i, min<u8>(MSHandle::smSeCategory[i].unk8 * 127.0f, 127));
		}
	}

	JAIGlobalParameter::setParamDistanceMax(fVar1);
	JAIGlobalParameter::setParamMinDistanceVolume(0.0);
	JAIGlobalParameter::setParamMaxVolumeDistance(1200.0);
	unkA8     = 3;
	MSGBasic  = JAIBasic::basic;
	MSGMSound = this;
	JALSystem::init();
	MSoundSESystem::MSoundSE::construct();
	MSBgm::init();
	unk88  = 0;
	unk98  = (MSModBgm*)new u64; // TODO: wrong type & size
	f32* p = new f32;            // TODO: wrong type & size
	if (p)
		*p = 0.0f;
	unk9C = (MSBgmXFade*)p;

	unk7C = 0;
	unk80 = 0;

	unkC8[0] = 0;
	unkC8[1] = 0;
	unkC8[2] = 0;
	unkC8[3] = 0;
	unkC8[4] = 0;

	unkAC[0] = JAInullCamera;
	unkAC[1] = JAInullCamera;

	unk84 = 0;
	unk94 = 0;
	unk8C[0] = 0;
	unk8C[1] = 0;
	unkC4 = 0;

	unkCF = 1;
	unkD0 = 1;
	unkCD = 0xFF;
	unkCE = 0xFF;

	unkA0 = 0;
	unkA4 = 0;
}

void MSound::requestShineAppearFanfare() { }

void MSound::mainLoop()
{
	if (unkCF == 0 && unkA8 == 0)
		return;

	if (unkD1 == 1) {
		MSBgm::startBGM(0x8001002D);
		unkD1 = 0;
	}

	if (unkC8[1] != 0) {
		MSMainProc::entranceDemoLoop(unkA4);
		++unkA4;
	}

	for (JSUListIterator<MSSetSound> it
	     = JALList<MSSetSound>::smList.getFirst();
	     it != JALList<MSSetSound>::smList.getEnd(); ++it)
		it.getObject()->frameLoopDyna();

	for (JSUListIterator<MSSetSoundGrp> it
	     = JALList<MSSetSoundGrp>::smList.getFirst();
	     it != JALList<MSSetSoundGrp>::smList.getEnd(); ++it)
		it.getObject()->frameLoopDyna();

	startFrameInterfaceWork();
	unk98->loop();
}

void MSound::startSoundSet(u32 id, const Vec* pos, u32 param3, f32 volume,
                           u32 param5, u32 param6, u8 param7)
{
	if (gateCheck(id))
		MSSetSound::startSoundSet(id, pos, param3, volume, param5, param6,
		                           param7);
}

void MSound::startSoundSetGrp(u32 id, const Vec* pos, u32 param3, f32 volume,
                              u32 param5, u32 param6, u8 param7)
{
	if (gateCheck(id))
		MSSetSoundGrp::startSoundSetGrp(id, pos, param3, volume, param5,
		                                 param6, param7);
}

void MSound::initSound()
{
	unkA8 |= 2;

	for (u8 i = 0; i < 16; ++i) {
		if (MSGMSound->unk0->unk88.unk2[i] && JAIBasic::basic != nullptr) {
			JAIBasic::basic->setSeCategoryVolume(
			    i, min<u8>(MSHandle::smSeCategory[i].unk8 * 127.0f, 127));
		}
	}

	if (unk7C != nullptr) {
		unk7C->stop(1);
		unk7C = nullptr;
	}

	if (unk80 != nullptr) {
		unk80->stop(1);
		unk80 = nullptr;
	}
}

void MSound::pauseOn(bool param)
{
	if (param) {
		bool canPlay;
		if (!(unkA8 & 2))
			canPlay = false;
		else
			canPlay = true;

		if (canPlay) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4802, 0,
			                                             nullptr, 0);
		}
	}

	u8 i;
	for (i = 0; i < 16; ++i) {
		if (i != 4 && MSGMSound->unk0->unk88.unk2[i])
			MSGMSound->setSeCategoryVolume(i, 0);
	}

	u8 mask = 7;
	if (param) {
		for (i = 0; i < 3; ++i) {
			if ((mask >> i) & 1)
				MSBgm::setTrackVolume(i, 0.0f, 60, 3);
		}
	} else {
		for (i = 0; i < 3; ++i) {
			if ((mask >> i) & 1)
				MSBgm::setTrackVolume(i, 0.0f, 0, 3);
		}
	}
}

void MSound::pauseOff(u8 param)
{
	switch (param) {
	case 0:
	{
		bool canPlay;
		if (!(unkA8 & 2))
			canPlay = false;
		else
			canPlay = true;

		if (canPlay) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4803, 0,
			                                             nullptr, 0);
		}
	}
	case 2:
		for (u8 i = 0; i < 16; ++i) {
			if (i != 4 && MSGMSound->unk0->unk88.unk2[i]
			    && JAIBasic::basic != nullptr) {
				JAIBasic::basic->setSeCategoryVolume(
				    i,
				    min<u8>(MSHandle::smSeCategory[i].unk8 * 127.0f, 127));
			}
		}

		for (u8 i = 0; i < 3; ++i) {
			if ((7 >> i) & 1)
				MSBgm::setTrackVolume(i, 1.0f, 10, 3);
		}
		break;
	case 1:
	{
		bool canPlay;
		if (!(unkA8 & 2))
			canPlay = false;
		else
			canPlay = true;

		if (canPlay) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x481B, 0,
			                                             nullptr, 0);
		}

		for (u8 i = 0; i < 16; ++i) {
			if (MSGMSound->unk0->unk88.unk2[i])
				MSGMSound->setSeCategoryVolume(i, 0);
		}

		for (u8 i = 0; i < 3; ++i) {
			if ((7 >> i) & 1)
				MSBgm::setTrackVolume(i, 0.0f, 15, 3);
		}
		break;
	}
	}
}

void MSound::demoModeIn(u16 param1, bool param2)
{
	u16 mask = param1;
	for (u8 i = 0; i < 16; ++i) {
		if (((mask >> i) & 1) && MSGMSound->unk0->unk88.unk2[i])
			MSGMSound->setSeCategoryVolume(i, 0);
	}

	if (param2) {
		MSBgm::setAllTracksVolume(0.0f, 15);
	}
}

void MSound::demoModeOut(bool param)
{
	for (u8 i = 0; i < 16; ++i) {
		if (MSGMSound->unk0->unk88.unk2[i] && JAIBasic::basic != nullptr) {
			JAIBasic::basic->setSeCategoryVolume(
			    i, min<u8>(MSHandle::smSeCategory[i].unk8 * 127.0f, 127));
		}
	}

	if (param) {
		u8 mask = 7;
		for (u8 i = 0; i < 3; ++i) {
			if ((mask >> i) & 1)
				MSBgm::setTrackVolume(i, 1.0f, 15, 3);
		}
	}
}

void MSound::talkModeIn(bool param)
{
	if (param) {
		bool canPlay;
		if (!(unkA8 & 2))
			canPlay = false;
		else
			canPlay = true;

		if (canPlay) {
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4804, 0, nullptr,
			                                             0);
		}
	}

	u8 mask = 0x44;
	for (u8 i = 0; i < 16; ++i) {
		if (MSGMSound->unk0->unk88.unk2[i] && ((mask >> i) & 1))
			MSGMSound->setSeCategoryVolume(i, 0);
	}

	mask = 7;
	for (u8 i = 0; i < 3; ++i) {
		if ((mask >> i) & 1)
			MSBgm::setTrackVolume(i, 0.6f, 30, 3);
	}
}

void MSound::talkModeOut()
{
	bool canPlay;
	if (!(unkA8 & 2))
		canPlay = false;
	else
		canPlay = true;

	if (canPlay) {
		MSoundSESystem::MSoundSE::startSoundSystemSE(0x4805, 0, nullptr, 0);
	}

	for (u8 i = 0; i < 16; ++i) {
		if (MSGMSound->unk0->unk88.unk2[i] && ((0x1FF >> i) & 1)
		    && JAIBasic::basic != nullptr) {
			JAIBasic::basic->setSeCategoryVolume(
			    i, min<u8>(MSHandle::smSeCategory[i].unk8 * 127.0f, 127));
		}
	}

	u8 mask = 7;
	for (u8 i = 0; i < 3; ++i) {
		if ((mask >> i) & 1)
			MSBgm::setTrackVolume(i, 1.0f, 15, 3);
	}
}

void MSound::setCategoryVOLs(u16 param1, f32 volume)
{
	u8 rawVolume = 127.0f * volume;
	u8 categoryVolume;
	if (rawVolume > 127)
		categoryVolume = 127;
	else
		categoryVolume = rawVolume;

	for (u8 i = 0; i < 16; ++i) {
		if (MSGMSound->unk0->unk88.unk2[i] && ((param1 >> i) & 1))
			MSGMSound->setSeCategoryVolume(i, categoryVolume);
	}
}

bool MSound::resetAudioAll(u16 param)
{
	if (unkD0 == 0)
		return true;

	f32 gain = JAIGlobalParameter::getParamOutputGainUp();
	if (gain <= 0.002f) {
		JASystem::Driver::setMixerLevel(0.802f, 0.0f);
		JASystem::AudioThread::stop();
		unkA8 = 0;
		unkD0 = 0;
		return true;
	}

	gain *= powf(0.0002f, 1.0f / (f32)param);
	JASystem::Driver::setMixerLevel(0.802f, gain);
	JAIGlobalParameter::setParamOutputGainUp(gain);
	return false;
}

void MSound::stopAllSeInCategory(u8 category, u32 param2) { }

void MSound::setCategoryAllVolume(u8 category, f32 volume, u32 param3,
                                  u8 param4)
{
}

void MSound::fadeOutAllSound(u32 fadeTime)
{
	u8 category = 0;
	unkA8 &= 1;

	for (; category < JAIGlobalParameter::getParamSeCategoryMax(); ++category) {
		if (unk0->unk88.unk2[category] && category != 4) {
			JAISound* sound = unk0->unk1E8[category].unk4;
			while (sound != nullptr) {
				sound->setVolume(0.0f, fadeTime, 2);
				sound = sound->unk30;
			}
		}
	}

	u8 mask = 7;
	category = 0;
	for (; category < 3; ++category) {
		if ((mask >> category) & 1)
			MSBgm::setTrackVolume(category, 0.0f, fadeTime, 3);
	}

	if (unkC4)
		unkC4->stop(fadeTime);
}

void MSound::stopAllSound()
{
	for (u8 i = 0; i < JAIGlobalParameter::getParamSeCategoryMax(); ++i)
		if (unk0->unk88.unk2[i])
			stopAllSe(i);

	MSBgm::stopTrackBGMs(7, 0);
	if (unkC4)
		unkC4->stop(0);
}

#pragma dont_inline on
f32 MSoundSESystem::MSRandVol::getRandomVolumeNormal(u32 param)
{
	JSULink<MSoundSESystem::MSRandVol>* link = smList.getFirst();
	if (link == nullptr)
		return 1.0f;

	return link->getObject()->getRandVol(param);
}
#pragma dont_inline off

void MSound::setSeExtParameter(JAISound* sound)
{
	if (sound != nullptr) {
		void* soundInfo;
		u32 soundID           = sound->unk8;
		JAISoundTable* table  = getInfoPointerFromID(soundID);
		soundInfo             = sound->unk3C;
		getInfoFormat(table, soundID);

		u32 flags = *(u32*)soundInfo;
		f32 volume;
		if (flags & 0x00C00000)
			volume = MSoundSESystem::MSRandVol::getRandomVolumeNormal(flags);
		else
			volume = 1.0f;

		f32 capped = 1.0f;
		f32 scaled = volume * (((u8*)soundInfo)[0xC] / 127.0f);
		capped = scaled > capped ? capped : scaled;

		sound->setVolume(capped, 0, 1);
		sound->setPitch(*(f32*)((u8*)soundInfo + 8), 0, 1);
	}
}

void MSound::playTimer(u32 time)
{
	bool canPlay;
	if (!(unkA8 & 1))
		canPlay = false;
	else
		canPlay = true;

	if (canPlay) {
		MSoundSESystem::MSoundSE::startSoundActorInner(
		    0x403A, nullptr, (JAIActor*)-1, 0, 4);

		if (time > 30000) {
			unk94 = 110;
		} else if (time > 15000) {
			unk94 = 50;
		} else if (time > 10000) {
			unk94 = 35;
		} else if (time > 5000) {
			unk94 = 25;
		} else if (time > 2000) {
			unk94 = 10;
		} else if (time > 1000) {
			unk94 = 3;
		} else {
			unk94 = 0;
		}
	}
}

u32 MSound::startMarioVoice(u32 id, s16 param2, u8 param3)
{
	if (((param3 & 1) ? true : false) == 1)
		return 0;

	bool slot        = param3 & 2 ? true : false;
	u32 actorIndex   = param3 & 2 ? true : false;
	bool shouldStart = true;

	if (param3 == 6) {
		switch (id) {
		case 0x78AB:
		case 0x78B1:
		case 0x78B6:
		case 0x78B9:
		case 0x78BF:
			JAIActor actor(unkAC[actorIndex].unk0, unkAC[actorIndex].unk0,
			               unkAC[actorIndex].unk0, 0);
			MSoundSESystem::MSoundSE::startSoundActorInner(
			    0x898C, unk8C + slot, &actor, 1, 4);
			return 0x898C;
		}

		return -1;
	}

	if (unk8C[slot] != nullptr) {
		switch (id) {
		case 0x78CF:
		case 0x78A8:
		case 0x7865:
		case 0x7852:
		case 0x78C7:
		case 0x78D3:
		case 0x78D9:
		case 0x78E0:
			shouldStart = false;
			break;
		}

		if (!shouldStart) {
			if (unk8C[0])
				return unk8C[0]->unk8;
			return -1;
		}
	}

	switch (id) {
	case 0xFFFF0003:
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.5f)
			id = 0x7806;
		if (id + 0x10000 == 3)
			id = 0x7817;
		break;
	case 0x7094:
	case 0x7849:
	case 0x7844:
	case 0x784F:
	case 0x786B:
	case 0x789E:
	case 0x78C1:
	case 0x78A3:
	case 0x78C4:
		break;
	case 0x7830:
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.25f)
			id = 0x7818;
		break;
	case 0x7833:
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.25f)
			id = 0x781C;
		break;
	case 0x783B:
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.25f)
			id = 0x781C;
		break;
	case 0x7852:
		if (param2 <= 2)
			id = 0x78EE;
		break;
	case 0x785D:
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.15f)
			id = 0x781B;
		break;
	case 0x7865:
		if (unkA8 & 2)
			MSoundSESystem::MSRandPlay::startSeRandPlay(0x7865, 0);
		if (unk8C[0] != nullptr)
			return unk8C[0]->unk8;
		return -1;
		break;
	case 0x7881:
		if (param2 <= 2)
			id = 0x78EE;
		break;
	case 0x7884:
		if (param2 <= 2)
			id = 0x78FB;
		break;
	case 0x788F:
		if (param2 <= 2)
			id = 0x78FE;
		break;
	case 0x7899:
		if (param2 <= 2)
			id = 0x78FB;
		break;
	case 0x78AB:
		if (param2 <= 2) {
			id = 0x7901;
			break;
		}
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.3f)
			id = 0x7807;
		break;
	case 0x78B1:
		if (param2 <= 2) {
			id = 0x7901;
			break;
		}
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.7f)
			id = 0x7803;
		break;
	case 0x78B6:
		if (param2 <= 2) {
			id = 0x7901;
			break;
		}
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.5f)
			id = 0x7800;
		break;
	case 0x78B9:
		if (param2 <= 2) {
			id = 0x7901;
			break;
		}
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.5f)
			id = 0x780A;
		break;
	case 0x78BF:
		if (param2 <= 2)
			id = 0x7906;
		break;
	case 0x78C7:
		if (param2 <= 2) {
			id = 0x78EE;
			break;
		}
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.5f)
			id = 0x780E;
		if (param3 != 2 && JALCalc::getRandom_0_1() < 0.5f)
			id = 0x7813;
		break;
	case 0x78E5:
		if (param2 <= 2)
			id = 0x790B;
		break;
	case -2:
		if (param2 <= 2)
			id = 0x790E;
		else
			id = 0x78E5;
		break;
	}

	JAIActor actor(unkAC[actorIndex].unk0, unkAC[actorIndex].unk0,
	               unkAC[actorIndex].unk0, 0);
	MSoundSESystem::MSoundSE::startSoundActorInner(id, unk8C + slot, &actor, 1,
	                                               4);

	if (unk8C[slot] != nullptr) {
		if (param3 == 2) {
			unk8C[slot]->setPortData(11, 1);
			unk8C[slot]->setPitch(1.2f, 0, 0);
			unk8C[slot]->setVolume(0.9f, 0, 0);
		} else {
			unk8C[slot]->setPortData(11, 0);
		}
	}

	u8 finalSlot = param3 & 2 ? 1 : 0;
	if (unk8C[finalSlot] != nullptr)
		return unk8C[finalSlot]->unk8;
	return -1;
}

u32 MSound::getMarioVoiceID(u8 param)
{
	u8 index;
	if (param & 2)
		index = 1;
	else
		index = 0;

	JAISound* sound = unk8C[index];
	if (sound != nullptr)
		return sound->unk8;

	return -1;
}

void MSound::stopMarioVoice(u32 id, u8 param2)
{
	u8 index;
	if (param2 & 2)
		index = 1;
	else
		index = 0;

	if (JAISound* sound = unk8C[index]) {
		if (id != -1) {
			if (id == sound->unk8)
				sound->stop(1);
		} else {
			sound->stop(1);
		}
	}
}

void* MSound::checkMarioVoicePlaying(u8 param)
{
	u8 index;
	if (param & 2)
		index = 1;
	else
		index = 0;

	return unk8C[index];
}

u32 MSound::getWallSound(u32 param1, f32 param2)
{
	if (param2 < 15.0f) {
		if (param1 == 0x1C)
			return 0x194B;
		else
			return 0x194A;
	}

	if (param2 < 30.0f) {
		if (param1 == 0x1C)
			return 0x1802;
		else
			return 0x1949;
	}

	if (param1 == 0x1C)
		return 0x1803;
	else
		return 0x1948;
}

void MSound::startBeeSe(Vec* pos, u32 id)
{
	if (id > 3) {
		JAISound* sound
		    = !gateCheck(0x2106)
		          ? nullptr
		          : MSoundSESystem::MSoundSE::startSoundActor(
		                0x2106, pos, 0, nullptr, 0, 4);

		if (sound != nullptr) {
			f32 volume = JALCalc::linearTransform((f32)id, 3.0f, 50.0f,
			                                      0.0f, 1.0f, false);
			sound->setVolume(volume, 0, 0);
		}
	}

	if (id > 2) {
		if (gateCheck(0x2107))
			MSoundSESystem::MSoundSE::startSoundActor(0x2107, pos, 0,
			                                          nullptr, 0, 4);
	} else if (id == 2) {
		if (gateCheck(0x2108))
			MSoundSESystem::MSoundSE::startSoundActor(0x2108, pos, 0,
			                                          nullptr, 0, 4);
	} else if (id == 1) {
		if (gateCheck(0x2109))
			MSoundSESystem::MSoundSE::startSoundActor(0x2109, pos, 0,
			                                          nullptr, 0, 4);
	}
}

void MSound::startSoundActorSpecial(u32 id, const Vec* pos, f32 param3,
                                    f32 param4, u32 param5,
                                    JAISound** soundPtr, u32 param7, u8 param8)
{
	if (gateCheck(id) && !JALSystem::gateCheckFunc(id, param3)
	    && !JALSystem::gateCheckFunc(id, param4)) {
		JAIActor actor(pos, pos, pos, param5);
		JAISound* sound = MSoundSESystem::MSoundSE::startSoundActorInner(
		    id, soundPtr, &actor, param7, param8);
		if (sound != nullptr) {
			switch (id) {
			case 0x212F: {
				f32 volume = 1.0f;
				f32 pitch  = 1.0f;
				if (JALSeModData<JALSeModVolFunk>::calc(id, param3, &volume))
					sound->setVolume(volume, 0, 0);
				if (JALSeModDataGrp<JALSeModPitFGrp>::calcGrp(id, param4,
				                                              &pitch))
					sound->setPitch(pitch, 0, 0);
				break;
			}
			}
		}
	}
}

bool MSound::cameraLooksAtMario()
{
	for (u8 i = 0; i < 5; ++i) {
		if (i == 0) {
			if (unkC8[i] == 0)
				return false;
		} else if (unkC8[i] == 1) {
			return false;
		}
	}

	return true;
}

bool MSound::gateCheck(u32 param)
{
	u8 flag = unkA8;
	if (!(flag & 1)) {
		u8 category = (param >> 11) & 1;
		category |= (param >> 24) & 0xC0;
		if (category == 0)
			return false;
	}

	if (!(flag & 2)) {
		u8 category = (param >> 11) & 1;
		category |= (param >> 24) & 0xC0;
		if (category == 1)
			return false;
	}

	return true;
}

u32 MSound::getBstSwitch(u32 param)
{
	JAISoundInfo* info = MSGBasic->getSoundInfoFromID(param);
	if (!info)
		return -1;

	return info->unk0;
}

u32 MSound::getSwitch(u32 param1, u32 param2, u32 param3)
{
	u32 bstSwitch = getBstSwitch(param1);
	return (bstSwitch & param2) >> param3;
}

u32 MSound::getBstPitch(u32 param) { return 0; }
