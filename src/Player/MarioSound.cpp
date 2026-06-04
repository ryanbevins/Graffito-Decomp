#include <Player/MarioMain.hpp>
#include <Map/MapData.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <System/Application.hpp>

class TNozzleBase {
public:
	u8 _pad[0x364];
	virtual void init();
	virtual s32 getNozzleKind() const;
};

class TWaterGun {
public:
	TNozzleBase* getCurrentNozzle() const;
};

TMarioSoundValues::TMarioSoundValues()
{
	unk00 = 0;
	unk04 = -1;
	unk08 = 0;
	unk0C = 0;
	unk10 = 0;
	unk14 = 0;
	unk18 = 0;
	unk1C = 0;
	unk20 = 0;
	unk22 = 0;
	unk24 = 0;
	unk26 = 0;
	unk14 = 0;
	unk29 = 0;
	unk2A = 1;
	unk2B = 0;
	unk2C = 0;
}

void TMario::startSoundActor(u32 soundID)
{
	MSound* sound = gpMSound;
	if (sound->gateCheck(soundID))
		MSoundSESystem::MSoundSE::startSoundActor(soundID, (const Vec*)&mPosition,
		                                          0, nullptr, 0, 4);
}

void TMario::stopVoice()
{
	MSound* sound = gpMSound;
	sound->stopMarioVoice(getVoiceStatus(), 0);
}

u32 TMario::startVoiceIfNoVoice(u32 soundID)
{
	MSound* sound = gpMSound;
	if (sound->getMarioVoiceID(0) == 0xffffffff) {
		if (onYoshi())
			return 0;

		sound = gpMSound;
		return sound->startMarioVoice(soundID, mHealth, getVoiceStatus());
	}

	return 0;
}

u32 TMario::startVoice(u32 soundID)
{
	if (onYoshi())
		return 0;

	MSound* sound = gpMSound;
	return sound->startMarioVoice(soundID, mHealth, getVoiceStatus());
}

u8 TMario::getVoiceStatus()
{
	if (onYoshi())
		return 1;

	switch (unk388) {
	case 1:
		return 2;
	case 2:
		return 6;
	default:
		return 0;
	}
}

void TMario::soundTorocco()
{
	JGeometry::TVec3<f32> diff = mPosition;
	diff.sub(mToroccoPos);
	JGeometry::TVec3<f32> distVec = diff;
	f32 dist = distVec.length();

	if (gpMSound->gateCheck(0x305a)) {
		MSoundSESystem::MSoundSE::startSoundActorWithInfo(
		    0x305a, (const Vec*)&mPosition, nullptr, dist, 0, 0, nullptr, 0,
		    4);
	}
}

void TMario::animSound()
{
	mSoundFlags = mGroundPlane->unk6;
	u32 state   = mState;

	bool hasState40 = (state & 0x40) ? true : false;
	if (hasState40) {
		if (unk350 == 0) {
			f32 limit = (f32)*(s16*)((u8*)this + 0x2428)
			            * *(f32*)((u8*)this + 0x24dc);
			if (unk368 >= limit) {
				mSoundFlags |= 0x600;
			} else {
				mSoundFlags |= 0x500;
			}
		} else {
			mSoundFlags |= 0x500;
		}
	} else {
		bool wetGround = (state & 0x10) ? true : false;
		if (!wetGround) {
			u16 bgType = mGroundPlane->mBGType;
			bool wetBGType;
			if (bgType == BG_TYPE_WET_GROUND
			    || bgType == BG_TYPE_SHADED_WET_GROUND
			    || bgType == BG_TYPE_CAM_NOCLIP_WET_GROUND
			    || bgType == BG_TYPE_CAM_NOCLIP_SHADED_WET_GROUND) {
				wetBGType = true;
			} else {
				wetBGType = false;
			}
			if (wetBGType)
				wetGround = true;
		}
		if (wetGround)
			mSoundFlags |= 0x700;
	}

	bool hasState30000 = (state & 0x30000) ? true : false;
	if (hasState30000) {
		if (*(f32*)((u8*)this + 0xf0) - mPosition.y > 30.0f)
			mSoundFlags |= 0x200;
		else
			mSoundFlags |= 0x100;
	}

	f32 speed = unk368;
	BOOL hasSpeed = speed > 0.0f ? TRUE : FALSE;
	if (hasSpeed) {
		f32 frameCount = (f32)*(s16*)((u8*)this + 0x2428);
		f32 scaled     = -(speed / frameCount) * *(f32*)((u8*)this + 0x2450);
		if (scaled > 30.0f)
			mSoundFlags |= 0x400;
		else
			mSoundFlags |= 0x300;
	}

	THitActor* held = *(THitActor**)((u8*)this + 0x68);
	bool holdingTurboNozzle;
	if (held != nullptr && held->mActorType == 0x40000098)
		holdingTurboNozzle = true;
	else
		holdingTurboNozzle = false;

	if (holdingTurboNozzle)
		mSoundFlags |= 0x15;

	bool hasState8000 = (state & 0x8000) ? true : false;
	if (!hasState8000)
		mSoundFlags |= 0x1000;

	if (unk388 == 1)
		mSoundFlags |= 0x20000000;
	else if (unk388 == 2)
		mSoundFlags |= 0x60000000;

	mSoundFlags += mHealth << 24;

	if (!onYoshi()) {
		f32 rate = mModel->unkC[0].getRate();
		mAnmSound->animeLoop((Vec*)&mPosition, getCurrentFrame(0), rate,
		                      mSoundFlags, 4);
	}
}

void TMario::soundMovement()
{
#define MARIO_START_SOUND(id, pos)                                             \
	do {                                                                       \
		u32 marioSoundID = (id);                                               \
		const Vec* marioSoundPos = (const Vec*)(pos);                         \
		if (gpMSound->gateCheck(marioSoundID))                                 \
			MSoundSESystem::MSoundSE::startSoundActor(                        \
			    marioSoundID, marioSoundPos, 0, nullptr, 0, 4);               \
	} while (0)

#define MARIO_START_SOUND_INFO(id, pos, volume)                                \
	do {                                                                       \
		u32 marioSoundID = (id);                                               \
		f32 vol     = (volume);                                                \
		if (gpMSound->gateCheck(marioSoundID))                                 \
			MSoundSESystem::MSoundSE::startSoundActorWithInfo(                \
			    marioSoundID, (const Vec*)(pos), nullptr, vol, 0, 0, nullptr,  \
			    0, 4);                                                         \
	} while (0)

#define MARIO_START_VOICE(id)                                                  \
	do {                                                                       \
		gpMSound->startMarioVoice((id), mHealth, getVoiceStatus());            \
	} while (0)

#define MARIO_START_VOICE_HP(high_id, low_id)                                  \
	do {                                                                       \
		if (mHealth > 2)                                                       \
			MARIO_START_VOICE(high_id);                                        \
		else                                                                   \
			MARIO_START_VOICE(low_id);                                         \
	} while (0)

#define MARIO_WATER_CURRENT(gun) (*(s32*)((u8*)(gun) + 0x1C80))
#define MARIO_WATER_NOZZLE_TYPE(gun) (*(u8*)((u8*)(gun) + 0x1C84))
#define MARIO_WATER_UNK1CEC(gun) (*(f32*)((u8*)(gun) + 0x1CEC))
#define MARIO_WATER_NOZZLE(gun)                                                \
	(*(u8**)((u8*)(gun) + 0x1C68 + MARIO_WATER_NOZZLE_TYPE(gun) * 4))
#define MARIO_NOZZLE_UNK378(nozzle) (*(f32*)((u8*)(nozzle) + 0x378))
#define MARIO_NOZZLE_TRIGGER_STATE(nozzle) (*(u8*)((u8*)(nozzle) + 0x385))

	u32 action = mAction;

	mSoundValues.unk18 = (*(u8*)((u8*)gpMSound + 0xA8) & 1) ? 0 : 1;

	if (onYoshi()) {
		TWaterGun* waterGun = mWaterGun;
		bool isSpraying     = false;

		if (MARIO_WATER_CURRENT(waterGun) == 0) {
			isSpraying = false;
		} else {
			TNozzleBase* nozzle = waterGun->getCurrentNozzle();
			if (nozzle->getNozzleKind() == 1) {
				nozzle = waterGun->getCurrentNozzle();
				if ((s32)MARIO_NOZZLE_TRIGGER_STATE(nozzle) == 1)
					isSpraying = true;
				else
					isSpraying = false;
			} else {
				nozzle = waterGun->getCurrentNozzle();
				if (MARIO_NOZZLE_UNK378(nozzle) > 0.0f)
					isSpraying = true;
				else
					isSpraying = false;
			}
		}

		if (isSpraying
		    && MARIO_NOZZLE_UNK378(mWaterGun->getCurrentNozzle()) > 0.0f)
			MARIO_START_SOUND(0x7129, &mYoshi->mTranslation);

		if (action & 0x40000) {
			MARIO_START_SOUND(0x1009, &mYoshi->mTranslation);

			if (action == ACTION_SLIP_FORE
			    && mSoundValues.unk00 != ACTION_SLIP_FORE)
				MARIO_START_SOUND(0x7926, &mYoshi->mTranslation);
		} else if (mSoundValues.unk00 == ACTION_SLIP_FORE) {
			MARIO_START_SOUND(0x792a, &mYoshi->mTranslation);
		}

		if ((action & 0x800) && !(mSoundValues.unk00 & 0x800))
			MARIO_START_SOUND(0x791d, &mYoshi->mTranslation);

		if (action == ACTION_HIP_ATTACK
		    && mSoundValues.unk00 != ACTION_HIP_ATTACK)
			MARIO_START_SOUND(0x792b, &mYoshi->mTranslation);

		if (action == ACTION_HIP_ATTACK_LAND
		    && mSoundValues.unk00 == ACTION_HIP_ATTACK)
			MARIO_START_SOUND(0x198d, &mYoshi->mTranslation);

		if (mSoundValues.unk00 != ACTION_HIP_ATTACK) {
			bool aPressed     = (mInput & 4) != 0;
			bool prevAPressed = (mSoundValues.unk0C & 4) != 0;

			if (aPressed && !prevAPressed)
				mSoundValues.unk1C = 0;

			if (aPressed)
				mSoundValues.unk1C++;

			if (mSoundValues.unk20 != 0) {
				mSoundValues.unk20--;
				if (mSoundValues.unk20 == 0) {
					if (!aPressed)
						MARIO_START_SOUND(0x195a, &mYoshi->mTranslation);
				} else if (mSoundValues.unk20 == 4) {
					if (!aPressed) {
						MARIO_START_SOUND(0x195a,
						                  &mYoshi->mTranslation);
						gpMSound->startMarioVoice(0x792a, 1, 1);
					}
					mSoundValues.unk20 = 0;
				}
			}

			if (!aPressed && prevAPressed) {
				if (mSoundValues.unk1C > 120 && mForwardVel < -74.0f)
					mSoundValues.unk20 = 8;
				else
					mSoundValues.unk20 = 4;
			}
		}
	} else {
		bool backFlip = action == ACTION_BACKFLIP
		                || action == ACTION_BACKFLIP_LEFT;
		if (backFlip) {
			if (mSoundValues.unk04 & 0x10) {
				MARIO_START_VOICE(0x78b9);
				mSoundValues.unk04 ^= 0x10;
			}
		} else {
			mSoundValues.unk04 |= 0x10;
		}

		if (backFlip) {
			mSoundValues.unk22++;
			if (mSoundValues.unk22 == 10)
				MARIO_START_SOUND(0x198e, &mPosition);
		} else {
			mSoundValues.unk22 = 0;
		}

		if (action == ACTION_GROUND_POUND_BOUNCE
		    && mSoundValues.unk08 != ACTION_GROUND_POUND_BOUNCE
		    && mGroundPlane != nullptr && mGroundPlane->mActor != nullptr) {
			gpMSound->startForceJumpSound((Vec*)&mPosition, mSoundFlags,
			                              0.0f, mGroundPlane->mData);
		}

		u8 holdingKind = 0;
		if (mSoundValues.unk00 == 0x383)
			holdingKind = 1;
		if (mSoundValues.unk00 == ACTION_NPC_TALK)
			holdingKind = 2;

		if (holdingKind != 0) {
			if (mHeldObject != nullptr) {
				u32 actorType = mHeldObject->mActorType;
				bool normalTakeObject
				    = actorType == 0x40000046
				      || (actorType >= 0x4000005a
				          && actorType < 0x4000005e)
				      || (actorType >= 0x40000064
				          && actorType < 0x40000066);
				bool npcTakeObject = actorType >= 0x40000390
				                     && actorType < 0x40000397;

				if (npcTakeObject) {
					if (mSoundValues.unk04 & 0x40) {
						if (holdingKind == 1)
							MARIO_START_SOUND(0x194d, &mPosition);
						else
							MARIO_START_SOUND(0x194e, &mPosition);
						mSoundValues.unk04 ^= 0x40;
					}
				} else if (normalTakeObject) {
					if (mSoundValues.unk04 & 0x40) {
						if (holdingKind == 1)
							MARIO_START_SOUND(0x1800, &mPosition);
						else
							MARIO_START_SOUND(0x194c, &mPosition);
						mSoundValues.unk04 ^= 0x40;
					}
				}
			}
		} else {
			mSoundValues.unk04 |= 0x40;
		}

		if (action == ACTION_FOOT_DOWNING) {
			if (mSoundValues.unk04 & 4) {
				MARIO_START_SOUND(0x197b, &mPosition);
				mSoundValues.unk04 ^= 4;
			}
		} else {
			mSoundValues.unk04 |= 4;
		}

		if (action == ACTION_FOOT_DOWNING
		    && mSoundValues.unk00 != ACTION_FOOT_DOWNING) {
			MARIO_START_VOICE(0x783b);
			gpMSound->stopMarioVoice(getVoiceStatus(), 0);
		}

		bool catchStopTrigger = false;
		if (mSoundValues.unk00 == ACTION_CATCH_STOP) {
			if (!(mInput & 4) && (mSoundValues.unk0C & 4))
				catchStopTrigger = true;
		}

		if (mSoundValues.unk24 != 0)
			mSoundValues.unk24--;

		if (catchStopTrigger) {
			MARIO_START_SOUND(0x193e, &mPosition);
			if (mSoundValues.unk24 == 0) {
				MARIO_START_VOICE(0x783b);
				mSoundValues.unk24 = 0xb4;
			}
		}

		if (action == ACTION_CATCH_STOP)
			mSoundValues.unk26++;
		else
			mSoundValues.unk26 = 0;

		if (mSoundValues.unk14 == 0x10000015) {
			if (mSoundValues.unk26 == 60)
				MARIO_START_VOICE(0x786b);
		} else if (mSoundValues.unk26 == 30) {
			MARIO_START_VOICE(0x78bf);
		}

		if (mColCount != 0 && mCollisions[0] != nullptr) {
			THitActor* hitActor = mCollisions[0];
			mSoundValues.unk29 = 4;
			mSoundValues.unk14 = hitActor->mActorType;
			if (hitActor->mActorType & 0x4000000)
				mSoundValues.unk28 = 1;
			else
				mSoundValues.unk28 = 2;
		} else {
			if (mSoundValues.unk29 != 0)
				mSoundValues.unk29--;
			if (mSoundValues.unk29 == 0)
				mSoundValues.unk28 = 0;
		}

		bool downAction = false;
		if (action & 0x20000) {
			downAction = action == ACTION_DOWNING_7
			             || action == ACTION_FIRE_DOWNING
			             || action == ACTION_BOARD_JUMPING;

			if (mSoundValues.unk00 == ACTION_RUNNING
			    && action == ACTION_DOWNING_3
			    && (mSoundValues.unk04 & 8)) {
				MARIO_START_VOICE(0x783b);
			}
		}

		if (downAction) {
			if (mSoundValues.unk04 & 8)
				MARIO_START_VOICE(0x783b);

			if (mSoundValues.unk28 == 0) {
				if (mSoundValues.unk04 & 8) {
					MARIO_START_SOUND(0x1965, &mPosition);
					mSoundValues.unk04 ^= 8;
				}
			} else if (mSoundValues.unk28 == 1) {
				if (mSoundValues.unk04 & 8) {
					MARIO_START_SOUND(0x1949, &mPosition);
					mSoundValues.unk04 ^= 8;
				}
			} else if (mSoundValues.unk28 == 2) {
				if (mSoundValues.unk04 & 8) {
					MARIO_START_SOUND(0x1948, &mPosition);
					mSoundValues.unk04 ^= 8;
				}
			}
		} else {
			mSoundValues.unk04 |= 8;
		}

		if ((mInput & 0x100) && MARIO_WATER_CURRENT(mWaterGun) == 0)
			MARIO_START_SOUND(0x802, &mPosition);

		s32 waterPressure = (s32)(100.0f * MARIO_WATER_UNK1CEC(mWaterGun));
		u8 waterPressureByte = waterPressure;

		if (mSoundValues.unk2A == 0 && waterPressureByte != 0)
			MARIO_START_SOUND(0x807, &mPosition);

		if (mSoundValues.unk2A == 0x12 && waterPressureByte == 0x13
		    && MARIO_WATER_NOZZLE_TYPE(mWaterGun) == 0)
			MARIO_START_SOUND(0x808, &mPosition);

		if (mSoundValues.unk2A == 0x31 && waterPressureByte == 0x32)
			MARIO_START_SOUND(0x815, &mPosition);

		mSoundValues.unk2A = waterPressureByte;

		if (isWearingCap()) {
			if (!(mSoundValues.unk04 & 0x80))
				MARIO_START_SOUND(0x1982, &mPosition);
			mSoundValues.unk04 |= 0x80;
		} else {
			if (mSoundValues.unk04 & 0x80)
				MARIO_START_SOUND(0x1983, &mPosition);
			if (mSoundValues.unk04 & 0x80)
				mSoundValues.unk04 ^= 0x80;
		}

		if (action == 0x560 || action == 0x4000561
		    || action == ACTION_SLIDE_JUMP) {
			if (mSoundValues.unk00 != 0x560
			    && mSoundValues.unk00 != 0x4000561
			    && mSoundValues.unk00 != ACTION_SLIDE_JUMP) {
				MARIO_START_VOICE_HP(0x788f, 0x78fb);
				mSoundValues.unk2B = 0x78;
			} else if (action == ACTION_SLIDE_JUMP
			           && mSoundValues.unk00 != ACTION_SLIDE_JUMP) {
				MARIO_START_VOICE_HP(0x7807, 0x7903);
			}
		}

		if (mSoundValues.unk2B != 0)
			mSoundValues.unk2B--;

		if (mHeldObject != nullptr && (action & 0x400)
		    && mSoundValues.unk2B == 0 && mSoundValues.unk18 == 0)
			MARIO_START_VOICE(0x7094);

		bool dryGround = true;
		if (mGroundPlane != nullptr) {
			u16 bgType = mGroundPlane->mBGType;
			if (bgType == BG_TYPE_WATER || bgType == BG_TYPE_DAMAGING_WATER
			    || bgType == BG_TYPE_SEA_WATER
			    || bgType == BG_TYPE_DAMAGING_SEA_WATER
			    || bgType == BG_TYPE_POOL
			    || bgType == BG_TYPE_INDOOR_POOL
			    || bgType == BG_TYPE_SHADED_POOL) {
				dryGround = false;
			} else if (bgType == BG_TYPE_SEA_WATER
			           || bgType == BG_TYPE_DAMAGING_SEA_WATER) {
				dryGround = false;
			} else {
				dryGround = true;
			}
		}

		if (action & 0x10000) {
			if (!(mSoundValues.unk00 & 0x10000)) {
				mSoundValues.unk2C = 30;
				if (mSoundValues.unk04 & 0x200)
					mSoundValues.unk04 ^= 0x200;
			}

			if (mSoundValues.unk2C != 0) {
				MARIO_START_SOUND(0x27, &mPosition);
				mSoundValues.unk2C--;
				if (mSoundValues.unk2C == 0)
					MARIO_START_SOUND(0x826, &mPosition);
			}

			MARIO_START_SOUND_INFO(0x117e, &mPosition, mForwardVel);
		} else {
			mSoundValues.unk2C = 0;
			mSoundValues.unk04 |= 0x200;
		}

		if (action == ACTION_SURFING) {
			mSoundValues.unk04 |= 0x100;
			if (mSoundValues.unk00 == ACTION_BOARD_JUMP && !dryGround) {
				MARIO_START_SOUND(0x828, &mPosition);
			} else {
				u32 soundID = dryGround ? 0x117f : 0x117d;
				if (mSoundValues.unk2C == 0)
					MARIO_START_SOUND_INFO(soundID, &mPosition,
					                       mForwardVel);
			}
		} else if (action == ACTION_BOARD_JUMP && !dryGround
		           && (mSoundValues.unk04 & 0x100)) {
			MARIO_START_SOUND(0x828, &mPosition);
			mSoundValues.unk04 ^= 0x100;
		}

		if (mState & 2) {
			if (action == ACTION_GROUND_POUND_BOUNCE
			    && mSoundValues.unk00 != ACTION_GROUND_POUND_BOUNCE)
				MARIO_START_VOICE_HP(0x78b9, 0x7907);
		} else {
			if (action == ACTION_GROUND_POUND_BOUNCE
			    && mSoundValues.unk00 != ACTION_GROUND_POUND_BOUNCE
			    && gpApplication.mCurrArea.unk0 == 2) {
				MARIO_START_SOUND(0x1812, &mPosition);
				MARIO_START_VOICE_HP(0x78b9, 0x7907);
			}
		}

		if ((mSoundValues.unk00 & 0x800) && action == ACTION_HANGING)
			MARIO_START_VOICE_HP(0x7890, 0x78fc);

		if (mSoundValues.unk00 != ACTION_ROPE_POSITION
		    && action == ACTION_ROPE_POSITION)
			MARIO_START_VOICE(0x784f);

		if (mSoundValues.unk00 == ACTION_JUMP_SLIP_1
		    && action == ACTION_SLIP_FALLING)
			MARIO_START_VOICE(0x7913);

		if (mSoundValues.unk00 & 0x800) {
			if (action == ACTION_WIRE_SWING_HOLD
			    || (action >= ACTION_WIRE_SHANG_R
			        && action < 0x10000557)) {
				MARIO_START_VOICE_HP(0x7890, 0x78f2);
			} else if (action >= ACTION_WIRE_WAIT
			           && action < 0x354
			           && (u32)(mSoundValues.unk00 - 0x892) > 1) {
				MARIO_START_VOICE_HP(0x788f, 0x78f3);
			}
		}

		if ((action == ACTION_SLIP_FORE || action == ACTION_SLIP_BACK)
		    && mSoundValues.unk18 == 0 && !isRunningInWater())
			MARIO_START_SOUND(0x1009, &mPosition);

		if (mSoundValues.unk00 == ACTION_HIP_ATTACK
		    && action != ACTION_HIP_ATTACK) {
			if (!(action & 0x20000) && !(action & 0x80000000))
				MARIO_START_SOUND(0x180e, &mPosition);
		} else if (action == ACTION_FENCE_SLIDE
		           && mSoundValues.unk00 == ACTION_HIP_ATTACK) {
			MARIO_START_SOUND(0x180e, &mPosition);
		}

		u32 wireAction = mAction & 0x1ff;
		if (wireAction >= 0x150 && wireAction <= 0x15c
		    && action != 0x10000358) {
			if (mWireBounceVelPrev > 0.0f && mWireBounceVel <= 0.0f) {
				MARIO_START_SOUND_INFO(0x381c, &mWireStartPos, mWireSag);
				mWireSfxTimer   = mWireSfxDelay;
				mWireQueuedSfxID = 0x381c;
			}

			if (mWireBounceVelPrev < 0.0f && mWireBounceVel >= 0.0f) {
				MARIO_START_SOUND_INFO(0x381d, &mWireStartPos, mWireSag);
				mWireSfxTimer   = mWireSfxDelay;
				mWireQueuedSfxID = 0x381d;
			}
		}

		if (mWireSfxTimer != 0) {
			mWireSfxTimer--;
			if (mWireSfxTimer == 0)
				MARIO_START_SOUND_INFO(mWireQueuedSfxID, &mWireEndPos,
				                       mWireSag);
		}

		if (action == ACTION_WARP_IN && mActionState == 0
		    && mActionTimer == 1)
			MARIO_START_SOUND(0x1979, &mPosition);

		if (action == ACTION_ELECTRIC_DAMAGE && mActionState == 0)
			MARIO_START_SOUND(0x1814, &mPosition);
	}

	mSoundValues.unk08 = mAction;
	mSoundValues.unk00 = action;
	mSoundValues.unk0C = mInput;
	mSoundValues.unk10 = unk78;

#undef MARIO_START_SOUND
#undef MARIO_START_SOUND_INFO
#undef MARIO_START_VOICE
#undef MARIO_START_VOICE_HP
#undef MARIO_WATER_CURRENT
#undef MARIO_WATER_NOZZLE_TYPE
#undef MARIO_WATER_UNK1CEC
#undef MARIO_WATER_NOZZLE
#undef MARIO_NOZZLE_UNK378
#undef MARIO_NOZZLE_TRIGGER_STATE
}
