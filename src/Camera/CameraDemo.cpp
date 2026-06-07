#include <Camera/Camera.hpp>
#include <Camera/CameraBck.hpp>
#include <Camera/CameraMapTool.hpp>
#include <Camera/cameralib.hpp>
#include <Camera/CameraMarioData.hpp>
#include <Camera/CameraInbetween.hpp>
#include <JSystem/JDrama/JDRActor.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JMath.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Player/MarioAccess.hpp>
#include <Player/MarioMain.hpp>
#include <System/MarDirector.hpp>
#include <System/MarioGamePad.hpp>
#include <dolphin/mtx.h>

extern "C" int snprintf(char*, unsigned long, const char*, ...);

extern const char* cCameraBckNameGate;

template <> s16 CLBRoundf<s16>(f32);

bool CPolarSubCamera::isNormalDeadDemo() const
{
	bool result = false;
	u16 flags   = *(u16*)((u8*)this + 0x64);
	if ((flags & 0x400) && !(flags & 0x800))
		result = true;
	return result;
}

bool CPolarSubCamera::isHellDeadDemo() const
{
	bool result = false;
	u16 flags   = *(u16*)((u8*)this + 0x64);
	if ((flags & 0x400) && (flags & 0x800))
		result = true;
	return result;
}

void CPolarSubCamera::execDeadDemoProc_()
{
	u16 timer = *(u16*)((u8*)this + 0x27C);
	if (timer != 0) {
		*(u16*)((u8*)this + 0x27C) = timer - 1;
		if (*(u16*)((u8*)this + 0x27C) != 0)
			return;
		*(u16*)((u8*)this + 0x64) |= 0x400;
		*(u32*)((u8*)this + 0x78) = 1;
		if (!(*(u16*)((u8*)this + 0x64) & 0x800))
			*(u16*)((u8*)this + 0x27E) = 1;
		return;
	}

	if (!SMS_CheckMarioFlag(0x400))
		return;

	bool shouldReturn  = true;
	TMarDirector* director = gpMarDirector;
	bool firstMatches      = true;
	u8 state               = director->unk124;
	if (state != 1 && state != 2)
		firstMatches = false;
	if (!firstMatches) {
		bool secondMatches = director->checkUnk124Thing2();
		if (!secondMatches)
			shouldReturn = false;
	}
	if (shouldReturn)
		return;
	*(u16*)((u8*)this + 0x27C) = 0x10;
}

void CPolarSubCamera::ctrlNormalDeadDemo_()
{
	{
		const TCameraMarioData* cam = gpCameraMario;
		unk8C.x                     = cam->mPosX;
		unk8C.y                     = cam->mPosY;
		unk8C.z                     = cam->mPosZ;
	}
	((TCameraInbetween*)*(void**)((u8*)this + 0x6C))
	    ->execCameraInbetween(
	        *(const JGeometry::TVec3<f32>*)((u8*)this + 0x10),
	        unk8C, *gpMarioPos);

	TCameraInbetween* inb
	    = (TCameraInbetween*)*(void**)((u8*)this + 0x6C);
	CLBChaseDecrease((f32*)((u8*)this + 0x3C), inb->mTargetAt.x, 0.03f, 0.0f);
	CLBChaseDecrease((f32*)((u8*)this + 0x40), inb->mTargetAt.y, 0.03f, 0.0f);
	CLBChaseDecrease((f32*)((u8*)this + 0x44), inb->mTargetAt.z, 0.03f, 0.0f);

	bool flagSet = false;
	if (gpMarioOriginal->mState & 0x1000)
		flagSet = true;
	if (flagSet)
		return;

	u16 t1 = *(u16*)((u8*)this + 0x27E);
	if (t1 != 0) {
		*(u16*)((u8*)this + 0x27E) = t1 - 1;
		if (*(u16*)((u8*)this + 0x27E) != 0)
			return;
		*(u16*)((u8*)this + 0x280) = 0x157;
		return;
	}

	u16 t2 = *(u16*)((u8*)this + 0x280);
	if (t2 == 0)
		return;

	*(u16*)((u8*)this + 0x280) = t2 - 1;

	Vec diff;
	diff.x = *(f32*)((u8*)this + 0x3C) - *(f32*)((u8*)this + 0x10);
	diff.y = *(f32*)((u8*)this + 0x40) - *(f32*)((u8*)this + 0x14);
	diff.z = *(f32*)((u8*)this + 0x44) - *(f32*)((u8*)this + 0x18);
	f32 mag2 = MsVECMag2(&diff);
	if (mag2 > 0.001f) {
		f32 v = 10500.0f * (1.0f / mag2);
		if (v > 80.0f)
			v = 80.0f;
		else if (v < 5.0f)
			v = 5.0f;
		CLBChaseConstantSpecifyFrame<f32>(
		    (f32*)((u8*)this + 0x48), v,
		    (f32)*(u16*)((u8*)this + 0x280));
	}
}

int CPolarSubCamera::getRestDemoFrames() const
{
	return *(int*)((u8*)*(void**)((u8*)this + 0x2B4) + 0x14);
}

bool CPolarSubCamera::isSimpleDemoCamera() const
{
	bool result = false;
	if (!isOnGoingDemoCamera()) {
		void* save       = *(void**)((u8*)this + 0x2B4);
		s32 currentFrame = *(s32*)((u8*)save + 0x14);
		if (currentFrame > 0 || (*(u8*)((u8*)save + 0xC) & 1))
			result = true;
	}
	return result;
}

void CPolarSubCamera::endDemoCamera()
{
	if (isOnGoingDemoCamera()) {
		if (mMode == 0x49) {
			((TCameraBck*)*(void**)((u8*)this + 0x2B0))->endDemo();
			void* save                = *(void**)((u8*)this + 0x2B4);
			*(int*)((u8*)save + 0x10) = 0;
			*(int*)((u8*)save + 0x14) = 0;
			changeCamModeSpecifyFrame_(-1, 1);
			((TMarioGamePad*)*(void**)((u8*)this + 0x120))
			    ->onNeutralMarioKey();
		}
	} else if (isSimpleDemoCamera()) {
		if (isSimpleDemoCamera()) {
			void* save                = *(void**)((u8*)this + 0x2B4);
			*(int*)((u8*)save + 0x10) = 0;
			*(int*)((u8*)save + 0x14) = 0;
			void* save2               = *(void**)((u8*)this + 0x2B4);
			*(u8*)((u8*)save2 + 0xC) &= ~1;
			((TMarioGamePad*)*(void**)((u8*)this + 0x120))
			    ->onNeutralMarioKey();
		}
	}
	*(u32*)((u8*)*(void**)((u8*)this + 0x2B4) + 0x0) = 0;
}

void CPolarSubCamera::startDemoCamera(const char* name,
                                       const JGeometry::TVec3<f32>* offset,
                                       s32 mode, f32 strength, bool flag)
{
	void* save = *(void**)((u8*)this + 0x2B4);
	*(const JGeometry::TVec3<f32>**)((u8*)save + 0x0) = offset;
	*(f32*)((u8*)*(void**)((u8*)this + 0x2B4) + 0x4) = strength;

	if (name == nullptr)
		return;

	bool didStart = false;
	if (((TCameraBck*)*(void**)((u8*)this + 0x2B0))->isFileExist(name)) {
		((TCameraBck*)*(void**)((u8*)this + 0x2B0))
		    ->startDemo(name, offset);
		s32 total
		    = ((TCameraBck*)*(void**)((u8*)this + 0x2B0))->getTotalDemoFrames();
		void* s2                = *(void**)((u8*)this + 0x2B4);
		*(s32*)((u8*)s2 + 0x10) = total;
		*(s32*)((u8*)s2 + 0x14) = total;
		changeCamModeSpecifyFrame_(0x49, 1);
		mNear = *(f32*)((u8*)*(void**)((u8*)this + 0x2D4) + 0x158);
		didStart = true;
	}

	if (didStart)
		return;

	bool isDemo = isOnGoingDemoCamera();
	if (isDemo)
		return;

	u16 keyCode          = JDrama::TNameRef::calcKeyCode(name);
	TCameraMapTool* tool = (TCameraMapTool*)gpCamMapToolTable->searchF(
	    keyCode, name);
	if (tool == nullptr)
		return;

	if (!flag) {
		void* s3                = *(void**)((u8*)this + 0x2B4);
		*(s32*)((u8*)s3 + 0x10) = -1;
		*(s32*)((u8*)s3 + 0x14) = -1;
		void* s4                = *(void**)((u8*)this + 0x2B4);
		*(u8*)((u8*)s4 + 0xC) |= 1;
	} else if (mode == -1) {
		s32 frames              = tool->unk2C;
		void* s5                = *(void**)((u8*)this + 0x2B4);
		*(s32*)((u8*)s5 + 0x10) = frames;
		*(s32*)((u8*)s5 + 0x14) = frames;
	} else {
		void* s6                = *(void**)((u8*)this + 0x2B4);
		*(s32*)((u8*)s6 + 0x10) = mode;
		*(s32*)((u8*)s6 + 0x14) = mode;
	}

	changeCamModeSpecifyCamMapTool_(tool);
}

void CPolarSubCamera::startGateDemoCamera(const JDrama::TActor* actor)
{
	char buf[128];
	snprintf(buf, 128, "%s\x91\x4F\x83\x4A\x83\x81\x83\x89",
	         actor->getName());
	u16 keyCode          = JDrama::TNameRef::calcKeyCode(buf);
	TCameraMapTool* tool = (TCameraMapTool*)gpCamMapToolTable->searchF(
	    keyCode, buf);
	if (tool != nullptr) {
		void* s1 = *(void**)((u8*)this + 0x2B4);
		*(u8*)((u8*)s1 + 0xC) |= 1;
		changeCamModeSpecifyCamMapToolAndFrame_(tool, 0x3C);
		*(u16*)((u8*)this + 0x64) |= 0x200;
		((TCameraBck*)*(void**)((u8*)this + 0x2B0))
		    ->startDemo(cCameraBckNameGate, nullptr);
		s32 total
		    = ((TCameraBck*)*(void**)((u8*)this + 0x2B0))->getTotalDemoFrames();
		void* s2                = *(void**)((u8*)this + 0x2B4);
		*(s32*)((u8*)s2 + 0x10) = total;
		*(s32*)((u8*)s2 + 0x14) = total;
	}

	snprintf(buf, 128, "%s\x83\x4A\x83\x81\x83\x89",
	         actor->getName());
	u16 keyCode2 = JDrama::TNameRef::calcKeyCode(buf);
	TCameraMapTool* tool2
	    = (TCameraMapTool*)gpCamMapToolTable->searchF(keyCode2, buf);
	if (tool2 != nullptr) {
		void* s3 = *(void**)((u8*)this + 0x2B4);
		*(TCameraMapTool**)((u8*)s3 + 0x8) = tool2;
	}
}

void CPolarSubCamera::updateGateDemoCamera_()
{
	f32 fov;
	((TCameraBck*)*(void**)((u8*)this + 0x2B0))
	    ->updateDemo(nullptr, nullptr, nullptr, &fov);

	void* save               = *(void**)((u8*)this + 0x2B4);
	TCameraInbetween* inb    = (TCameraInbetween*)*(void**)((u8*)this + 0x6C);
	TCameraMapTool* tool     = *(TCameraMapTool**)((u8*)this + 0x70);
	TCameraMapTool* saveTool = *(TCameraMapTool**)((u8*)save + 0x8);
	s32 frameCount           = inb->mFrameCount;
	if (tool != saveTool && frameCount > 0) {
		CLBChaseConstantSpecifyFrame<f32>(
		    &mFovy, fov, (f32)frameCount);
	} else {
		mFovy = fov;
	}

	C_MTXPerspective((Mtx44Ptr)((u8*)this + 0x16C), mFovy, mAspect, mNear, mFar);

	void* save2          = *(void**)((u8*)this + 0x2B4);
	void* endTool        = *(void**)((u8*)save2 + 0x8);
	if (*(void**)((u8*)this + 0x70) != endTool) {
		s32 framesLeft
		    = *(s32*)((u8*)save2 + 0x10) - *(s32*)((u8*)save2 + 0x14);
		if (framesLeft > 0xF0)
			changeCamModeSpecifyCamMapToolAndFrame_(
			    (TCameraMapTool*)endTool, 0x78);
	}
}

void CPolarSubCamera::updateDemoCamera_(bool flag)
{
	if (flag) {
		bool isDemo = isOnGoingDemoCamera();
		if (isDemo) {
			JGeometry::TVec3<f32>* at
			    = (JGeometry::TVec3<f32>*)((u8*)this + 0x124);
			JGeometry::TVec3<f32>* eye
			    = (JGeometry::TVec3<f32>*)((u8*)this + 0x148);
			((TCameraBck*)*(void**)((u8*)this + 0x2B0))
			    ->updateDemo(at, eye, &mUp, &mFovy);
			void* save   = *(void**)((u8*)this + 0x2B4);
			f32 strength = *(f32*)((u8*)save + 0x4);
			if (0.0f != strength) {
				s16 angle = CLBRoundf<s16>(182.04445f * strength);
				JGeometry::TVec3<f32> off(0.0f, 0.0f, 0.0f);
				void* save2 = *(void**)((u8*)this + 0x2B4);
				if (*(void**)((u8*)save2 + 0x0) != nullptr) {
					off = *(const JGeometry::TVec3<f32>*)(
					    *(void**)((u8*)save2 + 0x0));
				}

				u16 keyAngle = (u16)angle;
				f32 atX      = at->x - off.x;
				f32 atY      = at->y - off.y;
				f32 atZ      = at->z - off.z;
				f32 sin      = jmaSinTable[keyAngle >> jmaSinShift];
				f32 cos      = jmaCosTable[keyAngle >> jmaSinShift];
				JGeometry::TVec3<f32> rotated;
				rotated.x = atX * cos + atZ * sin;
				rotated.y = atY;
				rotated.z = -atX * sin + atZ * cos;
				JGeometry::TVec3<f32> rotatedAt = off + rotated;
				*at = rotatedAt;

				f32 eyeX = eye->x - off.x;
				f32 eyeY = eye->y - off.y;
				f32 eyeZ = eye->z - off.z;
				JGeometry::TVec3<f32> rotE;
				rotE.x = eyeX * cos + eyeZ * sin;
				rotE.y = eyeY;
				rotE.z = -eyeX * sin + eyeZ * cos;
				JGeometry::TVec3<f32> rotatedEye = off + rotE;
				*eye = rotatedEye;

				f32 upX  = mUp.x;
				f32 upZ  = mUp.z;
				f32 sin2 = jmaSinTable[keyAngle >> jmaSinShift];
				f32 cos2 = jmaCosTable[keyAngle >> jmaSinShift];
				mUp.x    = upX * cos2 + upZ * sin2;
				mUp.z    = -upX * sin2 + upZ * cos2;
			}
			calcFinalPosAndAt_();
			C_MTXPerspective((Mtx44Ptr)((u8*)this + 0x16C),
			                 mFovy, mAspect, mNear, mFar);
			C_MTXLookAt((MtxPtr)((u8*)this + 0x1EC),
			            (Vec*)at,
			            (Vec*)&mUp,
			            (Vec*)eye);
		}
	}

	void* save     = *(void**)((u8*)this + 0x2B4);
	s32* curFrame  = (s32*)((u8*)save + 0x14);
	s32 remaining  = *curFrame;
	if (remaining > 0) {
		*curFrame   = remaining - 1;
		void* save2 = *(void**)((u8*)this + 0x2B4);
		if (*(s32*)((u8*)save2 + 0x14) == 0) {
			((TMarioGamePad*)*(void**)((u8*)this + 0x120))
			    ->onNeutralMarioKey();
			void* save3                = *(void**)((u8*)this + 0x2B4);
			*(s32*)((u8*)save3 + 0x10) = 0;
			*(s32*)((u8*)save3 + 0x14) = 0;
			bool isDemo2 = isOnGoingDemoCamera();
			if (!isDemo2) {
				void* save4 = *(void**)((u8*)this + 0x2B4);
				*(u8*)((u8*)save4 + 0xC) |= 1;
			}
		}
	}
}
