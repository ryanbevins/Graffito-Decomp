#include <MSound/MSHandle.hpp>
#include <JSystem/JAudio/JAInterface/JAIConst.hpp>
#include <JSystem/JAudio/JAInterface/JAIGlobalParameter.hpp>
#include <JSystem/JAudio/JALibrary/JALCalc.hpp>
#include <JSystem/JAudio/JALibrary/JALSystem.hpp>
#include <JSystem/JAudio/JAInterface/JAIBasic.hpp>
#include <math.h>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static f32 dummy1431[3] = { 1.0f, 1.0f, 1.0f };
static f32 dummy1411[3] = { 1.0f, 1.0f, 1.0f };
static u32 dummy1210[4] = { 0, 2, 1, 3 };

f32 MSHandle::smACosPrm[101] = {
	3.141592,   2.941258,   2.857799,   2.793427,   2.738877,   2.690566,
	2.6466579,  2.606066,   2.568079,   2.532207,   2.4980919,  2.465462,
	2.434109,   2.403867,   2.374599,   2.346194,   2.3185589,  2.291615,
	2.265295,   2.2395389,  2.214298,   2.1895249,  2.165182,   2.141233,
	2.1176469,  2.0943949,  2.0714509,  2.0487909,  2.026395,   2.004241,
	1.982313,   1.960593,   1.939064,   1.917713,   1.896526,   1.875489,
	1.854591,   1.833819,   1.813162,   1.792611,   1.772154,   1.751783,
	1.731487,   1.711258,   1.691086,   1.670964,   1.650882,   1.630832,
	1.6108069,  1.590798,   1.570796,   1.550795,   1.530786,   1.5107599,
	1.490711,   1.470629,   1.450507,   1.430335,   1.4101059,  1.38981,
	1.369439,   1.348982,   1.328431,   1.3077739,  1.287002,   1.266104,
	1.245067,   1.223879,   1.202528,   1.181,      1.1592799,  1.137351,
	1.115198,   1.092801,   1.070142,   1.047198,   1.023945,   1.000359,
	0.97641098, 0.95206797, 0.927295,   0.902054,   0.876298,   0.849978,
	0.82303399, 0.795399,   0.766994,   0.73772597, 0.70748299, 0.676131,
	0.64350098, 0.609386,   0.57351297, 0.53552699, 0.49493399, 0.451027,
	0.402716,   0.34816599, 0.28379399, 0.200335,   0.0,
};
SeCategory MSHandle::smSeCategory[16] = {
	{ 0x02000000, 8000.0f, 0.75999999f, 150.0f },
	{ 0x02000000, 8000.0f, 1.0f, 150.0f },
	{ 0x02000000, 6000.0f, 1.0f, 500.0f },
	{ 0x03000000, 6000.0f, 0.81f, 500.0f },
	{ 0x02000000, 12000.0f, 0.83999997f, 500.0f },
	{ 0x04000000, 12000.0f, 0.58999997f, 500.0f },
	{ 0x02000000, 7000.0f, 0.89999998f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 6000.0f, 0.75999999f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
	{ 0x02000000, 8000.0f, 1.0f, 500.0f },
};
f32 MSHandle::cPan_MaxAmp           = 0.499f;
f32 MSHandle::cPan_CAdjust          = 0.02f;
f32 MSHandle::cPan_CShift           = 1.6394f;
f32 MSHandle::cPan_HiSence_Dist     = 12.0f;
f32 MSHandle::cMS_DistanceMax_Sence = 0.5f;
f32 MSHandle::cDol_0Rad             = 1.0316f;
f32 MSHandle::cDol_HalfRad          = 1.5707999f;
f32 MSHandle::cDol_FullRad          = 2.1099999f;

static s32 computeCategoryIdx(u32 unk8)
{
	s32 idx = (unk8 >> 12) & 0xF;
	u32 top = unk8 >> 30;
	switch (top) {
	case 0:
		break;
	default:
		if (top == 2)
			idx = 0x10;
		else if (top == 3)
			idx = 0x11;
		else
			idx = -1;
		break;
	}
	return idx;
}

f32 MSHandle::setDistanceVolumeCommon(f32 volume, u8 param)
{
	f32 distance = unk1C->unk18;
	f32 maxDist  = JAIGlobalParameter::getParamMaxVolumeDistance();
	s32 idx      = computeCategoryIdx(unk8);
	return calcVolume(distance, volume, maxDist, param, idx);
}

void MSHandle::setSeDistancePitch(u8 param)
{
	f32 pitch = 1.0f;
	if (getSwBit() & 0x10) {
		s32 r = (s32)(JAIConst::random.get_ufloat_1() * 16.0f) & 0xF;
		pitch = 1.0f - (f32)r / 192.0f;
	}
	if (getSwBit() & 0xC0) {
		pitch += (f32)unk3 / 192.0f;
	}
	setSeInterPitch(4, pitch, param, 0.0f);
}

void MSHandle::setSeDistanceVolume(u8 param)
{
	u32 sw = getSwBit();
	if (sw & 0x200000) {
		setSeInterVolume(
		    4, JALSystem::processModDistVolume(unk8, unk1C->unk18), param, 0);
		return;
	}
	f32 vol;
	if (sw & 0x2) {
		vol = 1.0f;
	} else {
		u8 curve = (u8)((getSwBit() >> 16) & 0x7);
		s32 idx  = computeCategoryIdx(unk8);
		vol      = setDistanceVolumeCommon(smSeCategory[idx].unk4, curve);
	}
	setSeInterVolume(4, vol, param, 0);
}

void MSHandle::setSeDistanceDolby(u8 param)
{
	f32 dolby = calcDolby(unk1C->unk0, unk1C->unk18);
	setSeInterDolby(4, dolby, param, 0);
}

void MSHandle::setSeDistancePan(u8 param)
{
	JAISound::FabricatedPositionInfo* basic = unk1C;
	f32 dist = basic->unk18;
	s32 idx  = computeCategoryIdx(unk8);
	f32 pan = calcPan(basic->unk0, dist, smSeCategory[idx].unk4);
	setSeInterPan(4, pan, param, 0);
}

void MSHandle::setSeDistanceParameters()
{
	s32 idx = computeCategoryIdx(unk8);
	u8 type = ((u8*)&smSeCategory[idx])[0];
	if (unk1 == 2)
		type = 0;

	setSeDistanceVolume(type);
	setSeDistancePan(type);
	setSeDistancePitch(type);
	setSePositionDopplar();
	setSeDistanceFxmix(type);

	if (!(getSwBit() & 0x400)) {
		setFxmix(interPointer->getMapInfoFxParameter(unk18), 0, 2);
	}
	setSeDistanceDolby(type);
}

f32 MSHandle::calcVolume(f32 param1, f32 param2, f32 param3, u8 param4,
                         u8 param5)
{
	if (param1 < param3)
		return 1.0f;

	f32 x     = param1 - param3;
	f32 range = param2 - param3;
	switch (param4) {
	case 0:
		break;
	case 1:
		range = 4.0f * range / 3.0f;
		break;
	case 2:
		range = 5.0f * range / 3.0f;
		break;
	case 3:
		range = 2.0f * range;
		break;
	case 4:
		range = 3.0f * range;
		range *= 0.25f;
		break;
	case 5:
		range *= 0.5f;
		break;
	case 6:
		range *= 0.25f;
		break;
	case 7:
		range = smSeCategory[param5].unkC;
		break;
	}
	return JALCalc::linearTransform(x, 0.0f, range, 1.0f, 0.0f, false);
}

f32 MSHandle::calcPan(const Vec& vec, f32 param1, f32 param2)
{
	f32 maxAmp = cPan_MaxAmp;
	f32 angle  = (param1 <= 0.0f) ? 0.0f : MSACos(-vec.x / param1);
	f32 x = cPan_CAdjust + 2.0f * maxAmp * angle / 3.14159265f - maxAmp
	    - cPan_CAdjust;

	f32 amp;
	if (x < 0.0f) {
		amp = -maxAmp * powf(-x / maxAmp, cPan_CShift);
	} else {
		amp = maxAmp * powf(x / maxAmp, cPan_CShift);
	}

	if (param1 < cPan_HiSence_Dist) {
		amp *= param1 / cPan_HiSence_Dist;
	} else {
		amp *= 1.0f
		    + (cMS_DistanceMax_Sence - 1.0f) / (param2 - cPan_HiSence_Dist)
		          * (param1 - cPan_HiSence_Dist);
	}

	f32 result = amp + maxAmp;
	if (result > 1.0f)
		result = 1.0f;
	if (result < 0.0f)
		result = 0.0f;
	return result;
}

f32 MSHandle::calcDolby(const Vec& vec, f32 param)
{
	f32 angle = (param <= 0.0f) ? 0.0f : MSACos(-vec.z / param);
	const f32 dol0    = cDol_0Rad;
	const f32 dolHalf = cDol_HalfRad;
	const f32 dolFull = cDol_FullRad;

	f32 d;
	if (angle < dol0) {
		d = 0.0f;
	} else if (angle < dolHalf) {
		d = 0.5f / (dolHalf - dol0) * (angle - dol0);
	} else if (angle < dolFull) {
		d = 0.5f
		    + 0.5f / (dolFull - dolHalf) * (angle - dolHalf);
	} else {
		d = 1.0f;
	}

	if (param < cPan_HiSence_Dist) {
		d = 0.5f + param * ((d - 0.5f) / cPan_HiSence_Dist);
	}

	d = d > 1.0f ? 1.0f : d;
	d = d < 0.0f ? 0.0f : d;
	return d;
}

f32 MSHandle::MSACos(f32 param)
{
	s32 idx = (s32)(50.0f * (1.0f + param));
	if (idx < 0)
		return smACosPrm[0];
	if (idx >= 101)
		return smACosPrm[100];
	return smACosPrm[idx];
}
