#include <GC2D/Talk2D2.hpp>
#include <Camera/Camera.hpp>
#include <GC2D/BoundPane.hpp>
#include <GC2D/GCConsole2.hpp>
#include <GC2D/MessageLoader.hpp>
#include <GC2D/MessageUtil.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JUtility/JUTTexture.hpp>
#include <System/Application.hpp>
#include <System/MarDirector.hpp>

class JAISound;

class MSound {
public:
	bool gateCheck(u32);
	void talkModeOut();
};

extern MSound* gpMSound;

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
void TTalk2D2::setupBoardTextBox(const void*, JMSMesgEntry*) { }
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
void TTalk2D2::eraseNormalWindow() { }
void TTalk2D2::closeNormalWindow() { }
void TTalk2D2::checkControler() { }
void TTalk2D2::moveTalkWindow() { }
void TTalk2D2::checkBoardControler() { }
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
void TTalk2D2::openNormalWindow() { }
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
