#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundStruct.hpp>
#include <MSound/MSound.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>
#include <JSystem/JAudio/JALibrary/JALSystem.hpp>

MSSetSound* MSSetSound::smSetSound[9];

void MSSetSound::init()
{
	smSetSound[0] = new MSSetSound(0x6800, "放水着地音", 2, 7, 6, 4, 184.0f, 1,
	                               22.12f, 150.0f, 0.85f, 0.937f, 295.2f, 0,
	                               937.3f, 0xaf, 0.95f, 0.97f, 51.63f, false);

	smSetSound[1] = new MSSetSound(
	    0x6809, "落書き消し音", 2, 4, 7, 5, 100.0f, 1, 11.0f, 17000.0f, 0.52f,
	    0.9f, 221.78f, 0x16, 8000.0f, 0xfa, 0.81f, 1.35f, 153.73f, true);

	smSetSound[2] = new MSSetSound(0x6807, "ヒノクリ汚染着地音", 2, 9, 15, 18,
	                               100.0f, 1, 44.0f, 3.0f, 1.0f, 1.0f, 0.0f,
	                               0xf, 200.0f, 0xb4, 1.0f, 1.0f, 0.0f, false);

	smSetSound[3] = new MSSetSound(0x3803, "火柱", 2, 9, 15, 18, 100.0f, 1,
	                               44.0f, 3.0f, 1.0f, 1.0f, 0.0f, 0xf, 200.0f,
	                               0xb4, 1.0f, 1.0f, 0.0f, false);

	smSetSound[4] = new MSSetSound(0x3805, "電気柱", 2, 9, 15, 18, 100.0f, 1,
	                               44.0f, 3.0f, 1.0f, 1.0f, 0.0f, 0xf, 200.0f,
	                               0xb4, 1.0f, 1.0f, 0.0f, false);

	smSetSound[5] = new MSSetSound(0x804, "水乾燥音", 2, 4, 1, 10, 320.0f, 1,
	                               44.0f, 3.0f, 1.0f, 1.0f, 0.0f, 0xf, 200.0f,
	                               0xb4, 1.0f, 1.0f, 0.0f, false);

	smSetSound[6] = new MSSetSound(
	    0x6802, "水ヒットマーク", 2, 3, 10, 2, 300.0f, 1, 22.12f, 150.0f, 0.85f,
	    0.937f, 295.2f, 0, 937.3f, 0xaf, 0.95f, 0.97f, 51.63f, true);

	smSetSound[7] = new MSSetSound(0x6801, "放水着地音", 2, 7, 6, 4, 184.0f, 1,
	                               22.12f, 150.0f, 0.85f, 0.937f, 295.2f, 0,
	                               937.3f, 0xaf, 0.95f, 0.97f, 51.63f, false);

	smSetSound[8] = new MSSetSound(0x899b, "マンタ襲撃声", 3, 4, 63, 4, 184.0f,
	                               1, 22.12f, 150.0f, 0.94f, 0.815f, 280.2f, 0,
	                               937.3f, 0xaf, 0.95f, 0.97f, 51.63f, false);
}

bool MSSetSound::startSoundSet(u32 param1, const Vec* param2, u32 param3,
                               f32 param4, u32 param5, u32 param6, u8 param7)
{
	if (!MSGMSound->gateCheck(param1))
		return false;

	MSSetSoundTL<MSSetSound>* which = nullptr;
	switch (param1) {
		// clang-format off
	case 0x6800: which = smSetSound[0]; break;
  case 0x6809: which = smSetSound[1]; break;
	case 0x6807: which = smSetSound[2]; break;
	case 0x3803: which = smSetSound[3]; break;
	case 0x3805: which = smSetSound[4]; break;
	case 0x804:  which = smSetSound[5]; break;
	case 0x6802: which = smSetSound[6]; break;
	case 0x6801: which = smSetSound[7]; break;
	case 0x899b: which = smSetSound[8]; break;
		// clang-format on
	}

	if (!which)
		return false;

	return which->startSoundSetDyna(param1, param2, param3, param4, param5,
	                                param6, param7, nullptr);
}

bool MSSetSoundGrp::startSoundSetGrp(u32 param1, const Vec* param2, u32 param3,
                                     f32 param4, u32 param5, u32 param6,
                                     u8 param7)
{
	if (!MSGMSound->gateCheck(param1))
		return false;

	MSSetSoundGrp* grp = MSSetSoundGrp::searchGroup(param1);

	if (!grp)
		return false;

	return grp->startSoundSetDyna(param1, param2, param3, param4, param5,
	                              param6, param7, grp);
}

template <typename T>
bool MSSetSoundTL<T>::startSoundSetDyna(u32 soundID, const Vec* pos,
                                        u32 param3, f32 distArg, u32 actorArg,
                                        u32 startArg, u8 category,
                                        MSSetSoundGrp* group)
{
	if (unkB8 == 1)
		return true;

	f32 dist = JALCalc::getDist(&unkAC, (Vec*)pos);
	JAISound* previous = unk5C[unk5A];
	if (previous == nullptr) {
		u32 startID  = soundID;
		f32 modDist = distArg;
		switch (soundID) {
		case 0x6809:
			if (previous != nullptr
			    && (f32)previous->unk14 < (f32)unk3C.unk0
			    && dist < unk40.unk0)
				startID = 0x680a;
			break;
		case 0x6800:
			modDist = MSGMSound->getDistFromCamera((Vec*)pos);
			break;
		}

		const Vec* actorPos;
		if (unkB9 == true) {
			unk70[unk59] = *pos;
			actorPos     = &unk70[unk59];
		} else {
			actorPos = nullptr;
		}
		if (actorPos == nullptr)
			actorPos = pos;

		JAIActor actor(actorPos, actorPos, actorPos, actorArg);
		MSoundSESystem::MSoundSE::startSoundActorInner(
		    startID, &unk5C[unk59], &actor, startArg, category);

		JAISound* current = unk5C[unk59];
		JALSystem::processModFunc(current, modDist, 0, 3);

		unkAC = *pos;
		unkB8 = 1;
	} else {
		u32 minFrames = unk1D.unk0
		                + (u32)(unk1E.unk0 * JALCalc::getRandom_0_1());
		JAISound* check = unk5C[unk5A];
		u32 frame       = check->unk14;
		BOOL canStart;

		if (frame < minFrames) {
			canStart = false;
		} else if (unk24.unk0 == 1 && frame < unk1F.unk0
		           && dist < unk20.unk0) {
			canStart = false;
		} else if (group != nullptr) {
			MSSetSoundMember* member = group->searchD(check->unk8);
			if (member == nullptr || frame < member->unk18)
				canStart = false;
			else
				canStart = true;
		} else {
			canStart = true;
		}

		if (!canStart)
			return true;

		u32 startID  = soundID;
		f32 modDist = distArg;
		switch (soundID) {
		case 0x6809: {
			JAISound* compare = unk5C[unk5A];
			if (compare != nullptr
			    && (f32)compare->unk14 < (f32)unk3C.unk0
			    && dist < unk40.unk0)
				startID = 0x680a;
			break;
		}
		case 0x6800:
			modDist = MSGMSound->getDistFromCamera((Vec*)pos);
			break;
		}

		const Vec* actorPos;
		if (unkB9 == true) {
			unk70[unk59] = *pos;
			actorPos     = &unk70[unk59];
		} else {
			actorPos = nullptr;
		}
		if (actorPos == nullptr)
			actorPos = pos;

		JAIActor actor(actorPos, actorPos, actorPos, actorArg);
		MSoundSESystem::MSoundSE::startSoundActorInner(
		    startID, &unk5C[unk59], &actor, startArg, category);

		JAISound* current = unk5C[unk59];
		JALSystem::processModFunc(current, modDist, 0, 3);

		unkAC = *pos;
		unkB8 = 1;

		current = unk5C[unk59];
		if (current != nullptr) {
			JAISound* compare = unk5C[unk5A];
			if (compare != nullptr) {
				u32 frame = compare->unk14;
				f32 vol   = 1.0f;
				f32 pitch = 1.0f;

				if ((f32)frame < unk28.unk0 && dist < unk2C.unk0) {
					JALCalc::linearTransform(
					    frame, unk1D.unk0, unk28.unk0, unk38.unk0,
					    0.0f, false);
					vol = JALCalc::linearTransform(
					    frame, unk1D.unk0, unk28.unk0, unk30.unk0,
					    1.0f, false);
					pitch = JALCalc::linearTransform(
					    frame, unk1D.unk0, unk28.unk0, 1.0f,
					    unk34.unk0, false);
				}

				f32 contVol   = 1.0f;
				f32 contPitch = 1.0f;
				if ((f32)frame < (f32)unk3C.unk0 && dist < unk40.unk0) {
					unk58 = 1;
					JALCalc::linearTransform(
					    unk54, 0.0f, unk44.unk0, 0.0f,
					    unk50.unk0, false);
					contVol = JALCalc::linearTransform(
					    unk54, 0.0f, unk44.unk0, 1.0f,
					    unk48.unk0, false);
					contPitch = JALCalc::linearTransform(
					    unk54, 0.0f, unk44.unk0, 1.0f,
					    unk4C.unk0, false);
				} else {
					unk58 = 0;
					unk54 = 0;
				}

				current->setVolume(vol * contVol, 3, 0);
				current->setPitch(pitch * contPitch, 3, 0);
			}
		} else {
			if (current != nullptr)
				current->setPortData(13, 1);
			unk58 = 0;
			unk54 = 0;
		}
	}

	u8 nextSlot;
	if (unk59 == unk1C.unk0 - 1)
		nextSlot = 0;
	else
		nextSlot = unk59 + 1;
	unk59 = nextSlot;

	u8 nextPrev;
	if (unk5A == unk1C.unk0 - 1)
		nextPrev = 0;
	else
		nextPrev = unk5A + 1;
	unk5A = nextPrev;

	return true;
}

template <class T, class U> T* JALListD<T, U>::searchD(U param_1)
{
	JSULink<T>* link = this->getFirst();
	while (link) {
		T* obj = link->getObject();
		if (param_1 == obj->unk0)
			return obj;
		link = link->getNext();
	}
	return nullptr;
}
