#include <Camera/Camera.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/cameralib.hpp>
#include <Enemy/Conductor.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JGeometry.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Strategic/HitActor.hpp>
#include <Strategic/LiveManager.hpp>
#include <System/MarioGamePad.hpp>

template <> f32 CLBLinearInbetween<f32>(f32, f32, f32);
template <> f32 CLBCalcRatio<f32>(f32, f32, f32);
template <> f32 CLBSquared<f32>(f32);
template <> s16 CLBRoundf<s16>(f32);
template <> BOOL CLBChaseGeneralConstantSpecifySpeed<s16>(s16*, s16, s16);

class TWaterGun {
public:
	MtxPtr getNozzleMtx();
};

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

static const char* sNoticeActorManagerName[] = {
	"ヒノクリマネージャー",
	"ヒノクリ２マネージャー",
	"ボスパックンマネージャー",
	nullptr,
};

const char* bossGesoViewObjName = "ボスゲッソー";

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
	unk2A0       = new TLiveActor*[16];
	mNoticeActor = nullptr;
	unk29C       = 0;

	for (int nameOffset = 0;
	     *(const char**)((u8*)sNoticeActorManagerName + nameOffset) != nullptr;
	     nameOffset += 4) {
		TLiveManager* mgr
		    = (TLiveManager*)gpConductor->getManagerByName(
		        *(const char**)((u8*)sNoticeActorManagerName + nameOffset));
		if (mgr == nullptr)
			continue;
		int n = mgr->mObjNum;
		for (int j = 0; j < n; j++) {
			unk2A0[unk29C] = mgr->getObj(j);
			unk29C++;
		}
	}

	unk2A8 = (TLiveActor*)JDrama::TNameRefGen::getInstance()
	              ->getRootNameRef()
	              ->search(bossGesoViewObjName);
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
			f32 sqX     = diff.x * diff.x;
			f32 sqY     = diff.y * diff.y;
			f32 sqZ     = diff.z * diff.z;
			f32 distSq  = sqX + sqY + sqZ;
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
		f32 sqX    = diff.x * diff.x;
		f32 sqY    = diff.y * diff.y;
		f32 sqZ    = diff.z * diff.z;
		f32 distSq = sqX + sqY + sqZ;
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
		f32 deg      = (f32)marioAng * (360.0f / 65536.0f);
		f32 farClip
		    = *(f32*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x68);
		if (MsIsInSight(*gpMarioPos, deg,
		                *(const JGeometry::TVec3<f32>*)((u8*)a + 0x10),
		                distSq, farClip, -1.0f)) {
			picked     = a;
			bestDistSq = distSq;
		}
	}

	return picked;
}

void CPolarSubCamera::execNoticeOnOffProc_(EnumNoticeOnOffMode mode)
{
	switch (mode) {
	case NOTICE_MODE_UNK0:
		mNoticeActor = nullptr;
		unk64 &= ~CAMERA_FLAG_NOTICE_ACTIVE;
		break;

	case NOTICE_MODE_UNK1: {
		void* actor = getNoticeActor_();
		if (actor != mNoticeActor && actor == nullptr) {
			mNoticeActor = nullptr;
			unk64 &= ~CAMERA_FLAG_NOTICE_ACTIVE;
		}
		break;
	}

	case NOTICE_MODE_UNK2: {
		void* actor = getNoticeActor_();
		if (actor != mNoticeActor && actor != nullptr) {
			mNoticeActor = (TLiveActor*)actor;
			unk64 |= CAMERA_FLAG_NOTICE_ACTIVE;
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
		JGeometry::TVec3<f32> diff(marioPos.x - target.x,
		                           marioPos.y - target.y,
		                           marioPos.z - target.z);
		MsVECNormalize(&diff, &diff);

		f32 dx  = diff.x * 500.0f + marioPos.x;
		f32 dz  = diff.z * 500.0f + marioPos.z;
		s16 ang = matan(dz - mCurrentTarget.mTarget.z, dx - mCurrentTarget.mTarget.x);

		s16 diffAng = mCurrentTarget.mYaw - ang;
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
		    mCurrentTarget.unk28);

		f32 deg
		    = (f32) * (s16*)((u8*)*(void**)((u8*)this + 0x2D0) + 0x7C);
		f32 speed = fAbsAng * deg * ratio * inb
		           * *(f32*)((u8*)this + 0x288);
		if (speed > 32766.998f)
			speed = 32766.998f;

		s16 deltaSpeed = CLBRoundf<s16>(speed);
		CLBChaseGeneralConstantSpecifySpeed<s16>(&mCurrentTarget.mYaw, ang,
		                                               deltaSpeed);
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
		up *= 30.0f;
		*out += up;
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
			mCurrentTarget.mTarget.x                 = mario->mPosX;
			mCurrentTarget.mTarget.y                 = mario->mPosY;
			mCurrentTarget.mTarget.z                 = mario->mPosZ;
		} else if (SMS_GetMarioWaterGun() == nullptr) {
			TCameraMarioData* mario = gpCameraMario;
			mCurrentTarget.mTarget.x                 = mario->mPosX;
			mCurrentTarget.mTarget.y                 = mario->mPosY;
			mCurrentTarget.mTarget.z                 = mario->mPosZ;
		} else {
			MtxPtr m = ((TWaterGun*)SMS_GetMarioWaterGun())->getNozzleMtx();
			mCurrentTarget.mTarget.x  = m[0][3];
			mCurrentTarget.mTarget.y  = m[1][3];
			mCurrentTarget.mTarget.z  = m[2][3];

			JGeometry::TVec3<f32> up(m[0][1], m[1][1], m[2][1]);
			up.normalize();
			up.x *= 30.0f;
			up.y *= 30.0f;
			up.z *= 30.0f;
			mCurrentTarget.mTarget.x += up.x;
			mCurrentTarget.mTarget.y += up.y;
			mCurrentTarget.mTarget.z += up.z;
		}
	}

	if (*(u32*)((u8*)this + 0x78) == 0) {
		u16& flags = *(u16*)((u8*)this + 0x64);
		if ((flags & 0x20) != 0) {
			if (stickX != 0.0f) {
				rotateY_ByStickX_(stickX);
			} else {
				void* notice = this->unk2A4;
				calcNoticeTargetYrot_(*(const Vec*)((u8*)notice + 0x10));
			}
		} else {
			rotateY_ByStickX_(stickX);
		}
		rotateX_ByStickY_(stickY);
	}

	calcPosAndAt_();
}
