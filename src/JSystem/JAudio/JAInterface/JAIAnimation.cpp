#include <JSystem/JAudio/JAInterface/JAIAnimation.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>

JAIAnimeSound::JAIAnimeSound()
{
	unk60 = 0;
	unk64 = 0;
	unk68 = 0;
	unk6C = 0;
	for (u8 i = 0; i < 8; ++i) {
		mSlots[i].mSound     = nullptr;
		mSlots[i].mIsPlaying = false;
	}

	for (u8 i = 0; i < 2; ++i)
		unk70[i] = nullptr;

	mDataCounter = 0;
	mLoopCount   = 0;
}

JAIAnimeSound::~JAIAnimeSound() { }

void JAIAnimeSound::initActorAnimSound(void* data, u32 param, f32 value)
{
	mData = (u8*)data;
	u32 soundIndex = 0;
	if (mData != nullptr) {
		mDataCounter = 0;
		mDataCounterInc = param;

		u16 soundCount = *(u16*)mData;
		while (soundIndex < soundCount) {
			JAIAnimeFrameSoundData* frameData
			    = (JAIAnimeFrameSoundData*)(mData + 8 + soundIndex * 0x20);
			if (frameData->unk4 >= value)
				break;
			soundIndex++;
		}

		if (param == 1) {
			mDataCounter      = 0;
			mDataCounterLimit = soundIndex;
			mCurrentTime      = 0.0f;
			mLoopCount        = 0;
		} else {
			mDataCounter      = soundIndex;
			mDataCounterLimit = 0;
			mCurrentTime      = 0.0f;
			mLoopCount        = -1;
		}
	}

	Slot* slot;
	u8    i;
	for (i = 0; i < 8; ++i) {
		slot = &mSlots[i];
		JAISound* sound = slot->mSound;
		JAIAnimeFrameSoundData* soundData
		    = (JAIAnimeFrameSoundData*)slot->mData;
		if (sound != nullptr && (soundData->unk10 & 4)) {
			sound->stop(1);
			slot->mIsPlaying = false;
		}

		if (sound == nullptr) {
			slot->mIsPlaying = false;
		} else if ((sound->unk8 & 0xC00) == 0) {
			slot->mIsPlaying = false;
		}
	}
}

void JAIAnimeSound::setAnimSoundVec(JAIBasic* param_1, Vec* param_2,
                                    f32 param_3, f32 param_4, u32 param_5,
                                    u8 param_6)
{
	JAIActor actor(param_2, param_2, param_2, param_5);
	setAnimSoundActor(param_1, &actor, param_3, param_4, param_6);
}

void JAIAnimeSound::setAnimSoundActor(JAIBasic* basic, JAIActor* actor,
                                      f32 param1, f32 param2, u8 param3)
{
	if (mData == nullptr)
		return;

	u8* data       = mData;
	u16 soundCount = *(u16*)data;
	if ((u32)mDataCounterInc == 1) {
		if (mCurrentTime > param1) {
			while (
			    mDataCounter < soundCount
			    && ((JAIAnimeFrameSoundData*)(data + 8 + mDataCounter * 0x20))
			               ->unk4
			        <= mCurrentTime + param2) {
				playActorAnimSound(basic, actor, param2, param3);
			}
			mDataCounter = mDataCounterLimit;
			mCurrentTime = param1;
			if ((u32)mLoopCount < 0x100)
				mLoopCount++;
		}

		for (u8 i = 0; i < 8; ++i) {
			Slot& slot = mSlots[i];
			JAISound** slotSound = &slot.mSound;
			if (slot.mIsPlaying) {
				JAIAnimeFrameSoundData* soundData
				    = (JAIAnimeFrameSoundData*)slot.mData;
				u32 soundID = soundData->mSoundID;
				if ((soundID & 0xC00) == 0) {
					if (param2 != 0.0f || (soundData->unk10 & 0x20) == 0) {
						f32 startFrame = soundData->unk4;
						f32 endFrame   = soundData->unk8;
						if (startFrame == endFrame
						    || (startFrame < endFrame && endFrame > param1
						        && startFrame <= param1)
						    || (startFrame > endFrame
						        && (endFrame > param1 || startFrame < param1))) {
							startAnimSound(basic, soundID, slotSound, actor,
							               param3);
						} else {
							slot.mIsPlaying = false;
						}
					}
				}

				if (*slotSound != nullptr) {
					setSpeedModifySound(*slotSound, soundData, param2);
					if ((soundData->unk10 & 0x10) != 0
					    && soundData->unk8 <= param1) {
						(*slotSound)->stop(1);
					}
				}
			} else {
				slot.mIsPlaying = false;
			}
		}

		while (
		    mDataCounter < soundCount
		    && ((JAIAnimeFrameSoundData*)(data + 8 + mDataCounter * 0x20))->unk4
		        <= param1) {
			playActorAnimSound(basic, actor, param2, param3);
		}
	} else {
		if (mCurrentTime < param1) {
			while (
			    mDataCounter < soundCount
			    && ((JAIAnimeFrameSoundData*)(data + 8 + mDataCounter * 0x20))
			               ->unk4
			        >= mCurrentTime - param2) {
				playActorAnimSound(basic, actor, param2, param3);
			}
			mDataCounter = soundCount - 1;
			mCurrentTime = param1;
			if ((u32)mLoopCount == 0xFFFFFFFF || (u32)mLoopCount < 0x100)
				mLoopCount++;
		}

		for (u8 i = 0; i < 8; ++i) {
			Slot& slot = mSlots[i];
			JAISound** slotSound = &slot.mSound;
			if (slot.mIsPlaying) {
				JAIAnimeFrameSoundData* soundData
				    = (JAIAnimeFrameSoundData*)slot.mData;
				u32 soundID = soundData->mSoundID;
				if ((soundID & 0xC00) == 0) {
					if (param2 != 0.0f || (soundData->unk10 & 0x20) == 0) {
						f32 startFrame = soundData->unk4;
						f32 endFrame   = soundData->unk8;
						if (startFrame == endFrame
						    || (startFrame > endFrame && endFrame < param1
						        && startFrame > param1)
						    || (startFrame < endFrame
						        && (endFrame < param1 || startFrame > param1))) {
							startAnimSound(basic, soundID, slotSound, actor,
							               param3);
						} else {
							slot.mIsPlaying = false;
						}
					}
				}

				if (*slotSound != nullptr) {
					setSpeedModifySound(*slotSound, soundData, param2);
					if ((soundData->unk10 & 0x10) != 0
					    && soundData->unk8 >= param1) {
						(*slotSound)->stop(1);
					}
				}
			} else {
				slot.mIsPlaying = false;
			}
		}

		while (
		    mDataCounter < soundCount
		    && ((JAIAnimeFrameSoundData*)(data + 8 + mDataCounter * 0x20))->unk4
		        >= param1) {
			playActorAnimSound(basic, actor, param2, param3);
		}
	}

	mCurrentTime = param1;
}

void JAIAnimeSound::playActorAnimSound(JAIBasic* basic, JAIActor* actor,
                                       f32 param, u8 flag)
{
	JAIAnimeFrameSoundData* soundData
	    = (JAIAnimeFrameSoundData*)(mData + 8 + mDataCounter * 0x20);

	u8 slotIndex = 0;
	while (slotIndex < 8) {
		Slot& slot = mSlots[slotIndex];
		if (!slot.mIsPlaying)
			break;

		JAIAnimeFrameSoundData* slotData
		    = (JAIAnimeFrameSoundData*)slot.mData;
		if (soundData->mSoundID != slotData->mSoundID) {
			++slotIndex;
			continue;
		}

		if ((soundData->mSoundID & 0xC00) == 0) {
			mDataCounter += mDataCounterInc;
			return;
		}
		break;
	}

	if (slotIndex != 8) {
		u32 flags = soundData->unk10;
		if (((flags & 0x8) == 0 || (u32)mLoopCount == soundData->unk16)
		    && (((u32)mDataCounterInc == 1 && (flags & 0x2) == 0)
		        || ((u32)mDataCounterInc == 0xFFFFFFFF && (flags & 0x1) == 0))) {
			Slot& slot = mSlots[slotIndex];
			JAISound** slotSound = &slot.mSound;
			startAnimSound(basic, soundData->mSoundID, slotSound, actor, flag);
			if (*slotSound != nullptr) {
				slot.mData      = soundData;
				slot.mIsPlaying = true;
				(*slotSound)->setVolume(soundData->unk14 / 127.0f, 0, 5);
				f32 pitch = soundData->unk15;
				pitch = soundData->unkC
				        + pitch * (param - 1.0f) * 0.03125f;
				(*slotSound)->setPitch(pitch, 0, 5);
				(*slotSound)->setPan(soundData->unk17 / 127.0f, 0, 5);
			}
		}
	}

	mDataCounter += mDataCounterInc;
}

void JAIAnimeSound::startAnimSound(void* param_1, u32 param_2,
                                   JAISound** param_3, JAIActor* param_4,
                                   u8 param_5)
{
	JAIBasic* basic = (JAIBasic*)param_1;
	basic->startSoundActor(param_2, param_3, param_4, 0, param_5);
}

void JAIAnimeSound::setSpeedModifySound(JAISound* param_1,
                                        JAIAnimeFrameSoundData* param_2,
                                        f32 param_3)
{
	f32 fVar1 = param_2->unkC;
	if (param_2->unk15 != 0)
		fVar1 += param_2->unk15 * (param_3 - 1.0f) / 32.0f;
	param_1->setPitch(fVar1, 0, 5);

	s16 uVar2 = param_2->unk14;
	if (param_2->unk15 != 0) {
		uVar2 += (s16)((f32)param_2->unk18 * 2.0f * (param_3 - 1.0f));
		if (uVar2 > 0x7F)
			uVar2 = 0x7F;
		else if (uVar2 < 0)
			uVar2 = 0;
	}
	param_1->setVolume((u8)uVar2 / 127.0f, 0, 5);
}

void JAIAnimeSound::stop()
{
	for (u8 i = 0; i < 8; i++)
		if (mSlots[i].mSound)
			mSlots[i].mSound->stop(0);
}
