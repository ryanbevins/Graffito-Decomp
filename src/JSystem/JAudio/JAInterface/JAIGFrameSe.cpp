#include <JSystem/JAudio/JAInterface/JAIBasic.hpp>
#include <JSystem/JAudio/JAInterface/JAIGlobalParameter.hpp>
#include <JSystem/JAudio/JAInterface/JAISystemInterface.hpp>
#include <JSystem/JAudio/JAInterface/JAIParameters.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>

static inline u8 getSeCategoryLimit(JAIData* data, u8 scene, u8 category)
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
	JAISeqUpdateData* update = getCurrentSeqUpdate(basic);
	update->unk44[sound->unk0] |= flag;
	JAISystemInterface::setSeqPortargsF32(update, sound->unk0, port, value);
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

void JAIBasic::checkNextFrameSe()
{
	// TODO: gl matching this awfulness

	JAISound sound;

	u8 local_180[12][4];
	struct FabricatedThing {
		u32 unk0;
		JAISound* unk4;
		u32 unk8;
	};
	FabricatedThing local_17C[16];

	f32 fVar6
	    = JAIGlobalParameter::distanceMax * JAIGlobalParameter::distanceMax;
	f32 fVar1 = JAIGlobalParameter::distanceMax / 1000.0f;
	if (fVar1 == 0.0f)
		fVar1 = 1.0f;

	for (int i = 0; i < JAIGlobalParameter::getParamSeCategoryMax(); ++i) {
		for (int j = 0; j < unk0->unk4[unk10][i]; ++j) {
			local_17C[j].unk0 = 0x7fffffff;
			local_17C[j].unk4 = nullptr;
			local_180[j][0]   = 0xff;
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
				sound.unk30 = it->unk2C;
				releaseSeRegist(it);
				it = &sound;
			} else if (it->unk1 != 0) {
				u8 uVar14 = it->unk4;
				u8 uVar8;
				if (uVar14 == 4) {
					uVar14 = 0;
					uVar8  = JAIGlobalParameter::audioCameraMax;
				} else {
					uVar8 = uVar14 + 1;
				}

				f32 fVar2 = 2.147484e+09;
				for (; uVar14 < uVar8; ++uVar14) {
					JAISound::FabricatedPositionInfo* pi = &it->unk1C[uVar14];

					pi->unkC = pi->unk0;
					if (it->unk24 == 0) {
						pi->unk0 = JAIConst::dummyZeroVec;
					} else {
						MTXMultVec(unk8[uVar14].unk8, it->unk24, &pi->unk0);
					}

					pi->unk18 = pi->unk0.x * pi->unk0.x
					            + pi->unk0.y * pi->unk0.y
					            + pi->unk0.z * pi->unk0.z;
					u32 prio = it->getInfoPriority();
					if (it->unk6) {
						prio += it->unk6;
						if (prio < 0)
							prio = 0;
						else if (prio > 0xff)
							prio = 0xff;
					}

					s32 iVar21 = (0xff - prio) * (0xff - prio) * 0x1690;
					it->unkC = (u32)(iVar21 / fVar1) + (u32)(pi->unk18 / fVar1);
					if (pi->unk0.z > 0.0f)
						it->unkC += (u32)(pi->unk0.z * 6.0f / fVar1);

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
							        + ((it->unk0 & 0xf) << 16),
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
					u8 bVar18  = unk0->unk4[unk10][uVar14];
					for (u8 j = 0; j < bVar18; ++j) {
						if (it->unkC < local_17C[uVar14].unk0
						    || (it->unkC == local_17C[uVar14].unk0
						        && it->unk1 <= local_180[uVar14][0])) {
							if (bVar19 < bVar18)
								++bVar19;
							for (u8 k = bVar18; k > j; --k) {
								// TODO:
								local_17C[k].unk0    = 0;
								local_17C[k].unk4    = 0;
								local_180[uVar14][0] = 0;
							}
							local_17C[uVar14].unk0 = it->unkC;
							local_17C[uVar14].unk4 = it;
							local_180[uVar14][0]   = it->unk1;

							j = bVar18;
						}
					}
				}
			}
			if (it != nullptr)
				it = it->unk30;
		}

		u8 bVar192 = unk0->unk4[unk10][i];
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
					if (unk0->unk8[i][j].unk8 == local_17C[k].unk4) {
						local_17C[k].unk4 = nullptr;
						k                 = bVar192;
					}
				}
			}

			if (bVar7) {
				u8 k;
				for (k = 0; k < bVar192; ++k) {
					JAISound* snd = local_17C[k].unk4;
					if (snd != nullptr && it->unk1 != 3) {
						for (u8 l = 0; l < bVar192; ++l) {
							if (unk0->unk8[i][l].unk8
							    && snd == unk0->unk8[i][l].unk8) {
								bVar7 = false;
								l     = bVar192;
							}
						}

						if (bVar7) {
							k                     = bVar192 + 1;
							unk0->unk8[i][k].unk8 = snd;
							local_17C[k].unk4     = nullptr;
						}
					}
				}
				if (k == bVar192) {
					unk0->unk8[i][j].unk8 = nullptr;
				}
			}
		}
	}
}

void JAIBasic::sendPlayingSeCommand() { }

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

void JAIBasic::clearSeqMuteFromSeStop(JAISound* sound) { }

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

		volume = seParamF32(param, 0x128);
		volume *= seParamF32(param, 0x138);
		volume *= seParamF32(param, 0x148);
		volume *= seParamF32(param, 0x158);
		volume *= seParamF32(param, 0x168);
		volume *= seParamF32(param, 0x178);
		volume *= seParamF32(param, 0x188);
	} else {
		volume = seParamF32(param, 0x198);
	}
	volume *= unk28[sound->getSeCategoryNumber()];
	updateSeqPortF32(this, sound, cached[1], volume, 1, 2);

	f32 pan = seParamF32(param, 0x218);
	if (pan == -1.0f) {
		f32* link = seParamF32Ptr(param, 0x428);
		if (link != nullptr)
			seParamF32(param, 0x1A8) = *link;

		pan = 0.0f;
		for (u32 off = 0x1A8; off <= 0x208; off += 0x10) {
			f32 v = seParamF32(param, off);
			if (v != 0.5f)
				pan += v - 0.5f;
		}
		pan += 0.5f;
		if (pan < 0.0f)
			pan = 0.0f;
		else if (pan > 1.0f)
			pan = 1.0f;
	}
	updateSeqPortF32(this, sound, cached[4], pan, 4, 4);

	f32 pitch = seParamF32(param, 0x298);
	if (pitch == -1.0f) {
		f32* link = seParamF32Ptr(param, 0x42C);
		if (link != nullptr)
			seParamF32(param, 0x228) = *link;

		pitch = seParamF32(param, 0x228);
		pitch *= seParamF32(param, 0x238);
		pitch *= seParamF32(param, 0x248);
		pitch *= seParamF32(param, 0x258);
		pitch *= seParamF32(param, 0x268);
		pitch *= seParamF32(param, 0x278);
		pitch *= seParamF32(param, 0x288);
	}
	updateSeqPortF32(this, sound, cached[2], pitch, 2, 3);

	f32 fxmix = seParamF32(param, 0x318);
	if (fxmix == -1.0f) {
		f32* link = seParamF32Ptr(param, 0x430);
		if (link != nullptr)
			seParamF32(param, 0x2A8) = *link;

		fxmix = seParamF32(param, 0x2A8);
		fxmix += seParamF32(param, 0x2B8);
		fxmix += seParamF32(param, 0x2C8);
		fxmix += seParamF32(param, 0x2D8);
		fxmix += seParamF32(param, 0x2E8);
		fxmix += seParamF32(param, 0x2F8);
		fxmix += seParamF32(param, 0x308);
	}
	updateSeqPortF32(this, sound, cached[3], fxmix, 8, 5);

	f32 dolby = seParamF32(param, 0x418);
	if (dolby == -1.0f) {
		f32* link = seParamF32Ptr(param, 0x438);
		if (link != nullptr)
			seParamF32(param, 0x3A8) = *link;

		f32 center = JAIGlobalParameter::seDolbyCenterValue / 127.0f;
		dolby      = 0.0f;
		for (u32 off = 0x3A8; off <= 0x408; off += 0x10)
			dolby += seParamF32(param, off) - center;
		dolby += center;
		if (dolby < 0.0f)
			dolby = 0.0f;
		else if (dolby > 1.0f)
			dolby = 1.0f;
	}
	updateSeqPortF32(this, sound, cached[5], dolby, 0x10, 6);

	JAISeqUpdateData* update = getCurrentSeqUpdate(this);
	u32 flags                = update->unk44[sound->unk0];
	if (flags != 0) {
		JAISystemInterface::setSeqPortargsU32(update, sound->unk0, 1, flags);
		JAISeqUpdateData::FabricatedUnk4CStruct* port
		    = &update->unk4C[sound->unk0];
		port->unk2C.unk0 = nullptr;
		port->unk2C.addPortCmdOnce();
	}
}

void JAIBasic::releaseSeRegist(JAISound* sound)
{
	if (sound->unk1 != 1) {
		u32 seq = unk38->getSeqParameter()->unk0;
		u8 id   = sound->unk0;
		JAISystemInterface::writePortApp(
		    seq, 0x20000000 + (id >> 4) + ((id & 0xf) << 4), 0);
		unk38->setTrackInterruptSwitch(id, 1);
	}

	if (unk30 != 0 && (sound->getSwBit() & 8)) {
		for (u32 i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
			JAISound* seqSound = unk0->unk180[i].unk48;
			if (i != unk38->unk0 && seqSound != nullptr
			    && !(seqSound->getSwBit() & 8)) {
				unk30 &= ~(1 << sound->unk0);
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
			break;
		}
	}

	sound->clearMainSoundPPointer();
	sound->unk1 = 0;

	releaseSeParameterPointer(sound->getSeParameter());
	releaseControllerHandle(&unk0->unk1E8[category], sound);
}
