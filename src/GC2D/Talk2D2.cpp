#include <GC2D/Talk2D2.hpp>
#include <Camera/Camera.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/GCConsole2.hpp>
#include <GC2D/MessageLoader.hpp>
#include <GC2D/MessageUtil.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DOrthoGraph.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JSupport/JSUMemoryOutputStream.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <MarioUtil/DrawUtil.hpp>
#include <MarioUtil/ReinitGX.hpp>
#include <NPC/NpcBase.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <stdio.h>

class JAISound;

class MSound {
public:
	bool gateCheck(u32);
	void talkModeIn(bool);
	void talkModeOut();
};

extern MSound* gpMSound;

class MSBgm {
public:
	static JAISound* startBGM(u32);
};

class TFlagManager {
public:
	s32 getFlag(u32) const;

	static TFlagManager* smInstance;
};

class RumbleMgr {
public:
	void finishPause();
	void startPause();
};

extern RumbleMgr* SMSRumbleMgr;

namespace MSoundSESystem {
class MSoundSE {
public:
	static void startSoundSystemSE(u32, u32, JAISound**, u32);
};
} // namespace MSoundSESystem

TTalk2D2* gpTalk2D;

GXColor TTalk2D2::cColorTable[6] = {
	{ 0xff, 0xff, 0xff, 0xff },
	{ 0xff, 0xff, 0xff, 0xff },
	{ 0xff, 0xb4, 0x8c, 0xff },
	{ 0x6e, 0xe6, 0xff, 0xff },
	{ 0xff, 0xff, 0x00, 0xff },
	{ 0xaa, 0xff, 0x50, 0xff },
};

static const u32 scTalkSoundList[] = {
	0x00008850, 0x00008851, 0x00008852, 0x00008853, 0x00008854,
	0x00008855, 0x00008856, 0x00008857, 0x00008858, 0x00008859,
	0x0000885A, 0x0000885B, 0x0000885C, 0x0000885D, 0x0000885E,
	0x0000885F, 0x00008860, 0x00008861, 0x00008862, 0x00008863,
	0x00008866, 0x00008867, 0x00008868, 0x00008869, 0x0000886A,
	0x0000886B, 0x0000886C, 0x0000886D, 0x0000886E, 0x0000886F,
	0x00008870, 0x00008871, 0x00008872, 0x00008873, 0x00008874,
	0x00008875, 0x00008876, 0x00008877, 0x00008878, 0x00008879,
	0x0000887A, 0x0000887B, 0x0000887C, 0x0000887D, 0x0000887E,
	0x0000887F, 0x00008880, 0x00008881, 0x00008882, 0x00008883,
	0x00008884, 0x00008885, 0x00008886, 0x00008887, 0x00008888,
	0x00008889, 0x0000888A, 0x0000888B, 0x0000888C, 0x0000888D,
	0x0000888E, 0x0000888F, 0x00008890, 0x00008891, 0x00008892,
	0x00008893, 0x00008894, 0x00008895, 0x00008896, 0xFFFFFFFF,
	0x00008899, 0x0000889A, 0x0000889B, 0x0000889C, 0x0000889D,
	0x0000889E, 0x0000889F, 0x000088A0, 0x000088A1, 0x000088A2,
	0x000088A3, 0x000088C0, 0x000088C1, 0x000088C2, 0x000088C3,
	0x000088C4, 0x000088C5, 0x000088C6, 0x000088C7, 0x000088C8,
	0x000088C9, 0x000088CA, 0x000088CB, 0x000088CC, 0x000088CD,
	0x000088CE, 0x000088CF, 0x000088E5, 0x000088E6, 0x000088E7,
	0x000088E8, 0x000088E9, 0x000088EA, 0x000088EB, 0x000088D0,
	0x000088EC, 0x000088ED, 0x000088D1, 0x000088EE, 0x000088D2,
	0x000088EF, 0x000088D3, 0x000088D4, 0x000088D5, 0x000088D6,
	0x000088D7, 0x000088D8, 0x000088D9, 0x000088DA, 0x000088DB,
	0x000088DC, 0x000088DD, 0x000088DE, 0x000088DF, 0x000088E0,
	0x000088E1, 0x000088E2, 0x000088E3, 0x000088E4, 0x00004849,
	0x80010025, 0x0000483D, 0x000088A6, 0x00008864, 0x00008865,
};

TTalk2D2::TTalk2D2(const char* name)
    : JDrama::TViewObj(name)
{
	unk28  = 0;
	unk2C  = 0;
	unk90  = 0;
	unk94  = 0.0f;
	unk214 = -1;
	unk220 = 0x1a;
	unk222 = 0;
	unk248 = 0;
	unk24C = 0;
	unk250 = 1;
	unk251 = 0;
	unk252 = 0;
	unk254 = 0;
	unk264 = 3;
	unk26A = 0;
	unk26B = 0;
	unk26C = 0;
	unk26D = 0;
	unk270 = 1;
	unk274 = 0;
	unk278 = 0;
	unk27C = cColorTable[0];
	unk280 = 0;
	unk2DC = 0;
	unk330 = 0;
	unk332 = 0;
	unk334 = 0;
	unk338 = 0.04f;
	unk33C = 1.0f;
	unk340 = 100;
	gpTalk2D = this;

	for (int i = 0; i < 90; ++i)
		unk9C[i] = 0;

	unk224 = 0;
	unk6C[0] = 0;
	unk78[0] = 0;
	unk228[0] = 0;
	unk225 = 0;
	unk6C[1] = 0;
	unk78[1] = 0;
	unk228[1] = 0;
	unk226 = 0;
	unk6C[2] = 0;
	unk78[2] = 0;
	unk228[2] = 0;
}

void TTalk2D2::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);

	JKRArchive* archive = (JKRArchive*)JKRFileLoader::getVolume("game_6");

	unk2C = new J2DSetScreen("message_2.blo", archive);

	for (int i = 0; i < 3; ++i) {
		unk30[i] = unk2C->search('me_1' + i);
		unk3C[i] = unk2C->search('bac1' + i);
		unk48[i] = unk2C->search('\0f_1' + i * 3);
		unk54[i] = unk2C->search('\0f_2' + i * 3);
		unk60[i] = unk2C->search('\0f_3' + i * 3);
		unk6C[i] = unk2C->search('cu_1' + i);
		unk78[i] = unk2C->search('cc_1' + i);
		unk84[i] = unk2C->search('cs_1' + i);
	}

	unk244 = new JUTTexture(
	    (const ResTIMG*)JKRFileLoader::getGlbResource(
	        "/game_6/timg/message_back_1.bti"));
	unk90 = unk2C->search('me_0');

	unk258 = new TMessageLoader;
	unk258->loadMessageData("/scene/map/message.bmg");

	unk25C = new TMessageLoader;
	unk25C->loadMessageData("/common/2d/sys_message.bmg");

	unk204 = unk2C->search('me_4');
	unk208 = (J2DTextBox*)unk2C->search('slct');
	unk208->setFont((JUTFont*)gpSystemFont);

	for (int i = 0; i < 2; ++i) {
		unk20C[i] = (J2DTextBox*)unk2C->search('sc_1' + i);
		unk218[i] = new char[0x11];
	}

	unk10 = new J2DSetScreen("message_board_1.blo", archive);
	unk14 = new TBoundPane(unk10, 'mb_0');
	unk18 = (J2DTextBox*)unk10->search('text');
	SMSMakeTextBuffer(unk18, 0x200);
	unk18->setFont((JUTFont*)gpSystemFont);
	unk1C = unk10->search('cu_1');
	unk20 = unk10->search('cc_1');
	unk24 = unk10->search('cs_1');
}

void TTalk2D2::loadAfter()
{
	JDrama::TViewObj::loadAfter();
}

void TTalk2D2::perform(u32 flags, JDrama::TGraphics* graphics)
{
	if (flags & 1) {
		if (gpMarDirector->unk124 == 2) {
			switch (unk248) {
			case 2: {
				CPolarSubCamera* camera = gpCamera;
				bool ready              = false;
				if (camera->isTalkCameraSpecifyMode(camera->mMode)
				    && !camera->isNowInbetween())
					ready = true;

				if (ready ? true : false) {
					unk251 = 0x14;
					unk248 = 3;
				}
				break;
			}
			case 4:
				if (unk28) {
					if (openBoardWindow())
						unk248 = 5;
				} else {
					if (openNormalWindow())
						unk248 = 5;
				}
				break;
			case 5:
				if (unk28) {
					moveBoardWindow();
					checkBoardControler();
				} else {
					moveTalkWindow();
					checkControler();
				}
				break;
			case 6: {
				bool closed = false;
				if (unk28) {
					if (unk14->update())
						closed = true;
				} else {
					closed = closeNormalWindow();
				}

				if (closed) {
					if (unk270 & 1)
						unk248 = 0;
					else
						unk248 = 1;
				}
				break;
			}
			case 7:
				if (unk28) {
					if (eraseBoardWindow())
						unk248 = 8;
				} else {
					if (eraseNormalWindow())
						unk248 = 4;
				}
				break;
			case 8: {
				bool complete = false;
				int alpha     = unk18->mAlpha + 4;
				if ((u16)alpha > 0xff) {
					alpha    = 0xff;
					complete = true;
				}

				unk18->mAlpha = alpha;
				if (complete)
					unk248 = 5;
				break;
			}
			}
		}
	}

	if (flags & 2) {
		if (gpMarDirector->unk124 == 2) {
			u32 mode = unk248;
			if (mode == 3) {
				--unk251;
				if ((s8)unk251 < 0) {
					unk2DE = 0;
					unk248 = 4;
				}
			} else if (mode == 7) {
				int alpha = unk90->mAlpha - 0x10;
				if ((s16)alpha < 0) {
					unk234 = 1.0f;
					unk238 = 2.0f;
					unk23C = 3.0f;

					for (int i = 0; i < 3; ++i) {
						unk3C[i]->mVisible = false;
						unk6C[i]->mVisible = false;
						(&unk224)[i]       = 0;
					}

					for (int i = 0; i < 90; ++i)
						if (unk9C[i])
							unk9C[i]->mVisible = false;

					TMessageLoader* loader = unk260;
					void* data             = loader->unk4;
					JMSMesgEntry* entry
					    = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
					setupTextBox(data, entry);
					unk26C = 0;

					if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
						unk340 = 0x20;
					else
						unk340 = 0x40;

					*(u32*)&unk27C = 0xffffffff;
					*(u32*)&unk27C = 0xffffffff;
					unk2DE         = 0;
					unk2DC         = 0;
					unk248         = 4;
					alpha          = 0xff;
				}

				unk90->mAlpha = alpha;
			}
		}
	}

	if (flags & 8) {
		if (gpMarDirector->unk124 == 2) {
			ReInitializeGX();
			SMS_DrawInit();

			J2DOrthoGraph graph(graphics->mViewportRect);
			graph.setup2D();

			if (unk250) {
				unk3C[0]->mVisible = true;
				unk3C[1]->mVisible = true;
				unk3C[2]->mVisible = true;

				J2DPane* root = unk2C->search('ROOT');
				root->mAlpha  = 0;
				unk2C->draw(0, 0, &graph);
				root->mAlpha = 0xff;

				graph.setup2D();
				unk250 = 0;

				unk3C[0]->mVisible = false;
				unk3C[1]->mVisible = false;
				unk3C[2]->mVisible = false;
			}

			u32 mode = unk248;
			if (mode == 4) {
				for (s8 i = 0; i <= unk274; ++i)
					openWindow(i, (&unk234)[i]);
			}

			if (mode >= 4 && mode < 8) {
				graph.setup2D();
				if (unk28) {
					unk10->draw(0, 0, &graph);
				} else {
					unk90->move(unk330, unk332);
					unk90->mRotation = (f32)unk334;
					unk2C->draw(0, 0, &graph);
				}
			}
		}
	}
}
void TTalk2D2::openWindow(s8, f32) { }
void TTalk2D2::setTagParam(JSUMemoryInputStream&, J2DTextBox&, int*, int*) { }
void TTalk2D2::setupTextBox(const void*, JMSMesgEntry*) { }
void TTalk2D2::setupBoardTextBox(const void* data, JMSMesgEntry* entry)
{
	TMessageLoader::EntryInfo* info = (TMessageLoader::EntryInfo*)entry;
	JSUMemoryInputStream input((const u8*)data + info->unk0 + unk278, 0x400);
	JSUMemoryOutputStream output(unk18->getStringPtr(), 0x200);
	unk254 = entry;

	int lineCount = 0;
	while (lineCount < 6) {
		u8 c;
		input.read(&c, 1);

		switch (c) {
		case '\n':
			output.write(&c, 1);
			++lineCount;
			break;
		case 0:
			unk26A    = 1;
			lineCount = 6;
			break;
		case 0x1a:
			break;
		default:
			input.skip(-1);
			input.read(&c, 1);
			output.write(&c, 1);
			if (c >= 0x80) {
				input.read(&c, 1);
				output.write(&c, 1);
			}
			break;
		}
	}

	if (!unk26A) {
		u8 c;
		input.peek(&c, 1);
		if ((s8)c == 0) {
			unk26A = 1;
			input.skip(1);
			c = 0;
			output.write(&c, 1);
		}
	}

	unk278 += input.getPosition();
}
bool TTalk2D2::eraseBoardWindow()
{
	bool result = false;
	int alpha   = unk18->mAlpha - 4;

	if ((s16)alpha < 0) {
		TMessageLoader* loader = unk260;
		void* data             = loader->unk4;
		JMSMesgEntry* entry
		    = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		setupBoardTextBox(data, entry);
		unk27C = cColorTable[0];
		unk2DE = 0;
		unk2DC = 0;
		alpha  = 0;
		result = true;
	}

	unk18->mAlpha = alpha;
	return result;
}
bool TTalk2D2::eraseNormalWindow()
{
	bool result = false;
	int alpha   = unk90->mAlpha - 0x10;

	if ((s16)alpha < 0) {
		unk234 = 1.0f;
		unk238 = 2.0f;
		unk23C = 3.0f;

		for (int i = 0; i < 3; ++i) {
			unk3C[i]->mVisible = false;
			unk6C[i]->mVisible = false;
			(&unk224)[i]       = 0;
		}

		for (int i = 0; i < 90; ++i)
			if (unk9C[i])
				unk9C[i]->mVisible = false;

		TMessageLoader* loader = unk260;
		void* data             = loader->unk4;
		JMSMesgEntry* entry
		    = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		setupTextBox(data, entry);
		unk26C = 0;

		if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
			unk340 = 0x20;
		else
			unk340 = 0x40;

		unk27C = cColorTable[0];
		unk2DE = 0;
		unk2DC = 0;
		alpha  = 0xff;
		result = true;
	}

	unk90->mAlpha = alpha;
	if (unk204->mVisible)
		unk204->mAlpha = (u8)alpha;

	return result;
}
bool TTalk2D2::closeNormalWindow()
{
	bool result = false;
	int alpha   = unk90->mAlpha - 0x10;

	if ((s16)alpha < 0) {
		unk234 = 1.0f;
		unk238 = 2.0f;
		unk23C = 3.0f;

		for (int i = 0; i < 3; ++i) {
			unk3C[i]->mVisible = false;
			unk6C[i]->mVisible = false;
			(&unk224)[i]       = 0;
		}

		for (int i = 0; i < 90; ++i)
			if (unk9C[i])
				unk9C[i]->mVisible = false;

		if (unk204->mVisible)
			unk204->mVisible = false;

		result = true;
	}

	unk90->mAlpha = alpha;
	if (unk204->mVisible)
		unk204->mAlpha = (u8)alpha;

	return result;
}
void TTalk2D2::checkControler()
{
	if (unk6C[unk274]->mVisible) {
		if (unk26A) {
			if (!unk26D) {
				u32 trigger = *(u32*)((u8*)unk24C + 0xd4);
				if (!(trigger & 0x20000) && !(trigger & 0x40000))
					return;
			}

			if (gpMSound->gateCheck(0x481c))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0,
				    0);

			if (unk270 & 1) {
				if (unk264 == 0x19) {
					if (gpMSound->gateCheck(0x4851))
						MSoundSESystem::MSoundSE::startSoundSystemSE(
						    0x4851, 0, 0, 0);
				} else if (unk28) {
					if (gpMSound->gateCheck(0x481a))
						MSoundSESystem::MSoundSE::startSoundSystemSE(
						    0x481a, 0, 0, 0);
					gpMSound->talkModeOut();
				} else {
					gpMSound->talkModeOut();
				}

				gpCamera->makeMtxForPrevTalk();
				gpMarDirector->mConsole->startAppearTelop(false);
				SMSRumbleMgr->finishPause();
				unk252 = 0;
			}

			unk248 = 6;
		} else {
			u32 trigger = *(u32*)((u8*)unk24C + 0xd4);

			if (trigger & 0x60000) {
				if (gpMSound->gateCheck(0x481c))
					MSoundSESystem::MSoundSE::startSoundSystemSE(
					    0x481c, 0, 0, 0);
				unk248 = 7;
			}
		}
	} else if (unk204->mVisible) {
		u32 trigger = *(u32*)((u8*)unk24C + 0xd4);

		if ((trigger & 0x80000) && unk214 == 1) {
			if (gpMSound->gateCheck(0x481e))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481e, 0, 0,
				    0);

			unk214            = 0;
			unk20C[1]->mAlpha = 0xfe;
			unk20C[1]->mVisible = false;
			unk20C[0]->mVisible = true;
		} else if ((trigger & 0x100000) && unk214 == 0) {
			if (gpMSound->gateCheck(0x481e))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481e, 0, 0,
				    0);

			unk214            = 1;
			unk20C[0]->mAlpha = 0xfe;
			unk20C[0]->mVisible = false;
			unk20C[1]->mVisible = true;
		} else if (unk26A) {
			if (trigger & 0x60000) {
				if (unk270 & 1)
					gpCamera->makeMtxForPrevTalk();

				if (gpMSound->gateCheck(0x481c))
					MSoundSESystem::MSoundSE::startSoundSystemSE(
					    0x481c, 0, 0, 0);
				unk248 = 6;
			}
		} else if (trigger & 0x60000) {
			if (gpMSound->gateCheck(0x481c))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0,
				    0);
			unk248 = 7;
		}
	}
}
void TTalk2D2::moveTalkWindow()
{
	for (int i = 0; i < 3; ++i) {
		u8& lineCount = (&unk224)[i];
		int textIndex = i * 30 + lineCount;

		if (lineCount != 0 && lineCount <= unk228[i]) {
			J2DTextBox* textBox = unk9C[textIndex];

			if (textBox->mVisible) {
				int alpha = textBox->mAlpha + unk340;
				int value = (s16)alpha;
				if (value > 0xff)
					value = 0xff;

				textBox->mAlpha = value;

				if ((s16)alpha >= 0xff)
					++lineCount;
			} else if (unk2DC <= 0) {
				unk2DC            = unk281[textIndex];
				textBox->mVisible = true;
				textBox->mAlpha   = 0;
			} else {
				--unk2DC;
			}
		}
	}

	int line = unk274;
	u8 lineCount = (&unk224)[line];
	if (lineCount < 30 && lineCount <= unk228[line])
		return;

	J2DPane* pane;
	J2DPane* cursor;
	if (unk214 == -1) {
		pane = unk6C[line];

		if (unk26A) {
			unk78[line]->mVisible = false;
			cursor                = unk84[line];
			unk84[line]->mVisible = true;
		} else {
			cursor                = unk78[line];
			unk78[line]->mVisible = true;
			unk84[line]->mVisible = false;
		}
	} else {
		pane   = unk204;
		cursor = unk20C[unk214];
	}

	if (pane->mVisible) {
		int alpha = pane->mAlpha;
		if ((s16)alpha < 0xff) {
			alpha += 0x10;
			if ((s16)alpha >= 0xff)
				alpha = 0xff;
			pane->mAlpha = alpha;
		}

		int cursorAlpha = cursor->mAlpha;
		if (unk26B) {
			cursorAlpha += 2;
			if ((s16)cursorAlpha > 0xff) {
				unk26B      = 0;
				cursorAlpha = 0xff;
			}
		} else {
			cursorAlpha -= 4;
			if ((s16)cursorAlpha < 0x3c) {
				unk26B      = 1;
				cursorAlpha = 0x3c;
			}
		}
		cursor->mAlpha = cursorAlpha;

		u8 alphaByte = cursorAlpha;
		if (unk214 == 1) {
			snprintf(unk208->getStringPtr(), 0x5e,
			    "\033CC[ffffff60]\033GC[ffffff60]%s"
			    "\033CC[ffffff%02x]\033GC[ffffff%02x]\n%s",
			    unk218[0], alphaByte, alphaByte, unk218[1]);
		} else if (unk214 == 0) {
			snprintf(unk208->getStringPtr(), 0x5e,
			    "\033CC[ffffff%02x]\033GC[ffffff%02x]%s\n"
			    "\033CC[ffffff60]\033GC[ffffff60]%s",
			    alphaByte, alphaByte, unk218[0], unk218[1]);
		}
	} else {
		pane->mVisible   = true;
		pane->mAlpha     = 0;
		cursor->mAlpha   = 0xff;
	}
}
void TTalk2D2::checkBoardControler()
{
	if (unk26A) {
		if (!unk26D) {
			u32 trigger = *(u32*)((u8*)unk24C + 0xd4);
			if (!(trigger & 0x20000) && !(trigger & 0x40000))
				return;
		}

		if (gpMSound->gateCheck(0x481c))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0, 0);

		JUTPoint initial(0, -0x258);
		JUTPoint current(0, 0);
		JUTPoint target(0, 0);
		unk14->setPanePosition(0x3c, target, current, initial);

		if (unk270 & 1) {
			if (unk264 == 0x19) {
				if (gpMSound->gateCheck(0x4851))
					MSoundSESystem::MSoundSE::startSoundSystemSE(0x4851, 0,
					    0, 0);
			} else if (unk28) {
				if (gpMSound->gateCheck(0x481a))
					MSoundSESystem::MSoundSE::startSoundSystemSE(0x481a, 0,
					    0, 0);
				gpMSound->talkModeOut();
			} else {
				gpMSound->talkModeOut();
			}

			gpCamera->makeMtxForPrevTalk();
			gpMarDirector->mConsole->startAppearTelop(false);
			SMSRumbleMgr->finishPause();
			unk252 = 0;
		}

		unk248 = 6;
	} else {
		u32 trigger = *(u32*)((u8*)unk24C + 0xd4);

		if (trigger & 0x60000) {
			if (gpMSound->gateCheck(0x481c))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x481c, 0, 0,
				    0);
			unk248 = 7;
		}
	}
}
void TTalk2D2::moveBoardWindow()
{
	int alpha = unk1C->mAlpha;

	if (alpha < 0xff) {
		alpha += 4;
		if (alpha > 0xff) {
			unk24->mAlpha = 0;
			alpha         = 0xff;
			unk20->mAlpha = 0;
			unk26B        = 1;
		}

		unk1C->mAlpha = alpha;
		return;
	}

	J2DPane* pane = unk26A ? unk24 : unk20;
	alpha         = pane->mAlpha;

	if (unk26B) {
		alpha += 4;
		if (alpha > 0xff) {
			unk26B = 0;
			alpha  = 0xff;
		}
	} else {
		alpha -= 4;
		if (alpha < 0) {
			unk26B = 1;
			alpha  = 0;
		}
	}

	pane->mAlpha = alpha;
}
bool TTalk2D2::openNormalWindow()
{
	bool result = false;

	if (unk2DE > 2 && (*(u32*)((u8*)unk24C + 0xd0) & 0x20000)) {
		unk26C = 1;
		if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
			unk340 = 0x5a;
		else
			unk340 = 0x80;
	}

	bool allSettled = true;

	for (int i = 0; i <= unk274; ++i) {
		f32* lineMove = &unk234 + i;

		if (*lineMove > -0.109f) {
			*lineMove -= unk338;
			allSettled = false;
		}

		if (*lineMove < 1.0f && i != 0) {
			int prevIndex = unk228[i - 1] + (i - 1) * 30;
			if (!unk9C[prevIndex]->mVisible)
				*lineMove = 1.0f;
		}

		if ((&unk224)[i] == 0 && *lineMove < unk33C) {
			if (gpMSound->gateCheck(0x4827))
				MSoundSESystem::MSoundSE::startSoundSystemSE(0x4827, 0, 0,
				                                             0);

			(&unk224)[i]             = 1;
			unk9C[i * 30]->mVisible = true;
		}
	}

	if (allSettled) {
		for (int i = 0; i <= unk274; ++i)
			unk3C[i]->mVisible = true;
		result = true;
	}

	for (int line = 0; line < 3; ++line) {
		if ((&unk224)[line] == 0)
			continue;

		int textIndex = (&unk224)[line] + line * 30;

		while ((&unk224)[line] <= unk228[line]) {
			J2DTextBox* textBox = unk9C[textIndex];

			if (textBox->mVisible) {
				int alpha = textBox->mAlpha + unk340;
				int alphaSum = alpha;
				if (alpha > 0xff)
					alpha = 0xff;

				textBox->mAlpha = alpha;
				unk2DE          = textIndex;

				if ((s16)alphaSum < 0xff)
					break;

				++textIndex;
				++(&unk224)[line];
				continue;
			}

			if (unk2DC <= 0) {
				if (unk26C)
					unk2DC = 0;
				else
					unk2DC = unk281[textIndex];

				textBox->mVisible = true;
				textBox->mAlpha   = 0;
			} else {
				--unk2DC;
			}
			break;
		}
	}

	return result;
}
bool TTalk2D2::openBoardWindow()
{
	bool result = false;

	switch (unk29[0]) {
	case 0:
		if (unk14->update()) {
			JUTPoint initial(0, 0);
			JUTPoint current(0, 0x50);
			JUTPoint target(0, 0x50);
			unk14->setPanePosition(0x19, target, current, initial);
			++unk29[0];
		}
		break;
	case 1:
		if (unk14->update()) {
			unk1C->mAlpha   = 0;
			unk1C->mVisible = true;
			result          = true;
			++unk29[0];
		}
		break;
	}

	return result;
}
void TTalk2D2::makeBoxLine(s8, char*) { }
void TTalk2D2::openTalkWindow(TBaseNPC* npc)
{
	if (npc)
		gpCamera->makeMtxForTalk(npc);

	if (unk28) {
		JUTPoint end(0, 0x50);
		JUTPoint current(0, 0x50);
		JUTPoint start(0, -0x320);
		unk14->setPanePosition(0x3c, start, current, end);
		unk14->update();

		unk1C->mAlpha = 0;
		unk248        = 4;
	} else {
		unk248 = 2;
	}

	unk90->mAlpha = 0xff;

	switch (gpCamera->mMode) {
	case 0x2d:
		unk330 = 0xa0;
		unk332 = 0x87;
		unk334 = 0x14;
		break;
	case 0xc:
	default:
		unk330 = 0x184;
		unk332 = 0x73;
		unk334 = -0x12;
		break;
	}

	unk90->move(unk330, unk332);
	unk90->mRotation = unk334;
	unk250           = 1;

	gpMarDirector->mConsole->startDisappearTelop();
	{
		TGCConsole2* console = gpMarDirector->mConsole;
		console->startDisappearBalloon(console->unk3E0, true);
	}
	gpMarDirector->mConsole->startDisappearMario();

	if (unk264 == 0x19) {
		if (gpMSound->gateCheck(0x4848))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4848, 0, 0, 0);
	} else if (unk28) {
		if (gpMSound->gateCheck(0x4819))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4819, 0, 0, 0);
		gpMSound->talkModeIn(false);
	} else {
		gpMSound->talkModeIn(true);
	}

	SMSRumbleMgr->startPause();
}
void TTalk2D2::forceCloseTalk()
{
	gpCamera->makeMtxForPrevTalk();

	if (unk28) {
		if (gpMSound->gateCheck(0x4851))
			MSoundSESystem::MSoundSE::startSoundSystemSE(0x4851, 0, 0, 0);
	} else {
		gpMSound->talkModeOut();
	}

	gpMarDirector->mConsole->startAppearTelop(false);

	if (unk248 == 1)
		unk248 = 0;
	else
		unk248 = 6;
}
void TTalk2D2::setMessageID(u32 message_id, u32 flags)
{
	TBaseNPC* npc = gpMarDirector->unkA0;

	if (npc->mActionFlag & 0x200) {
		bool isMonte = true;
		bool isMonteType = isMonte;

		if (!npc->isNormalMonteM() && !npc->isNormalMonteW())
			isMonteType = false;

		if (!isMonteType) {
			isMonteType = true;
			if (!npc->isSpecialMonteM() && !npc->isSpecialMonteW())
				isMonteType = false;
			if (!isMonteType)
				isMonte = false;
		}

		if (isMonte) {
			if (npc->isNormalMonteW() || npc->isSpecialMonteW()) {
				if (npc->isChild())
					unk264 = 0x2c;
				else
					unk264 = 0x27;
			} else {
				if (npc->isChild())
					unk264 = 0x29;
				else
					unk264 = 0x23;
			}
		} else {
			bool isMare = true;
			bool isMareType = isMare;

			if (!npc->isNormalMareM() && !npc->isNormalMareW())
				isMareType = false;

			if (!isMareType) {
				isMareType = true;
				if (!npc->isSpecialMareM() && !npc->isSpecialMareW())
					isMareType = false;
				if (!isMareType)
					isMare = false;
			}

			if (isMare) {
				if (npc->isNormalMareW() || npc->isSpecialMareW()) {
					if (npc->isChild())
						unk264 = 0x2d;
					else
						unk264 = 0x28;
				} else {
					if (npc->isChild())
						unk264 = 0x2a;
					else
						unk264 = 0x24;
				}
			} else if (npc->mActorType == 0x04000016) {
				unk264 = 0x25;
			} else if (npc->mActorType == 0x04000010) {
				unk264 = 0x2b;
			}
		}

		for (int i = 0; i < 10; ++i) {
			if (unk2E0[i].unk0 == npc) {
				unk264 = unk2E0[i].unk4;
				break;
			}
		}
	} else {
		unk264 = message_id;
	}

	if (npc->mActorType == 0x0400001d)
		unk28 = 1;
	else
		unk28 = 0;

	unk270 = flags;
	unk278 = 0;
	unk214 = -1;
	unk26A = 0;
	*(u32*)&unk27C = 0xffffffff;
	unk280 = 0;
	unk26C = 0;
	unk26D = 0;
	unk254 = 0;
	unk29[0] = 0;

	if (TFlagManager::smInstance->getFlag(0xa0001) == 0x100)
		unk340 = 0x20;
	else
		unk340 = 0x40;

	TMessageLoader* loader;
	if (unk264 & 0xffff0000)
		loader = unk258;
	else
		loader = unk25C;

	JMSMesgEntry* entry;
	if (loader->unk4) {
		entry = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		if (!entry) {
			unk264 = 4;
			loader = unk25C;
			entry = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		}
		setupBoardTextBox(loader->unk4, entry);
	} else {
		unk264 = 3;
		loader = unk25C;
		entry = (JMSMesgEntry*)loader->getMessageEntry((u16)unk264);
		setupBoardTextBox(loader->unk4, entry);
	}

	unk260 = loader;
	unk2DC = 0;

	if (unk254) {
		s32 sound = scTalkSoundList[((u8*)unk254)[8]];
		if (sound != -1 && gpMSound->gateCheck(sound)) {
			if ((u32)sound & 0x80000000)
				MSBgm::startBGM(sound);
			else
				MSoundSESystem::MSoundSE::startSoundSystemSE(sound, 0, 0, 0);
		}
	}

	if (unk248 == 1) {
		unk248 = 3;
		unk90->mAlpha = 0xff;
	}

	unk252 = 1;
}
