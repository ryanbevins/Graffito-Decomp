#include <Camera/CameraBck.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DTransform.hpp>
#include <JSystem/JUtility/JUTNameTab.hpp>
#include <M3DUtil/MActor.hpp>
#include <M3DUtil/MActorData.hpp>
#include <MarioUtil/ModelUtil.hpp>

const char* cPositionJointName   = "cam_position";
const char* cLookatJointName     = "cam_interest";
const char* cCameraBckVolumeName = "/scene/map/camera";

static const char* sAddBckFileNameTable[] = {
	"/common/camera/camera_demo_shine_get_inside.bck",
	"/common/camera/camera_demo_shine_get_outside.bck",
	"/common/camera/camera_demo_gate_in.bck",
	nullptr,
};

void TCameraBck::setFrame(f32 frame)
{
	if ((mActor->getCurAnmIdx(0) != -1) ? true : false) {
		J3DFrameCtrl* ctrl = mActor->getFrameCtrl(0);
		if (ctrl != nullptr) {
			ctrl->setFrame(frame);
		}
	}
}

bool TCameraBck::updateDemo(JGeometry::TVec3<f32>* pos,
                            JGeometry::TVec3<f32>* lookat,
                            JGeometry::TVec3<f32>* up, f32* fov)
{
	mActor->calcAnm();

	if (pos != nullptr) {
		Mtx& m = *mPositionJointMtx;
		pos->x = m[0][3];
		pos->y = m[1][3];
		pos->z = m[2][3];
	}
	if (lookat != nullptr) {
		Mtx& m    = *mLookatJointMtx;
		lookat->x = m[0][3];
		lookat->y = m[1][3];
		lookat->z = m[2][3];
	}
	if (up != nullptr) {
		Mtx& m = *mPositionJointMtx;
		up->x  = m[0][1];
		up->y  = m[1][1];
		up->z  = m[2][1];
	}
	if (fov != nullptr) {
		J3DAnmTransform* bckAnm
		    = mActor->unkC != nullptr ? mActor->unkC->unk24 : nullptr;
		if (bckAnm != nullptr) {
			J3DTransformInfo info;
			bckAnm->getTransform((u16)mFovJointIdx, &info);
			*fov = info.mScale.y;
		}
	}

	if (mOffset != nullptr) {
		if (pos != nullptr) {
			pos->x += mOffset->x;
			pos->y += mOffset->y;
			pos->z += mOffset->z;
		}
		if (lookat != nullptr) {
			lookat->x += mOffset->x;
			lookat->y += mOffset->y;
			lookat->z += mOffset->z;
		}
	}

	bool result        = true;
	J3DFrameCtrl* ctrl = mActor->getFrameCtrl(0);
	if (ctrl != nullptr) {
		u8 stateByte = *((u8*)ctrl + 5);
		if ((stateByte & 1) == 0)
			result = false;
	}
	return result;
}

void TCameraBck::endDemo() { mActor->setBckFromIndex(-1); }

s32 TCameraBck::getTotalDemoFrames() const
{
	s32 result = 0;
	J3DFrameCtrl* ctrl = mActor->getFrameCtrl(0);
	if (ctrl != nullptr) {
		u8 mode = *((u8*)ctrl + 4);
		if (mode != 0) {
			result = -1;
		} else {
			s16 end = *(s16*)((u8*)ctrl + 8);
			result = (end + 1) << 1;
		}
	}
	return result;
}

void TCameraBck::startDemo(const char* name,
                           const JGeometry::TVec3<f32>* offset)
{
	mActor->setBck(name);
	mOffset = offset;
}

bool TCameraBck::isFileExist(const char* name) const
{
	return mActor->checkAnmFileExist(name, 0);
}

TCameraBck::TCameraBck()
{
	mOffset           = nullptr;
	SDLModel* model   = SMS_CreateMinimumSDLModel("/common/camera/camera_model.bmd");
	mAnmData          = new MActorAnmData();
	mAnmData->init(cCameraBckVolumeName, sAddBckFileNameTable);
	mActor = new MActor(mAnmData);
	mActor->setModel((J3DModel*)model, 0);

	J3DModel* j3dModel = (J3DModel*)model;
	JUTNameTab* joints = j3dModel->getModelData()->getJointName();
	mFovJointIdx       = joints->getIndex(cPositionJointName);
	mPositionJointMtx  = (Mtx*)j3dModel->getAnmMtx((u16)mFovJointIdx);
	mLookatJointMtx
	    = (Mtx*)j3dModel->getAnmMtx((u16)joints->getIndex(cLookatJointName));
}
