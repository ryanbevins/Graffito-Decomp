#define MSL_STDSQRTF_OUT_OF_LINE

#include <MSound/MAnmSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <MSound/MSHandle.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>
#include <math.h>

#undef MSL_STDSQRTF_OUT_OF_LINE

static inline u8 npc_get_uint8(u8 limit)
{
	return JAIConst::random.get_ufloat_1() * limit;
}

MAnmSound::MAnmSound(MSound* sound) { mData = nullptr; }

void MAnmSound::initAnmSound(void* ptr, u32 ul, f32 f)
{
	initActorAnimSound(ptr, ul, f);
}

void MAnmSound::animeLoop(Vec* vec, f32 f1, f32 f2, u32 ul, u8 uc)
{
	if (mData != nullptr) {
		setAnimSoundVec(JAIBasic::basic, vec, f1, f2, ul, uc);
	}
}

void MAnmSound::startAnimSound(void* ptr, u32 ul, JAISound** sound,
                               JAIActor* actor, u8 uc)
{
	if (!MSGMSound->gateCheck(ul))
		return;

	u32 topBits = ul >> 30;
	s32 cat     = (ul >> 12) & 0xf;
	if (topBits != 0) {
		if (topBits == 2)
			cat = 0x10;
		else if (topBits == 3)
			cat = 0x11;
		else
			cat = -1;
	}

	switch (cat) {
	case 0:
		if ((actor->unkC & 0x1000) == 0x1000)
			return;
		break;
	case 7: {
		s16 voiceIdx = (actor->unkC >> 24) & 0xf;
		MSGMSound->startMarioVoice(ul, voiceIdx, actor->unkC >> 28);
		return;
	}
	default: break;
	}

	MSoundSESystem::MSoundSE::startSoundActorInner(ul, sound, actor, 0, uc);
}

void MAnmSound::setSpeedModifySound(JAISound* sound,
                                    JAIAnimeFrameSoundData* data, f32 f)
{
	u32 sw = sound->unk8;
	if (MSound::getSwitch(sw, 0x100000, 0x14)) {
		JAIAnimeSound::setSpeedModifySound(sound, data, f);
	}
}

void MAnmSoundNPC::startAnimSound(void* ptr, u32 ul, JAISound** sound,
                                  JAIActor* actor, u8 uc)
{
	if (!MSGMSound->gateCheck(ul))
		return;

	JAIAnimeFrameSoundData* data = (JAIAnimeFrameSoundData*)mData;
	u32 v                       = data[mDataCounter].unk18;
	if (v & 0xFFFF0000) {
		if (v & 0xFF000000) {
			u32 mod = (v >> 24) + 1;
			if (mLoopCount != 0) {
				if ((mLoopCount + unk98 % mod) % mod != 0)
					return;
			}
		}
		if (v & 0x00FF0000) {
			if (npc_get_uint8(((v >> 16) & 0xff) + 1) != 0)
				return;
		}
	}

	if (!MSoundSESystem::MSoundSE::checkMonoSound(ul, actor))
		return;
	MSoundSESystem::MSoundSE::startSoundActorInner(ul, sound, actor, 0, uc);

	if (*sound == nullptr)
		return;

	u32 v2 = data[mDataCounter].unk18;
	if (v2 & 0x8000)
		return;

	f32 vol         = 1.0f;
	const Vec* actorPos = actor->unk4;
	f32 dist;
	if (MSGMSound->cameraLooksAtMario()) {
		Vec* camPos = MSGMSound->unkAC[0].unk0;
		f32 dy = actorPos->y - camPos->y;
		f32 dy2 = powf(dy, 2.0f);
		f32 dx = actorPos->x - camPos->x;
		f32 dx2 = powf(dx, 2.0f);
		f32 dz = actorPos->z - camPos->z;
		f32 dz2 = powf(dz, 2.0f);
		dist = std::sqrtf(dx2 + dy2 + dz2);
	} else {
		dist = 0.0f;
	}
	if (dist != 0.0f) {
		vol = MSHandle::calcVolume(dist, 2000.0f, 600.0f,
		                           (v2 >> 12) & 0x7, 8);
	}
	(*sound)->setSeInterVolume(0, vol, 0, 0);
}
