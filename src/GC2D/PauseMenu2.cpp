#include <GC2D/PauseMenu2.hpp>
#include <GC2D/CardSave.hpp>
#include <GC2D/GCConsole2.hpp>
#include <MarioUtil/RumbleMgr.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSoundSE.hpp>
#include <System/Application.hpp>
#include <System/MarioGamePad.hpp>
#include <System/FlagManager.hpp>
#include <System/MarDirector.hpp>
#include <System/StageUtil.hpp>
#include <GC2D/MessageUtil.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/J2D/J2DPicture.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/J2D/J2DOrthoGraph.hpp>
#include <JSystem/JParticle/JPAEmitterManager.hpp>
#include <JSystem/JKernel/JKRArchive.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <dolphin/gx.h>
#include <dolphin/os.h>
#include <stdio.h>

extern JPAEmitterManager* gpEmitterManager4D2;

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";

TPauseMenu2::TPauseMenu2(const char* name)
    : JDrama::TViewObj(name)
    , unk10(5)
    , unk14(nullptr)
    , unk18(nullptr)
    , unk1C(nullptr)
{
	unkD4 = nullptr;
	unkD8 = nullptr;
	unkDC = nullptr;
	unkE0 = 0;
	unkE8 = 0.0f;
	unkEC = 0.0f;
	unkF8 = 0;
	unkFC = 6;
	unk100 = 3;
	unk102 = 0x81;
	unk104 = 3;
	unk108 = 0;
	unk109 = 0;
	unk110 = nullptr;
	unk114 = 0;
}

void TPauseMenu2::load(JSUMemoryInputStream& stream)
{
	JDrama::TNameRef::load(stream);
	JKRArchive* arc = (JKRArchive*)JKRFileLoader::getVolume("game_6");

	unk14 = new J2DSetScreen("pause_1.blo", arc);
	unk14->setCullBack(GX_CULL_BACK);

	if (gpApplication.mCurrArea.getStage() <= 1) {
		unk104 = 2;
	}

	unk18 = unk14->search('mask');
	unk1C = unk14->search('t_0');

	for (int i = 0; i < 5; i++) {
		unk20[i] = unk14->search('pa00' + i);
	}

	for (int i = 0; i < 3; i++) {
		unk98[i] = (J2DPicture*)unk14->search('tx_1' + i);
		if (unk104 == 2) {
			((J2DPane*)unk98[i])->add(0, 0x14);
		}
		unk98[i]->mVisible = false;
	}

	unkD4 = (J2DTextBox*)unk14->search('map');
	unkD4->setFont((JUTFont*)gpSystemFont);
	unkDC = (J2DTextBox*)unk14->search('task');
	SMSMakeTextBuffer(unkDC, 0x80);
	unkDC->setFont((JUTFont*)gpSystemFont);
	unkD8 = unk14->search('brek');

	u32 shineStage  = SMS_getShineStage(gpMarDirector->mMap);
	s32 ehasFlag   = TFlagManager::smInstance->getFlag(0x40003);
	void* stagename = JKRFileLoader::getGlbResource("/common/2d/stagename.bmg");
	unkD4->setString((const char*)SMSGetMessageData(stagename, shineStage));

	if (gpMarDirector->mMap == 0xF) {
		return;
	}

	void* scenarioname
	    = JKRFileLoader::getGlbResource("/common/2d/scenarioname.bmg");
	s16 idx = SMS_getShineID(shineStage, ehasFlag, false);

	const char* scenStr;
	if (scenarioname == nullptr || idx == -1 || shineStage == 0) {
		scenStr = "";
		((J2DPane*)unkD4)->add(0, 0xF);
		unk1C->add(0, 0x1E);
	} else {
		u16 nameIdx = SMS_getNormalStage(idx);
		scenStr     = (const char*)SMSGetMessageData(scenarioname, nameIdx);
	}

	snprintf((char*)unkDC->getStringPtr(), 0x80, "%s", scenStr);
}

void TPauseMenu2::loadAfter()
{
	unk118 = JDrama::TNameRefGen::search<TCardSave>(
	    "\x83\x66\x81\x5B\x83\x5E\x83\x5A\x81\x5B\x83\x75");

	JUtility::TColor tmpW(unk98[0]->mWhite.toUInt32());
	unkE4 = tmpW.toUInt32();

	for (int i = 0; i < unk104; i++) {
		unkA4[i] = unk98[i]->mBounds;
	}

	unk34[0] = unk20[0]->mBounds;
	unk84[0] = unk20[0]->mRotation;
	unk34[1] = unk20[1]->mBounds;
	unk84[1] = unk20[1]->mRotation;
	unk34[2] = unk20[2]->mBounds;
	unk84[2] = unk20[2]->mRotation;
	unk34[3] = unk20[3]->mBounds;
	unk84[3] = unk20[3]->mRotation;
	unk34[4] = unk20[4]->mBounds;
	unk84[4] = unk20[4]->mRotation;

	unkF0 = unk18->mAlpha;
	unkF4 = (f32)unkF0 / 45.0f;

	unkF8 = (s16)unk98[0]->mRotation;
	if (unkF8 > 180) {
		unkF8 -= 360;
	}

	unk10C = gpMarDirector->unk18[0];
}

void TPauseMenu2::appearWindow()
{
	if (unkEC <= 45.0f) {
		for (int i = 0; i < 5; i++) {
			f32 t = unkEC - 5.0f * (f32)i;
			drawAppearPane((J2DPicture*)unk20[i], t, unk34[i],
			               180.0f + unk84[i] - 9.0f * t);
		}

		s32 alpha = (s32)(unkF4 * unkEC * 1.5f);
		alpha = alpha > unkF0 ? unkF0 : alpha;
		unk18->mAlpha = (u8)alpha;
	}

	{
		s32 alpha = (s32)(unkEC * unkF4 * 1.5f);
		if (alpha > unkF0) {
			alpha = 0xff;
		}
		unkD8->mAlpha = (u8)alpha;
	}

	if (unkEC >= 30.0f) {
		for (int i = 0; i < unk104; i++) {
			if (!unk98[i]->mVisible) {
				unk98[i]->mVisible = true;
				unk98[i]->mAlpha   = 0;
			}

			JUTRect local(unkA4[i]);
			if (unkEC <= 40.0f) {
				s32 t1 = (s32)(3.0f * (unkEC - 35.0f));
				s32 t2 = (s32)(1.5f * (unkEC - 35.0f));
				local.reform(-t1, -t2, t1, t2);
				unk98[i]->mBounds = local;

				s32 a = (s32)(26.0f * (unkEC - 30.0f));
				if (a > 0xff) {
					a = 0xff;
				}
				unk98[i]->mAlpha = (u8)a;

				if (i == 0) {
					unk98[i]->mRotation = (f32)(s16)(s32)(0.1f * (f32)unkF8
					                                      * (unkEC - 30.0f));
				}
			} else if (unkEC <= 45.0f) {
				s32 t1 = (s32)(3.0f * (unkEC - 45.0f));
				s32 t2 = (s32)(1.5f * (unkEC - 45.0f));
				local.reform(-t1, -t2, t1, t2);
				unk98[i]->mBounds = local;
			} else if (unkEC > 46.0f) {
				if (unk10 != 1) {
					unk10 = 1;
				}
			}
		}
	}

	unkEC += 0.5f;
}

void TPauseMenu2::disappearWindow()
{
	if (unkEC == 0.0f && unk110 != nullptr) {
		gpEmitterManager4D2->forceDeleteEmitter(unk110);

		u8 idx              = unkE0;
		unk98[idx]->mBounds = unkA4[idx];
	}

	if (unkEC <= 10.0f) {
		s32 cc1 = unk1C->mAlpha - 12;
		if (cc1 < 0) cc1 = 0;
		unk1C->mAlpha = (u8)cc1;

		if (cc1 < unk18->mAlpha) {
			unk18->mAlpha = (u8)cc1;
		}

		if (cc1 < unkD8->mAlpha) {
			unkD8->mAlpha = (u8)cc1;
		}

		for (int i = 0; i < 5; i++) {
			JUTRect r = unk20[i]->mBounds;
			r.add(0.025f * -r.y1 + 0.01f * r.getWidth(),
			      0.025f * -r.x1 + 0.01f * r.getHeight());
			r.resize((s32)(0.98f * (f32)(r.x2 - r.x1)),
			         (s32)(0.98f * (f32)(r.y2 - r.y1)));
			unk20[i]->mBounds = r;
		}

		for (int i = 0; i < unk104; i++) {
			JUTRect r = unk98[i]->mBounds;
			r.add(0.025f * -r.y1 + 0.01f * r.getWidth(),
			      0.025f * -r.x1 + 0.01f * r.getHeight());
			r.resize((s32)(0.98f * (f32)(r.x2 - r.x1)),
			         (s32)(0.98f * (f32)(r.y2 - r.y1)));
			unk98[i]->mBounds = r;
		}

		unk1C->mRotation = -unkEC * 2.0f;
	} else {
		unk10 = 5;
	}

	unkEC += 0.5f;
}

void TPauseMenu2::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	if (gpMarDirector->mState != 5) {
		return;
	}

	if (unk10 == 3) {
		if (param_1 & 1) {
			if (unk118->unk2DF) {
				unk109 = 0;
				unk10  = 1;
			}
			s16 a = unk1C->mAlpha - 16;
			if (a < 0) a = 0;
			unk1C->mAlpha = (u8)a;
		}

		if (param_1 & 8) {
			switch (unk10) {
			case 0: {
				J2DOrthoGraph graph(param_2->mViewportRect);
				graph.setup2D();
				unk14->draw(0, 0, &graph);
				GXSetScissor(param_2->mScissorRect.x1, param_2->mScissorRect.y1,
				             param_2->mScissorRect.x2 - param_2->mScissorRect.x1,
				             param_2->mScissorRect.y2 - param_2->mScissorRect.y1);
				break;
			}
			case 1:
			case 2:
			case 3:
			case 4: {
				J2DOrthoGraph graph(param_2->mViewportRect);
				graph.setup2D();
				unk14->draw(0, 0, &graph);
				GXSetScissor(param_2->mScissorRect.x1, param_2->mScissorRect.y1,
				             param_2->mScissorRect.x2 - param_2->mScissorRect.x1,
				             param_2->mScissorRect.y2 - param_2->mScissorRect.y1);
				break;
			}
			default:
				break;
			}
		}
		return;
	}

	if (param_1 & 1) {
		switch (unk10) {
		case 0:
			appearWindow();
			break;
		case 1: {
			s32 cur    = unkE0;
			u32 btnVal = unk10C->mEnabledFrameMeaning;
			if (btnVal & 0x21) {
				switch (cur) {
				case 0:
					unk109 = 1;
					SMSRumbleMgr->finishPause();
					gpMSound->pauseOff(0);
					gpMarDirector->mConsole->pauseOut();
					unkEC = 0.0f;
					unk10 = 4;
					break;
				case 2:
					unk109 = 1;
					SMSRumbleMgr->finishPause();
					gpMSound->pauseOff(0);
					gpMarDirector->mConsole->pauseOut();
					unkEC = 0.0f;
					unk10 = 4;
					break;
				case 3:
					unk109 = 1;
					SMSRumbleMgr->finishPause();
					gpMSound->pauseOff(0);
					gpMarDirector->mConsole->pauseOut();
					unkEC = 0.0f;
					unk10 = 4;
					break;
				case 1:
					if (gpMSound->gateCheck(0x4802)) {
						MSoundSESystem::MSoundSE::startSoundSystemSE(0x4802, 0,
						                                              nullptr, 0);
					}
					unk109 = 1;
					unk118->init(0);
					unk10 = 3;
					break;
				default:
					break;
				}
			} else if (btnVal & 0x40) {
				unk108 = 1;
				unkEC  = 0.0f;
				SMSRumbleMgr->finishPause();
				gpMSound->pauseOff(0);
				gpMarDirector->mConsole->pauseOut();
				unkEC = 0.0f;
				unk10 = 4;
			} else if (btnVal & 0x4) {
				if (unk104 > 2) {
					if (cur < unk104 - 1) {
						unkE0 = cur + 1;
					} else {
						unkE0 = 0;
					}
				} else {
					unkE0 = unk104 - 1;
				}
				if (unkE0 != cur) {
					if (gpMSound->gateCheck(0x480F)) {
						MSoundSESystem::MSoundSE::startSoundSystemSE(0x480F, 0,
						                                              nullptr, 0);
					}
					unk98[unkE0]->mWhite = unkE4;
					unk98[cur]->mWhite   = -1;
					unk98[cur]->mBounds  = unkA4[cur];
					unkE8                = 0.0f;
					if (unk110 != nullptr) {
						gpEmitterManager4D2->forceDeleteEmitter(unk110);
					}
				}
			} else if (btnVal & 0x2) {
				if (unk104 > 2) {
					if (cur == 0) {
						unkE0 = unk104 - 1;
					} else {
						unkE0 = cur - 1;
					}
				} else {
					unkE0 = 0;
				}
				if (unkE0 != cur) {
					if (gpMSound->gateCheck(0x480F)) {
						MSoundSESystem::MSoundSE::startSoundSystemSE(0x480F, 0,
						                                              nullptr, 0);
					}
					unk98[unkE0]->mWhite = unkE4;
					unk98[cur]->mWhite   = -1;
					unk98[cur]->mBounds  = unkA4[cur];
					unkE8                = 0.0f;
					if (unk110 != nullptr) {
						gpEmitterManager4D2->forceDeleteEmitter(unk110);
					}
				}
			}

			JUTRect rect(unkA4[unkE0]);

			if (unkE8 == (f32)(s32)unk100) {
				f32 width = unk98[unkE0]->getWidth();
				f32 zoom  = width / unk102;
				JUTRect rect2(unk98[unkE0]->getGlobalBounds());

				JGeometry::TVec3<f32> pos;
				pos.set((f32)rect2.x1 + 0.5f * (f32)rect2.getWidth(),
				        (f32)rect2.y1 + 0.5f * (f32)rect2.getHeight(), 0.0f);
				gpEmitterManager4D2->createEmitter(pos, 0x1FA, nullptr, nullptr);
				unk110 = gpEmitterManager4D2->unkC8[0][0];

				JGeometry::TVec3<f32> off;
				off.set(4.0f * zoom, 1.0f, 1.0f);
				((JGeometry::TVec3<f32>*)((u8*)unk110 + 0x190))->set(off);
			}

			if (unkE8 < 18.0f) {
				s32 dy = (s32)(0.5f * unkE8);
				rect.reform(-(s32)unkE8, -dy, (s32)unkE8, dy);
				unk98[unkE0]->mBounds = rect;

				unk98[unkE0]->mWhite = unkE4 + ((u8)(s32)(10.0f * unkE8) << 24);
			} else if (unkE8 < 35.0f) {
				f32 t = unkE8 - 18.0f;
				s32 dy = (s32)(0.5f * t);
				rect.reform(-(s32)t, -dy, (s32)t, dy);
				unk98[unkE0]->mBounds = rect;

				unk98[unkE0]->mWhite
				    = unkE4 + ((u8)(s32)(10.0f * (35.0f - unkE8)) << 24);
			} else {
				unkE8 = -0.5f;
				unkFC *= -1;
			}

			unkE8 += 0.5f;
			break;
		}
		case 4:
			disappearWindow();
			break;
		default:
			break;
		}
	}

	if (param_1 & 8) {
		switch (unk10) {
		case 0: {
			J2DOrthoGraph graph(param_2->mViewportRect);
			graph.setup2D();
			unk14->draw(0, 0, &graph);
			GXSetScissor(param_2->mScissorRect.x1, param_2->mScissorRect.y1,
			             param_2->mScissorRect.x2 - param_2->mScissorRect.x1,
			             param_2->mScissorRect.y2 - param_2->mScissorRect.y1);
			break;
		}
		case 1:
		case 2:
		case 3:
		case 4: {
			J2DOrthoGraph graph(param_2->mViewportRect);
			graph.setup2D();
			unk14->draw(0, 0, &graph);
			GXSetScissor(param_2->mScissorRect.x1, param_2->mScissorRect.y1,
			             param_2->mScissorRect.x2 - param_2->mScissorRect.x1,
			             param_2->mScissorRect.y2 - param_2->mScissorRect.y1);
			break;
		}
		default:
			break;
		}
	}
}

u8 TPauseMenu2::getNextState()
{
	u8 result = 2;
	if (unk109 != 0) {
		switch (unkE0) {
		case 0:
			if (unk10 == 5) result = 0;
			break;
		case 1:
			if (unk10 == 5) result = unk118->getNextState();
			break;
		case 2:
			if (unk10 == 5) result = 5;
			break;
		default:
			result = 0;
			break;
		}
	} else if (unk108 != 0) {
		if (unk10 == 5) result = 0;
	}
	return result;
}

void TPauseMenu2::setDrawStart()
{
	unk108 = 0;
	unk109 = 0;
	unkE0  = 0;
	unkEC  = 0.0f;
	unk10  = 0;
	unkE8  = 0.0f;

	unk98[0]->mWhite   = unkE4;
	unk98[0]->mVisible = false;
	for (int i = 1; i < unk104; i++) {
		unk98[i]->mWhite   = -1;
		unk98[i]->mVisible = false;
	}

	unk20[0]->mRotation = (f32)(u16)(s32)(180.0f + unk84[0]);
	unk20[1]->mRotation = (f32)(u16)(s32)(180.0f + unk84[1]);
	unk20[2]->mRotation = (f32)(u16)(s32)(180.0f + unk84[2]);
	unk20[3]->mRotation = (f32)(u16)(s32)(180.0f + unk84[3]);
	unk20[4]->mRotation = (f32)(u16)(s32)(180.0f + unk84[4]);

	unk1C->mAlpha    = 0xff;
	unk1C->mRotation = 0.0f;

	gpMarDirector->mConsole->pauseIn();
	SMSRumbleMgr->startPause();
	gpMSound->pauseOn(true);
}

void TPauseMenu2::setDrawEnd()
{
	SMSRumbleMgr->finishPause();
	gpMSound->pauseOff(0);
	gpMarDirector->mConsole->pauseOut();
	unkEC = 0.0f;
	unk10 = 4;
}

void TPauseMenu2::drawAppearPane(J2DPicture* pic, f32 time, JUTRect& rect,
                                 f32 factor)
{
	if (time < 0.0f) {
		if (pic->isVisible()) {
			pic->hide();
		}
	} else if (!(time >= 20.0f)) {
		if (!pic->isVisible()) {
			pic->show();
			pic->setAlpha(0);
		}

		if (time == 2.0f) {
			JUTRect local = pic->getGlobalBounds();
			gpEmitterManager4D2->createEmitter(
			    JGeometry::TVec3<f32>(
			        local.x1 + 0.5f * local.getWidth(),
			        local.y1 + 0.5f * local.getHeight(), 0.0f),
			    0x1F9, nullptr, nullptr);
		}

		f32 t      = 0.05f * time;
		s32 t1     = (s32)(2.0f * t * (1.0f - t) * 80.0f);
		s32 t2     = (s32)(0.75f * (19.0f - time));
		pic->mRotation = factor;
		JUTRect paneBounds(rect.x1 + t2, rect.y1 - t1 + t2, rect.x2 - t2,
		                   rect.y2 - t1 - t2);
		pic->setBounds(paneBounds);

		u16 alpha = (u16)(time * 12.8f);
		if (alpha > 0xff) {
			alpha = 0xff;
		}
		pic->setAlpha(alpha);
	}
}
