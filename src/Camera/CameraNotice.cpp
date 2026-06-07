#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGeometry.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/Watergun.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/LiveManager.hpp>
#include <System/MarioGamePad.hpp>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBCalcRatio<f32>(f32, f32, f32);
template <> f32 CLBSquared<f32>(f32);
template <> s16 CLBRoundf<s16>(f32);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);

const char* bossGesoViewObjName = "ボスゲッソー";

static const char* sNoticeActorManagerName[] = {
	"ヒノクリマネージャー",
	"ヒノクリ２マネージャー",
	"ボスパックンマネージャー",
	nullptr,
};

namespace {

inline bool inViewCone(const JGeometry::TVec2<f32>& screen, f32 tan)
{
	bool a = false;
	bool b = false;
	bool c = false;
	if (-tan <= screen.x && screen.x <= tan)
		a = true;
	if (a && -tan <= screen.y)
		b = true;
	if (b && screen.y <= tan)
		c = true;
	return c ? true : false;
}

} // namespace

void CPolarSubCamera::setNoticeInfo()
{
	*(void***)((u8*)this + 0x2A0) = new void*[16];
	this->unk2A4  = nullptr;
	*(s32*)((u8*)this + 0x29C)    = 0;

	for (const char** name = sNoticeActorManagerName; *name != nullptr;
	     ++name) {
		TLiveManager* mgr
		    = (TLiveManager*)gpConductor->getManagerByName(*name);
		if (mgr == nullptr)
			continue;
		int n = mgr->mObjNum;
		for (int j = 0; j < n; j++) {
			(*(void***)((u8*)this + 0x2A0))[*(s32*)((u8*)this + 0x29C)]
			    = (void*)mgr->getObj(j);
			(*(s32*)((u8*)this + 0x29C))++;
		}
	}

	*(JDrama::TNameRef**)((u8*)this + 0x2A8)
	    = JDrama::TNameRefGen::getInstance()->getRootNameRef()->search(
	        bossGesoViewObjName);
}

void* CPolarSubCamera::getNoticeActor_()
{
	if (this->unk2A4 != nullptr) {
		u32 status
		    = *(u32*)((u8*)this->unk2A4 + 0xF0);
		if ((status & 1) == 0 && (status & 2) == 0) {
			Vec diff;
			diff.x = *(f32*)((u8*)this->unk2A4 + 0x10)
			         - gpMarioPos->x;
			diff.y = *(f32*)((u8*)this->unk2A4 + 0x14)
			         - gpMarioPos->y;
			diff.z = *(f32*)((u8*)this->unk2A4 + 0x18)
			         - gpMarioPos->z;
			f32 distSq  = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
			f32 nearLim = CLBSquared<f32>(
			    *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x2C));
			if (distSq < nearLim) {
				JGeometry::TVec2<f32> screen;
				f32 tan
				    = *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x54);
				CLBCalc2DFPos(
				    &screen, (MtxPtr)((u8*)this + 0x1EC),
				    (MtxPtr)((u8*)this + 0x16C),
				    *(const Vec*)((u8*)this->unk2A4 + 0x10),
				    nullptr, false);
				if (inViewCone(screen, tan))
					return this->unk2A4;
			}
		}
	}

	void* picked   = nullptr;
	f32 bestDistSq = CLBSquared<f32>(
	    *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x18));
	int count = *(int*)((u8*)this + 0x29C);
	for (int i = 0; i < count; i++) {
		void* a = (*(void***)((u8*)this + 0x2A0))[i];
		u32 status = *(u32*)((u8*)a + 0xF0);
		if ((status & 1) != 0)
			continue;
		if ((status & 2) != 0)
			continue;

		if (this->unk2A4 != nullptr
		    && this->unk2A4 == a)
			continue;

		Vec diff;
		diff.x = *(f32*)((u8*)a + 0x10) - gpMarioPos->x;
		diff.y = *(f32*)((u8*)a + 0x14) - gpMarioPos->y;
		diff.z = *(f32*)((u8*)a + 0x18) - gpMarioPos->z;
		f32 distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
		if (distSq >= bestDistSq)
			continue;

		f32 tan = *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x40);
		JGeometry::TVec2<f32> screen;
		CLBCalc2DFPos(&screen,
		              (MtxPtr)((u8*)this + 0x1EC),
		              (MtxPtr)((u8*)this + 0x16C),
		              *(const Vec*)((u8*)a + 0x10),
		              nullptr, false);
		if (!inViewCone(screen, tan))
			continue;

		s16 marioAng = *gpMarioAngleY;
		f32 deg      = (f32)(s16)(marioAng ^ 0x8000) * (360.0f / 65536.0f);
		f32 farClip
		    = *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x68);
		if (MsIsInSight(*gpMarioPos, deg,
		                *(const JGeometry::TVec3<f32>*)((u8*)a + 0x10),
		                farClip, distSq, -1.0f)) {
			picked     = a;
			bestDistSq = distSq;
		}
	}

	return picked;
}

void CPolarSubCamera::execNoticeOnOffProc_(EnumNoticeOnOffMode mode)
{
	switch ((int)mode) {
	case 0:
		this->unk2A4 = nullptr;
		*(u16*)((u8*)this + 0x64) &= ~0x20;
		break;

	case 1: {
		void* actor = getNoticeActor_();
		if (actor != this->unk2A4 && actor == nullptr) {
			this->unk2A4 = nullptr;
			*(u16*)((u8*)this + 0x64) &= ~0x20;
		}
		break;
	}

	case 2: {
		void* actor = getNoticeActor_();
		if (actor != this->unk2A4 && actor != nullptr) {
			this->unk2A4 = actor;
			*(u16*)((u8*)this + 0x64) |= 0x20;
		}
		break;
	}

	default:
		break;
	}
}

void CPolarSubCamera::calcNoticeTargetYrot_(const Vec& target)
{
	JGeometry::TVec3<f32> marioPos
	    = *(const JGeometry::TVec3<f32>*)gpCameraMario;

	f32 sqDZ  = CLBSquared<f32>(marioPos.z - target.z);
	f32 sqXZ  = CLBSquared<f32>(marioPos.x - target.x) + sqDZ;
	f32 farSq = CLBSquared<f32>(
	    *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x90));
	f32 nearSq = CLBSquared<f32>(
	    *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0xA4));

	if (sqXZ > farSq) {
		Vec diff;
		diff.x = marioPos.x - target.x;
		diff.y = marioPos.y - target.y;
		diff.z = marioPos.z - target.z;
		MsVECNormalize(&diff, &diff);

		f32 dx  = diff.x * 500.0f + marioPos.x;
		f32 dz  = diff.z * 500.0f + marioPos.z;
		s16 ang = matan(dz - unk8C.z, dx - unk8C.x);

		s16 diffAng = unkA6 - ang;
		s16 absAng;
		if ((s16)diffAng < 0)
			absAng = -(s16)diffAng;
		else
			absAng = (s16)diffAng;
		f32 fAbsAng = (f32)absAng * (1.0f / 32768.0f);

		f32 ratio;
		if (sqXZ > nearSq) {
			ratio = 1.0f;
		} else {
			ratio = CLBCalcRatio<f32>(farSq, nearSq, sqXZ);
		}

		f32 inb = CLBLinearInbetween<f32>(
		    1.0f, *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0xB8),
		    unkA8);

		f32 deg
		    = (f32) * (s16*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x7C);
		f32 speed = fAbsAng * deg * ratio * inb
		           * *(f32*)((u8*)this + 0x288);
		if (speed > 32766.998f)
			speed = 32766.998f;

		s16 deltaSpeed = CLBRoundf<s16>(speed);
		CLBChaseGeneralConstantSpecifySpeed<s16>(&unkA6, ang, deltaSpeed);
	}
}

void CPolarSubCamera::getNozzleTopPos_(JGeometry::TVec3<f32>* out) const
{
	if (SMS_GetMarioWaterGun() == nullptr) {
		TCameraMarioData* mario = gpCameraMario;
		out->x                  = mario->mPosX;
		out->y                  = mario->mPosY;
		out->z                  = mario->mPosZ;
	} else {
		MtxPtr m = ((TWaterGun*)SMS_GetMarioWaterGun())->getNozzleMtx();
		out->x   = m[0][3];
		out->y   = m[1][3];
		out->z   = m[2][3];

		JGeometry::TVec3<f32> up(m[0][1], m[1][1], m[2][1]);
		up.normalize();
		up.x *= 30.0f;
		up.y *= 30.0f;
		up.z *= 30.0f;
		out->x += up.x;
		out->y += up.y;
		out->z += up.z;
	}
}

void CPolarSubCamera::ctrlLButtonCamera_()
{
	TMarioGamePad* pad = unk120;
	f32 stickX         = -pad->mCompSPos[4];
	f32 stickY         = -pad->mCompSPos[5];

	if (unk7C == 0) {
		if (!SMS_CheckMarioFlag(0x8000)) {
			TCameraMarioData* mario = gpCameraMario;
			unk8C.x                 = mario->mPosX;
			unk8C.y                 = mario->mPosY;
			unk8C.z                 = mario->mPosZ;
		} else if (SMS_GetMarioWaterGun() == nullptr) {
			TCameraMarioData* mario = gpCameraMario;
			unk8C.x                 = mario->mPosX;
			unk8C.y                 = mario->mPosY;
			unk8C.z                 = mario->mPosZ;
		} else {
			MtxPtr m = ((TWaterGun*)SMS_GetMarioWaterGun())->getNozzleMtx();
			unk8C.x  = m[0][3];
			unk8C.y  = m[1][3];
			unk8C.z  = m[2][3];

			JGeometry::TVec3<f32> up(m[0][1], m[1][1], m[2][1]);
			up.normalize();
			up.x *= 30.0f;
			up.y *= 30.0f;
			up.z *= 30.0f;
			unk8C.x += up.x;
			unk8C.y += up.y;
			unk8C.z += up.z;
		}
	}

	if (*(s32*)((u8*)this + 0x78) == 0) {
		u16& flags = *(u16*)((u8*)this + 0x64);
		if ((flags & 0x20) != 0) {
			void* notice = this->unk2A4;
			if (notice == nullptr) {
				rotateY_ByStickX_(stickX);
			} else {
				calcNoticeTargetYrot_(*(const Vec*)((u8*)notice + 0x10));
			}
		} else {
			rotateY_ByStickX_(stickX);
		}
		rotateX_ByStickY_(stickY);
	}

	calcPosAndAt_();
}
