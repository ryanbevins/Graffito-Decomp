#define MSL_STDSQRTF_OUT_OF_LINE

#include <MSound/MSoundSE.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSRandVol.hpp>
#include <Camera/CubeManagerBase.hpp>
#include <JSystem/JAudio/JALibrary/JALSystem.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>
#include <MarioUtil/MapUtil.hpp>
#include <System/MSoundMainSide.hpp>
#include <math.h>

#undef MSL_STDSQRTF_OUT_OF_LINE

namespace MSoundSESystem {

JSUList<MSRandVol> MSRandVol::smList;

JSUList<MSRandPlay> MSRandPlay::smList;

SeInfo::TSeSetting SeInfo::smSeSetting;

MSoundSE* MSoundSE::mObj = 0;

void MSRandVol::construct(u32 param)
{
	smList.append(&(new MSRandVol(param))->unk4);
}

MSRandVol::MSRandVol(u32 param)
    : unk4(this)
    , unk14(param)
    , unk18(0.5f)
    , unk1C(0.0f)
    , unk20(0.25f)
    , unk24(0.5f)
    , unk28(0.75f)
    , unk2C(1.0f)
    , unk30(1.5f)
    , unk34(2.0f)
    , unk38(4.0f)
    , unk3C(0.25f)
    , unk40(0.5f)
    , unk44(0.75f)
    , unk48(1.0f)
{
}

f32 MSRandVol::getRandVol(u32 param)
{
	u32 minIndex  = (param >> 26) & 3;
	u32 maxIndex  = (param >> 24) & 3;
	u32 stepIndex = (param >> 22) & 3;
	f32 min       = *(&unk3C + minIndex) * unk18;
	f32 step      = *(&unk1C + stepIndex);
	f32 max       = *(&unk2C + maxIndex);
	f32 volume    = 1.0f + JALCalc::getRandom(min, max, step);
	f32 result    = volume < 0.0f ? 0.0f : volume;
	f32 capped    = result > 2.0f ? 2.0f : result;

	return capped;
}

void MSRandPlay::construct(u32 param_1, s32 param_2, s32 param_3, f32 param_4,
                           f32 param_5)
{
	smList.append(
	    &(new MSRandPlay(param_1, param_2, param_3, param_4, param_5))->unk0);
}

s32 MSRandPlay::registerTrans(u32 param, const Vec* vec)
{
	JSUListIterator<MSRandPlay> it;
	for (it = smList.getFirst(); it != smList.getEnd(); ++it) {
		MSRandPlay* play = it.getObject();
		if (param == play->unk1C) {
			MSRandPlayVec* playVec = &play->unk10[play->unk16];
			playVec->unk0          = vec;
			s32 index              = play->unk16;
			play->unk16++;
			return index;
		}
	}

	return -1;
}

void MSRandPlay::createRandPlayVec(u32 param1, u16 param2)
{
	for (JSULink<MSRandPlay>* link = smList.getFirst(); link != nullptr;
	     link = link->getNext()) {
		MSRandPlay* play = link->getObject();
		if (param1 == play->unk1C) {
			play->unk10 = new MSRandPlayVec[param2];
			play->unk14 = param2;
			play->unk16 = 0;
			break;
		}
	}
}

void MSRandPlay::startSeRandPlay(u32 param1, u32 param2)
{
	JSUListIterator<MSRandPlay> it;
	for (it = MSRandPlay::smList.getFirst(); it != MSRandPlay::smList.getEnd();
	     ++it) {
		if (param1 == it.getObject()->unk1C) {
			it.getObject()->randPlay(param2);
			break;
		}
	}
}

MSRandPlay::MSRandPlay(u32 param_1, s32 param_2, s32 param_3, f32 param_4,
                       f32 param_5)
    : unk0(this)
    , unk10(0)
    , unk14(0)
    , unk16(0)
    , unk1C(param_1)
    , unk20(param_2)
    , unk24(param_3)
    , unk28(param_4)
    , unk2C(param_5)
{
}

void MSRandPlay::randPlay(u32 param)
{
	MSRandPlayVec* playVec = &unk10[param];

	switch (playVec->unk4) {
	case 0: {
		f32 limit = unk24 * 0.5f;
		f32 delay = JALCalc::getRandom(limit, unk28, unk2C);
		playVec->unk8 = (s32)delay > unk20 ? (s32)delay : unk20;
		playVec->unk8
		    = playVec->unk8 < unk24 ? playVec->unk8 : unk24;

		if (playVec->unk8 == 0) {
			playVec->unk4 = 2;
			break;
		}

		playVec->unkC = 0;
		playVec->unk4 = 1;
		return;
	}
	case 1:
		if (playVec->unk8 > playVec->unkC) {
			playVec->unkC++;
			return;
		}
		playVec->unk4 = 2;
		break;
	}

	switch (playVec->unk4) {
	case 2:
		switch ((s32)unk1C) {
		case 0x3813:
			MSGMSound->startSoundSetGrp(unk1C, playVec->unk0, 0, 0.0f, 0,
			                            0, 4);
			break;
		default: {
			JAIActor actor(playVec->unk0, playVec->unk0, playVec->unk0, 0);
			MSoundSE::startSoundActorInner(unk1C, &playVec->unk20, &actor, 0,
			                               4);
			break;
		}
		}
		playVec->unk4 = 3;
		playVec->unkC = 0;
		break;
	case 3:
		if (playVec->unk20 == nullptr)
			playVec->unk4 = 0;
		break;
	}
}

void MSoundSE::construct()
{
	mObj = new MSoundSE;

	MSRandVol::construct(0);
	// clang-format off
	MSRandPlay::construct(0x3813, 0x1e, 0xf0,  JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x7865, 3,    0x14,  JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x7094, 3,    0x14,  JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x1950, 5,    0x3c,  JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x3869, 0x66, 0x181, JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x386d, 0x66, 0x181, JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x5814, 0x66, 0x181, JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x581b, 0x66, 0x181, JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x5820, 0x66, 0x181, JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x3870, 0x66, 0x181, JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x5814, 0x56, 0xf3,  JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x581b, 0x56, 0xf3,  JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	MSRandPlay::construct(0x5820, 8,    0xd,   JALCalc::cEqualCSlope, JALCalc::cPlusPSlope);
	// clang-format on

	// clang-format off
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ボスパックン・汚染飛行擬音",
    0x2052, 0.0f,     55000.0f, 6.0f,  0.5f,  2.0f, JALCalc::CS_NEGATIVE_CURVE,  0.0f, 100000.0f, 0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "水音・距離によるピッチ下げ",
    0x6800, 2000.0f,  0.0f,     6.0f,  0.82f, 1.0f, JALCalc::CS_NEGATIVE_CURVE,  0.0f, 20000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "潜水艦の風車ピッチ",
    0x3031, 0.0f,     3.5f,     3.4f,  0.2f,  1.35f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 5.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "潜水艦の鎖のこすれる音",
    0x3030, 0.0f,     0.7f,     6.0f,  0.5f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 1.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "潜水艦の風車音量",
    0x3031, 0.0f,     3.5f,     3.4f,  0.0f,  1.0f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 1.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "潜水艦の浮上する水音",
    0x3023, 0.0f,     0.7f,     6.0f,  0.5f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 1.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "コイン風車",
    0x3045, 0.14f,    10.5f,    0.78f, 0.2f,  1.93f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 20.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "コイン風車",
    0x3045, 0.14f,    10.5f,    0.78f, 0.5f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 20.0f,     0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "上下向回転門",
    0x3044, 0.0f,     10.0f,    3.4f,  0.2f,  1.35f, JALCalc::CS_UNKNOWN_2,      0.0f, 20.0f,     0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "上下向回転門",
    0x3044, 0.0f,     10.0f,    3.4f,  0.44f, 1.35f, JALCalc::CS_UNKNOWN_2,      0.0f, 20.0f,     0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "足台風車(足台移動音)",
    0x3042, 0.0f,     0.05f,    6.0f,  0.0f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 20.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "足台風車(風車音)",
    0x3040, 0.0f,     2.36f,    6.0f,  0.3f,  1.5f,  JALCalc::CS_UNKNOWN_2,      0.0f, 20.0f,     0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "足台風車(風車音)",
    0x3040, 0.0f,     2.36f,    6.0f,  0.2f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 20.0f,     0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "放水音",
    0,      0.01f,    1.0f,     6.0f,  0.3f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 5.0f,      1);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "マリオしりもち音",
    0x195a, 0.01f,    50.0f,    10.0f, 0.2f,  1.0f,  JALCalc::CS_POSITIVE_CURVE, 0.0f, 100.0f,    1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ジャンプキノコ",
    0x3866, 15000.0f, 3000.0f,  1.0f,  1.0f,  1.3f,  JALCalc::CS_UNKNOWN_2,      0.0f, 30000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "ジャンプキノコ",
    0x3866, 3000.0f,  15000.0f, 1.0f,  0.68f, 1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 30000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "マーレー壷を支えるロープ",
    0x3060, 0.0f,     20.0f,    1.0f,  0.0f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 50.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "マーレー壷を支えるロープ",
    0x3060, 0.0f,     20.0f,    1.0f,  0.5f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 50.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "マーレー壷",
    0x3061, 0.0f,     20.0f,    1.0f,  0.5f,  1.5f,  JALCalc::CS_UNKNOWN_2,      0.0f, 300.0f,    0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "コインバウンド音",
    0x4842, 0.0f,     20.0f,    1.0f,  0.0f,  1.0f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 100.0f,    1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ノーマルノズルのエアノイズ",
    0x24,   0.01f,    0.56f,    5.0f,  0.25f, 1.5f,  JALCalc::CS_POSITIVE_CURVE, 0.0f, 100.0f,    1);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "お化けスイカバウンド",
    0x3889, 0.7f,     15.0f,    10.0f, 0.2f,  1.0f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 30.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "通常スイカバウンド",
    0x388c, 0.7f,     15.0f,    10.0f, 0.2f,  1.0f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 30.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "お化けスイカ回転",
    0x308a, 0.01f,    10.0f,    10.0f, 0.2f,  1.0f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 30.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "通常スイカバ回転",
    0x308b, 0.01f,    10.0f,    10.0f, 0.2f,  1.0f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 30.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "お化けスイカ回転",
    0x308a, 0.01f,    10.0f,    1.0f,  0.8f,  1.2f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 30.0f,     1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "通常スイカバ回転",
    0x308b, 0.01f,    10.0f,    1.0f,  0.8f,  1.2f,  JALCalc::CS_NEGATIVE_CURVE, 0.0f, 30.0f,     1);

  JALSystem::append(JALSystem::ModType_JALSeModVolFGrp, "ロープ揺れ音",
    0,      1.0f,     52.0f,    3.98f, 0.0f,  1.0f,  JALCalc::CS_UNKNOWN_2,      0.0f, 300.0f,    1);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 0, 0x381c);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 0, 0x381d);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 0, 0x381e);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 0, 0x381f);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 0, 0x3820);

  JALSystem::append(JALSystem::ModType_JALSeModVolFGrp, "ロープ揺れ時のマリオの風切音",
    1,      1.0f,     83.0f,    1.2f,  0.0f,  1.0f, JALCalc::CS_UNKNOWN_2,       0.0f, 300.0f,    1);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 1, 0x1815);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 1, 0x1816);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 1, 0x1817);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 1, 0x180f);

  JALSystem::append(JALSystem::ModType_JALSeModPitFGrp, "ボール系のバウンド音(ピッチ上昇)",
    2,      30.0f,    0.0f,     1.0f,  1.0f,  2.0f, JALCalc::CS_UNKNOWN_2,       0.0f, 100.0f,    1);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModPitFGrp, 2, 0x3804);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModPitFGrp, 2, 0x3862);

  JALSystem::append(JALSystem::ModType_JALSeModVolFGrp, "ボール系のバウンド音(ボリューム減少)",
    2,      0.0f,     20.0f,    1.0f,  0.0f,  1.0f, JALCalc::CS_NEGATIVE_CURVE,  0.0f, 100.0f,    1);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 2, 0x3804);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 2, 0x3862);

  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ジェットコースター実音",
    0x305a, 0.0f,     50.0f,    2.87f, 1.0f,  2.3f, JALCalc::CS_NEGATIVE_CURVE,  0.0f, 300.0f,    0);

  JALSystem::append(JALSystem::ModType_JALSeModVolFGrp, "ブランコ(vol)",
    3,      0.0f,     30.0f,    1.0f,  0.0f,  1.0f, JALCalc::CS_UNKNOWN_2,       0.0f, 100.0f,    1);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 3, 0x3867);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModVolFGrp, 3, 0x3868);

  JALSystem::append(JALSystem::ModType_JALSeModPitFGrp, "ブランコ(pitch)",
    3,      0.0f,     30.0f,    1.0f, 0.3f,   1.0f, JALCalc::CS_UNKNOWN_2,       0.0f, 100.0f,    1);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModPitFGrp, 3, 0x3867);
  JALSystem::appendGrpMember(JALSystem::ModType_JALSeModPitFGrp, 3, 0x3868);

  JALSystem::append(JALSystem::ModType_JALSeModVolDist, "コロパク着地音",
    0x2844, 38468.0f, 0.0f,    12.2f, 0.0f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 100000.0f, 0);
  JALSystem::append(JALSystem::ModType_JALSeModVolDist, "コロパク回転音",
    0x2054, 40966.0f, 0.0f,    16.8f, 0.0f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 100000.0f, 0);
  JALSystem::append(JALSystem::ModType_JALSeModVolDist, "ボスパックン汚染飛行音(real)",
    0x2045, 50000.0f, 0.0f,    20.0f, 0.0f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 100000.0f, 0);
  JALSystem::append(JALSystem::ModType_JALSeModVolDist, "ボスパックン汚染飛行音(imit)",
    0x2052, 50000.0f, 0.0f,    28.2f, 0.0f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 100000.0f, 0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "メカクッパ戦・キラー飛行音",
    0x20ff, 5000.0f,  50.0f,   1.0f,  0.7f,  1.2f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 50000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ポポのふくらみ音",
    0x20c2, 2.5f,     1.0f,    3.0f,  0.54f, 1.1f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 3.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "ポポのふくらみ音",
    0x20c2, 2.5f,     1.0f,    3.0f,  0.55f, 1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 3.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "キャンキャン尻尾ひっぱり音",
    0x20df, 100.0f,   500.0f,  5.0f,  0.0f,  1.3f, JALCalc::CS_UNKNOWN_2,      0.0f, 500.0f,    0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "ハチの巣揺れ音",
    0x28f7, 0.04f,    1.2f,    1.0f,  0.0f,  1.0f, JALCalc::CS_UNKNOWN_2,      0.0f, 150.0f,    0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "イガイガバウンド音",
    0x28ad, 0.8f,     1.01f,   10.0f, 0.4f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 1.5f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "イガイガバウンド音",
    0x28ad, 0.8f,     1.01f,   10.0f, 0.4f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 1.5f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "イガイガ転がり音",
    0x212f, 0.95f,    2.3f,    4.0f,  0.2f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 2.5f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "イガイガ転がり音",
    0x212f, 100.0f,   500.0f,  5.0f,  0.0f,  1.3f, JALCalc::CS_UNKNOWN_2,      0.0f, 500.0f,    0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ボスパックン竜巻音",
    0x210c, 1538.0f,  500.0f,  3.0f,  0.65f, 1.7f, JALCalc::CS_UNKNOWN_2,      0.0f, 10000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ボスゲッソー回転攻撃音",
    0x215c, 13500.0f, 9700.0f, 4.0f,  0.6f,  1.8f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 14000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "ボスゲッソー回転攻撃音",
    0x215c, 13500.0f, 9700.0f, 5.0f,  0.3f,  1.0f, JALCalc::CS_POSITIVE_CURVE, 0.0f, 14000.0f,  0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ボスワンワン引きずり音",
    0x20d2, 4.0f,     8.0f,    1.0f,  0.9f,  1.1f, JALCalc::CS_NEGATIVE_CURVE, 0.0f, 500.0f,    0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "メカクッパ火炎",
    0x8135, 0.3f,     1.6f,    2.0f,  0.3f,  1.0f, JALCalc::CS_NEGATIVE_CURVE, 0.0f, 2.0f,      1);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "ウナギ回転",
    0x8921, 0.0f,     9.0f,    2.0f,  0.2f,  1.0f, JALCalc::CS_NEGATIVE_CURVE, 0.0f, 2.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModVolFunk, "ウナギ回転",
    0x8921, 0.0f,     9.0f,    2.0f,  0.2f,  1.0f, JALCalc::CS_NEGATIVE_CURVE, 0.0f, 2.0f,      0);
  JALSystem::append(JALSystem::ModType_JALSeModPitFunk, "クッパ風呂あふれ水",
    0x819d, 0.0f,     1.0f,    2.0f,  0.3f,  1.0f, JALCalc::CS_UNKNOWN_2,      0.0f, 2.0f,      0);
  MSSetSound::init();
	// clang-format on

	{
		MSSetSoundGrp* grp = new MSSetSoundGrp(
		    0, "カモメ", 3, 2, 13, 2, 3.0f, 1, 44.0f, 3.0f, 1.0f, 1.0f, 0.0f,
		    0xf, 200.0f, 0xb4, 1.0f, 1.0f, 0.0f, false);
		// clang-format off
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		grp->append(new MSSetSoundMember(0x3813, nullptr, 60.0f));
		// clang-format on
	}
}

u32 MSoundSE::getRandomID(u32 param)
{
	u8 baseCategory = (param >> 11) & 1;
	baseCategory |= (param >> 24) & 0xC0;

	u32 weights[15];
	u32 count = 0;
	for (; count < 15; ++count) {
		u32 id    = param + count;
		u32 flags = MSound::getBstSwitch(id);

		u8 category = (id >> 11) & 1;
		category |= (id >> 24) & 0xC0;
		if (category != baseCategory)
			break;
		if (flags == (u32)-1)
			break;
		if (count != 0 && (flags & 0x80000000))
			break;

		u32 weight = flags & 0x70000000;
		if (weight == 0)
			break;

		weights[count] = weight >> 28;
	}

	if (count <= 1)
		return param;

	f32 total = 0.0f;
	for (u32 i = 0; i < count; ++i)
		total += (f32)weights[i];

	f32 random = total * JALCalc::getRandom_0_1();
	f32 sum    = 0.0f;
	u32 choice = 0;
	for (u32 i = 0; i < count; ++i) {
		sum += (f32)weights[i];
		if (random < sum) {
			choice = i;
			break;
		}
	}

	return param + choice;
}

JAISound* MSoundSE::startSoundActor(u32 p1, const Vec* p2, u32 p3,
                                    JAISound** p4, u32 p5, u8 p6)
{
	JAIActor actor(p2, p2, p2, p3);
	return startSoundActorInner(p1, p4, &actor, p5, p6);
}

JAISound* MSoundSE::startSoundSystemSE(u32 p1, u32 p2, JAISound** p3, u32 p4)
{
	u32 soundID     = p1;
	s32 originalID  = soundID;
	JAISound* sound;
	u32 variantSlot = p2;

	switch (originalID) {
	case 0x481E:
		variantSlot--;
		switch (variantSlot) {
		case 1:
			soundID = 0x482E;
			break;
		case 2:
			soundID = 0x482F;
			break;
		case 3:
			soundID = 0x4830;
			break;
		case 4:
			soundID = 0x4831;
			break;
		case 5:
			soundID = 0x4832;
			break;
		}
		break;
	}

	sound = startSoundActorInner(soundID, p3, (JAIActor*)-1, p4, 4);
	if (sound == nullptr)
		return nullptr;

	switch (originalID) {
	case 0x481E:
		f32 pan   = 0.5f;
		f32 dolby = pan;

		switch (variantSlot) {
		case 0:
			pan   = 0.1f;
			dolby = pan;
			break;
		case 1:
			dolby = 0.0f;
			break;
		case 2:
			pan   = 0.9f;
			dolby = 0.1f;
			break;
		case 3:
			pan   = 0.1f;
			dolby = 0.9f;
			break;
		case 4:
			dolby = 1.0f;
			break;
		case 5:
			pan   = 0.9f;
			dolby = pan;
			break;
		}

		sound->setPan(pan, 0, 0);
		sound->setDolby(dolby, 0, 0);
		break;
	}

	return sound;
}

void MSoundSE::startSoundActorWithInfo(u32 p1, const Vec* p2, Vec* p3, f32 p4,
                                       u32 p5, u32 p6, JAISound** p7, u32 p8,
                                       u8 p9)
{
	u32 soundID     = p1;
	f32 gateParam   = p4;
	f32 volumeParam = gateParam;

	switch (soundID) {
	case 0x2052:
	case 0x305B:
		gateParam = p2->y;
		break;
	case 0x3048:
		return;
	case 0x3804:
	case 0x3862:
		gateParam
		    = __fabsf(std::sqrtf(p3->x * p3->x + p3->y * p3->y + p3->z * p3->z));
		break;
	case 0x1818:
		if (p5 < 4)
			soundID += p5;
		else
			soundID += 4;
		break;
	default:
		if (soundID >= 0x381C && soundID < 0x3821)
			gateParam = __fabsf(gateParam);
		break;
	}

	if (JALSystem::gateCheckFunc(soundID, gateParam))
		return;

	JAIActor actor(p2, p2, p2, p6);
	JAISound* sound = startSoundActorInner(soundID, p7, &actor, p8, p9);
	if (sound == nullptr)
		return;

	if (soundID == 0x2007) {
		f32 pitch = SeInfo::smSeSetting.unk4;
		for (u32 i = 0; i < sound->unk14; ++i)
			pitch *= SeInfo::smSeSetting.unk0;
		sound->setSeInterPitch(0, pitch, 0, 0.0f);
	} else if (soundID == 0x305B) {
		f32 volume = JALCalc::linearTransform(volumeParam, 0.0f, 20.0f, 0.0f,
		                                      1.0f, true);
		sound->setVolume(volume, 0, 0);
	}

	JALSystem::processModFunc(sound, gateParam, 0, 0);
}

bool MSoundSE::checkSoundArea(u32 param, const Vec& vec)
{
	bool result = true;

	switch ((s32)param) {
	case 7: {
		Vec marioCubePos;
		Vec marioPos = *MSGMSound->unkAC[0].unk0;
		marioPos.y += 100.0f;
		marioCubePos = marioPos;
		int marioCube = gpCubeCamera->getInCubeNo(marioCubePos);

		Vec actorCubePos;
		Vec actorPos = vec;
		actorPos.y += 100.0f;
		actorCubePos = actorPos;
		int actorCube = gpCubeCamera->getInCubeNo(actorCubePos);

		if (actorCube != -1) {
			if (actorCube == marioCube)
				result = true;
			else
				result = false;
		} else {
			result = true;
		}
		break;
	}
	case 8: {
		int marioArea = SMS_GetMonteVillageAreaInMario();
		int actorArea = MSMainProc::getMonteVillageActorArea(vec);
		if (marioArea == 1)
			marioArea = 3;

		if (marioArea == 3)
			result = true;
		else if (marioArea == actorArea)
			result = true;
		else
			result = false;
		break;
	}
	}

	return result;
}

#pragma dont_inline on
JAISound* MSoundSE::startSoundActorInner(u32 p1, JAISound** p2, JAIActor* p3,
                                         u32 p4, u8 p5)
{
	u32 soundID = p1;
	u32 flags   = MSound::getBstSwitch(soundID);

	if (p3 != (JAIActor*)-1) {
		if (MSGMSound->unkCD == 7) {
			if (!checkSoundArea(MSGMSound->unkCD, *p3->unk4)) {
				u32 category = soundID >> 30;
				if (category == 0)
					category = (soundID >> 12) & 0xF;
				else if (category == 2)
					category = 0x10;
				else if (category == 3)
					category = 0x11;
				else
					category = -1;

				if (category != 1) {
					category = soundID >> 30;
					if (category == 0)
						category = (soundID >> 12) & 0xF;
					else if (category == 2)
						category = 0x10;
					else if (category == 3)
						category = 0x11;
					else
						category = -1;

					if (category != 0)
						return nullptr;
				}
			}
		} else if (MSGMSound->unkCD == 8) {
			if (!checkSoundArea(MSGMSound->unkCD, *p3->unk4)) {
				u32 category = soundID >> 30;
				if (category == 0)
					category = (soundID >> 12) & 0xF;
				else if (category == 2)
					category = 0x10;
				else if (category == 3)
					category = 0x11;
				else
					category = -1;

				if (category != 1) {
					category = soundID >> 30;
					if (category == 0)
						category = (soundID >> 12) & 0xF;
					else if (category == 2)
						category = 0x10;
					else if (category == 3)
						category = 0x11;
					else
						category = -1;

					if (category != 0)
						return nullptr;
				}
			}

			switch (soundID) {
			case 0x3824:
				soundID = 0x38AD;
				break;
			case 0x3825:
				soundID = 0x38AF;
				break;
			case 0x193A:
				soundID = 0x38AB;
				break;
			case 0x193B:
				soundID = 0x38AC;
				break;
			}
		}

		if (flags & 0x800) {
			u32 actorFlags = p3->unkC;
			u32 surfFlag   = actorFlags & 0x10000000;

			if (surfFlag != 0) {
				if (soundID == 0x1820) {
					startSoundActorInner(0x1942, nullptr, p3, p4, p5);
				} else if (soundID == 0x1824) {
					return startSoundActorInner(0x1943, nullptr, p3, p4,
					                            p5);
				}
			}

			soundID = getNewIDBySurfaceCode(soundID, p3);
			if (soundID == (u32)-1)
				return nullptr;

			if ((p3->unkC & 0xF00) == 0) {
				switch (soundID) {
				case 0x1820:
				case 0x1822:
				case 0x1824:
				case 0x1826:
					soundID += (p3->unkC & 0xFF) * 8;
					break;
				}
			}

			if (soundID == (u32)-1)
				return nullptr;

			if (surfFlag != 0) {
				switch (soundID) {
				case 0x1820:
				case 0x1824:
				case 0x1828:
				case 0x182C:
				case 0x1830:
				case 0x1834:
					return nullptr;
				}
			}
		}
	}

	if (flags & 0x80000000)
		soundID = getRandomID(soundID);

	if (MSGMSound->unkCD == 8 && soundID >= 0x1878 && soundID <= 0x187F)
		soundID -= 8;

	if (p3 == (JAIActor*)-1) {
		if (p2 != nullptr) {
			MSGBasic->startSoundDirectID(soundID, p2, nullptr, p4, 4);
			return *p2;
		}
		return MSGBasic->startSoundActorReturnHandle(soundID, nullptr, p4, 4);
	}

	if (p2 != nullptr) {
		MSGBasic->startSoundActor(soundID, p2, p3, p4, p5);
		return *p2;
	}
	return MSGBasic->startSoundActorReturnHandle(soundID, p3, p4, p5);
}
#pragma dont_inline off

u32 MSoundSE::getNewIDBySurfaceCode(u32 param, JAIActor* actor)
{
	u32 surface = actor->unkC & 0xF00;
	if (surface == 0)
		return param;

	switch (param) {
	case 0x1820:
		switch (surface) {
		case 0x100:
		case 0x700:
			return 0x1924;
		case 0x200:
			return 0x1928;
		case 0x300:
		case 0x500:
			return 0x192C;
		case 0x400:
		case 0x600:
			return 0x1930;
		default:
			break;
		}
		break;
	case 0x1824:
		switch (surface) {
		case 0x100:
		case 0x700:
			return 0x1926;
		case 0x200:
			return 0x192A;
		case 0x300:
		case 0x500:
			return 0x192E;
		case 0x400:
		case 0x600:
			return 0x1932;
		default:
			break;
		}
		break;
	case 0x1822:
	case 0x1826:
		return -1;
	default:
		break;
	}
	return param;
}

void MSoundSE::startSoundNpcActor(u32 p1, const Vec* p2, u32 p3, JAISound** p4,
                                  u32 p5, u8 p6)
{
	JAIActor actor(p2, p2, p2, p3);

	void* infoPtr;
	JAIBasic::basic->unk0->getInfoPointer(p1, &infoPtr);
	JAISoundInfo* info = (JAISoundInfo*)infoPtr;
	if (info->unk0 & 0x4000) {
		u8 category = JAIBasic::basic->changeIDToCategory(p1);
		JAISound* sound
		    = JAIBasic::basic->unk0->unk1E8[category].unk4;
		while (sound != nullptr) {
			JAISound* next       = sound->unk30;
			JAISoundInfo* sInfo  = (JAISoundInfo*)sound->unk3C;
			if (sound->unk20 == actor.unk0 && (sInfo->unk0 & 0x4000)
			    && p1 != sound->unk8) {
				JAIBasic::basic->stopSoundHandle(sound, 0);
				break;
			}
			sound = next;
		}
	}

	startSoundActorInner(p1, p4, &actor, p5, p6);
}

bool MSoundSE::checkMonoSound(u32 param, JAIActor* actor)
{
	void* infoPtr;
	JAIBasic::basic->unk0->getInfoPointer(param, &infoPtr);
	JAISoundInfo* info = (JAISoundInfo*)infoPtr;

	if (info->unk0 & 0x4000) {
		u8 category = JAIBasic::basic->changeIDToCategory(param);
		JAISound* sound
		    = JAIBasic::basic->unk0->unk1E8[category].unk4;
		while (sound != nullptr) {
			JAISound* next       = sound->unk30;
			JAISoundInfo* sInfo  = (JAISoundInfo*)sound->unk3C;
			if (sound->unk20 == actor->unk0 && (sInfo->unk0 & 0x4000)
			    && param != sound->unk8) {
				JAIBasic::basic->stopSoundHandle(sound, 0);
				break;
			}
			sound = next;
		}
	}

	return true;
}

} // namespace MSoundSESystem
