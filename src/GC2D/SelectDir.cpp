#include <System/SelectDir.hpp>
#include <GC2D/SelectMenu.hpp>
#include <GC2D/SelectShine2.hpp>
#include <GC2D/ScrnFader.hpp>
#include <System/Application.hpp>
#include <System/EmitterViewObj.hpp>
#include <System/FlagManager.hpp>
#include <System/MarioGamePad.hpp>
#include <System/Resolution.hpp>
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JDrama/JDRDStage.hpp>
#include <JSystem/JDrama/JDRDStageGroup.hpp>
#include <JSystem/JDrama/JDREfbCtrl.hpp>
#include <JSystem/JDrama/JDRScreen.hpp>
#include <JSystem/JDrama/JDRCamera.hpp>
#include <JSystem/JDrama/JDRViewObjPtrList.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <JSystem/JParticle/JPAResourceManager.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>

extern OSThread gSetupThread;
extern u8* gpSetupThreadStack;

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

inline JDrama::TLookAtCamera::TLookAtCamera()
    : TCamera(50.0f, 10000.0f, "<TLookAtCamera>")
{
}

TSelectDir::TSelectDir()
{
	mMenu           = nullptr;
	mGrad           = nullptr;
	mShineManager   = nullptr;
	mArchive        = nullptr;
	mEmitterMgr1    = nullptr;
	mEmitterMgr2    = nullptr;
	mInitDone       = 0;
	unk3C           = 0;
	mCupId          = 0;
	mScreenGrad     = nullptr;
	mScreen2D       = nullptr;
	mResetTriggered = 0;
}

TSelectDir::~TSelectDir()
{
	JKRMemArchive* arc = (JKRMemArchive*)JKRFileLoader::getVolume("select");
	if (arc)
		arc->unmountFixed();
	mGamePad->mFlags &= ~1;
}

void TSelectDir::setup(JDrama::TDisplay* display, TMarioGamePad* pad, u8 cup)
{
	mDisplay = display;
	mGamePad = pad;
	SMSRumbleMgr->reset();
	mCupId = cup;
	OSCreateThread(&gSetupThread, &setupThreadFunc, this,
	               gpSetupThreadStack + 0x10000, 0x10000, 0x11, 0);
	OSResumeThread(&gSetupThread);
}

void* TSelectDir::setupThreadFunc(void* param)
{
	// BUG: return missing in original
	((TSelectDir*)param)->rsetup();
}

int TSelectDir::rsetup()
{
	void* arcBlob = SMSLoadArchive("/data/select.arc", nullptr, 0, nullptr);
	JKRMemArchive* arc = new JKRMemArchive;
	if (!arc->mountFixed(arcBlob, MBF_0))
		return 1;
	mArchive = arc;

	JDrama::TViewObjPtrListT<JDrama::TViewObj>* root
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("root View Objs");
	unk10 = root;

	unk14 = new JDrama::TDStageGroup(mDisplay);

	mMenu         = new TSelectMenu("<TSelectMenu>");
	mShineManager = new TSelectShineManager("<SelectShineManger>");
	mGrad         = new TSelectGrad("<TSelectGrad>");
	mGrad->setStageColor(mCupId);

	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group3d
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("Group 3D");
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group2d
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("Group 2D");
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* groupGrad
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("Group Grad");
	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group2dParticle
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("Group 2D Particle");

	root->getChildren().push_back(group2d);
	root->getChildren().push_back(group3d);
	root->getChildren().push_back(groupGrad);
	root->getChildren().push_back(group2dParticle);

	group2d->getChildren().push_back(mMenu);
	group3d->getChildren().push_back(mShineManager);
	groupGrad->getChildren().push_back(mGrad);

	mGamePad->mFlags = 1;
	mMenu->mGamePad  = mGamePad;

	JPAResourceManager* res1 = new JPAResourceManager(9, 0x200, nullptr);
	JPAResourceManager* res2 = new JPAResourceManager(9, 0x200, nullptr);

	res1->load("/select/particle/ms_2d_shine_promi.jpa", 0);
	res1->load("/select/particle/ms_2d_shine_senko.jpa", 1);
	res1->load("/select/particle/ms_2d_shine_kira.jpa", 2);
	res1->load("/select/particle/ms_2d_shine_kira_em.jpa", 3);

	res2->load("/select/particle/ms_2d_scsel_on_a.jpa", 4);
	res2->load("/select/particle/ms_2d_scsel_on_a2.jpa", 5);
	res2->load("/select/particle/ms_2d_scsel_on_b.jpa", 6);
	res2->load("/select/particle/ms_2d_scsel_on_c.jpa", 7);
	res2->load("/select/particle/ms_2d_scsel_on_d.jpa", 8);

	mEmitterMgr1 = new JPAEmitterManager(res1, 0x400, 0x80, 0x100, nullptr);
	mEmitterMgr2 = new JPAEmitterManager(res2, 0x400, 0x80, 0x100, nullptr);

	TEmitterViewObj* emitter1
	    = new TEmitterViewObj(mEmitterMgr1, "<EmitterViewObj>");
	group3d->getChildren().push_back(emitter1);
	TEmitterViewObj* emitter2
	    = new TEmitterViewObj(mEmitterMgr2, "<EmitterViewObj>");
	group2dParticle->getChildren().push_back(emitter2);

	JDrama::TDStageDisp* stageDisp
	    = new JDrama::TDStageDisp("<DStageDisp>", 0);
	unk14->getChildren().push_back(stageDisp);

	JDrama::TRect rect(0, 0, SMSGetTitleRenderWidth(),
	                   SMSGetTitleRenderHeight());
	stageDisp->getEfbCtrlDisp()->TEfbCtrl::setSrcRect(rect);

	JDrama::TOrthoProj* projGrad = new JDrama::TOrthoProj(
	    -100.0f, 100.0f, 0.0f, 16.0f, 600.0f, 464.0f);
	groupGrad->getChildren().push_back(projGrad);

	JDrama::TScreen* screenGrad = new JDrama::TScreen(rect, "Screen Grad");
	stageDisp->getUnk14()->getChildren().push_back(screenGrad);
	screenGrad->assignCamera(projGrad);
	screenGrad->assignViewObj(groupGrad);

	JDrama::TOrthoProj* proj2d = new JDrama::TOrthoProj(
	    -100.0f, 100.0f, 0.0f, 16.0f, 600.0f, 464.0f);
	group2d->getChildren().push_back(proj2d);

	JDrama::TScreen* screen2d = new JDrama::TScreen(rect, "Screen 2D");
	stageDisp->getUnk14()->getChildren().push_back(screen2d);
	screen2d->assignCamera(proj2d);
	screen2d->assignViewObj(group2d);
	mScreen2D = screen2d;

	JDrama::TLookAtCamera* lookCam = new JDrama::TLookAtCamera();
	lookCam->mNear   = 50.0f;
	lookCam->mFar    = 10000.0f;
	lookCam->mPosition.set(300.0f, 240.0f, 1300.0f);
	lookCam->mUp.set(0.0f, 1.0f, 0.0f);
	lookCam->mTarget.set(300.0f, 240.0f, 0.0f);
	lookCam->mFovy   = 30.0f;
	lookCam->mAspect = 1.3333334f;
	group3d->getChildren().push_back(lookCam);

	JDrama::TScreen* screen3d = new JDrama::TScreen(rect, "Screen 3D");
	stageDisp->getUnk14()->getChildren().push_back(screen3d);
	screen3d->assignCamera(lookCam);
	screen3d->assignViewObj(group3d);

	JDrama::TOrthoProj* proj2dBack = new JDrama::TOrthoProj(
	    -100.0f, 100.0f, 0.0f, 16.0f, 600.0f, 464.0f);
	group2d->getChildren().push_back(proj2dBack);

	JDrama::TScreen* screen2dBack = new JDrama::TScreen(rect, "Screen 2D");
	stageDisp->getUnk14()->getChildren().push_back(screen2dBack);
	screen2dBack->assignCamera(proj2dBack);
	screen2dBack->assignViewObj(group2d);
	mScreenGrad = screen2dBack;

	JDrama::TOrthoProj* projParticle = new JDrama::TOrthoProj(
	    -500.0f, 500.0f, 0.0f, 16.0f, 600.0f, 464.0f);
	group2dParticle->getChildren().push_back(projParticle);

	JDrama::TScreen* screenParticle
	    = new JDrama::TScreen(rect, "Screen Grad");
	stageDisp->getUnk14()->getChildren().push_back(screenParticle);
	screenParticle->assignCamera(projParticle);
	screenParticle->assignViewObj(group2dParticle);

	mScreenGrad->unkC.off(0xb);
	mScreen2D->unkC.on(0xb);

	return 0;
}

void TSelectDir::changeOrder()
{
	mScreenGrad->unkC.on(0xb);
	mScreen2D->unkC.off(0xb);
}

int TSelectDir::direct()
{
	if (!mInitDone) {
		if (!OSIsThreadTerminated(&gSetupThread))
			return 0;
		u32 res;
		OSJoinThread(&gSetupThread, &res);
		if (res != 0)
			return 5;

		mInitDone = 1;
		mMenu->initData(mCupId, mArchive, mShineManager, this);
		mMenu->startMove();
		mMenu->startOpenWindow();

		gpApplication.mFader->startWipe(14, 0.4f, 0.0f);

		gpApplication.mFader->setColor(mCupId == 9
		    ? JUtility::TColor(0xff, 0xff, 0xff, 0xff)
		    : JUtility::TColor(0, 0, 0, 0xff));
		gpMSound->initSound();
		return 0;
	}

	JDrama::TDirector::direct();

	switch (gpApplication.mFader->mFadeStatus) {
	case 1:
	case 2: {
		u8 ready = mMenu->_104[0x14B - 0x104];
		if (ready) {
			return 4;
		}

		u8 selected = mMenu->_104[0x14A - 0x104];
		if (selected) {
			gpApplication.mNextArea.unk1 = mMenu->_104[0x13B - 0x104];
			TFlagManager::getInstance()->setFlag(
			    0x40003, mMenu->_104[0x13B - 0x104]);
			gpApplication.mFader->startWipe(15, 1.0f, 0.0f);

			JUtility::TColor color(0xff, 0xff, 0xff, 0xff);
			gpApplication.mFader->setColor(color);
			MSound* sound = gpMSound;
			sound->fadeOutAllSound((u32)((f32)SMSGetVSyncTimesPerSec()));
		}
		break;
	}
	}

	if ((TMarioGamePad::mResetFlag.check(1 << mGamePad->getPortNum()))
	    && !mResetTriggered) {
		mResetTriggered = 1;
		gpApplication.mFader->startWipe(4, 1.0f, 0.0f);
		MSound* sound = gpMSound;
		sound->fadeOutAllSound(
		    (u32)((f32)SMSGetVSyncTimesPerSec() * 0.4f));
		((JDrama::TViewObj*)unk10)->unkC.on(3);
	}

	if (gpApplication.mFader->isFullyFadedOut()) {
		gpMSound->stopAllSound();
		if (TMarioGamePad::mResetFlag.check(1 << mGamePad->getPortNum()))
			return 4;
		return 5;
	}
	return 1;
}
