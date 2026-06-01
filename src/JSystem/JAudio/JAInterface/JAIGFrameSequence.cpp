#include <JSystem/JAudio/JAInterface/JAIBasic.hpp>
#include <JSystem/JAudio/JAInterface/JAIGlobalParameter.hpp>
#include <JSystem/JAudio/JAInterface/JAISystemInterface.hpp>
#include <JSystem/JAudio/JAInterface/JAIParameters.hpp>
#include <JSystem/JAudio/JASystem/JASTrackMgr.hpp>
#include <JSystem/JAudio/JASystem/JASCmdStack.hpp>
#include <JSystem/JAudio/JASystem/JASVload.hpp>
#include <math.h>

void JAIBasic::stopSeq(JAISound* param_1)
{
	if (param_1->getSwBit() & 1) {
		for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
			JAISound* sound = unk0->unk180[i].unk48;
			if (sound != param_1 && sound) {
				if (sound->unk1 >= 3 && !(sound->getSwBit() & 2)) {
					sound->setSeqInterVolume(10, 1.0f, 10);
					JASystem::TrackMgr::handleToSeq(
					    sound->getSeqParameter()->unk0)
					    ->unPauseTrackAll();
				}
			}
		}
	}

	param_1->unk34                      = nullptr;
	param_1->getSeqParameter()->unk1850 = nullptr;
	if (param_1->unk1 >= 3) {
		unk0->releaseAutoHeapPointer(param_1->getSeqParameter()->unk1754);
	}
	param_1->unk1 = 0;
	releaseSeqParameterPointer(param_1->getSeqParameter());
	releaseControllerHandle(&unk0->unk210, param_1);
	unk0->unk180[param_1->unk0].unk48 = nullptr;
}

void JAIBasic::checkEntriedSeq()
{
	for (u32 i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISeqUpdateData* seqData = &unk0->unk180[i];
		JAISound** soundSlot      = &seqData->unk48;
		u32* flags                = &seqData->unk8;
		if (*soundSlot == nullptr)
			continue;

		if ((*flags & 1) == 0)
			continue;

		if (seqData->unk3 != 0)
			return;

		u32 size
		    = JASystem::Vload::checkSize(unk2C + ((*soundSlot)->unk8 & 0x3FF));

		u8 heapNo;
		u8* buffer
		    = (u8*)unk0->checkOnMemory((*soundSlot)->unk8 & 0x3FF, &heapNo);
		if (buffer == nullptr) {
			if ((*soundSlot)->checkSwBit(0x10)) {
				buffer = (u8*)unk0->getFreeStayHeapPointer(
				    size, (*soundSlot)->unk8 & 0x3FF);
				heapNo = 0xFF;
				(*soundSlot)->getSeqParameter()->unk1754 = heapNo;
				if (buffer == nullptr)
					(void)(*soundSlot)->checkSwBit(0x20);
			}

			if (buffer == nullptr) {
				if ((*soundSlot)->checkSwBit(0x20)
				    || !(*soundSlot)->checkSwBit(0x10)) {
					heapNo = unk0->checkUsefulAutoHeapPosition();

					if (heapNo >= JAIGlobalParameter::autoHeapMax) {
						for (u32 autoIdx = 0;
						     autoIdx < JAIGlobalParameter::autoHeapMax;
						     ++autoIdx) {
							if (unk0->unk1EC[autoIdx].unk10 == (u32)-1)
								continue;

							u32 track;
							for (track = 0;
							     track < JAIGlobalParameter::seqPlayTrackMax;
							     ++track) {
								JAISound* playing
								    = unk0->unk180[track].unk48;
								if (playing != nullptr
								    && (playing->unk8 & 0xFF)
								        == unk0->unk1EC[autoIdx].unk8)
									break;
							}

							if (track == JAIGlobalParameter::seqPlayTrackMax) {
								unk0->releaseAutoHeapPointer(autoIdx);
								heapNo = autoIdx;
							}
						}

						if (heapNo >= JAIGlobalParameter::autoHeapMax) {
							(*soundSlot)->stop(0);
							return;
						}
					} else if (size >= JAIGlobalParameter::autoHeapRoomSize) {
						(*soundSlot)->stop(0);
						return;
					}

					(*soundSlot)->getSeqParameter()->unk1754 = heapNo;
					buffer = (u8*)unk0->getFreeAutoHeapPointer(
					    heapNo, (*soundSlot)->unk8 & 0x3FF);
				}
			}

			if (!(*soundSlot)->checkSwBit(0x40)) {
				(*soundSlot)->unk1 = 1;
				u32 seqNo = (*soundSlot)->unk8 & 0x3FF;
				u32 loadId = i | (seqNo << 16) | (heapNo << 8) | 1;
				unk0->setAutoHeapLoadedFlag(heapNo, 1);
				JASystem::Vload::loadFileAsync(
				    unk2C + seqNo, buffer, 0, size, &checkDvdLoadArc, loadId);
				seqData->unk3 = 1;
			} else {
				JASystem::Vload::loadFile(
				    unk2C + ((*soundSlot)->unk8 & 0x3FF), buffer, 0, size);
				(*soundSlot)->unk1 = 2;
			}
		} else {
			if (buffer == (u8*)-1)
				return;

			if (heapNo != 0xFF)
				unk0->getFreeAutoHeapPointer(heapNo,
				                              (*soundSlot)->unk8 & 0x3FF);

			(*soundSlot)->getSeqParameter()->unk1754 = heapNo;
			(*soundSlot)->unk1                      = 2;
		}

		if (buffer != nullptr) {
			seqData->unk40 = buffer;
			*flags ^= 1;
		} else {
			stopSeq(*soundSlot);
		}
	}
}

#pragma dont_inline on
void JAIBasic::checkPlayingSeqTrack(unsigned long trackID)
{
	JAISeqUpdateData* seqData = &unk0->unk180[trackID];
	JAISound* sound           = seqData->unk48;
	JAISeqParameter* seqParam = sound->getSeqParameter();

	if (seqParam->unk1755 == 2)
		return;

	for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax + 1; ++i)
		seqData->unk44[i] = 0;

	if (seqData->unk8 & 0x2) {
		if (sound->unk10 == 0 || sound->unk1 < 4) {
			if (sound->unk1 >= 3)
				JAISystemInterface::stopSeq(seqParam->unk0);
			sound->clearMainSoundPPointer();
			stopSeq(sound);
			seqData->unk8 = 0;
			return;
		}

		sound->setSeqInterVolume(6, 0.0f, sound->unk10);
		sound->unk1 = 5;
		seqData->unk8 ^= 0x2;
	}

	if (sound != nullptr && sound->unk20 != nullptr) {
		u32 startCamera;
		u32 endCamera;
		if (sound->unk4 == 4) {
			startCamera = 0;
			endCamera   = JAIGlobalParameter::audioCameraMax;
		} else {
			startCamera = sound->unk4;
			endCamera   = sound->unk4 + 1;
		}

		for (u32 i = startCamera; i < endCamera; ++i) {
			JAISound::FabricatedPositionInfo* info = &sound->unk1C[i];

			info->unkC = info->unk0;
			PSMTXMultVec(unk8[i].unk8, sound->unk24, &info->unk0);

			f32 distance = info->unk0.x * info->unk0.x
			               + info->unk0.y * info->unk0.y
			               + info->unk0.z * info->unk0.z;
			if (distance > 0.0f) {
				double root = __frsqrte(distance);
				root        = 0.5 * root * (3.0 - distance * (root * root));
				root        = 0.5 * root * (3.0 - distance * (root * root));
				root        = 0.5 * root * (3.0 - distance * (root * root));
				volatile f32 sqrtValue = distance * root;
				distance               = sqrtValue;
			}
			info->unk18 = distance;

			f32 volume = (u8)(sound->setDistanceVolumeCommon(
			                     JAIGlobalParameter::distanceMax, 0)
			                 * 127.0f);
			sound->setSeqInterVolume(
			    4, volume, JAIGlobalParameter::distanceParameterMoveTime);

			f32 pan = (u8)sound->setDistancePanCommon();
			sound->setSeqInterPan(
			    4, pan, JAIGlobalParameter::distanceParameterMoveTime);

			f32 pitch = sound->setPositionDopplarCommon(0x100);
			sound->setSeqInterPitch(4, pitch, JAIGlobalParameter::dopplarMoveTime);
		}
	}

	if (sound != nullptr)
		sound->unk14++;

	if (seqData->unk8 == 0)
		return;

	u32 seqMoveCount = (u8)(JAIGlobalParameter::seqPlayTrackMax + 0xC);

	if (seqData->unk8 & 0x40000) {
		f32 value = 1.0f;
		for (u8 i = 0; i < seqMoveCount; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1760 & bit) {
				if (!unk0->moveParameter(&seqParam->unk114[i]))
					seqParam->unk1760 ^= bit;
			}
			value *= seqParam->unk114[i].unk4;
		}

		if (seqData->unkC != value) {
			seqData->unkC = value;
			JAISystemInterface::setSeqPortargsF32(
			    seqData, JAIGlobalParameter::seqTrackMax, 2, value);
			seqData->unk44[JAIGlobalParameter::seqTrackMax] |= 0x1;
		}

		if (seqParam->unk1760 == 0)
			seqData->unk8 ^= 0x40000;
	}

	if (seqData->unk8 & 0x80000) {
		f32 value = 0.0f;
		for (u8 i = 0; i < seqMoveCount; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1764 & bit) {
				if (!unk0->moveParameter(&seqParam->unk254[i]))
					seqParam->unk1764 ^= bit;
			}
			value += seqParam->unk254[i].unk4 - 0.5f;
		}

		value += 0.5f;
		if (value > 1.0f)
			value = 1.0f;
		else if (value < 0.0f)
			value = 0.0f;

		if (seqData->unk18 != value) {
			seqData->unk18 = value;
			JAISystemInterface::setSeqPortargsF32(
			    seqData, JAIGlobalParameter::seqTrackMax, 4, value);
			seqData->unk44[JAIGlobalParameter::seqTrackMax] |= 0x4;
		}

		if (seqParam->unk1764 == 0)
			seqData->unk8 ^= 0x80000;
	}

	if (seqData->unk8 & 0x100000) {
		f32 value = 1.0f;
		for (u8 i = 0; i < seqMoveCount; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1768 & bit) {
				if (!unk0->moveParameter(&seqParam->unk394[i]))
					seqParam->unk1768 ^= bit;
			}
			value *= seqParam->unk394[i].unk4;
		}

		if (seqData->unk10 != value) {
			seqData->unk10 = value;
			JAISystemInterface::setSeqPortargsF32(
			    seqData, JAIGlobalParameter::seqTrackMax, 3, value);
			seqData->unk44[JAIGlobalParameter::seqTrackMax] |= 0x2;
		}

		if (seqParam->unk1768 == 0)
			seqData->unk8 ^= 0x100000;
	}

	if (seqData->unk8 & 0x200000) {
		f32 value = 0.0f;
		for (u8 i = 0; i < seqMoveCount; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk176C & bit) {
				if (!unk0->moveParameter(&seqParam->unk4D4[i]))
					seqParam->unk176C ^= bit;
			}
			value += seqParam->unk4D4[i].unk4;
		}

		if (value > 1.0f)
			value = 1.0f;

		if (seqData->unk14 != value) {
			seqData->unk14 = value;
			JAISystemInterface::setSeqPortargsF32(
			    seqData, JAIGlobalParameter::seqTrackMax, 5, value);
			seqData->unk44[JAIGlobalParameter::seqTrackMax] |= 0x8;
		}

		if (seqParam->unk176C == 0)
			seqData->unk8 ^= 0x200000;
	}

	if (seqData->unk8 & 0x400000) {
		f32 value = 1.0f;
		for (u8 i = 0; i < seqMoveCount; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1770 & bit) {
				if (!unk0->moveParameter(&seqParam->unk614[i]))
					seqParam->unk1770 ^= bit;
			}
			value *= seqParam->unk614[i].unk4;
		}

		if (seqData->unk1C != value) {
			seqData->unk1C = value;
			JAISystemInterface::setSeqPortargsF32(
			    seqData, JAIGlobalParameter::seqTrackMax, 6, value);
			seqData->unk44[JAIGlobalParameter::seqTrackMax] |= 0x10;
		}

		if (seqParam->unk1770 == 0)
			seqData->unk8 ^= 0x400000;
	}

	if (seqData->unk8 & 0x4) {
		if (!unk0->moveParameter(&seqParam->unk4))
			seqData->unk8 ^= 0x4;

		if (seqData->unk20 != seqParam->unk4.unk4) {
			seqData->unk20 = seqParam->unk4.unk4;
			JAISystemInterface::setSeqPortargsF32(
			    seqData, JAIGlobalParameter::seqTrackMax, 9,
			    seqParam->unk4.unk4);
			seqData->unk44[JAIGlobalParameter::seqTrackMax] |= 0x80;
		}
	}

	if (seqData->unk8 & 0x10) {
		for (u8 i = 0; i < 16; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk175C & bit) {
				if (!unk0->moveParameter(&seqParam->unk14[i]))
					seqParam->unk175C ^= bit;
			}

			u16 portValue;
			u32 route  = i << 16;
			u16 target = (u16)seqParam->unk14[i].unk4;
			JAISystemInterface::readPortApp(seqParam->unk0, route, &portValue);
			if (portValue != target)
				JAISystemInterface::writePortApp(seqParam->unk0, route, target);
		}

		if (seqParam->unk175C == 0)
			seqData->unk8 ^= 0x10;
	}

	if (seqData->unk8 & 0x40) {
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1774 & bit) {
				if (!unk0->moveParameter(&seqParam->unk754[i]))
					seqParam->unk1774 ^= bit;

				if (seqData->unk24[i] != seqParam->unk754[i].unk4) {
					seqData->unk24[i] = seqParam->unk754[i].unk4;
					seqData->unk44[i] |= 0x1;
					JAISystemInterface::setSeqPortargsF32(
					    seqData, i, 2, seqParam->unk754[i].unk4);
				}
			}
		}

		if (seqParam->unk1774 == 0)
			seqData->unk8 ^= 0x40;
	}

	if (seqData->unk8 & 0x20) {
		seqData->unk8 ^= 0x20;
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			MuteBit* mute = &seqParam->unk1830[i];
			if (mute->flag3 == 1 && mute->flag1 != mute->flag2) {
				JASystem::TTrack* track
				    = JAISystemInterface::trackToSeqp(sound, i);
				if (track != nullptr)
					track->muteTrack(mute->flag2);
				mute->flag1 = mute->flag2;
			}
		}
	}

	if (seqData->unk8 & 0x80) {
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1778 & bit) {
				if (!unk0->moveParameter(&seqParam->unk954[i]))
					seqParam->unk1778 ^= bit;

				if (seqData->unk30[i] != seqParam->unk954[i].unk4) {
					seqData->unk30[i] = seqParam->unk954[i].unk4;
					seqData->unk44[i] |= 0x4;
					JAISystemInterface::setSeqPortargsF32(
					    seqData, i, 4, seqParam->unk954[i].unk4);
				}
			}
		}

		if (seqParam->unk1778 == 0)
			seqData->unk8 ^= 0x80;
	}

	if (seqData->unk8 & 0x200) {
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk177C & bit) {
				if (!unk0->moveParameter(&seqParam->unkB54[i]))
					seqParam->unk177C ^= bit;

				if (seqData->unk28[i] != seqParam->unkB54[i].unk4) {
					seqData->unk28[i] = seqParam->unkB54[i].unk4;
					seqData->unk44[i] |= 0x2;
					JAISystemInterface::setSeqPortargsF32(
					    seqData, i, 3, seqParam->unkB54[i].unk4);
				}
			}
		}

		if (seqParam->unk177C == 0)
			seqData->unk8 ^= 0x200;
	}

	if (seqData->unk8 & 0x800) {
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1780 & bit) {
				if (!unk0->moveParameter(&seqParam->unkD54[i]))
					seqParam->unk1780 ^= bit;

				if (seqData->unk2C[i] != seqParam->unkD54[i].unk4) {
					seqData->unk2C[i] = seqParam->unkD54[i].unk4;
					seqData->unk44[i] |= 0x8;
					JAISystemInterface::setSeqPortargsF32(
					    seqData, i, 5, seqParam->unkD54[i].unk4);
				}
			}
		}

		if (seqParam->unk1780 == 0)
			seqData->unk8 ^= 0x800;
	}

	if (seqData->unk8 & 0x100) {
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			u32 bit = 1 << i;
			if (seqParam->unk1784 & bit) {
				if (!unk0->moveParameter(&seqParam->unkF54[i]))
					seqParam->unk1784 ^= bit;

				if (seqData->unk34[i] != seqParam->unkF54[i].unk4) {
					seqData->unk34[i] = seqParam->unkF54[i].unk4;
					seqData->unk44[i] |= 0x10;
					JAISystemInterface::setSeqPortargsF32(
					    seqData, i, 6, seqParam->unkF54[i].unk4);
				}
			}
		}

		if (seqParam->unk1784 == 0)
			seqData->unk8 ^= 0x100;
	}

	if (seqData->unk8 & 0x800000) {
		seqData->unk8 ^= 0x800000;
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			if (seqParam->unk1810[i] == 1) {
				seqData->unk44[i] |= 0x40;
				JAISystemInterface::setSeqPortargsU32(seqData, i, 8, 1);
				seqParam->unk1810[i] = 0;
			}
		}
	}

	if (seqData->unk8 & 0x1000) {
		for (u8 i = 0; i < JAIGlobalParameter::seqTrackMax; ++i) {
			u32 trackBit = 1 << i;
			if (seqParam->unk178C & trackBit) {
				seqParam->unk178C ^= trackBit;
				for (u8 port = 0; port < 16; ++port) {
					u32 portBit = 1 << port;
					if (seqParam->unk1790[i] & portBit) {
						u32 route = sound->getTrackPortRoute(i, port);
						JAISystemInterface::writePortApp(
						    seqParam->unk0, route, seqParam->unk1354[i][port]);
						seqParam->unk1790[i] ^= portBit;
					}
				}
			}
		}

		if (seqParam->unk178C == 0)
			seqData->unk8 ^= 0x1000;
	}
}
#pragma dont_inline off

void JAIBasic::checkPlayingSeq()
{
	for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISound** sound = &unk0->unk180[i].unk48;
		if (*sound && (*sound)->unk1 >= 4) {
			checkPlayingSeqTrack(i);
			for (u8 j = 0; j < JAIGlobalParameter::seqTrackMax + 1; ++j) {
				if (unk0->unk180[i].unk44[j] != 0) {
					JAISystemInterface::setSeqPortargsU32(
					    &unk0->unk180[i], j, 1, unk0->unk180[i].unk44[j]);

					unk0->unk180[i].unk4C[j].unk2C.unk0 = nullptr;
					unk0->unk180[i].unk4C[j].unk2C.addPortCmdOnce();
				}
			}
		}
	}
}

void JAIBasic::checkStoppedSeq()
{
	for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISound** sound = &unk0->unk180[i].unk48;

		if (!*sound)
			continue;

		if ((*sound)->unk1 == 4 || (*sound)->unk1 == 5) {
			u8 flag = JAISystemInterface::checkSeqActiveFlag(
			    (*sound)->getSeqParameter()->unk0);
			if (flag == 0) {
				(*sound)->clearMainSoundPPointer();
				stopSeq(*sound);
				unk0->unk180[i].unk8 = 0;
			}
		}
	}
}

void JAIBasic::checkStartedSeq()
{
	for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISound** sound = &unk0->unk180[i].unk48;

		if (!*sound)
			continue;

		if ((*sound)->unk1 == 3) {
			u8 flag = JAISystemInterface::checkSeqActiveFlag(
			    (*sound)->getSeqParameter()->unk0);
			if (flag != 0) {
				(*sound)->unk1 = 4;
				JAISystemInterface::trackInit(&unk0->unk180[i]);
			}
		}
	}
}

void JAIBasic::checkFadeoutSeq()
{
	for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISound** sound = &unk0->unk180[i].unk48;

		if (!*sound)
			continue;

		if ((*sound)->unk1 == 5 && (*sound)->getSeqInterVolume(6) == 0.0f) {
			JAISystemInterface::stopSeq((*sound)->getSeqParameter()->unk0);
			(*sound)->clearMainSoundPPointer();
			stopSeq(*sound);
			unk0->unk180[i].unk8 = 0;
		}
	}
}

void JAIBasic::checkReadSeq()
{
	for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISeqUpdateData* sud = &unk0->unk180[i];
		JAISound** sound      = &sud->unk48;
		if (!*sound)
			continue;
		if ((*sound)->unk1 != 2)
			continue;
		if ((*sound)->getSeqParameter()->unk1758 != -1)
			continue;
		if ((*sound)->getSeqParameter()->unk1850->unk2 != 0)
			continue;

		u32 lVar2
		    = JASystem::Vload::checkSize(unk2C + ((*sound)->unk8 & 0x3FF));
		int uVar3 = JAISystemInterface::setSeqData(
		    nullptr, sud->unk40, lVar2, JASystem::Player::SEQ_PLAYMODE_UNK_0);

		(*sound)->getSeqParameter()->unk0 = uVar3;
		(*sound)->getSeqParameter();
		if ((*sound)->getSeqParameter()->unk0 != -1) {
			unk0->initSeqTrackInfoParameter((*sound)->unk0);
			(*sound)->unk1 = 3;
			if ((*sound)->unk10 > 1) {
				(*sound)->setSeqInterVolume(6, 0.0f, 0);
				(*sound)->setSeqInterVolume(6, 1.0f, (*sound)->unk10);
			}
			if (sud->unk0 != 0) {
				(*sound)->setPauseMode(sud->unk0, sud->unk1);
				sud->unkC = 1.1f;
			}
			setSeExtParameter(*sound);
			checkPlayingSeqTrack(i);
			if (*sound != nullptr) {
				JAISystemInterface::rootInit(sud);
				JAISystemInterface::startSeq((*sound)->getSeqParameter()->unk0);
			}
		} else {
			JAISound* snd = *sound;
			stopSeq(snd);
		}
	}
}

void JAIBasic::checkSeqWave()
{
	for (int i = 0; i < JAIGlobalParameter::seqPlayTrackMax; ++i) {
		JAISeqUpdateData* sud = &unk0->unk180[i];
		JAISound** sound      = &sud->unk48;
		if (!*sound)
			continue;
		if (!(*sound)->getSeqParameter())
			continue;
		if ((*sound)->getSeqParameter()->unk1758 == 0xffffffff)
			continue;
		if (unk34 == 0xffffffff)
			continue;

		u32 uVar3 = (*sound)->getSeqParameter()->unk1758;

		JAISound* snd = *sound;
		if (unk34 == uVar3 || uVar3 == 0xff00ff00
		    || (((unk34 & 0xffff0000) == (uVar3 & 0xffff0000)
		         && uVar3 == 0xffff))
		    || (((unk34 & 0xffff) == (uVar3 & 0xffff)
		         && uVar3 == 0xffff0000))) {
			snd->getSeqParameter()->unk1758 = 0xffffffff;
		}
	}
}

void JAIBasic::checkDvdLoadArc(u32 param_1)
{
	u8 hi   = param_1 >> 8;
	u8 lo   = param_1 & 0xff;
	u16 hi2 = ((param_1 >> 16) & 0x3FF);
	if (hi != 0xff)
		basic->unk0->setAutoHeapLoadedFlag(hi, 0);

	if (lo < 0xFE) {
		JAISound* sound              = basic->unk0->unk180[lo].unk48;
		basic->unk0->unk180[lo].unk3 = 0;
		if (sound && sound->unk1 == 1 && hi2 == (sound->unk8 & 0x3FF))
			sound->unk1 = 2;
		else
			basic->unk0->releaseAutoHeapPointer(hi);
	} else if (lo == 0xFE) {
		basic->unk0->releaseAutoHeapPointer(hi);
	}
}
