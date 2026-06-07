#include <GC2D/Talk2D2.hpp>
#include <Camera/Camera.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/GCConsole2.hpp>
#include <GC2D/MessageLoader.hpp>
#include <GC2D/MessageUtil.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JSupport/JSUMemoryOutputStream.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>
#include <stdio.h>

class JAISound;

class MSound {
public:
	bool gateCheck(u32);
	void talkModeOut();
};

extern MSound* gpMSound;

class TFlagManager {
public:
	s32 getFlag(u32) const;

	static TFlagManager* smInstance;
};

class RumbleMgr {
public:
	void finishPause();
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

void TTalk2D2::perform(u32, JDrama::TGraphics*) { }
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
		unk234 = 180.0f;
		unk238 = 3.1415927f;
		unk23C = 1.1f;

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
		setupBoardTextBox(data, entry);
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
		unk234 = 180.0f;
		unk238 = 3.1415927f;
		unk23C = 1.1f;

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
void TTalk2D2::openTalkWindow(TBaseNPC*) { }
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
void TTalk2D2::setMessageID(u32, u32) { }
