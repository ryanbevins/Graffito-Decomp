#include <MarioUtil/MtxUtil.hpp>

#include <Camera/Camera.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <Map/Map.hpp>
#include <Map/MapData.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <Strategic/HitActor.hpp>
#include <dolphin/mtx.h>
#include <printf.h>

void MtxToQuat(MtxPtr mtx, Quaternion* quat)
{
	f32 diag0 = mtx[0][0];
	f32 diag1 = mtx[1][1];
	f32 diag2 = mtx[2][2];
	f32 trace = 1.0f + diag0 + diag1 + diag2;

	if (trace >= 1.0f) {
		f32 root = trace;
		if (root > 0.0f)
			root = JGeometry::TUtil<f32>::sqrt(root);

		f32 doubleRoot = 2.0f * root;
		f32 scale      = 1.0f / doubleRoot;
		quat->w        = 0.25f * doubleRoot;
		quat->x        = (mtx[2][1] - mtx[1][2]) * scale;
		quat->y        = (mtx[0][2] - mtx[2][0]) * scale;
		quat->z        = (mtx[1][0] - mtx[0][1]) * scale;
	} else {
		int i = diag0 > diag1 ? 0 : 1;
		if (diag2 > mtx[i][i])
			i = 2;

		int j = (i + 1) % 3;
		int k = (j + 1) % 3;
		f32 root = 1.0f + mtx[i][i] - mtx[j][j] - mtx[k][k];
		if (root > 0.0f)
			root = JGeometry::TUtil<f32>::sqrt(root);

		f32 doubleRoot = 2.0f * root;
		f32 scale      = 1.0f / doubleRoot;
		f32* q         = (f32*)quat;
		q[i]           = 0.25f * doubleRoot;
		q[j]           = (mtx[i][j] + mtx[j][i]) * scale;
		q[k]           = (mtx[i][k] + mtx[k][i]) * scale;
		quat->w        = (mtx[k][j] - mtx[j][k]) * scale;
	}
}

void TMtxTimeLag::calc(MtxPtr mtx)
{
	if (mFlags & 2) {
		mFlags &= ~2;
		unk08.zero();
		unk14.set(mtx[0][3], mtx[1][3], mtx[2][3]);
		unk20.x = 0.0f;
		unk20.y = 0.0f;
		unk20.z = 0.0f;
		unk20.w = 0.0f;

		Quaternion quat;
		MtxToQuat(mtx, &quat);
		unk30 = *(JGeometry::TQuat4<f32>*)&quat;
		return;
	}

	TDeParams* params = getSwingRZParams();
	unk08.x += params->mPosAccel.value * (mtx[0][3] - unk14.x);
	unk08.y += params->mPosAccel.value * (mtx[1][3] - unk14.y);
	unk08.z += params->mPosAccel.value * (mtx[2][3] - unk14.z);

	unk08.x *= params->mPosBrake.value;
	unk08.y *= params->mPosBrake.value;
	unk08.z *= params->mPosBrake.value;

	unk14.x += unk08.x;
	unk14.y += unk08.y;
	unk14.z += unk08.z;

	f32 limit = params->mPosLimit.value;
	if (unk14.x < mtx[0][3] - limit)
		unk14.x = mtx[0][3] - limit;
	if (unk14.x > mtx[0][3] + limit)
		unk14.x = mtx[0][3] + limit;
	if (unk14.y < mtx[1][3] - limit)
		unk14.y = mtx[1][3] - limit;
	if (unk14.y > mtx[1][3] + limit)
		unk14.y = mtx[1][3] + limit;
	if (unk14.z < mtx[2][3] - limit)
		unk14.z = mtx[2][3] - limit;
	if (unk14.z > mtx[2][3] + limit)
		unk14.z = mtx[2][3] + limit;

	f32 len0 = mtx[0][0] * mtx[0][0] + mtx[1][0] * mtx[1][0]
	           + mtx[2][0] * mtx[2][0];
	if (len0 > 0.0f)
		len0 = JGeometry::TUtil<f32>::sqrt(len0);
	f32 len1 = mtx[0][1] * mtx[0][1] + mtx[1][1] * mtx[1][1]
	           + mtx[2][1] * mtx[2][1];
	if (len1 > 0.0f)
		len1 = JGeometry::TUtil<f32>::sqrt(len1);
	f32 len2 = mtx[0][2] * mtx[0][2] + mtx[1][2] * mtx[1][2]
	           + mtx[2][2] * mtx[2][2];
	if (len2 > 0.0f)
		len2 = JGeometry::TUtil<f32>::sqrt(len2);

	Mtx normalized;
	normalized[0][0] = mtx[0][0] / len0;
	normalized[1][0] = mtx[1][0] / len0;
	normalized[2][0] = mtx[2][0] / len0;
	normalized[0][1] = mtx[0][1] / len1;
	normalized[1][1] = mtx[1][1] / len1;
	normalized[2][1] = mtx[2][1] / len1;
	normalized[0][2] = mtx[0][2] / len2;
	normalized[1][2] = mtx[1][2] / len2;
	normalized[2][2] = mtx[2][2] / len2;

	Quaternion quat;
	MtxToQuat(normalized, &quat);
	if (unk30.x * quat.x + unk30.y * quat.y + unk30.z * quat.z
	        + unk30.w * quat.w
	    < 0.0f) {
		quat.x = -quat.x;
		quat.y = -quat.y;
		quat.z = -quat.z;
		quat.w = -quat.w;
	}

	unk20.x += params->mQuatAccel.value * (quat.x - unk30.x);
	unk20.y += params->mQuatAccel.value * (quat.y - unk30.y);
	unk20.z += params->mQuatAccel.value * (quat.z - unk30.z);
	unk20.w += params->mQuatAccel.value * (quat.w - unk30.w);

	unk20.x *= params->mQuatBrake.value;
	unk20.y *= params->mQuatBrake.value;
	unk20.z *= params->mQuatBrake.value;
	unk20.w *= params->mQuatBrake.value;

	unk30.x += unk20.x;
	unk30.y += unk20.y;
	unk30.z += unk20.z;
	unk30.w += unk20.w;

	if (mFlags & 1)
		PSMTXQuat(mtx, (Quaternion*)&unk30);

	if (mFlags & 1) {
		mtx[0][0] *= len0;
		mtx[1][0] *= len0;
		mtx[2][0] *= len0;
		mtx[0][1] *= len1;
		mtx[1][1] *= len1;
		mtx[2][1] *= len1;
		mtx[0][2] *= len2;
		mtx[1][2] *= len2;
		mtx[2][2] *= len2;
		mtx[0][3] = unk14.x;
		mtx[1][3] = unk14.y;
		mtx[2][3] = unk14.z;
	}
}

int TMtxTimeLagCallBack(J3DNode* node, int timing)
{
	if (timing == 0)
		((TMtxTimeLag*)node->mCallBackUserData)->calc(J3DSys::mCurrentMtx);
	return 1;
}

void TMtxSwingRZ::calcLocalXY(MtxPtr mtx, Vec* x_out, Vec* y_out)
{
	if (mFlags & 2) {
		mFlags &= ~2;
		unk14.zero();
		unk08.set(mtx[0][3], mtx[1][3] - mParams.mL.value, mtx[2][3]);

		x_out->x = mtx[0][0];
		x_out->y = mtx[1][0];
		x_out->z = mtx[2][0];
		y_out->x = mtx[0][1];
		y_out->y = mtx[1][1];
		y_out->z = mtx[2][1];
		return;
	}

	PSVECAdd((Vec*)&unk14, (Vec*)&mParams.mAcc.value, (Vec*)&unk14);
	PSVECScale((Vec*)&unk14, (Vec*)&unk14, mParams.mBrake.value);
	PSVECAdd((Vec*)&unk14, (Vec*)&unk08, (Vec*)&unk08);

	Vec target;
	target.x = mtx[0][3];
	target.y = mtx[1][3];
	target.z = mtx[2][3];

	Vec dir;
	dir.x = unk08.x - target.x;
	dir.y = unk08.y - target.y;
	dir.z = unk08.z - target.z;
	PSVECNormalize(&dir, &dir);
	PSVECScale(&dir, &dir, mParams.mL.value);

	Vec newPos;
	newPos.x = target.x + dir.x;
	newPos.y = target.y + dir.y;
	newPos.z = target.z + dir.z;

	Vec delta;
	PSVECSubtract(&unk08, &newPos, &delta);
	PSVECAdd((Vec*)&unk14, &delta, (Vec*)&unk14);
	unk08.set(newPos);

	Vec zAxis;
	zAxis.x = mtx[0][2];
	zAxis.y = mtx[1][2];
	zAxis.z = mtx[2][2];

	x_out->x = unk08.x - target.x;
	x_out->y = unk08.y - target.y;
	x_out->z = unk08.z - target.z;
	PSVECNormalize(x_out, x_out);
	PSVECCrossProduct(&zAxis, x_out, y_out);
	PSVECNormalize(x_out, x_out);
	PSVECNormalize(y_out, y_out);
	PSVECCrossProduct(y_out, &zAxis, x_out);
}

int TMtxSwingRZCallBack(J3DNode* node, int timing)
{
	if (timing == 0) {
		TMtxSwingRZ* effect = (TMtxSwingRZ*)node->mCallBackUserData;
		Vec xAxis;
		Vec yAxis;
		effect->calcLocalXY(J3DSys::mCurrentMtx, &xAxis, &yAxis);
		if (effect->mFlags & 1) {
			J3DSys::mCurrentMtx[0][0] = xAxis.x;
			J3DSys::mCurrentMtx[1][0] = xAxis.y;
			J3DSys::mCurrentMtx[2][0] = xAxis.z;
			J3DSys::mCurrentMtx[0][1] = yAxis.x;
			J3DSys::mCurrentMtx[1][1] = yAxis.y;
			J3DSys::mCurrentMtx[2][1] = yAxis.z;
		}
	}
	return 1;
}

int TMtxSwingRZReverseXZCallBack(J3DNode* node, int timing)
{
	if (timing == 0) {
		TMtxSwingRZ* effect = (TMtxSwingRZ*)node->mCallBackUserData;
		Vec xAxis;
		Vec yAxis;
		effect->calcLocalXY(J3DSys::mCurrentMtx, &xAxis, &yAxis);
		if (effect->mFlags & 1) {
			J3DSys::mCurrentMtx[0][0] = -xAxis.x;
			J3DSys::mCurrentMtx[1][0] = -xAxis.y;
			J3DSys::mCurrentMtx[2][0] = -xAxis.z;
			J3DSys::mCurrentMtx[0][1] = -yAxis.x;
			J3DSys::mCurrentMtx[1][1] = -yAxis.y;
			J3DSys::mCurrentMtx[2][1] = -yAxis.z;
		}
	}
	return 1;
}

void TMultiMtxEffect::setup(J3DModel* model, const char* prmLocation)
{
	mModel        = model;
	mMtxEffectTbl = new TMtxEffectBase*[mNumBones];

	for (u16 i = 0; i < mNumBones; ++i) {
		char* path = new char[0x40];
		snprintf(path, 0x40, "/%s/MtxEffect%d.prm", prmLocation, mBoneIDs[i]);

		switch (mMtxEffectType[i]) {
		case TMTX_EFFECT_TIME_LAG: {
			TMtxTimeLag* timeLag = new TMtxTimeLag(path);
			model->getModelData()
			    ->getJointNodePointer(mBoneIDs[i])
			    ->setCallBack(TMtxTimeLagCallBack);
			model->getModelData()
			    ->getJointNodePointer(mBoneIDs[i])
			    ->setCallBackUserData(timeLag);
			mMtxEffectTbl[i] = timeLag;
			break;
		}
		case TMTX_EFFECT_SWING_RZ: {
			TMtxSwingRZ* swingRz = new TMtxSwingRZ(path);
			model->getModelData()
			    ->getJointNodePointer(mBoneIDs[i])
			    ->setCallBack(TMtxSwingRZCallBack);
			model->getModelData()
			    ->getJointNodePointer(mBoneIDs[i])
			    ->setCallBackUserData(swingRz);
			mMtxEffectTbl[i] = swingRz;
			break;
		}
		case TMTX_EFFECT_SWING_RZ_REVERSE_XZ: {
			TMtxSwingRZ* swingRzReverse = new TMtxSwingRZReverseXZ(path);
			model->getModelData()
			    ->getJointNodePointer(mBoneIDs[i])
			    ->setCallBack(TMtxSwingRZReverseXZCallBack);
			model->getModelData()
			    ->getJointNodePointer(mBoneIDs[i])
			    ->setCallBackUserData(swingRzReverse);
			mMtxEffectTbl[i] = swingRzReverse;
			break;
		}
		}
	}
	for (u16 i = 0; i < mNumBones; ++i) {
		mMtxEffectTbl[i]->mFlags |= 2;
	}
}

void TMultiMtxEffect::setUserArea()
{
	for (u16 i = 0; i < mNumBones; i++) {
		mModel->getModelData()
		    ->getJointNodePointer(mBoneIDs[i])
		    ->setCallBackUserData(mMtxEffectTbl[i]);
	}
}

void SMS_MakeJointsToArc(J3DModel* model,
                         const JGeometry::TVec3<f32>& start,
                         const JGeometry::TVec3<f32>& up,
                         const JGeometry::TVec3<f32>& end)
{
	model->calc();

	JGeometry::TVec3<f32> dir;
	dir = end;
	dir.sub(start);
	f32 length = PSVECMag((Vec*)&dir);
	dir.scale(1.0f / length);

	JGeometry::TVec3<f32> upDir;
	upDir = up;
	if (upDir.x * upDir.x + upDir.y * upDir.y + upDir.z * upDir.z
	    <= 0.0000038146973f) {
		upDir.zero();
	} else {
		f32 len = upDir.x * upDir.x + upDir.y * upDir.y + upDir.z * upDir.z;
		upDir.scale(JGeometry::TUtil<f32>::inv_sqrt(len));
	}

	u16 count = model->getModelData()->getJointNum();
	for (u16 i = 0; i < count; ++i) {
		f32 t = (f32)i / (f32)(count - 1);
		JGeometry::TVec3<f32> axisA;
		axisA = dir * t;
		JGeometry::TVec3<f32> axisB;
		axisB = upDir * (1.0f - t);
		JGeometry::TVec3<f32> axis;
		axis = axisA;
		axis.add(axisB);
		if (axis.x * axis.x + axis.y * axis.y + axis.z * axis.z
		    <= 0.0000038146973f) {
			axis.zero();
		} else {
			f32 len = axis.x * axis.x + axis.y * axis.y + axis.z * axis.z;
			axis.scale(JGeometry::TUtil<f32>::inv_sqrt(len));
		}

		MtxPtr mtx = model->mNodeMatrices[i];
		JGeometry::TVec3<f32> oldZ;
		oldZ.x = mtx[0][2];
		oldZ.y = mtx[1][2];
		oldZ.z = mtx[2][2];

		JGeometry::TVec3<f32> yAxis;
		yAxis.cross(oldZ, axis);
		if (yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z
		    <= 0.0000038146973f) {
			yAxis.zero();
		} else {
			f32 len
			    = yAxis.x * yAxis.x + yAxis.y * yAxis.y + yAxis.z * yAxis.z;
			yAxis.scale(JGeometry::TUtil<f32>::inv_sqrt(len));
		}

		JGeometry::TVec3<f32> zAxis;
		zAxis.cross(axis, yAxis);
		if (zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z
		    <= 0.0000038146973f) {
			zAxis.zero();
		} else {
			f32 len
			    = zAxis.x * zAxis.x + zAxis.y * zAxis.y + zAxis.z * zAxis.z;
			zAxis.scale(JGeometry::TUtil<f32>::inv_sqrt(len));
		}

		f32 dist = (f32)i * (length / (f32)(count - 1));
		mtx[0][0] = axis.x;
		mtx[1][0] = axis.y;
		mtx[2][0] = axis.z;
		mtx[0][1] = yAxis.x;
		mtx[1][1] = yAxis.y;
		mtx[2][1] = yAxis.z;
		mtx[0][2] = zAxis.x;
		mtx[1][2] = zAxis.y;
		mtx[2][2] = zAxis.z;
		mtx[0][3] = start.x + axis.x * dist;
		mtx[1][3] = start.y + axis.y * dist;
		mtx[2][3] = start.z + axis.z * dist;
	}
}

void SMS_GetLightPerspectiveForEffectMtx(MtxPtr mtx)
{
	f32 far    = gpCamera->mFar;
	f32 near   = gpCamera->mNear;
	f32 aspect = gpCamera->mAspect;
	f32 fovy   = gpCamera->mFovy;
	C_MTXPerspective((Mtx44Ptr)mtx, fovy, aspect, near, far);
	mtx[2][0] = 0.0f;
	mtx[2][1] = 0.0f;
	mtx[2][2] = -1.0f;
	mtx[2][3] = 0.0f;
	mtx[3][0] = 0.0f;
	mtx[3][1] = 0.0f;
	mtx[3][2] = 0.0f;
	mtx[3][3] = 1.0f;
}

TRopePoint::TRopePoint() { }

TRope::TRope(u16 numPoints, const JGeometry::TVec3<f32>& pos,
             f32 segmentLength, f32 collisionRadius, f32 velocityScale,
             f32 accelY)
{
	mNumPoints       = numPoints;
	mPoints          = new TRopePoint[numPoints];
	mVelocityScale   = velocityScale;
	mAccelY          = accelY;
	mCollisionRadius = collisionRadius;

	for (int i = 0; i < numPoints; ++i) {
		TRopePoint& point = mPoints[i];
		point.mPosition   = pos;
		point.mPrevPos    = point.mPosition;
		point.mVelocity.zero();
		point.mSegmentLength = segmentLength;
		point.mFlags         = 0;
	}
}

void TRope::collision()
{
	for (int i = 0; i < mNumPoints; ++i) {
		TRopePoint& point = mPoints[i];
		if (point.mFlags & 1)
			continue;

		JGeometry::TVec3<f32>& pos = point.mPosition;
		const TBGCheckData* checkData;
		f32 roof = gpMap->checkRoof(pos.x, pos.y + mCollisionRadius, pos.z,
		                            &checkData);
		if (checkData != nullptr && !checkData->isIllegalData()
		    && pos.y + mCollisionRadius > roof) {
			pos.y = roof - 2.0f * mCollisionRadius;
		}

		f32 ground
		    = gpMap->checkGround(pos.x, pos.y + mCollisionRadius, pos.z,
		                         &checkData);
		ground += 3.0f;
		if (!checkData->isIllegalData() && pos.y < ground) {
			pos.y = ground;
		}

		f32 radius = mCollisionRadius;
		gpMap->isTouchedOneWallAndMoveXZ(&pos.x, pos.y, &pos.z, radius);
	}
}

void TRope::constraintHead(const JGeometry::TVec3<f32>& head)
{
	mPoints[0].mPosition = head;

	for (int i = 0; i < (int)mNumPoints - 1; ++i) {
		TRopePoint& point = mPoints[i];
		TRopePoint& next  = mPoints[i + 1];

		Vec dir = next.mPosition;
		dir.x -= point.mPosition.x;
		dir.y -= point.mPosition.y;
		dir.z -= point.mPosition.z;
		if (dir.x * dir.x + dir.y * dir.y + dir.z * dir.z
		    <= 0.0000038146973f) {
			dir.x = 0.0f;
			dir.y = 1.0f;
			dir.z = 0.0f;
		}

		PSVECNormalize(&dir, &dir);
		f32 length = point.mSegmentLength;
		dir.x *= length;
		dir.y *= length;
		dir.z *= length;
		next.mPosition = point.mPosition;
		next.mPosition.x += dir.x;
		next.mPosition.y += dir.y;
		next.mPosition.z += dir.z;
	}

	collision();
}

void TRope::constraintTail(const JGeometry::TVec3<f32>& tail)
{
	mPoints[mNumPoints - 1].mPosition = tail;

	for (int i = mNumPoints - 1; i > 0; --i) {
		TRopePoint& point = mPoints[i];
		TRopePoint& prev  = mPoints[i - 1];
		bool nearlySame
		    = (-0.0000038146973f
		           <= prev.mPosition.x - point.mPosition.x
		       && prev.mPosition.x - point.mPosition.x <= 0.0000038146973f)
		      && (-0.0000038146973f
		              <= prev.mPosition.y - point.mPosition.y
		          && prev.mPosition.y - point.mPosition.y
		                 <= 0.0000038146973f)
		      && (-0.0000038146973f
		              <= prev.mPosition.z - point.mPosition.z
		          && prev.mPosition.z - point.mPosition.z
		                 <= 0.0000038146973f);
		if (!nearlySame) {
			Vec dir = prev.mPosition;
			dir.x -= point.mPosition.x;
			dir.y -= point.mPosition.y;
			dir.z -= point.mPosition.z;
			PSVECNormalize(&dir, &dir);
			f32 length = point.mSegmentLength;
			dir.x *= length;
			dir.y *= length;
			dir.z *= length;
			prev.mPosition = point.mPosition;
			prev.mPosition.x += dir.x;
			prev.mPosition.y += dir.y;
			prev.mPosition.z += dir.z;
		}
	}

	collision();
}

void TRope::moveHead(const JGeometry::TVec3<f32>& head)
{
	for (int i = 0; i < mNumPoints; ++i) {
		TRopePoint& point = mPoints[i];
		point.mVelocity.y += mAccelY;
		point.mPosition += point.mVelocity;
	}

	constraintHead(head);

	for (int i = 0; i < mNumPoints; ++i) {
		TRopePoint& point = mPoints[i];
		JGeometry::TVec3<f32> velocity = point.mPosition - point.mPrevPos;
		point.mVelocity = velocity * mVelocityScale;
		point.mPrevPos  = point.mPosition;
	}
}

void SMS_GetActorMtx(const THitActor& actor, MtxPtr mtx)
{
	MsMtxSetTRS(mtx, actor.mPosition.x, actor.mPosition.y, actor.mPosition.z,
	            actor.mRotation.x, actor.mRotation.y, actor.mRotation.z,
	            actor.mScaling.x, actor.mScaling.y, actor.mScaling.z);
}
