#include <JSystem/JAudio/JAInterface/JAIBasic.hpp>
#include <JSystem/JAudio/JAInterface/JAIInter.hpp>
#include <JSystem/JAudio/JAInterface/JAIParameters.hpp>
#include <JSystem/JAudio/JAInterface/JAIGlobalParameter.hpp>
#include <JSystem/JAudio/JASystem/JASCalc.hpp>
#include <JSystem/JAudio/JASystem/JASHeapCtrl.hpp>
#include <JSystem/JAudio/JASystem/JASCallback.hpp>
#include <JSystem/JAudio/JASystem/JASDvdThread.hpp>
#include <JSystem/JAudio/JASystem/JASSystemHeap.hpp>
#include <macros.h>

namespace JAInter {
namespace StreamLib {

	// TODO: from TWW, might be wrong
	struct StreamHeader {
		int unk0;
		int unk4;
		u16 unk8;
		u16 unkA;
		u16 unkC;
		u16 unkE;
		u32 unk10;
		u32 unk14;
		int unk18;
		int unk1C;
	};

	static DVDFileInfo finfo;
	static StreamHeader header;
	static char Filename[100];
	JASystem::Kernel::TSolidHeap streamHeap;

	static s16 filter_table[32] = {
		0x0000, 0x0000, 0x0800, 0x0000, 0x0000, 0x0800, 0x0400, 0x0400,
		0x1000, -0x800, 0x0E00, -0x600, 0x0C00, -0x400, 0x1200, -0xA00,
		0x1068, -0x8C8, 0x12C0, -0x8FC, 0x1400, -0xC00, 0x0800, -0x800,
		0x0400, -0x400, -0x400, 0x0400, -0x400, 0x0000, -0x800, 0x0000,
	};

	s16 table4[] = {
		0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
		-0x008, -0x007, -0x006, -0x005, -0x004, -0x003, -0x002, -0x001,
	};

	static u32 adpcm_remain        = 0;
	static u32 adpcm_loadpoint     = 0;
	static u32 loadsize            = 0;
	static s16* adpcm_buffer       = nullptr;
	static s16*** loop_buffer      = nullptr;
	static void** store_buffer     = nullptr;
	static JASystem::TDSPChannel* assign_ch[2];
	static u32 playside            = 0;
	static u32 playback_samples    = 0;
	static u32 loadup_samples      = 0;
	static u32 adpcmbuf_state      = 0;
	static u32 movieframe          = 0;
	static bool stopflag           = false;
	static bool stopflag2          = false;
	static u8 playflag             = false;
	static u8 playflag2            = false;
	static u8 prepareflag          = 0;
	static bool dspch_deallockflag = false;
	static f32 outvolume           = 0.0f;
	static f32 outpitch            = 0.0f;
	static f32 outpan              = 0.0f;
	static f32 stackvolume         = 0.0f;
	static f32 stackpitch          = 0.0f;
	static f32 stackpan            = 0.0f;
	static bool outflag_volume     = false;
	static bool outflag_pan        = false;
	static bool outflag_pitch      = false;
	static u32 loop_start_flag     = 0;
	static u32 outpause            = 0;
	static u32 playmode            = 0;
	static u32 shift_sample        = 0;
	static u32 extra_sample        = 0;
	static u32 DvdLoadFlag         = false;
	static u32 startInitFlag       = false;
	static u32 Mode                = 0;
	static void* Head              = nullptr;

	bool bufferMode = false;
	bool allocFlag  = false;

	static u32 LOOP_BLOCKS     = 0xC;
	static u32 LOOP_SAMPLESIZE = 0xf000;
	static u32 outputmode      = 1;

} // namespace StreamLib
} // namespace JAInter

void JAIBasic::checkEntriedStream()
{
	JAISound* it;
	for (it = unk0->unk21C.unk4; it != nullptr; it = it->unk30) {
		bool bVar1 = false;
		if (it->unk1 == 1) {
			if (!unk0->unk184->unk14) {
				JAInter::StreamLib::stop();
				bVar1 = true;
			} else if (unk0->unk184->unk14->unk2 == 0) {
				JAInter::StreamLib::stop();
				bVar1 = true;
			}
			if (bVar1) {
				it->unk1 = 2;

				it->getStreamParameter()->unk3D4 = unk0->unk184;
				unk0->initStreamUpdateParameter();
				unk0->unk184->unk14 = it;
			}
		}
	}
}

void JAIBasic::checkWaitStream()
{
	JAISound* sound = unk0->unk184->unk14;
	if (!sound)
		return;
	if (sound->unk1 != 2)
		return;

	char buffer[64];
	strcpy(buffer, JAIGlobalParameter::streamPath);
	strcat(buffer, unk0->unk1F8[sound->unk8 & 0x3FF].unk10);
	setSeExtParameter(sound);
	sound->unk1 = 3;
	checkPlayingStream();
	JAInter::StreamLib::start(buffer, sound->getStreamParameter()->unk4,
	                          &unk0->unk1F8[sound->unk8 & 0x3FF].unk20);
	JAInter::StreamLib::setPrepareFlag(1);
}

void JAIBasic::checkRequestStream()
{
	JAISound* sound = unk0->unk184->unk14;
	if (!sound)
		return;
	if (sound->unk1 != 3)
		return;
	if (unk0->unk184->unk2 != 0)
		return;
	sound->unk1 = 4;
	if (sound->unk10 > 1) {
		sound->setStreamInterVolume(6, 0.0f, 0);
		sound->setStreamInterVolume(6, 1.0f, sound->unk10);
	}
	JAInter::StreamLib::setPrepareFlag(0);
}

void JAIBasic::checkPlayingStream()
{
	JAIStreamUpdateParameter* streamData = unk0->unk184;
	JAISound* sound                      = streamData->unk14;
	if (sound == nullptr)
		return;

	u8 status        = sound->unk1;
	u32* streamFlags = &streamData->unk10;
	if (status >= 4) {
		sound->getStreamParameter();
		if (JAInter::StreamLib::getPlayingFlag() == 2) {
			sound->unk1 = 0;
			if (sound->getStreamParameter()->unk3D4 != nullptr)
				sound->getStreamParameter()->unk3D4->unk14 = nullptr;

			releaseStreamParameterPointer(sound->getStreamParameter());
			sound->clearMainSoundPPointer();
			releaseControllerHandle(&unk0->unk21C, sound);
			return;
		}

		if (sound->unk2 != 0)
			--sound->unk2;

		if (*streamFlags & 0x2) {
			sound->setStreamInterVolume(6, 0.0f, sound->unk10);
			sound->unk1 = 5;
			*streamFlags ^= 0x2;
		}

		if (sound->unk1 == 5) {
			if ((sound->getStreamInterVolume(6) == 0.0f || sound->unk10 == 0)
			    && sound->unk2 == 0) {
				JAInter::StreamLib::stop();
				sound->unk1 = 0;
				if (sound->getStreamParameter()->unk3D4 != nullptr)
					sound->getStreamParameter()->unk3D4->unk14 = nullptr;

				releaseStreamParameterPointer(sound->getStreamParameter());
				sound->clearMainSoundPPointer();
				releaseControllerHandle(&unk0->unk21C, sound);
			}
		}
	}

	if (sound->unk1 < 3)
		return;

	JAIStreamParameter* streamParam = sound->getStreamParameter();

	if (*streamFlags & 0x40000) {
		f32 value = 1.0f;
		for (u8 i = 0; i < 13; ++i) {
			u32 bit = 1 << i;
			JAIMoveParaSet* moveParam = &streamParam->unk14[i];
			if (streamParam->unk8 & bit) {
				if (!unk0->moveParameter(moveParam))
					streamParam->unk8 ^= bit;
			}
			value *= moveParam->unk4;
		}

		if (unk0->unk184->unk4 != value) {
			JAInter::StreamLib::setVolume(value);
			unk0->unk184->unk4 = value;
		}

		if (streamParam->unk8 == 0)
			*streamFlags ^= 0x40000;
	}

	if (*streamFlags & 0x100000) {
		f32 value = 1.0f;
		for (u8 i = 0; i < 13; ++i) {
			u32 bit = 1 << i;
			JAIMoveParaSet* moveParam = &streamParam->unk154[i];
			if (streamParam->unkC & bit) {
				if (!unk0->moveParameter(moveParam))
					streamParam->unkC ^= bit;
			}
			value *= moveParam->unk4;
		}

		if (unk0->unk184->unk8 != value) {
			JAInter::StreamLib::setPitch(value);
			unk0->unk184->unk8 = value;
		}

		if (streamParam->unkC == 0)
			*streamFlags ^= 0x100000;
	}

	if (*streamFlags & 0x80000) {
		f32 value = 0.0f;
		for (u8 i = 0; i < 13; ++i) {
			u32 bit = 1 << i;
			JAIMoveParaSet* moveParam = &streamParam->unk294[i];
			if (streamParam->unk10 & bit) {
				if (!unk0->moveParameter(moveParam))
					streamParam->unk10 ^= bit;
			}
			value += moveParam->unk4 - 0.5f;
		}

		value += 0.5f;
		if (value > 1.0f)
			value = 1.0f;
		else if (value < 0.0f)
			value = 0.0f;

		if (unk0->unk184->unkC != value) {
			JAInter::StreamLib::setPan(value);
			unk0->unk184->unkC = value;
		}

		if (streamParam->unk10 == 0)
			*streamFlags ^= 0x80000;
	}

	++sound->unk14;
}

namespace JAInter {
namespace StreamLib {

	void Play_DirectPCM(JASystem::TDSPChannel* channel, s16* param_2,
	                    u16 param_3, u32 param_4)
	{
		JASystem::DSPInterface::DSPBuffer* pDVar1;

		pDVar1         = JASystem::DSPInterface::getDSPHandle(channel->unk0);
		pDVar1->unk118 = param_2;
		pDVar1->unk102 = 0;
		pDVar1->unk100 = 0x21;
		pDVar1->unk74  = param_4;
		pDVar1->unk110 = param_2;
		pDVar1->unk114 = param_3 << 0x10;

		JASystem::DSPInterface::getDSPHandle(channel->unk0)
		    ->setMixerInitDelayMax(0);

		for (u8 i = 0; i < 6; ++i) {
			if (i < 2) {
				JASystem::DSPInterface::getDSPHandle(channel->unk0)
				    ->setMixerInitVolume(i, 0x7fff, '\0');
			} else {
				JASystem::DSPInterface::getDSPHandle(channel->unk0)
				    ->setMixerInitVolume(i, 0, '\0');
			}

			JASystem::DSPInterface::getDSPHandle(channel->unk0)
			    ->setBusConnect(i, i + 1);
		}
		JASystem::DSPInterface::getDSPHandle(channel->unk0)->setPitch(0x800);
		JASystem::DSPInterface::getDSPHandle(channel->unk0)->playStart();
		JASystem::DSPInterface::getDSPHandle(channel->unk0)->flushChannel();
	}

	void* Get_DirectPCM_LoopRemain(JASystem::DSPInterface::DSPBuffer* buffer)
	{
		return nullptr;
	}

	void* Get_DirectPCM_Counter(JASystem::DSPInterface::DSPBuffer* buffer)
	{
		return nullptr;
	}

	void* Get_DirectPCM_Remain(JASystem::DSPInterface::DSPBuffer* buffer)
	{
		return nullptr;
	}

	void init(bool mode)
	{
		bufferMode = mode;
		if (!mode) {
			u32 sz = getNeedBufferSize();
			allocBuffer(JASDram->alloc(sz, 0), sz);
		}
	}

	void allocBuffer(void* buffer, s32 size)
	{
		if (allocFlag)
			return;

		streamHeap.init((u8*)buffer, size);
		loop_buffer = (s16***)streamHeap.alloc(sizeof(s16**) * 2);
		for (u32 i = 0; i < 2; ++i) {
			loop_buffer[i]
			    = (s16**)streamHeap.alloc(LOOP_BLOCKS * sizeof(s16*));
			for (u32 j = 0; j < LOOP_BLOCKS; ++j) {
				loop_buffer[i][j] = (s16*)streamHeap.alloc(0x2800);
			}
		}
		store_buffer = (void**)streamHeap.alloc(sizeof(void*) * 2);
		for (u32 i = 0; i < 2; ++i) {
			store_buffer[i] = streamHeap.alloc(0x5000);
		}

		adpcm_buffer = (s16*)streamHeap.alloc(0x5000);

		allocFlag = 1;
	}

	void deallocBuffer() { }

	bool checkBufferStatus() { return false; }

	void setBufferMode(bool mode) { }

	u32 getNeedBufferSize()
	{
		u32 size = 0x20;

		for (u32 i = 0; i < 2; ++i) {
			size += ALIGN_PREV(LOOP_BLOCKS, 8) * 4 + 0x20;
			for (u32 j = 0; j < LOOP_BLOCKS; ++j) {
				size += 0x2820;
			}
		}

		return size + 0xF080;
	}

	void sync(s32 param) { }

	void Hvqm_SetAudioDmaBuffers(u32 buffers) { }

	inline void setPlayingFrame(s32 frame)
	{
		static s32 before = -1;
		before            = frame;
	}

	static void __DecodePCM()
	{
		s16* p1;
		s16* p2;
		s16* src;

		u32 lsz     = loadsize;
		u32 samples = loadsize / 4;

		p1  = loop_buffer[0][playside];
		p2  = loop_buffer[1][playside];
		src = adpcm_buffer;
		for (s32 i = 0; i < samples; ++i) {
			*p1 = src[0];
			*p2 = src[1];
			++p1;
			++p2;
			src += 2;
		}

		loadup_samples += loadsize / 4;
		DCStoreRange(loop_buffer[0][playside], lsz / 2);

		DCStoreRange(loop_buffer[1][playside], loadsize / 2);
	}

	static void __DecodeADPCM()
	{
		static s16 L1;
		static s16 L2;
		static s16 R1;
		static s16 R2;
		u32 lsz;
		u8* src;
		s16* left;
		s16* right;
		u32 skipBytes = 0;
		u32 blocks;

		if (movieframe == 0 && playside == 0) {
			R2 = 0;
			R1 = 0;
			L2 = 0;
			L1 = 0;
		}

		src   = (u8*)adpcm_buffer;
		left  = (s16*)store_buffer[0];
		right = (s16*)store_buffer[1];

		if (loop_start_flag != 0) {
			skipBytes = ((header.unk14 >> 4) & 7) * 0x12;
			loop_start_flag = false;
			loadsize        = 0x1680 - skipBytes;
			src             = (u8*)adpcm_buffer + skipBytes;
		}

		lsz    = loadsize;
		blocks = lsz / 0x12;
		for (u32 block = 0; block < blocks; ++block) {
			u8 predictor = *src++;
			s16 coef1    = filter_table[(predictor & 0xF) * 2];
			s16 coef2    = filter_table[(predictor & 0xF) * 2 + 1];
			u32 shift    = predictor >> 4;

			for (u32 i = 0; i < 4; ++i) {
				u8 data = src[0];
				s16 sample
				    = (s16)((table4[data >> 4] << shift)
				            + ((coef1 * L1 + coef2 * L2) >> 11));
				*left++ = sample;
				L2      = sample;

				sample = (s16)((table4[data & 0xF] << shift)
				               + ((coef1 * L2 + coef2 * L1) >> 11));
				*left++ = sample;
				L1      = sample;

				data = src[1];
				sample
				    = (s16)((table4[data >> 4] << shift)
				            + ((coef1 * L1 + coef2 * L2) >> 11));
				*left++ = sample;
				L2      = sample;

				sample = (s16)((table4[data & 0xF] << shift)
				               + ((coef1 * L2 + coef2 * L1) >> 11));
				*left++ = sample;
				L1      = sample;

				src += 2;
			}

			predictor = *src++;
			coef1     = filter_table[(predictor & 0xF) * 2];
			coef2     = filter_table[(predictor & 0xF) * 2 + 1];
			shift     = predictor >> 4;

			for (u32 i = 0; i < 4; ++i) {
				u8 data = src[0];
				s16 sample
				    = (s16)((table4[data >> 4] << shift)
				            + ((coef1 * R1 + coef2 * R2) >> 11));
				*right++ = sample;
				R2       = sample;

				sample = (s16)((table4[data & 0xF] << shift)
				               + ((coef1 * R2 + coef2 * R1) >> 11));
				*right++ = sample;
				R1       = sample;

				data = src[1];
				sample
				    = (s16)((table4[data >> 4] << shift)
				            + ((coef1 * R1 + coef2 * R2) >> 11));
				*right++ = sample;
				R2       = sample;

				sample = (s16)((table4[data & 0xF] << shift)
				               + ((coef1 * R2 + coef2 * R1) >> 11));
				*right++ = sample;
				R1       = sample;

				src += 2;
			}
		}

		loadup_samples += (((lsz - skipBytes) / 0x12) & 0x7FFFFFF) << 4;

		u32 pos;
		u32 sampleIdx;
		for (sampleIdx = 0; sampleIdx < (loadsize / 0x12) * 0x10;
		     ++sampleIdx) {
			pos = sampleIdx + shift_sample;
			if (pos == 0x1400) {
				DCStoreRange(loop_buffer[0][playside] + shift_sample,
				             (0x1400 - shift_sample) * 2);
				DCStoreRange(loop_buffer[1][playside] + shift_sample,
				             (0x1400 - shift_sample) * 2);
				playside = (playside + 1) % LOOP_BLOCKS;
			}

			if (pos >= 0x1400)
				pos -= 0x1400;

			loop_buffer[0][playside][pos] = ((s16*)store_buffer[0])[sampleIdx];
			loop_buffer[1][playside][pos] = ((s16*)store_buffer[1])[sampleIdx];
		}

		u32 endPos = sampleIdx + shift_sample;
		DCStoreRange(loop_buffer[0][playside], 0x2800);
		DCStoreRange(loop_buffer[1][playside], 0x2800);

		if (endPos == 0x1400)
			playside = (playside + 1) % LOOP_BLOCKS;

		if (endPos >= 0x1400) {
			shift_sample = endPos - 0x1400;
		} else {
			shift_sample = endPos;
			if (endPos > 0x1400)
				shift_sample -= 0x1400;
		}
	}

	static void __Decode() { }

	static void __LoadFin(s32 param, DVDFileInfo* info)
	{
		DvdLoadFlag = 0;
		if (adpcmbuf_state == 3)
			return;
		adpcmbuf_state = 2;
	}

	static void LoadADPCM()
	{
		if (adpcmbuf_state != 0)
			return;

		switch (header.unkA) {
		case 2:
			loadsize = 0x5000;
			break;
		case 4:
			loadsize = 0x1680;
			break;
		}

		extra_sample = 0;

		if (adpcm_remain < loadsize) {
			if ((adpcm_remain & 0x1F) != 0) {
				loadsize     = adpcm_remain + (0x20 - (adpcm_remain & 0x1F));
				extra_sample = loadsize - adpcm_remain;
			} else {
				loadsize = adpcm_remain;
			}
		}

		adpcmbuf_state = 1;
		DVDReadAsyncPrio(&finfo, adpcm_buffer, loadsize, adpcm_loadpoint,
		                 &__LoadFin, 2);
		DvdLoadFlag = 1;
		adpcm_loadpoint += loadsize;
		if (adpcm_remain < loadsize) {
			adpcm_remain = 0;
		} else {
			adpcm_remain = adpcm_remain - loadsize;
		}
	}

	void setVolume(f32 volume)
	{
		stackvolume    = volume;
		outflag_volume = true;
	}

	void setPitch(f32 pitch)
	{
		stackpitch    = pitch;
		outflag_pitch = true;
	}

	void setPan(f32 pan)
	{
		stackpan    = pan;
		outflag_pan = true;
	}

	void stop()
	{
		stopflag  = true;
		stopflag2 = true;
	}

	void setPauseFlag(u8 flag) { outpause |= flag; }

	void clearPauseFlag(u8 flag) { outpause &= (flag ^ 0xff); }

	void setPrepareFlag(u8 flag) { prepareflag = flag; }

	void getPrepareFlag() { }

	void setOutputMode(u32 mode) { outputmode = mode; }

	u8 getPlayingFlag() { return playflag2; }

	void setDecodedBufferBlocks(u32 blocks) { }

	void LoopInit() { }

	static s32 callBack(void* param);

	void start(char* filename, u32 mode, void* param)
	{
		if (startInitFlag == 0) {
			strcpy(Filename, filename);
			Mode = mode;
			Head = param;
			++startInitFlag;
			JASystem::Kernel::registerSubframeCallback(&callBack, nullptr);
		}
	}

	void __start()
	{
		startInitFlag = 0;
		playmode      = Mode;

		if (playflag != 0) {
			assign_ch[0]->forceStop();
			assign_ch[1]->forceStop();
			++playflag;
		}

		DVDOpen(Filename, &finfo);
		if (Head == nullptr) {
			DVDReadPrio(&finfo, adpcm_buffer, 0x20, 0, 2);
		} else {
			for (int i = 0; i < 2; ++i) {
				for (int j = 0; j < 16; ++j) {
					((u8*)adpcm_buffer)[i * 16 + j]
					    = ((u8*)Head)[i * 16 + j];
				}
			}
		}

		adpcm_loadpoint = 0x20;
		header          = *(StreamHeader*)adpcm_buffer;
		adpcm_remain    = header.unk0;
		playback_samples = header.unk4;
		if (playmode != 0)
			header.unk10 = 0;

		stopflag        = false;
		stopflag2       = false;
		playflag2       = 0;
		prepareflag     = 0;
		outvolume       = 1.0f;
		outpitch        = 1.0f;
		outpan          = 0.5f;
		loadup_samples  = 0;
		movieframe      = 0;
		loop_start_flag = false;
		adpcmbuf_state  = 0;
		playside        = 0;
		shift_sample    = 0;
		LOOP_SAMPLESIZE = LOOP_BLOCKS * 0x1400;

		JASystem::Dvd::pauseDvdT();
		LoadADPCM();

		for (u32 i = 0; i < 2; ++i) {
			if (assign_ch[i] != nullptr && assign_ch[i]->unk8 != 0)
				JASystem::TDSPChannel::free(assign_ch[i], (u32)&assign_ch[i]);
			assign_ch[i] = nullptr;
		}

		dspch_deallockflag = true;
	}

	s32 callBack(void* param)
	{
		static s32 oldstat;
		static u32 old_dspside;

		bool started = false;

		if (startInitFlag != 0) {
			if (DvdLoadFlag != 0)
				return 0;
			__start();
		}

		if (startInitFlag == 0) {
			if (outflag_volume) {
				outflag_volume = false;
				outvolume      = stackvolume;
			}
			if (outflag_pitch) {
				outflag_pitch = false;
				outpitch      = stackpitch;
			}
			if (outflag_pan) {
				outflag_pan = false;
				outpan      = stackpan;
			}
		}

		if (assign_ch[0] == nullptr) {
			assign_ch[0] = JASystem::TDSPChannel::alloc(
			    0, (u32)&assign_ch[0]);
			assign_ch[1] = JASystem::TDSPChannel::alloc(
			    0, (u32)&assign_ch[1]);
			if (assign_ch[0] != nullptr && assign_ch[1] != nullptr) {
				assign_ch[0]->unk3 = 0x7F;
				assign_ch[1]->unk3 = 0x7F;
			}
		}

		if (assign_ch[0] == nullptr || assign_ch[1] == nullptr) {
			setPlayingFrame(-1);
			playflag  = 0;
			playflag2 = 2;
			JASystem::Dvd::unpauseDvdT();
			return -1;
		}

		s32 status = DVDGetDriveStatus();
		switch (status) {
		case 5:
			JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
			    ->setPauseFlag(1);
			JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
			    ->setPauseFlag(1);
			outpause = 1;
			JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
			    ->flushChannel();
			JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
			    ->flushChannel();
			break;
		case 0:
			status = DVDGetDriveStatus();
			if (oldstat != status) {
				JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
				    ->setPauseFlag(0);
				JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
				    ->setPauseFlag(0);
				JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
				    ->flushChannel();
				JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
				    ->flushChannel();
				outpause = 0;
			}
			break;
		}

		oldstat = DVDGetDriveStatus();
		if (outpause != 0)
			return 0;

		BOOL decode = false;
		if (movieframe != 0) {
			JASystem::DSPInterface::DSPBuffer* buffer
			    = JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0);
			if (buffer->unk2 != 0) {
				if (adpcmbuf_state != 2) {
					JASystem::TDSPChannel::free(assign_ch[0],
					                            (u32)&assign_ch[0]);
					JASystem::TDSPChannel::free(assign_ch[1],
					                            (u32)&assign_ch[1]);
					setPlayingFrame(-1);
					playflag  = 0;
					playflag2 = 2;
					JASystem::Dvd::unpauseDvdT();
					return -1;
				}
				return 0;
			}

			setPlayingFrame(((playback_samples - buffer->unk74) * header.unkE)
			                / header.unk8);
			++movieframe;

			u32 dspside = (LOOP_SAMPLESIZE - (buffer->unk6C >> 16)) / 0x1400;
			if (old_dspside != dspside)
				old_dspside = dspside;

			if (dspside != (playside + 1) % LOOP_BLOCKS)
				decode = true;
		}

		if (decode || movieframe == 0) {
			if (adpcmbuf_state == 2 || adpcmbuf_state == 4) {
				if (adpcmbuf_state == 2) {
					switch (header.unkA) {
					case 3:
						break;
					case 2:
						__DecodePCM();
						break;
					case 4:
						__DecodeADPCM();
						break;
					}
					adpcmbuf_state = 0;
				}

				if (movieframe == 0 && playside == LOOP_BLOCKS - 2) {
					if (prepareflag != 0) {
						prepareflag    = 2;
						adpcmbuf_state = 4;
						return 0;
					}

					++movieframe;
					prepareflag = 2;
					playflag2   = 1;

					for (u8 i = 0; i < 2; ++i) {
						u16 loopSize = (u16)LOOP_SAMPLESIZE;
						Play_DirectPCM(assign_ch[i], loop_buffer[i][0],
						               loopSize, playback_samples);

						s16 mainVolume;
						s16 subVolume;
						if (outputmode == 1) {
							mainVolume = 0x7fff;
							subVolume  = 0;
						} else {
							mainVolume = 0x5a7e;
							subVolume  = 0x5a7e;
						}

						JASystem::DSPInterface::getDSPHandle(assign_ch[i]->unk0)
						    ->setMixerVolume(i, mainVolume, 0);
						JASystem::DSPInterface::getDSPHandle(assign_ch[i]->unk0)
						    ->setMixerVolume(1 - i, subVolume, 0);

						u16 pitch = (u16)((header.unk8 << 12) / 32000);
						JASystem::DSPInterface::getDSPHandle(assign_ch[i]->unk0)
						    ->setPitch(pitch);
						if (header.unk10 != 0)
							JASystem::DSPInterface::getDSPHandle(assign_ch[i]->unk0)
							    ->unk74
							    = -1;
						JASystem::DSPInterface::getDSPHandle(assign_ch[i]->unk0)
						    ->flushChannel();
					}

					started = true;
					if (adpcmbuf_state != 3)
						adpcmbuf_state = 0;
				}
			}
		}

		if (stopflag && stopflag2) {
			stopflag2 = false;
			assign_ch[0]->forceStop();
			assign_ch[1]->forceStop();
			adpcmbuf_state = 3;
		}

		if (!started) {
			f32 leftPan  = 1.0f;
			f32 rightPan = 1.0f;
			u16 baseVolume;

			if (outputmode == 1) {
				if (outpan < 0.5f) {
					rightPan = 1.4142f * JASystem::Calc::sinfT(outpan);
				} else {
					leftPan = 1.4142f * JASystem::Calc::sinfT(1.0f - outpan);
				}

				baseVolume = 0x7fff;
				JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
				    ->setMixerVolume(1, 0, 0);
				JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
				    ->setMixerVolume(0, 0, 0);
			} else {
				baseVolume  = 0x5a7e;
				s16 volume = (s16)(23166.0f * outvolume);
				JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
				    ->setMixerVolume(1, volume, 0);
				JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
				    ->setMixerVolume(0, volume, 0);
			}

			JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
			    ->setMixerVolume(0, (s16)(outvolume * (f32)baseVolume * leftPan),
			                     0);
			JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
			    ->setMixerVolume(1, (s16)(outvolume * (f32)baseVolume * rightPan),
			                     0);

			u16 pitch = (u16)(outpitch * ((header.unk8 << 12) / 32000));
			JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
			    ->setPitch(pitch);
			JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
			    ->setPitch(pitch);
			JASystem::DSPInterface::getDSPHandle(assign_ch[0]->unk0)
			    ->flushChannel();
			JASystem::DSPInterface::getDSPHandle(assign_ch[1]->unk0)
			    ->flushChannel();
		}

		if (adpcmbuf_state == 0) {
			if (adpcm_remain == 0) {
				if (header.unk10 != 0) {
					loop_start_flag = true;
					adpcm_loadpoint = (((header.unk14 - (header.unk14 & 0x7F))
					                    >> 4)
					                   * 0x12)
					                  + 0x20;
					adpcm_remain = header.unk0 - (adpcm_loadpoint - 0x20);
				} else {
					adpcmbuf_state = 3;
				}
			} else if (stopflag) {
				adpcmbuf_state = 3;
			} else {
				LoadADPCM();
			}
		}

		return 0;
	}

} // namespace StreamLib
} // namespace JAInter
