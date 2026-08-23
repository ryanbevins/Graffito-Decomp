#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossHanachanSaveParams.hpp>

#include <Camera/cameralib.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DAnimation.hpp>
#include <M3DUtil/MActor.hpp>
#include <Strategic/Spine.hpp>
#include <System/Application.hpp>

void TBossHanachan::changeAnmRateAndFrameUpdate_()
{
	bool shouldSetRate = true;
	f32 rate           = SMSGetAnmFrameRate();

	if (mSpine->getLatestNerve() == &TNerveBossHanachanTumble::theNerve()) {
		mHead->mPalFrame->unk28 = 0.0f;
		for (int i = 0; i < 8; i++)
			mBody[i]->mPalFrame->unk28 = 0.0f;
		mHead->changeTumbleAnmRate_();
		for (int i = 0; i < 8; i++)
			mBody[i]->changeTumbleAnmRate_();
		shouldSetRate = false;
	} else if (mHead->mCurAnm < 2 && mHead->mCurAnm >= 0) {
		f32 walkSpeed = mChangeParams->mSLWalkAnmMarchSpeed.value;
		if (mMarchSpeed <= walkSpeed) {
			mHead->mPalFrame->unk28 = 0.0f;
			for (int i = 0; i < 8; i++)
				mBody[i]->mPalFrame->unk28 = 0.0f;
			if (mHead->mCurAnm == 0) {
				setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_OFF);
				mHead->copyFrameFromOldAnmToNewAnm_();
				for (int i = 0; i < 8; i++)
					mBody[i]->copyFrameFromOldAnmToNewAnm_();
			} else {
				setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_ON);
			}
		} else if (mMarchSpeed >= mChangeParams->mSLRunAnmMarchSpeed.value) {
			mHead->mPalFrame->unk28 = 0.0f;
			for (int i = 0; i < 8; i++)
				mBody[i]->mPalFrame->unk28 = 0.0f;
			if (mHead->mCurAnm == 0) {
				setHeadAndBodyAnm(BHANM_KIND_01, BHANM_STOP_OFF);
				mHead->copyFrameFromOldAnmToNewAnm_();
				for (int i = 0; i < 8; i++)
					mBody[i]->copyFrameFromOldAnmToNewAnm_();
			} else if (mHead->mCurAnm != 1) {
				setHeadAndBodyAnm(BHANM_KIND_01, BHANM_STOP_ON);
			}
		} else {
			f32 ratio = CLBCalcRatio<f32>(walkSpeed,
			                              mChangeParams->mSLRunAnmMarchSpeed.value,
			                              mMarchSpeed);
			if (mHead->mCurAnm == 0) {
				if (mHead->mPrevAnm != 1) {
					setHeadAndBodyAnm(BHANM_KIND_01, BHANM_STOP_OFF);
					mHead->copyFrameFromOldAnmToNewAnm_();
					for (int i = 0; i < 8; i++)
						mBody[i]->copyFrameFromOldAnmToNewAnm_();
					ratio = 1.0f - ratio;
				}
				mHead->mPalFrame->unk28 = ratio;
				for (int i = 0; i < 8; i++)
					mBody[i]->mPalFrame->unk28 = ratio;
			} else if (mHead->mCurAnm == 1) {
				if (mHead->mPrevAnm == 0) {
					ratio = 1.0f - ratio;
				} else {
					setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_OFF);
					mHead->copyFrameFromOldAnmToNewAnm_();
					for (int i = 0; i < 8; i++)
						mBody[i]->copyFrameFromOldAnmToNewAnm_();
				}
				mHead->mPalFrame->unk28 = ratio;
				for (int i = 0; i < 8; i++)
					mBody[i]->mPalFrame->unk28 = ratio;
			} else {
				mHead->mPalFrame->unk28 = 0.0f;
				for (int i = 0; i < 8; i++)
					mBody[i]->mPalFrame->unk28 = 0.0f;
				setHeadAndBodyAnm(BHANM_KIND_00, BHANM_STOP_ON);
			}
		}
		rate = SMSGetAnmFrameRate() * mMarchSpeed
		       * mChangeParams->mSLWalkBckRateMagnif.value;
		f32 minRate = mChangeParams->mSLWalkBckRateMin.value;
		if (rate < minRate)
			rate = minRate;
	} else {
		mHead->mPalFrame->unk28 = 0.0f;
		for (int i = 0; i < 8; i++)
			mBody[i]->mPalFrame->unk28 = 0.0f;
		rate = SMSGetAnmFrameRate();
	}

	{
		MActor* m = mHead->mMActor;
		if (shouldSetRate)
			m->getFrameCtrl(0)->setRate(rate);
		mHead->updateAnmSound();
		m->frameUpdate();
	}
	for (int i = 0; i < 8; i++) {
		MActor* m = mBody[i]->mMActor;
		if (shouldSetRate)
			m->getFrameCtrl(0)->setRate(rate);
		mBody[i]->updateAnmSound();
		m->frameUpdate();
	}
}

bool TBossHanachan::isAllBckAlreadyEnd(EnumBossHanachanAnmKind anmKind) const
{
	bool found;
	bool partDone;
	bool result;
	int i;

	result = true;
	found  = false;
	if (mHead->mCurAnm == anmKind && mHead->isCurBckAlreadyEnd_())
		found = true;
	if (!found) {
		result = false;
	} else {
		for (i = 0; i < 8; i++) {
			partDone = false;
			if (mBody[i]->mCurAnm == anmKind && mBody[i]->isCurBckAlreadyEnd_())
				partDone = true;
			if (!partDone) {
				result = false;
				break;
			}
		}
	}
	return result;
}

bool TBossHanachan::isFinishedGetUp() const
{
	bool result = false;
	int cur     = mHead->mCurAnm;
	switch (cur) {
	case 0x9:
	case 0xC:
		if (mHead->isCurBckAlreadyEnd_())
			result = true;
		break;
	}
	return result;
}

void TBossHanachan::considerSetAnm(EnumBossHanachanNerveAnm nerveAnm)
{
	mHead->considerSetAnm_(nerveAnm);
	for (int i = 0; i < 8; i++)
		mBody[i]->considerSetAnm_(nerveAnm);
}

static inline int absDist(int a, int b)
{
	int d = a;
	d -= b;
	if (d < 0)
		d = -d;
	return d;
}

void TBossHanachan::setAnmTimerWhenDead()
{
	u8 frameDiff = mChangeParams->mSLDeadFrameDiff.value;
	for (int i = 0; i < 8; ++i)
		mBody[i]->mAnmCounter = frameDiff * absDist(unk174, i);
	mHead->mAnmCounter = frameDiff * absDist(unk174, -1);
}

void TBossHanachan::setAnmTimerWhenDamage()
{
	u8 frameDiff = mChangeParams->mSLDamageFrameDiff.value;
	for (int i = 0; i < 8; ++i)
		mBody[i]->mAnmCounter = frameDiff * absDist(unk174, i);
	mHead->mAnmCounter = frameDiff * absDist(unk174, -1);
}

void TBossHanachan::setAnmTimerWhenSnort()
{
	u8 frameDiff       = mChangeParams->mSLSnortFrameDiff.value;
	mHead->mAnmCounter = 0;
	for (int i = 0; i < 8; i++) {
		mBody[i]->mAnmCounter = frameDiff * (i + 1);
	}
}

void TBossHanachan::setAnmTimerWhenGetUp()
{
	u8 frameDiff          = mChangeParams->mSLGetUpFrameDiff.value;
	mBody[7]->mAnmCounter = 0;
	mBody[6]->mAnmCounter = frameDiff;
	mBody[5]->mAnmCounter = frameDiff * 2;
	mBody[4]->mAnmCounter = frameDiff * 3;
	mBody[3]->mAnmCounter = frameDiff * 4;
	mBody[2]->mAnmCounter = frameDiff * 5;
	mBody[1]->mAnmCounter = frameDiff * 6;
	mBody[0]->mAnmCounter = frameDiff * 7;
	mHead->mAnmCounter    = frameDiff * 8;
}

void TBossHanachan::setTumbleAnm(EnumBossHanachanStopMotionBlendOnOff blend)
{
	int anmKind;
	if (179.0f == unk194) {
		anmKind = 0x11;
	} else if (-179.0f == unk194) {
		anmKind = 0x10;
	} else {
		return;
	}

	mHead->setAnm_((EnumBossHanachanAnmKind)anmKind, blend);
	{
		TBossHanachanPartsBase* head = mHead;
		J3DFrameCtrl* fc             = head->mMActor->getFrameCtrl(0);
		f32 diff                     = unk194 - head->getRotation().z;
		diff = diff >= 0.0f ? diff : -diff;
		f32 rateForDelta = (1.0f / unk198) * diff;
		fc->setRate((1.0f / rateForDelta) * (40.0f * (2.0f * SMSGetAnmFrameRate())));
	}
	for (int i = 0; i < 8; i++) {
		TBossHanachanPartsBase** pp = &mBody[i];
		(*pp)->setAnm_((EnumBossHanachanAnmKind)anmKind, blend);
		TBossHanachanPartsBase* part = *pp;
		J3DFrameCtrl* fc             = part->mMActor->getFrameCtrl(0);
		f32 diff                     = unk194 - part->getRotation().z;
		diff = diff >= 0.0f ? diff : -diff;
		f32 rateForDelta = (1.0f / unk198) * diff;
		fc->setRate((1.0f / rateForDelta) * (40.0f * (2.0f * SMSGetAnmFrameRate())));
	}
}

void TBossHanachan::setHeadAndBodyAnm(EnumBossHanachanAnmKind kind,
                                      EnumBossHanachanStopMotionBlendOnOff blend)
{
	mHead->setAnm_(kind, blend);
	for (int i = 0; i < 8; i++) {
		TBossHanachanPartsBase* part = mBody[i];
		if ((u8)part->setAnm_(kind, blend)) {
			J3DFrameCtrl* fc = part->mMActor->getFrameCtrl(0);
			int t = (mChangeParams->mSLNormalBckFrameDiff.value * i) % fc->getEnd();
			fc->setFrame((f32)t);
			f32 ft            = (f32)t;
			J3DFrameCtrl* fc3 = part->mMActor->getFrameCtrl(3);
			if (fc3 != 0)
				fc3->setFrame(ft);
			J3DFrameCtrl* fc4 = part->mMActor->getFrameCtrl(4);
			if (fc4 != 0)
				fc4->setFrame(ft);
		}
	}
}
