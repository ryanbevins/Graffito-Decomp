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

u8 MSSeCallBack::smTrackCategory;
u8 MSSeCallBack::smPolifonic;
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

u16 MSSeCallBack::setParameterSeqSync(JASystem::TTrack* track, u16 param) { }

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
	unkC8 = 0;
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
			if (fVar1 < MSHandle::smSeCategory[i].unk4)
				fVar1 = MSHandle::smSeCategory[i].unk4;
			u8 uVar2 = (u8)(MSHandle::smSeCategory[i].unk4 * 127.0f);
			if ((u8)(MSHandle::smSeCategory[i].unk4 * 127.0f) > 0x7E)
				uVar2 = 0x7F;
			setSeCategoryVolume(i, uVar2);
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
	unk98  = new u64; // TODO: wrong type & size
	f32* p = new f32; // TODO: wrong type & size
	if (p)
		*p = 0.0f;
	unk9C = p;

	unk7C = 0;
	unk80 = 0;

	unkC8 = 0;
	unkC9 = 0;
	unkCA = 0;
	unkCB = 0;
	unkCC = 0;

	unkAC[0] = JAInullCamera;
	unkAC[1] = JAInullCamera;

	unk84 = 0;
	unk94 = 0;
	unk8C = 0;
	unk90 = 0;
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

	if (unkC9 != 0) {
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
	((MSModBgm*)unk98)->loop();
}

JAISound* MSound::startSoundSet(u32 id, const Vec* pos, u32 param3, f32 volume,
                                u32 param5, u32 param6, u8 param7)
{
	return nullptr;
}

JAISound* MSound::startSoundSetGrp(u32 id, const Vec* pos, u32 param3,
                                   f32 volume, u32 param5, u32 param6,
                                   u8 param7)
{
	return nullptr;
}

void MSound::initSound() { }

void MSound::pauseOn(bool param) { }

void MSound::pauseOff(u8 param) { }

void MSound::demoModeIn(u16 param1, bool param2) { }

void MSound::demoModeOut(bool param) { }

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

void MSound::talkModeOut() { }

void MSound::setCategoryVOLs(u16 param1, f32 volume)
{
	s32 rawVolume = 127.0f * volume;
	s32 categoryVolume;
	if ((u8)rawVolume > 127)
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
		u32 soundID           = sound->unk8;
		JAISoundTable* table  = getInfoPointerFromID(soundID);
		void* soundInfo       = sound->unk3C;
		getInfoFormat(table, soundID);

		u32 flags = *(u32*)soundInfo;
		f32 volume;
		if (flags & 0x00C00000)
			volume = MSoundSESystem::MSRandVol::getRandomVolumeNormal(flags);
		else
			volume = 1.0f;

		f32 capped = 1.0f;
		f32 scaled = volume * (((u8*)soundInfo)[0xC] / 127.0f);
		if (scaled < capped)
			capped = scaled;

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

u32 MSound::startMarioVoice(u32 id, s16 param2, u8 param3) { return 0; }

u32 MSound::getMarioVoiceID(u8 param)
{
	u8 index;
	if (param & 2)
		index = 1;
	else
		index = 0;

	JAISound* sound = ((JAISound**)&unk8C)[index];
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

	JAISound* sound = ((JAISound**)&unk8C)[index];
	if (sound != nullptr) {
		if (id != -1) {
			if (id == sound->unk8)
				sound->stop(1);
		} else {
			sound->stop(1);
		}
	}
}

JAISound* MSound::checkMarioVoicePlaying(u8 param)
{
	u8 index;
	if (param & 2)
		index = 1;
	else
		index = 0;

	return ((JAISound**)&unk8C)[index];
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

JAISound* MSound::startBeeSe(Vec* pos, u32 id) { return nullptr; }

JAISound* MSound::startSoundActorSpecial(u32 id, const Vec* pos, f32 param3,
                                         f32 param4, u32 param5,
                                         JAISound** soundPtr, u32 param7,
                                         u8 param8)
{
	return nullptr;
}

bool MSound::cameraLooksAtMario()
{
	for (u8 i = 0; i < 5; ++i) {
		if (i == 0) {
			if ((&unkC8)[i] == 0)
				return false;
		} else if ((&unkC8)[i] == 1) {
			return false;
		}
	}

	return true;
}

bool MSound::gateCheck(u32 param)
{
	u8 flag = unkA8;
	if (!(flag & 1)) {
		u8 category = (param >> 24) & 0xC0;
		category |= (param >> 11) & 1;
		if (category == 0)
			return false;
	}

	if (!(flag & 2)) {
		u8 category = (param >> 24) & 0xC0;
		category |= (param >> 11) & 1;
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
