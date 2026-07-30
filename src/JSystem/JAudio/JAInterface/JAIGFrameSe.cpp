#include <JSystem/JAudio/JAInterface/JAIBasic.hpp>
#include <JSystem/JAudio/JAInterface/JAIGlobalParameter.hpp>
#include <JSystem/JAudio/JAInterface/JAISystemInterface.hpp>
#include <JSystem/JAudio/JAInterface/JAIParameters.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>

static inline u8 getSeCategoryLimit(JAIData* data, u8 scene, u8 category)
{
	return data->unk4[scene][category * 2];
}

static inline u8 getSeCategoryLimitInt(JAIData* data, u8 scene, int category)
{
	return data->unk4[scene][category * 2];
}

static inline JAISound** getSeRegistSlot(JAIData* data, u8 category, u8 index)
{
	return &data->unk8[category][index].unk8;
}

static inline JAISeqUpdateData* getCurrentSeqUpdate(JAIBasic* basic)
{
	return &basic->unk0->unk180[basic->unk38->unk0];
}

static inline f32& seParamF32(JAISeParameter* param, u32 offset)
{
	return *(f32*)((u8*)param + offset);
}

static inline f32* seParamF32Ptr(JAISeParameter* param, u32 offset)
{
	return *(f32**)((u8*)param + offset);
}

static inline void markSeqPortF32(JAIBasic* basic, JAISound* sound, u32 flag,
                                  u8 port, f32 value)
{
	basic->unk0->unk180[basic->unk38->unk0].unk44[sound->unk0] |= flag;
	JAISystemInterface::setSeqPortargsF32(
	    &basic->unk0->unk180[basic->unk38->unk0], sound->unk0, port, value);
}

static inline void updateSeqPortF32(JAIBasic* basic, JAISound* sound,
                                    f32& cache, f32 value, u32 flag, u8 port)
{
	if (cache != value) {
		cache = value;
		if (sound->unk1 != 2)
			markSeqPortF32(basic, sound, flag, port, value);
	}
}

static inline f32 calcSeDistance(f32 value)
{
	if (value > 0.0f) {
		double root = __frsqrte(value);
		root        = 0.5 * root * (3.0 - value * (root * root));
		root        = 0.5 * root * (3.0 - value * (root * root));
		root        = 0.5 * root * (3.0 - value * (root * root));
		return value * root;
	}
	return value;
}

static inline void setSeStateAfterCommand(JAISound* sound)
{
	if (sound->unk8 & 0xC00)
		sound->unk1 = 4;
	else
		sound->unk1 = 5;
}

void JAIBasic::checkNextFrameSe()
{
	// TODO: gl matching this awfulness

	struct Candidate {
		u8 state;
		u32 score;
		JAISound* sound;
	};

	JAISound sound;
	Candidate candidates[16];

	f32 fVar6
	    = JAIGlobalParameter::distanceMax * JAIGlobalParameter::distanceMax;
	f32 fVar1 = JAIGlobalParameter::distanceMax / 1000.0f;
	if (fVar1 == 0.0f)
		fVar1 = 1.0f;

	for (int i = 0; i < JAIGlobalParameter::getParamSeCategoryMax(); ++i) {
		for (u8 j = 0; j < getSeCategoryLimitInt(unk0, unk10, i); ++j) {
			candidates[j].score = 0x7fffffff;
			candidates[j].sound = nullptr;
			candidates[j].state = 0xff;
		}

		u8 bVar19 = 0;

		JAISound* it = unk0->unk1E8[i].unk4;
		while (it) {
			if (it->unk1 == 1 && (it->unk8 & 0xC00)) {
				--it->unk2;
			} else if (!(it->unk8 & 0xC00) && it->unk1 == 5) {
				sound.unk30 = it->unk30;
				releaseSeRegist(it);
				it = &sound;
			}

				if (it->unk2 == 0) {
					sound.unk30 = it->unk30;
					releaseSeRegist(it);
					it = &sound;
				} else if (it->unk1 != 0) {
					u8 uVar14 = it->unk4;
					u8 uVar8;
					f32 fVar2 = 2.1474836e+09f;
					if (uVar14 == 4) {
						uVar14 = 0;
						uVar8  = JAIGlobalParameter::audioCameraMax;
					} else {
						uVar8 = uVar14 + 1;
					}

					for (; uVar14 < uVar8; ++uVar14) {
						JAISound::FabricatedPositionInfo* pi
						    = &it->unk1C[uVar14];

						pi->unkC = pi->unk0;
						if (it->unk24 == 0) {
							pi->unk0 = JAIConst::dummyZeroVec;
						} else {
							MTXMultVec(unk8[uVar14].unk8, it->unk24,
							          &pi->unk0);
						}

						pi->unk18 = pi->unk0.x * pi->unk0.x
						            + pi->unk0.y * pi->unk0.y
						            + pi->unk0.z * pi->unk0.z;
						s32 prio   = it->getInfoPriority();
						s16 offset = (s16)it->unk6;
						if (offset) {
							prio += offset;
							if (prio < 0)
								prio = 0;
							else if (prio > 0xff)
								prio = 0xff;
						}

						s32 iVar21
						    = (0xff - prio) * (0xff - prio) * 0x1690;
						it->unkC = (u32)(iVar21 / fVar1)
						           + (u32)(pi->unk18 / fVar1);
						if (pi->unk0.z > 0.0f)
							it->unkC
							    += (u32)(pi->unk0.z * 6.0f / fVar1);

						if (uVar14 == 0 || pi->unk18 < fVar2)
							fVar2 = pi->unk18;
					}

				if (it->unk4 == 4)
					it->unkC /= JAIGlobalParameter::audioCameraMax;

				f32 fVar3 = fVar6;
				if (!(it->getSwBit() & 0x20))
					fVar3 = 1e+10;

				if (fVar2 > fVar3) {
					if (!(it->unk8 & 0xC00)) {
						if (it->unk1 != 1) {
							JAISystemInterface::writePortApp(
							    unk38->getSeqParameter()->unk0,
							    (it->unk0 >> 4) + 0x20000000
							        + ((it->unk0 & 0xf) << 4),
							    0);
							unk38->setTrackInterruptSwitch(it->unk0, 1);
						}
						it->unk1 = 1;
					} else {
						sound.unk30 = it->unk30;
						stopSoundHandle(it, 0);
						it = &sound;
					}
				} else {
					u32 uVar14 = it->getSeCategoryNumber();
					u8 bVar18  = getSeCategoryLimit(unk0, unk10, uVar14);
					for (u8 j = 0; j < bVar18; ++j) {
						if (it->unkC < candidates[j].score
						    || (it->unkC == candidates[j].score
						        && it->unk1 <= candidates[j].state)) {
							if (bVar19 < bVar18)
								++bVar19;
							for (u8 k = bVar18 - 1; k > j; --k) {
								candidates[k].score
								    = candidates[k - 1].score;
								candidates[k].sound
								    = candidates[k - 1].sound;
								candidates[k].state
								    = candidates[k - 1].state;
							}
							candidates[j].score = it->unkC;
							candidates[j].sound = it;
							candidates[j].state = it->unk1;

							j = bVar18;
						}
					}
				}
			}
			if (it != nullptr)
				it = it->unk30;
		}

		for (u8 j = 0; j < bVar19; ++j) {
			JAISound* snd = candidates[j].sound;
			if (snd->unk1 == 1)
				snd->unk1 = 2;
			else if (snd->unk1 == 4)
				snd->unk1 = 3;
		}

		u8 bVar192 = getSeCategoryLimitInt(unk0, unk10, i);
		for (u8 j = 0; j < bVar192; ++j) {
			JAISound** cur = &unk0->unk8[i][j].unk8;
			bool bVar7     = false;
			if (*cur == nullptr) {
				bVar7 = true;
			} else if ((*cur)->unk1 == 4) {
				if (!((*cur)->unk8 & 0xC00)) {
					releaseSeRegist(*cur);
				} else {
					(*cur)->unk1 = 1;
					(*cur)->unk2 = 0;
				}
				bVar7 = true;
			} else if ((*cur)->unk1 == 0) {
				*cur  = nullptr;
				bVar7 = true;
			} else {
				for (u8 k = 0; k < bVar192; ++k) {
					if (unk0->unk8[i][j].unk8 == candidates[k].sound) {
						candidates[k].sound = nullptr;
						k                 = bVar192;
					}
				}
			}

			if (bVar7) {
				bool assigned = false;
				for (u8 k = 0; k < bVar192; ++k) {
					JAISound* snd = candidates[k].sound;
					if (snd != nullptr && snd->unk1 != 3) {
						for (u8 l = 0; l < bVar192; ++l) {
							if (unk0->unk8[i][l].unk8
							    && snd == unk0->unk8[i][l].unk8) {
								bVar7 = false;
								l     = bVar192;
							}
						}

						if (bVar7) {
							*cur              = snd;
							candidates[k].sound = nullptr;
							assigned          = true;
							break;
						}
					}
				}
				if (!assigned) {
					unk0->unk8[i][j].unk8 = nullptr;
				}
			}
		}
	}
}

void JAIBasic::sendPlayingSeCommand()
{
	u8 globalIndex = 0;
	for (u8 category = 0; category < JAIGlobalParameter::getParamSeCategoryMax();
	     ++category) {
		for (u8 i = 0; i < getSeCategoryLimit(unk0, unk10, category);
		     ++i, ++globalIndex) {
			JAISound* sound = *getSeRegistSlot(unk0, category, i);
			if (sound == nullptr)
				continue;

			++sound->unk14;

			u32 portBase = 0x20000000 + (globalIndex >> 4)
			               + ((globalIndex & 0xf) << 4);
			u32 seq = unk38->getSeqParameter()->unk0;
			u16 portStart;
			u16 portStatus;
			JAISystemInterface::readPortApp(seq, portBase + 0x20000,
			                                &portStart);
			JAISystemInterface::readPortApp(seq, portBase, &portStatus);

			JAISound::FabricatedPositionInfo* positions = sound->unk1C;
			for (u8 camera = 0; camera < JAIGlobalParameter::audioCameraMax;
			     ++camera) {
				f32* distance = &positions[camera].unk18;
				*distance     = calcSeDistance(*distance);
			}

			if (sound->unk1 == 2) {
				u32 swBit = sound->getSwBit();
				sound->unk0 = globalIndex;
				if (swBit & 8)
					setSeqMuteFromSeStart(sound);

				u32 randomMode = swBit & 0xC0;
				if (randomMode != 0) {
					s32 random
					    = (s32)(JAIConst::random.get_ufloat_1() * 255.0f);
					switch (randomMode) {
					case 0x40:
						sound->unk3 = random & 0xf;
						break;
					case 0x80:
						sound->unk3 = random & 0x1f;
						break;
					case 0xC0:
						sound->unk3 = random & 0x3f;
						break;
					default:
						sound->unk3 = 0;
						break;
					}
				}

				JAISeParameter* param = sound->getSeParameter();
				u16* portBits         = &param->unk20;
				for (u8 port = 0; *portBits != 0; ++port) {
					u16 bit = 1 << port;
					if (*portBits & bit) {
						unk38->setTrackPortData(
						    sound->unk0, port,
						    sound->getSeParameter()->unk0[port]);
						*portBits ^= bit;
					}
				}

				sound->setSeDistanceParameters();
				setSeExtParameter(sound);

				if (sound->unk10 > 1) {
					sound->setSeInterVolume(6, 0.0f, 0, 0);
					sound->setSeInterVolume(6, 127.0f, sound->unk10, 0);
					sound->unk10 = 0;
				}

				sendSeAllParameter(sound);

				u32 wait = sound->unk8 & 0x3ff;
				if (sound->checkSwBit(0x800))
					wait += getMapInfoGround(sound->unk18);

				u16 distanceWait = 0;
				if (JAIGlobalParameter::audioCameraMax == 1
				    && sound->checkSwBit(0x1000)) {
					f32 distance = sound->unk1C[0].unk18;
					if (distance < JAIGlobalParameter::distanceMax) {
						distanceWait
						    = (JAIGlobalParameter::seDistanceWaitMax
						       * (u32)distance)
						      / (u32)JAIGlobalParameter::distanceMax;
					} else {
						distanceWait
						    = JAIGlobalParameter::seDistanceWaitMax;
					}
				}

				JAISystemInterface::writePortApp(seq, portBase + 0x30000,
				                                 distanceWait);
				JAISystemInterface::writePortApp(
				    seq, portBase + 0x60000, getMapInfoFxline(sound->unk18));
				JAISystemInterface::writePortApp(seq, portBase + 0x40000,
				                                 wait);
				JAISystemInterface::writePortApp(seq, portBase, 1);
				setSeStateAfterCommand(sound);
			} else {
				if (portStart == 0 && portStatus != 1) {
					releaseSeRegist(sound);
				} else if (sound->unk10 != 0) {
					if (seParamF32(sound->getSeParameter(), 0x188) != 0.0f) {
						sound->setSeDistanceParameters();
						sendSeAllParameter(sound);
						setSeStateAfterCommand(sound);
					} else {
						releaseSeRegist(sound);
					}
				} else if (sound->unk1 == 3) {
					sound->setSeDistanceParameters();
					sendSeAllParameter(sound);
					setSeStateAfterCommand(sound);
				}
			}
		}
	}
}

void JAIBasic::setSeqMuteFromSeStart(JAISound* param_1)
{
	for (u32 i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISound* sound = unk0->unk180[i].unk48;
		if (i != unk38->unk0 && sound && !(sound->getSwBit() & 8)) {
			sound->setSeqInterVolume(
			    9, JAIGlobalParameter::seqMuteVolumeSePlay / 127.0f,
			    JAIGlobalParameter::seqMuteMoveSpeedSePlay);
			unk30 |= 1 << param_1->unk0;
		}
	}
}

void JAIBasic::checkSeMovePara()
{
	if (!unk38 || unk38->getSeqParameter()->unk1755 == 2)
		return;

	for (u8 i = 0; i < JAIGlobalParameter::getParamSeCategoryMax(); ++i) {
		for (JAISound* it = unk0->unk1E8[i].unk4; it != nullptr;
		     it           = it->unk30) {
			unk0->setSeMovePara(it->getSeParameter()->unk124);
			unk0->setSeMovePara(it->getSeParameter()->unk1A4);
			unk0->setSeMovePara(it->getSeParameter()->unk2A4);
			unk0->setSeMovePara(it->getSeParameter()->unk324);
			unk0->setSeMovePara(it->getSeParameter()->unk3A4);
			unk0->setSeMovePara(it->getSeParameter()->unk224);
		}
	}
}

void JAIBasic::sendSeAllParameter(JAISound* sound)
{
	f32* cached = (f32*)((u8*)unk0->unk0 + sound->unk0 * 0x18);
	JAISeParameter* param = sound->getSeParameter();

	f32 volume = seParamF32(param, 0x198);
	if (volume == -1.0f) {
		f32* link = seParamF32Ptr(param, 0x424);
		if (link != nullptr)
			seParamF32(param, 0x128) = *link;

		volume = 1.0f;
		volume *= seParamF32(param, 0x128);
		volume *= seParamF32(param, 0x138);
		volume *= seParamF32(param, 0x148);
		volume *= seParamF32(param, 0x158);
		volume *= seParamF32(param, 0x168);
		volume *= seParamF32(param, 0x178);
		volume *= seParamF32(param, 0x188);
	} else {
		volume = seParamF32(param, 0x198);
	}
	volume *= unk28[(u8)sound->getSeCategoryNumber()];
	updateSeqPortF32(this, sound, cached[1], volume, 1, 2);

	f32 pan = seParamF32(param, 0x218);
	if (-1.0f == seParamF32(param, 0x218)) {
		f32* link = seParamF32Ptr(param, 0x428);
		if (link != nullptr)
			seParamF32(param, 0x1A8) = *link;

		f32 panCenter = 0.5f;
		pan           = 0.0f;
		f32 panValue  = seParamF32(param, 0x1A8);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		panValue = seParamF32(param, 0x1B8);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		panValue = seParamF32(param, 0x1C8);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		panValue = seParamF32(param, 0x1D8);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		panValue = seParamF32(param, 0x1E8);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		panValue = seParamF32(param, 0x1F8);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		panValue = seParamF32(param, 0x208);
		if (panCenter != panValue)
			pan += panValue - panCenter;
		pan += 0.5f;
		if (pan < 0.0f)
			pan = 0.0f;
		else if (pan > 1.0f)
			pan = 1.0f;
	} else {
		pan = seParamF32(param, 0x218);
	}
	updateSeqPortF32(this, sound, cached[4], pan, 4, 4);

	f32 pitch = seParamF32(param, 0x298);
	if (-1.0f == seParamF32(param, 0x298)) {
		f32* link = seParamF32Ptr(param, 0x42C);
		if (link != nullptr)
			seParamF32(param, 0x228) = *link;

		pitch = 1.0f;
		pitch *= seParamF32(param, 0x228);
		pitch *= seParamF32(param, 0x238);
		pitch *= seParamF32(param, 0x248);
		pitch *= seParamF32(param, 0x258);
		pitch *= seParamF32(param, 0x268);
		pitch *= seParamF32(param, 0x278);
		pitch *= seParamF32(param, 0x288);
	} else {
		pitch = seParamF32(param, 0x298);
	}
	updateSeqPortF32(this, sound, cached[2], pitch, 2, 3);

	f32 fxmix = seParamF32(param, 0x318);
	if (-1.0f == seParamF32(param, 0x318)) {
		f32* link = seParamF32Ptr(param, 0x430);
		if (link != nullptr)
			seParamF32(param, 0x2A8) = *link;

		fxmix = 0.0f;
		fxmix += seParamF32(param, 0x2A8);
		fxmix += seParamF32(param, 0x2B8);
		fxmix += seParamF32(param, 0x2C8);
		fxmix += seParamF32(param, 0x2D8);
		fxmix += seParamF32(param, 0x2E8);
		fxmix += seParamF32(param, 0x2F8);
		fxmix += seParamF32(param, 0x308);
	} else {
		fxmix = seParamF32(param, 0x318);
	}
	updateSeqPortF32(this, sound, cached[3], fxmix, 8, 5);

	f32 dolby = seParamF32(param, 0x418);
	if (-1.0f == seParamF32(param, 0x418)) {
		f32* link = seParamF32Ptr(param, 0x438);
		if (link != nullptr)
			seParamF32(param, 0x3A8) = *link;

		f32 center = JAIGlobalParameter::seDolbyCenterValue / 127.0f;
		dolby      = 0.0f;
		dolby += seParamF32(param, 0x3A8) - center;
		dolby += seParamF32(param, 0x3B8) - center;
		dolby += seParamF32(param, 0x3C8) - center;
		dolby += seParamF32(param, 0x3D8) - center;
		dolby += seParamF32(param, 0x3E8) - center;
		dolby += seParamF32(param, 0x3F8) - center;
		dolby += seParamF32(param, 0x408) - center;
		dolby += center;
		if (dolby < 0.0f)
			dolby = 0.0f;
		else if (dolby > 1.0f)
			dolby = 1.0f;
	} else {
		dolby = seParamF32(param, 0x418);
	}
	updateSeqPortF32(this, sound, cached[5], dolby, 0x10, 6);

	u32 flags = getCurrentSeqUpdate(this)->unk44[sound->unk0];
	if (flags != 0) {
		JAISystemInterface::setSeqPortargsU32(getCurrentSeqUpdate(this),
		                                      sound->unk0, 1, flags);
		getCurrentSeqUpdate(this)->unk4C[sound->unk0].unk2C.unk0 = nullptr;
		getCurrentSeqUpdate(this)->unk4C[sound->unk0].unk2C.addPortCmdOnce();
	}
}

void JAIBasic::releaseSeRegist(JAISound* sound)
{
	if (sound->unk1 != 1) {
		u32 seq = unk38->getSeqParameter()->unk0;
		JAISystemInterface::writePortApp(
		    seq,
		    0x20000000 + (sound->unk0 >> 4) + ((sound->unk0 & 0xf) << 4),
		    0);
		unk38->setTrackInterruptSwitch(sound->unk0, 1);
	}

	if (unk30 != 0 && (sound->getSwBit() & 8)) {
		for (u32 i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
			JAISound* seqSound = unk0->unk180[i].unk48;
			if (i != unk38->unk0 && seqSound != nullptr
			    && !(seqSound->getSwBit() & 8)) {
				unk30 &= (1 << sound->unk0) ^ 0xFFFFFFFF;
				if (unk30 == 0) {
					seqSound->setSeqInterVolume(
					    9, 1.0f,
					    JAIGlobalParameter::seqMuteMoveSpeedSePlay);
				}
			}
		}
	}

	u8 count = getSeCategoryLimit(unk0, unk10, sound->getSeCategoryNumber());
	u8 category = sound->getSeCategoryNumber();
	for (u8 i = 0; i < count; ++i) {
		JAISound** slot = getSeRegistSlot(unk0, category, i);
		if (*slot == sound) {
			*slot = nullptr;
			i     = count;
		}
	}

	sound->clearMainSoundPPointer();
	sound->unk1 = 0;

	releaseSeParameterPointer(sound->getSeParameter());
	releaseControllerHandle(&unk0->unk1E8[category], sound);
}
