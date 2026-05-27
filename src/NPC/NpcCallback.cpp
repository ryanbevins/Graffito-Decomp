#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DModel.hpp>
#include <JSystem/J3D/J3DGraphBase/J3DSys.hpp>
#include <JSystem/JGeometry.hpp>
#include <MarioUtil/MathUtil.hpp>
#include <NPC/NpcBase.hpp>
#include <NPC/NpcSave.hpp>
#include <Player/MarioAccess.hpp>
#include <dolphin/mtx.h>

extern TBaseNPC* gpCurrentNpc;

void MsVECNormalize(Vec*, Vec*);

BOOL NPCNeckCallBack(J3DNode* node, int phase)
{
	if (phase != 0) {
		return TRUE;
	}

	TBaseNPC* npc = gpCurrentNpc;
	if (npc == NULL) {
		return FALSE;
	}

	bool ok = false;
	if (npc->mNpcKind != -1 && (npc->mLiveFlag & 6) == 0) {
		ok = true;
	}
	if (!(ok ? true : false)) {
		return TRUE;
	}

	J3DJoint* jnt    = (J3DJoint*)node;
	Mtx*      mtxArr = j3dSys.getModel()->mNodeMatrices;
	Mtx&      jntMtx = mtxArr[jnt->mJntNo];

	s16 yaw   = npc->mNeckAngles[0];
	s16 pitch = npc->mNeckAngles[1];

	bool wantTrack = false;
	if (npc->isNeedNeckStraight()) {
		wantTrack = false;
	} else {
		wantTrack = true;
	}

	s16                   chasePitch = 0;
	s16                   chaseYaw   = 0;
	JGeometry::TVec3<f32> diff(0.0f, 0.0f, 0.0f);
	JGeometry::TVec3<f32> marioPos;
	JGeometry::TVec3<f32> dir;

	if (wantTrack) {
		marioPos = *gpMarioPos;
		marioPos.y += 85.0f;

		dir.x     = marioPos.x - jntMtx[0][3];
		dir.y     = marioPos.y - jntMtx[1][3];
		dir.z     = marioPos.z - jntMtx[2][3];
		f32 dist2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;

		if (dist2 > 0.001f
		    && dist2 < CLBSquared<f32>(npc->mNpcSaveIndividual->mNeckTurnSearchDist.value)
		    && __fabsf(marioPos.y - jntMtx[1][3])
		           < npc->mNpcSaveIndividual->mNeckTurnSearchHeight.value) {

			JGeometry::TVec3<f32> ax(jntMtx[0][1], jntMtx[1][1], jntMtx[2][1]);
			MsVECNormalize((Vec*)&dir, (Vec*)&dir);
			JGeometry::TVec3<f32> rotAx  = MsGetRotFromZaxis(ax);
			JGeometry::TVec3<f32> rotDir = MsGetRotFromZaxis(dir);
			JGeometry::TVec3<f32> tmp    = rotDir;
			tmp.sub(rotAx);
			diff = tmp;
		}

		s16 v    = CLBRoundf<s16>(diff.y * 182.04445f);
		s16 maxP = npc->mNpcSaveIndividual->mNeckMaxAngleY.value;
		if (v > maxP) {
			v = maxP;
		} else if (v < -maxP) {
			v = -maxP;
		}
		chasePitch = v;
	}

	s16 pitchSpeed = CLBPalIntSpeed<s16>(npc->mNpcSaveIndividual->mNeckAngleYSpeed.value);
	CLBChaseGeneralConstantSpecifySpeed<s16>(&pitch, chasePitch, pitchSpeed);

	if (wantTrack) {
		s16 v    = CLBRoundf<s16>(diff.x * 182.04445f);
		s16 maxY = npc->mNpcSaveIndividual->mNeckMaxAngleX.value;
		s16 minY = npc->mNpcSaveIndividual->mNeckMinAngleX.value;
		if (v > maxY) {
			v = maxY;
		} else if (v < minY) {
			v = minY;
		}
		s16 absP = (pitch < 0) ? -pitch : pitch;
		f32 ratio
		    = CLBCalcRatio<int>(npc->mNpcSaveIndividual->mNeckMaxAngleY.value, 0, absP);
		s16 half = (s16)((f32)v * 0.5f);
		chaseYaw = CLBEaseOutInbetween<s16>(half, v, ratio);
	}

	s16 yawSpeed = CLBPalIntSpeed<s16>(npc->mNpcSaveIndividual->mNeckAngleXSpeed.value);
	CLBChaseGeneralConstantSpecifySpeed<s16>(&yaw, chaseYaw, yawSpeed);

	npc->mNeckAngles[0] = yaw;
	npc->mNeckAngles[1] = pitch;

	Mtx tmp;
	MsMtxSetRotRPH(tmp, (f32)pitch * 0.005493164f, 0.0f,
	               (f32)yaw * 0.005493164f);
	PSMTXConcat(jntMtx, tmp, jntMtx);
	PSMTXCopy(jntMtx, j3dSys.mCurrentMtx);

	return TRUE;
}
