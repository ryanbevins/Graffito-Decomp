#define JGEOMETRY_MARIORECEIVEMSG_TVEC3_SUB_OUT_OF_LINE
#define JSULIST_DTOR_DECL_ONLY
#include <Player/MarioMain.hpp>
#undef JGEOMETRY_MARIORECEIVEMSG_TVEC3_SUB_OUT_OF_LINE
#include <Strategic/HitActor.hpp>
#include <Strategic/LiveActor.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>
#include <Map/Map.hpp>
#include <Map/MapWireManager.hpp>
#include <Player/MarioCap.hpp>
#include <Player/Yoshi.hpp>
#include <Player/Watergun.hpp>
#include <System/MarDirector.hpp>
#include <System/Particles.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <MoveBG/MapObjManager.hpp>
#include <M3DUtil/MActor.hpp>
#include <MarioUtil/MathUtil.hpp>
#undef JSULIST_DTOR_DECL_ONLY

// NOTE: -inline deferred, so reverse address order.
// getGesso is after receiveMessage in the binary.

static inline bool getNozzle(TMario* mario, THitActor* sender,
                             TWaterGun::TNozzleType type)
{
	if (mario->onYoshi())
		return false;

	mario->mState |= 0x8000;
	if (mario->checkFlag(MARIO_FLAG_HAS_FLUDD))
		mario->mWaterGun->changeNozzle(type, true);

	mario->unk144 = 3600;
	mario->resetNozzle();
	mario->unk148 = (u32)sender;
	if (mario->checkFlag(MARIO_FLAG_HAS_FLUDD))
		mario->mWaterGun->resetWaterToFull();

	mario->emitGetEffect();
	return true;
}

BOOL TMario::receiveMessage(THitActor* sender, u32 message)
{
	if (checkFlag(MARIO_FLAG_GAME_OVER))
		return 0;

	// 0x50: Check sender has ACTOR_TYPE_UNK4000000 (bit 2)
	bool hasEarlyItemType = sender->checkActorType(0x20000000);
	if (hasEarlyItemType) {
		u32 senderType = sender->mActorType;
		// List of actor types that should NOT trigger the sound
		bool skip = true;
		if ((senderType - 0x20000000) == 0x0E) skip = 0;
		if ((senderType - 0x20000000) == 0x0F) skip = 0;
		if ((senderType - 0x20000000) == 0x10) skip = 0;
		if ((senderType - 0x20000000) == 0x11) skip = 0;
		if ((senderType - 0x20000000) == 0x13) skip = 0;
		if ((senderType - 0x20000000) == 0x1F) skip = 0;
		if ((senderType - 0x20000000) == 0x26) skip = 0;
		if ((senderType - 0x20000000) == 0x22) skip = 0;
		if ((senderType - 0x20000000) == 0x2A) skip = 0;

		if (skip == true) {
			if (gpMSound->gateCheck(0x180C)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x180C, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			}
		}
	}

	// message == 7: grab/take
	if (message == 7) {
		u32 aType = sender->mActorType;

		BOOL playTakeSound = (aType & 0x80000000) ? true : false;
		if (!playTakeSound) {
			playTakeSound = (aType & 0x04000000) ? true : false;
			if (!playTakeSound) {
				playTakeSound = (aType & 0x10000000) ? true : false;
				if (!playTakeSound)
					playTakeSound = (aType & 0x08000000) ? true : false;
			}
		}

		// Play sound 0x193D
		if ((bool)playTakeSound && gpMSound->gateCheck(0x193D)) {
			MSoundSESystem::MSoundSE::startSoundActor(
			    0x193D, (const Vec*)&mPosition, 0,
			    (JAISound**)&mSound, 0, 4);
		}

		// Check actor type 0x10000015 (specific actor)
		if (sender->mActorType - 0x10000000 == 0x15) {
			startVoice(0x78CF);
		}

		// Check if sender has bit 0x40000000
		bool hasObjectType = sender->checkActorType(0x40000000);
		if (hasObjectType) {
			if (gpMSound->getMarioVoiceID(0) != 0x78D3) {
				startVoice(0x78D3);
			}
		}

		// Check onYoshi
		if (onYoshi()) {
			if (gpMSound->gateCheck(0x791C)) {
				MSoundSESystem::MSoundSE::startSoundActor(
				    0x791C, (const Vec*)&mPosition, 0, nullptr, 0, 4);
			}
		}

		changePlayerStatus(0x000208B8, 0, false);
		return 1;
	}

	// After message 7, check sender type bits for enemy contact
	u32 senderType = sender->mActorType;

	bool hasItemType = (senderType & 0x20000000) ? true : false;
	if (hasItemType) {
		// Jump table based on (senderType - 0x20000001) with 60 entries
		u32 tableIdx = (senderType - 0x20000001);
		if (tableIdx > 59)
			goto check_sender_bit3;

		switch (senderType) {
		case 0x20000001: // Enemy type 1
			if (message != 0x0E)
				goto check_sender_bit3;
			// Check MARIO_FLAG_HAS_FLUDD
			if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
				mWaterGun->addWater(mWaterGun->getMaxWater() / 2);
			}
			emitGetWaterEffect();
			return 1;

		case 0x20000002: // Enemy type 2
			if (message != 0x0E)
				goto check_sender_bit3;
			// Same pattern but multiply by full rate
			if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
				mWaterGun->addWater(mWaterGun->getMaxWater());
			}
			emitGetWaterEffect();
			return 1;

		case 0x2000002C: // HP+8
			if (message != 0x0E)
				goto check_sender_bit3;
			incHP(8);
			emitGetEffect();
			return 1;

		case 0x20000003: // HP+4
			if (message != 0x0E)
				goto check_sender_bit3;
			incHP(4);
			// Check FLUDD
			if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
				mWaterGun->addWater(mWaterGun->getMaxWater());
			}
			emitGetEffect();
			return 1;

		case 0x20000004: // HP+1
			if (message != 0x0E)
				goto check_sender_bit3;
			incHP(1);
			emitGetEffect();
			return 1;

		case 0x20000005:
		case 0x20000006:
		case 0x20000007: // Special item with blooper check
			if (message != 0x0E)
				goto check_sender_bit3;
			// Check blooper not-stuck and timer < 120
			if (*(s8*)((u8*)sender + 0x13A) == 0
			    && !(*(s32*)((u8*)sender + 0x13C) < 120 ? true : false)) {
				mHealth = mDeParams.mHpMax.get();
				// Check FLUDD
				if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
					mWaterGun->addWater(mWaterGun->getMaxWater());
				}
				TFlagManager::smInstance->incMario(1);
				emitGetEffect();
				return 1;
			}
			return 0;

		case 0x20000022: // Nozzle pickup - Rocket
			return getNozzle(this, sender, TWaterGun::Rocket);

		case 0x20000026: // Nozzle pickup - Hover
			return getNozzle(this, sender, TWaterGun::Hover);

		case 0x2000002A: // Nozzle pickup - Turbo
			return getNozzle(this, sender, TWaterGun::Turbo);

		case 0x2000002B: // Underwater nozzle (changePlayerStatus + pickup)
			changePlayerStatus(0x891, 0, false);
			return getNozzle(this, sender, TWaterGun::Underwater);

		case 0x2000001F: // Nozzle pickup - Spray
			getNozzle(this, sender, TWaterGun::Spray);
			return 1;

		case 0x2000003C: // Yoshi release flag
		{
			mCap->setModelActive(TMarioCap::E_CAP_MODEL_HAT);
			mHealth = mDeParams.mHpMax.get();
			emitGetEffect();
			return 1;
		}

		case 0x2000000E: // Coin pickup
		{
			mCoinCount++;
			emitGetCoinEffect((JGeometry::TVec3<f32>*)&mPosition);
			incHP(1);

			s32 coinFlag = TFlagManager::smInstance->getFlag(0x40002);
			// Check if coins are multiple of 50
			if (coinFlag % 50 == 0) {
				TFlagManager::smInstance->incMario(1);
				if (gpMSound->gateCheck(0x4841)) {
					MSoundSESystem::MSoundSE::startSoundSystemSE(0x4841, 0, nullptr, 0);
				}
			}
			return 1;
		}

		case 0x2000000F: // Red coin
		{
			incHP(2);
			emitGetCoinEffect((JGeometry::TVec3<f32>*)&mPosition);
			s32 redCoins = TFlagManager::smInstance->getFlag(0x60000);
			s32 adjustedCoins = redCoins + 70;
			gpMarioParticleManager->emitAndBindToPosPtr(adjustedCoins, (const JGeometry::TVec3<f32>*)&mPosition, 0, this);
			return 1;
		}

		case 0x20000010: // Blue coin
		{
			incHP(2);
			emitGetCoinEffect((JGeometry::TVec3<f32>*)&mPosition);
			return 1;
		}

		case 0x20000013: // Wire actor
		{
			if (message != 0x0E)
				goto check_sender_bit3;
			if (mStatus == 0x1302)
				goto check_sender_bit3;

			unk384 = sender;
			mPosition.x = sender->mPosition.x;
			mPosition.z = sender->mPosition.z;

			s16 angle = (s16)(65536.0f * *(f32*)((u8*)sender + 0x11C));
			mFaceAngle.y = angle;
			mModelFaceAngle = mFaceAngle.y;

			setPlayerVelocity(0.0f);
			mHealth = mDeParams.mHpMax.get();
			unk12C = unk130;
			changePlayerStatus(0x1302, 0, true);
			return 1;
		}

		default:
			goto check_sender_bit3;
		}
	}

check_sender_bit3:
	senderType = sender->mActorType;

	bool hasMapObjType = (senderType & 0x40000000) ? true : false;
	if (hasMapObjType) {
		switch (senderType) {
		case 0x4000001D:
			if (message == 0x0A) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsLampTrapIron.mDamage.get(),
					           mDmgParamsLampTrapIron.mDownType.get(),
					           mDmgParamsLampTrapIron.mWaterEmit.get(),
					           mDmgParamsLampTrapIron.mMinSpeed.get(),
					           mDmgParamsLampTrapIron.mMotor.get(),
					           mDmgParamsLampTrapIron.mDirty.get(),
					           mDmgParamsLampTrapIron.mInvincibleTime.get());

					changePlayerStatus(0x000208B7, 1, false);

					if (gpMSound->gateCheck(0x1813)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1813, (const Vec*)&mPosition, 0, nullptr, 0, 4);
					}
					gpMarioParticleManager->emitAndBindToPosPtr(
					    6, (const JGeometry::TVec3<f32>*)&mPosition, 0, nullptr);
					return 1;
				}
			}
			break;

		case 0x4000001E:
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsLampTrapSpike.mDamage.get(),
					           mDmgParamsLampTrapSpike.mDownType.get(),
					           mDmgParamsLampTrapSpike.mWaterEmit.get(),
					           mDmgParamsLampTrapSpike.mMinSpeed.get(),
					           mDmgParamsLampTrapSpike.mMotor.get(),
					           mDmgParamsLampTrapSpike.mDirty.get(),
					           mDmgParamsLampTrapSpike.mInvincibleTime.get());
					return 1;
				}
			}
			break;

		case 0x400000C5:
		case 0x400000C6:
		case 0x400000C7:
			getGesso(sender);
			return 1;

		case 0x40000258:
			if (message == 4) {
				mHolder = (TTakeActor*)sender;
				changePlayerStatus(0x133E, 0, false);
				return 1;
			}
			break;

		case 0x4000025B:
			if (message == 0x0A) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsFire.mDamage.get(),
					           mDmgParamsFire.mDownType.get(),
					           mDmgParamsFire.mWaterEmit.get(),
					           mDmgParamsFire.mMinSpeed.get(),
					           mDmgParamsFire.mMotor.get(),
					           mDmgParamsFire.mDirty.get(),
					           mDmgParamsFire.mInvincibleTime.get());
					startVoice(0x78CF);
					changePlayerStatus(0x000208B8, 1, false);
					return 1;
				}
				return 0;
			}
			break;

		case 0x40000098:
		{
			if (mHolder == nullptr) {
				if (mStatus == 0x892 && mVel.y > 0.0f)
					return 0;

				if (mStatus == 0x893 && mVel.y > 0.0f)
					return 0;

				if (mStatus == 0x000208BA)
					return 0;

				if (onYoshi())
					return 0;

				mHolder = (TTakeActor*)sender;

				TMapWireActor* wireActor = (TMapWireActor*)sender;
				wireActor->getTipPoints(&mWireStartPos, &mWireEndPos);
				mWirePosRatio = wireActor->getPosInWire();

				wireMove(0.0f);
				mState &= ~0x100;

				JGeometry::TVec3<f32> wireDiff = mWireEndPos - mWireStartPos;
				int angleDiff
				    = (s16)(matan(wireDiff.z, wireDiff.x) - mFaceAngle.y);

				mWireBounceVel = 0.2f * -mVel.y;
				mWireSag       = 0.0f;

				bool hangFromWire;
				if (mStatus == 0x893 || mStatus == ACTION_DIVE_RECOVERY) {
					hangFromWire = true;
				} else if (mVel.y < 0.0f) {
					hangFromWire = false;
				} else {
					hangFromWire = true;
				}

				if (hangFromWire == true) {
					if (angleDiff > 0) {
						JGeometry::TVec3<f32> temp = mWireStartPos;
						mWireStartPos              = mWireEndPos;
						mWireEndPos                = temp;
						mWirePosRatio              = 1.0f - mWirePosRatio;
					}

					changeWireHanging();
					return 1;
				}

				if (angleDiff <= -0x4000 || angleDiff > 0x4000) {
					JGeometry::TVec3<f32> temp = mWireStartPos;
					mWireStartPos              = mWireEndPos;
					mWireEndPos                = temp;
					mWirePosRatio              = 1.0f - mWirePosRatio;
				}

				changePlayerStatus(0x350, 0, false);
				return 1;
			}

			return 0;
		}
		}
	}

	// Check sender type bit 3 (0x10000000 = ACTOR_TYPE_ENEMY)
	if (sender->checkActorType(0x10000000)) {
		// Large binary search tree on enemy actor types
		// This is an enormous switch statement with many cases
		// Each case calls damageExec with different TEParams

		switch (sender->getActorType()) {
		case 0x10000003: // Enemy common
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsNamekuri.mDamage.get(),
					           mDmgParamsNamekuri.mDownType.get(),
					           mDmgParamsNamekuri.mWaterEmit.get(),
					           mDmgParamsNamekuri.mMinSpeed.get(),
					           mDmgParamsNamekuri.mMotor.get(),
					           mDmgParamsNamekuri.mDirty.get(),
					           mDmgParamsNamekuri.mInvincibleTime.get());
					return 1;
				}
			}
			break;

		case 0x10000013: // Namekuri (snail enemy)
		case 0x10000037:
			if (message == 0x0E) {
				if (!isInvincible()) {
					mCap->setModelInactive(TMarioCap::E_CAP_MODEL_HAT);
					// damageExec with EnemyCommon params
					damageExec(sender,
					           mDmgParamsEnemyCommon.mDamage.get(),
					           mDmgParamsEnemyCommon.mDownType.get(),
					           mDmgParamsEnemyCommon.mWaterEmit.get(),
					           mDmgParamsEnemyCommon.mMinSpeed.get(),
					           mDmgParamsEnemyCommon.mMotor.get(),
					           mDmgParamsEnemyCommon.mDirty.get(),
					           mDmgParamsEnemyCommon.mInvincibleTime.get());
					return 1;
				}
			}
			break;

		case 0x10000002: // Hamakuri
		case 0x4000019B: // Also hamakuri variant
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsHamakuri.mDamage.get(),
					           mDmgParamsHamakuri.mDownType.get(),
					           mDmgParamsHamakuri.mWaterEmit.get(),
					           mDmgParamsHamakuri.mMinSpeed.get(),
					           mDmgParamsHamakuri.mMotor.get(),
					           mDmgParamsHamakuri.mDirty.get(),
					           mDmgParamsHamakuri.mInvincibleTime.get());
					return 1;
				}
			}
			break;

		case 0x1000000A: // Hinokuri
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsHamakuri.mDamage.get(),
					           mDmgParamsHamakuri.mDownType.get(),
					           mDmgParamsHamakuri.mWaterEmit.get(),
					           mDmgParamsHamakuri.mMinSpeed.get(),
					           mDmgParamsHamakuri.mMotor.get(),
					           mDmgParamsHamakuri.mDirty.get(),
					           mDmgParamsHamakuri.mInvincibleTime.get());
					return 1;
				}
			}
			// Fall through to keepDistance

		case 0x1000000B:
		case 0x10000021:
		case 0x10000034:
			if (message == 9) {
				if (!isInvincible()) {
					elecEffect();
					changePlayerStatus(0x20338, 0, false);
					return 1;
				}
			}
			keepDistance(sender->mPosition, 30.0f + sender->getDamageRadius(),
			             0.0f);
			return 1;

		case 0x1000001F: // Boss (damageExec with sound + particles)
			if (message == 0x0A || message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsKiller.mDamage.get(),
					           mDmgParamsKiller.mDownType.get(),
					           mDmgParamsKiller.mWaterEmit.get(),
					           mDmgParamsKiller.mMinSpeed.get(),
					           mDmgParamsKiller.mMotor.get(),
					           mDmgParamsKiller.mDirty.get(),
					           mDmgParamsKiller.mInvincibleTime.get());

					changePlayerStatus(0x000208B7, 1, false);

					if (gpMSound->gateCheck(0x1813)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1813, (const Vec*)&mPosition, 0, nullptr, 0, 4);
					}
					gpMarioParticleManager->emitAndBindToPosPtr(
					    6, (const JGeometry::TVec3<f32>*)&mPosition, 0, nullptr);
					return 1;
				}
			}
			break;

		case 0x0800000F:
		case 0x08000010:
		case 0x08000011:
		case 0x08000012:
		case 0x10000004: // Common damage with keepDistance
		case 0x10000006:
		case 0x10000007:
		case 0x10000008:
		case 0x10000009:
		case 0x1000000C:
		case 0x10000010:
		case 0x10000012:
		case 0x10000014:
		case 0x10000016:
		case 0x10000017:
		case 0x10000018:
		case 0x10000019:
		case 0x1000001A:
		case 0x1000001B:
		case 0x1000001C:
		case 0x1000001D:
		case 0x10000020:
		case 0x10000022:
		case 0x10000024:
		case 0x10000025:
		case 0x10000029:
		case 0x1000002A:
		case 0x1000002C:
		case 0x1000002D:
		case 0x1000002E:
		case 0x1000002F:
		case 0x10000033:
		case 0x10000036:
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsEnemyCommon.mDamage.get(),
					           mDmgParamsEnemyCommon.mDownType.get(),
					           mDmgParamsEnemyCommon.mWaterEmit.get(),
					           mDmgParamsEnemyCommon.mMinSpeed.get(),
					           mDmgParamsEnemyCommon.mMotor.get(),
					           mDmgParamsEnemyCommon.mDirty.get(),
					           mDmgParamsEnemyCommon.mInvincibleTime.get());
					return 1;
				}
			}
			// Fall through to keepDistance
			keepDistance(sender->mPosition, 30.0f + sender->getDamageRadius(),
			             0.0f);
			return 1;

		case 0x10000053: // Special boss
		case 0x40000053:
		{
			// Check mState bits 14-15
			bool hasStateBits = (mState & 0x00030000) ? true : false;
			if (!hasStateBits)
				break;
			if (message != 0x0E)
				break;
			if (!isInvincible()) {
				damageExec(sender,
				           mDmgParamsEnemyCommon.mDamage.get(),
				           mDmgParamsEnemyCommon.mDownType.get(),
				           mDmgParamsEnemyCommon.mWaterEmit.get(),
				           mDmgParamsEnemyCommon.mMinSpeed.get(),
				           mDmgParamsEnemyCommon.mMotor.get(),
				           mDmgParamsEnemyCommon.mDirty.get(),
				           mDmgParamsEnemyCommon.mInvincibleTime.get());
				return 1;
			}
			break;
		}

		case 0x1000000D:
		case 0x1000002B: // Water surface damage (with poison flag)
			if (message == 5) {
				if (checkFlag(MARIO_FLAG_HAS_FLUDD)) {
					u16 flags2 = *(u16*)((u8*)mWaterGun + 4);
					*(u16*)((u8*)mWaterGun + 4) = flags2 | 4;
				}
				break;
			}
			break;

		case 0x10000015: // Poihana
			if (!isInvincible()) {
				if (message != 0x0E)
					break;
				damageExec(sender,
				           mDmgParamsPoihana.mDamage.get(),
				           mDmgParamsPoihana.mDownType.get(),
				           mDmgParamsPoihana.mWaterEmit.get(),
				           mDmgParamsPoihana.mMinSpeed.get(),
				           mDmgParamsPoihana.mMotor.get(),
				           mDmgParamsPoihana.mDirty.get(),
				           mDmgParamsPoihana.mInvincibleTime.get());
				return 1;
			}
			break;

		case 0x10000005:
		case 0x1000000E:
		case 0x10000011:
		case 0x1000001E:
		case 0x10000026:
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsFire.mDamage.get(),
					           mDmgParamsFire.mDownType.get(),
					           mDmgParamsFire.mWaterEmit.get(),
					           mDmgParamsFire.mMinSpeed.get(),
					           mDmgParamsFire.mMotor.get(),
					           mDmgParamsFire.mDirty.get(),
					           mDmgParamsFire.mInvincibleTime.get());
					return 1;
				}
			}
			if (message - 9 <= 1) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsFire.mDamage.get(),
					           mDmgParamsFire.mDownType.get(),
					           mDmgParamsFire.mWaterEmit.get(),
					           mDmgParamsFire.mMinSpeed.get(),
					           mDmgParamsFire.mMotor.get(),
					           mDmgParamsFire.mDirty.get(),
					           mDmgParamsFire.mInvincibleTime.get());

					changePlayerStatus(0x000208B7, 1, false);

					if (gpMSound->gateCheck(0x1813)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1813, (const Vec*)&mPosition, 0, nullptr, 0, 4);
					}
					gpMarioParticleManager->emitAndBindToPosPtr(
					    6, (const JGeometry::TVec3<f32>*)&mPosition, 0, nullptr);
					return 1;
				}
			}
			break;

		case 0x1000000F:
		case 0x10000035:
			break;

		case 0x10000030:
			break;

		default:
			break;
		}
	}

	// Check sender type for various other actor types
	{
		u32 aType = sender->mActorType;

		switch (aType) {
		case 0x08000029: // Hit from enemy
		{
			if (message == 0x0A) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsFire.mDamage.get(),
					           mDmgParamsFire.mDownType.get(),
					           mDmgParamsFire.mWaterEmit.get(),
					           mDmgParamsFire.mMinSpeed.get(),
					           mDmgParamsFire.mMotor.get(),
					           mDmgParamsFire.mDirty.get(),
					           mDmgParamsFire.mInvincibleTime.get());

					changePlayerStatus(0x000208B7, 1, false);

					if (gpMSound->gateCheck(0x1813)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1813, (const Vec*)&mPosition, 0, nullptr, 0, 4);
					}
					gpMarioParticleManager->emitAndBindToPosPtr(
					    6, (const JGeometry::TVec3<f32>*)&mPosition, 0, nullptr);
				}
				return 1;
			}
			return 0;
		}

		case 0x0800002A:
		case 0x0800002C: // Hit from object
		{
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsEnemyCommon.mDamage.get(),
					           mDmgParamsEnemyCommon.mDownType.get(),
					           mDmgParamsEnemyCommon.mWaterEmit.get(),
					           mDmgParamsEnemyCommon.mMinSpeed.get(),
					           mDmgParamsEnemyCommon.mMotor.get(),
					           mDmgParamsEnemyCommon.mDirty.get(),
					           mDmgParamsEnemyCommon.mInvincibleTime.get());
					return 1;
				}
			}
			if (message == 3) {
				if (!isInvincible()) {
					mState |= 0x800;
					if (!(mAction & 0x800))
						rumbleStart(21, 10);
					return 1;
				}
			}
			break;
		}

		case 0x08000001: // Basic enemy contact/damage
		{
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsHinokuri.mDamage.get(),
					           mDmgParamsHinokuri.mDownType.get(),
					           mDmgParamsHinokuri.mWaterEmit.get(),
					           mDmgParamsHinokuri.mMinSpeed.get(),
					           mDmgParamsHinokuri.mMotor.get(),
					           mDmgParamsHinokuri.mDirty.get(),
					           mDmgParamsHinokuri.mInvincibleTime.get());
					return 1;
				}
			}
			if (message == 3) {
				if (!isInvincible()) {
					mState |= 0x800;
					bool actionFlag = (mAction & 0x800) ? true : false;
					if (!actionFlag)
						rumbleStart(21, 10);
					return 1;
				}
			}
			break;
		}

		case 0x08000013: // NPC interaction (talk)
		{
			switch (message) {
			case 4:
				if (mHeldObject == nullptr && mHolder == nullptr) {
					mHolder = (TTakeActor*)sender;
					changePlayerStatus(0x0C400201, 0, false);
					return 1;
				}
				break;
			case 8:
				if (checkFlag(0x1000)) {
					changePlayerStatus(0x891, 0, true);
				} else {
					changePlayerStatus(0x0C400201, 0, false);
				}
				mHolder = nullptr;
				return 1;
			case 0x0E:
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsBGTentacle.mDamage.get(),
					           mDmgParamsBGTentacle.mDownType.get(),
					           mDmgParamsBGTentacle.mWaterEmit.get(),
					           mDmgParamsBGTentacle.mMinSpeed.get(),
					           mDmgParamsBGTentacle.mMotor.get(),
					           mDmgParamsBGTentacle.mDirty.get(),
					           mDmgParamsBGTentacle.mInvincibleTime.get());
					return 1;
				}
				break;
			default:
				break;
			}
			break;
		}

		case 0x0800000B:
		case 0x0800000C:
		case 0x0800000D:
		case 0x0800000E: // Specific enemy damage
		{
			if (message == 0x0A) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsFire.mDamage.get(),
					           mDmgParamsFire.mDownType.get(),
					           mDmgParamsFire.mWaterEmit.get(),
					           mDmgParamsFire.mMinSpeed.get(),
					           mDmgParamsFire.mMotor.get(),
					           mDmgParamsFire.mDirty.get(),
					           mDmgParamsFire.mInvincibleTime.get());

					changePlayerStatus(0x000208B7, 1, false);

					if (gpMSound->gateCheck(0x1813)) {
						MSoundSESystem::MSoundSE::startSoundActor(
						    0x1813, (const Vec*)&mPosition, 0, nullptr, 0, 4);
					}
					gpMarioParticleManager->emitAndBindToPosPtr(
					    6, (const JGeometry::TVec3<f32>*)&mPosition, 0, nullptr);
					return 1;
				}
			}
			break;
		}

		case 0x08000003:
		case 0x08000004:
		case 0x08000006:
		case 0x08000007:
		case 0x08000008:
		case 0x0800001F:
		case 0x08000022:
		case 0x08000023:
		case 0x08000027: // General enemy interaction
		case 0x1000000F:
		case 0x10000035:
		{
			switch (message) {
			case 4: // Take
				if (!isInvincible() && mHeldObject == nullptr
				    && mHolder == nullptr) {
					mHolder = (TTakeActor*)sender;
					changePlayerStatus(0x10020370, 0, false);
					return 1;
				}
				break;
			case 0x0E: // Touch
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsBGTentacle.mDamage.get(),
					           mDmgParamsBGTentacle.mDownType.get(),
					           mDmgParamsBGTentacle.mWaterEmit.get(),
					           mDmgParamsBGTentacle.mMinSpeed.get(),
					           mDmgParamsBGTentacle.mMotor.get(),
					           mDmgParamsBGTentacle.mDirty.get(),
					           mDmgParamsBGTentacle.mInvincibleTime.get());
					return 1;
				}
				break;
			case 8: // Release
				if (checkFlag(0x1000)) {
					changePlayerStatus(0x891, 0, true);
				} else {
					changePlayerStatus(0x0C400201, 0, false);
				}
				mHolder = nullptr;
				return 1;
			default:
				break;
			}
			break;
		}

		case 0x08000024: // BathtubKiller hit
		{
			if (!isInvincible() && message == 0x0E
			    && (mAction != 0x800008A9 || mActionState != 3)) {
				damageExec(sender,
				           mDmgParamsBGTentacle.mDamage.get(),
				           mDmgParamsBGTentacle.mDownType.get(),
				           mDmgParamsBGTentacle.mWaterEmit.get(),
				           mDmgParamsBGTentacle.mMinSpeed.get(),
				           mDmgParamsBGTentacle.mMotor.get(),
				           mDmgParamsBGTentacle.mDirty.get(),
				           mDmgParamsBGTentacle.mInvincibleTime.get());

				changePlayerStatus(0x000208B8, 0, false);
				return 1;
			}
		}

		case 0x08000014:
		case 0x08000015: // Hanachan boss parts
		{
			if (message == 0x0E) {
				if (!isInvincible()) {
					damageExec(sender,
					           mDmgParamsHanachanBoss.mDamage.get(),
					           mDmgParamsHanachanBoss.mDownType.get(),
					           mDmgParamsHanachanBoss.mWaterEmit.get(),
					           mDmgParamsHanachanBoss.mMinSpeed.get(),
					           mDmgParamsHanachanBoss.mMotor.get(),
					           mDmgParamsHanachanBoss.mDirty.get(),
					           mDmgParamsHanachanBoss.mInvincibleTime.get());
					return 1;
				}
			}
			return 0;
		}

		case 0x4000002C: // Wire actor grab
		{
			if (mInput & 0x8000) {
				s16 attackAngle = getAttackAngle(sender);
				s16 faceAngle = mFaceAngle.y;
				s16 angleDiff = attackAngle - faceAngle;
				if (angleDiff > -8192 && angleDiff < 8192) {
					f32 angle = (sender->mRotation.y * (1.0f / 360.0f)) * 65536.0f;
					s16 sAngle = (s16)angle;
					s16 adjAngle = sAngle - faceAngle;
					if (adjAngle > -8192 && adjAngle < 8192) {
						mPosition = sender->mPosition;
						mFaceAngle.y = sAngle;
						changePlayerStatus(0x1320, 0, false);
						bool hasTakenActor = mHeldObject != nullptr ? true : false;
						if (hasTakenActor) {
							mPumpState = 2;
							setAnimation(233, 0.0f);
						} else {
							setAnimation(95, 0.0f);
						}
						startVoice(0x78E5);
						return 1;
					}
					if (adjAngle < -24576 || adjAngle > 24576) {
						if (message == 0x11) {
							mPosition = sender->mPosition;
							s16 newAngle = sAngle + 0x8000;
							mFaceAngle.y = newAngle;
							changePlayerStatus(0x1321, 0, false);
							bool hasTakenActor = mHeldObject != nullptr ? true : false;
							if (hasTakenActor) {
								mPumpState = 2;
								setAnimation(233, 0.0f);
							} else {
								setAnimation(96, 0.0f);
							}
							startVoice(0x78E5);
							return 1;
						}
						bool hasTakenActor = mHeldObject != nullptr ? true : false;
						if (hasTakenActor)
							return 0;
						mPosition = sender->mPosition;
						s16 newAngle2 = sAngle + 0x8000;
						mFaceAngle.y = newAngle2;
						changePlayerStatus(0x1321, 0, false);
						setAnimation(313, 0.0f);
						return 0;
					}
				}
			}
			break;
		}

		case 0x08000002:
		case 0x80000001: // Player contact
		{
			if (!isInvincible()) {
				switch (message) {
				case 4:
					if (mHeldObject == nullptr && mHolder == nullptr) {
						mHolder = (TTakeActor*)sender;
						changePlayerStatus(0x10020370, 0, false);
						return 1;
					}
					break;
				case 6:
				case 7:
					mHolder = nullptr;
					changePlayerStatus(0x02000880, 0, false);
					setPlayerVelocity(40.0f);
					mVel.y = 10.0f;
					unk78 &= ~0x100;
					return 1;
				case 0x0E:
					damageExec(sender,
					           mDmgParamsEnemyMario.mDamage.get(),
					           mDmgParamsEnemyMario.mDownType.get(),
					           mDmgParamsEnemyMario.mWaterEmit.get(),
					           mDmgParamsEnemyMario.mMinSpeed.get(),
					           mDmgParamsEnemyMario.mMotor.get(),
					           mDmgParamsEnemyMario.mDirty.get(),
					           mDmgParamsEnemyMario.mInvincibleTime.get());
					return 1;
				}
			}
			break;
		}

		case 0x08000005: // NPC greeting
			break;

		case 0x4000002D:
		case 0x4000002E:
		case 0x40000032:
		case 0x40000034:
		case 0x40000035:
		case 0x40000036:
		case 0x40000037:
		case 0x40000039:
		case 0x4000003A:
		case 0x4000005A:
			if (message == 8) {
				changePlayerStatus(0x0C400201, 0, false);
				mHeldObject = nullptr;
			}
			return 0;

		case 0x40000038:
		case 0x0800002B:
		case 0x40000033:
		case 0x40000246:
			return 0;

		case 0x080000C0: // Water surface contact
		{
			if (mStatus != 0x1336 && message == 4) {
				mHolder = (TTakeActor*)sender;
				// Check state flag for grounded
				bool isJumping = (mAction & 0x800) ? true : false;
				if (!isJumping) {
					setAnimation(77, 0.0f);
					J3DFrameCtrl& ctrl = getMotionFrameCtrl();
					s16 frameAngle = ctrl.getEnd();
					getMotionFrameCtrl().setFrame((f32)frameAngle);
				}
				changePlayerDropping(0x1336, 0);
				return 1;
			}
			break;
		}

		case 0x4000009C:
		case 0x400000A5:
			if (message == 0x0E) {
				keepDistance(sender->mPosition,
				             30.0f + sender->getDamageRadius(), 0.0f);
				return 1;
			}
			break;

		case 0x40000064:
		case 0x40000393:
			// Damage touch with rumble.
			if (*(s16*)&unk150 > 0)
				break;

			*(s16*)((u8*)this + 0x14E) = mDeParams.mKickFreezeTime.get();
			rumbleStart(21, mMotorParams.mMotorTrample.get());
			calcDamagePos(sender->mPosition);
			kickFruitEffect();
			return 1;

		default:
		default_msg:
		default_enemy_contact:
			return 0;
		}
	}

	return 0;
}

void TMario::getGesso(THitActor* sender)
{
	if (mStatus != 0x10000) {
		mFaceAngle.y = DEG2SHORTANGLE(sender->mRotation.y);
		mModelFaceAngle = mFaceAngle.y;

		changePlayerStatus(0x810446, 0, false);

		mActionTimer = mDeParams.mSurfStartFreezeTime.get();
		emitGetEffect();

		// Set gesso type based on sender's actor type
		switch (sender->getActorType()) {
		case 0x400000C5:
			mSurfGesso = gpMapObjManager->mRedGesso;
			unk389 = 0;
			break;
		case 0x400000C6:
			mSurfGesso = gpMapObjManager->mYellowGesso;
			unk389 = 1;
			break;
		case 0x400000C7:
		default:
			unk389 = 2;
			mSurfGesso = gpMapObjManager->mGreenGesso;
			break;
		}

		mSurfGesso->setBck("surfgeso_run1");
		mSurfGesso->getFrameCtrl(0)->setRate(0.5f);
	}
}
