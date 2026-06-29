#include <System/MenuDir.hpp>
#include <stdio.h>
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JDrama/JDRScreen.hpp>
#include <JSystem/JDrama/JDRDStageGroup.hpp>
#include <JSystem/JDrama/JDRDStage.hpp>
#include <JSystem/JDrama/JDREfbCtrl.hpp>
#include <JSystem/J2D/J2DScreen.hpp>
#include <JSystem/J2D/J2DTextBox.hpp>
#include <System/Resolution.hpp>
#include <System/MarioGamePad.hpp>
#include <System/Application.hpp>
#include <System/FlagManager.hpp>
#include <System/MovieDirector.hpp>
#include <MSound/MSound.hpp>
#include <GC2D/ScrnFader.hpp>
#include <GC2D/Menu.hpp>
#include <GC2D/MessageUtil.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

// DEBUG: Set to 0 to restore the original BMG/Japanese debug-menu strings.
#define SMS_DEBUG_STAGE_SELECT_ENGLISH 1
// DEBUG: Set to 0 to show test maps, movie entries, and No Data rows again.
#define SMS_DEBUG_STAGE_SELECT_PROPER_LEVELS_ONLY 1
#define SMS_DEBUG_STAGE_SELECT_PROPER_LEVEL_COUNT 10

#if SMS_DEBUG_STAGE_SELECT_ENGLISH
static const char* const sDebugStageSelectLines[19] = {
	"00 Delfino Airstrip",
	"01 Delfino Plaza",
	"02 Bianco Hills",
	"03 Ricco Harbor",
	"04 Gelato Beach",
	"05 Pinna Park",
	"06 Sirena Beach",
	"07 Pianta Village",
	"08 Noki Bay",
	"09 Corona Mountain",
	"10 Scale Map",
	"11 Test Map 1",
	"12 No Data",
	"13 No Data",
	"14 No Data",
	"15 No Data",
	"16 No Data",
	"show movie 1",
	"show movie 2",
};

#define SMS_DEBUG_MENU_BEACH "Beach %d"
#define SMS_DEBUG_MENU_HOTEL "Hotel %d"
#define SMS_DEBUG_MENU_CASINO "Casino %d"
#define SMS_DEBUG_MENU_BOSS "Boss"
#define SMS_DEBUG_MENU_BOSS_NUM "Boss %d"
#define SMS_DEBUG_MENU_BEACHSIDE "Beachside %d"
#define SMS_DEBUG_MENU_PINNA_PARK "Pinna Park %d"
#define SMS_DEBUG_MENU_DEMO "Demo %d"
#define SMS_DEBUG_MENU_NOKI "Noki %d"
#define SMS_DEBUG_MENU_UNDERSEA "Undersea"
#define SMS_DEBUG_MENU_SCENE "%02d Scene %d"
#endif

static J2DTextBox* getDebugStageTextBox(J2DSetScreen* screen, int index)
{
	int code = index < 9 ? 'tx01' + index : 'tx10' + index - 9;
	return (J2DTextBox*)screen->search(code);
}

#if SMS_DEBUG_STAGE_SELECT_PROPER_LEVELS_ONLY
struct TDebugScenarioEntry {
	u8 mStage;
	u8 mScenario;
	const char* mName;
};

struct TDebugScenarioMenu {
	u8 mCount;
	const TDebugScenarioEntry* mEntries;
};

static const TDebugScenarioEntry sAirportDebugScenarios[] = {
	{ 0, 0, "Airstrip 0" },
	{ 0, 1, "Airstrip 1" },
};

static const TDebugScenarioEntry sDolpicDebugScenarios[] = {
	{ 1, 0, "Plaza 0" },     { 1, 1, "Plaza 1" },
	{ 1, 2, "Plaza 10" },    { 1, 5, "Plaza 5" },
	{ 1, 6, "Plaza 6" },     { 1, 7, "Plaza 7" },
	{ 1, 8, "Plaza 8" },     { 1, 9, "Plaza 9" },
	{ 20, 0, "Plaza EX 0" }, { 21, 0, "Plaza EX 1" },
	{ 22, 0, "Plaza EX 2" }, { 23, 0, "Plaza EX 3" },
	{ 24, 0, "Plaza EX 4" }, { 25, 0, "Plaza EX 5" },
	{ 26, 0, "Plaza EX 6" }, { 27, 0, "Plaza EX 7" },
};

static const TDebugScenarioEntry sBiancoDebugScenarios[] = {
	{ 2, 0, "Scene 0" },  { 2, 1, "Scene 1" },
	{ 2, 2, "Scene 2" },  { 2, 3, "Scene 3" },
	{ 2, 4, "Scene 4" },  { 2, 5, "Scene 5" },
	{ 2, 6, "Scene 6" },  { 2, 7, "Scene 7" },
	{ 55, 0, "Boss" },    { 28, 0, "EX 0" },
	{ 29, 0, "EX 1" },
};

static const TDebugScenarioEntry sRiccoDebugScenarios[] = {
	{ 3, 0, "Scene 0" }, { 3, 1, "Scene 1" },
	{ 3, 2, "Scene 2" }, { 3, 3, "Scene 3" },
	{ 3, 4, "Scene 4" }, { 3, 5, "Scene 5" },
	{ 3, 6, "Scene 6" }, { 3, 7, "Scene 7" },
	{ 30, 0, "EX 0" },   { 31, 0, "EX 1" },
	{ 59, 0, "Boss" },
};

static const TDebugScenarioEntry sMammaDebugScenarios[] = {
	{ 4, 0, "Scene 0" }, { 4, 1, "Scene 1" },
	{ 4, 2, "Scene 2" }, { 4, 3, "Scene 3" },
	{ 4, 4, "Scene 4" }, { 4, 5, "Scene 5" },
	{ 4, 6, "Scene 6" }, { 4, 7, "Scene 7" },
	{ 32, 0, "EX 0" },   { 33, 0, "EX 1" },
};

static const TDebugScenarioEntry sPinnaDebugScenarios[] = {
	{ 5, 0, "Beach 0" }, { 5, 1, "Beach 1" },
	{ 5, 2, "Beach 2" }, { 5, 3, "Beach 3" },
	{ 5, 4, "Beach 4" }, { 13, 0, "Park 0" },
	{ 13, 1, "Park 1" }, { 13, 2, "Park 2" },
	{ 13, 3, "Park 3" }, { 13, 4, "Park 4" },
	{ 13, 5, "Park 5" }, { 13, 6, "Park 6" },
	{ 13, 7, "Park 7" }, { 37, 0, "EX 3" },
	{ 38, 0, "EX 4" },   { 39, 0, "EX 5" },
	{ 58, 0, "Boss 0" }, { 58, 1, "Boss 1" },
};

static const TDebugScenarioEntry sSirenaDebugScenarios[] = {
	{ 6, 0, "Beach 0" },  { 6, 1, "Beach 1" },
	{ 6, 2, "Beach 2" },  { 6, 3, "Beach 3" },
	{ 6, 4, "Beach 4" },  { 6, 5, "Beach 5" },
	{ 6, 6, "Beach 6" },  { 6, 7, "Beach 7" },
	{ 7, 0, "Hotel 0" },  { 7, 1, "Hotel 1" },
	{ 7, 2, "Hotel 2" },  { 7, 3, "Hotel 3" },
	{ 7, 4, "Hotel 4" },  { 14, 0, "Casino 0" },
	{ 14, 1, "Casino 1" }, { 40, 0, "EX 0" },
	{ 41, 0, "EX 1" },    { 56, 0, "Boss" },
};

static const TDebugScenarioEntry sMonteDebugScenarios[] = {
	{ 8, 0, "Scene 0" }, { 8, 1, "Scene 1" },
	{ 8, 2, "Scene 2" }, { 8, 3, "Scene 3" },
	{ 8, 4, "Scene 4" }, { 8, 5, "Scene 5" },
	{ 8, 6, "Scene 6" }, { 8, 7, "Scene 7" },
	{ 42, 0, "EX 0" },   { 43, 0, "EX 1" },
};

static const TDebugScenarioEntry sMareDebugScenarios[] = {
	{ 9, 0, "Scene 0" }, { 9, 1, "Scene 1" },
	{ 9, 2, "Scene 2" }, { 9, 3, "Scene 3" },
	{ 9, 4, "Scene 4" }, { 9, 5, "Scene 5" },
	{ 9, 6, "Scene 6" }, { 9, 7, "Scene 7" },
	{ 16, 0, "Undersea" }, { 44, 0, "EX 0" },
	{ 45, 0, "EX 1" },     { 57, 0, "Boss" },
};

static const TDebugScenarioEntry sCoronaDebugScenarios[] = {
	{ 46, 0, "Corona 0" }, { 47, 0, "Corona 1" },
	{ 48, 0, "Corona 2" }, { 49, 0, "Corona 3" },
	{ 50, 0, "Corona 4" }, { 51, 0, "Corona 5" },
	{ 52, 0, "Corona 6" }, { 60, 0, "Bowser" },
};

#define DEBUG_SCENARIO_MENU(entries)                                           \
	{ sizeof(entries) / sizeof(entries[0]), entries }

static const TDebugScenarioMenu sDebugScenarioMenus[] = {
	DEBUG_SCENARIO_MENU(sAirportDebugScenarios),
	DEBUG_SCENARIO_MENU(sDolpicDebugScenarios),
	DEBUG_SCENARIO_MENU(sBiancoDebugScenarios),
	DEBUG_SCENARIO_MENU(sRiccoDebugScenarios),
	DEBUG_SCENARIO_MENU(sMammaDebugScenarios),
	DEBUG_SCENARIO_MENU(sPinnaDebugScenarios),
	DEBUG_SCENARIO_MENU(sSirenaDebugScenarios),
	DEBUG_SCENARIO_MENU(sMonteDebugScenarios),
	DEBUG_SCENARIO_MENU(sMareDebugScenarios),
	DEBUG_SCENARIO_MENU(sCoronaDebugScenarios),
};

static J2DTextBox* getDebugScenarioTextBox(J2DSetScreen* screen, int index)
{
	int code = index < 9 ? 'st_1' + index : 'st_a' + index - 9;
	return (J2DTextBox*)screen->search(code);
}

static void applyDebugScenarioMenu(J2DSetScreen* screen, TMenuPlane* menu,
                                   int stageIndex)
{
	const TDebugScenarioMenu& scenarioMenu = sDebugScenarioMenus[stageIndex];

	for (int i = 0; i < 20; ++i) {
		J2DTextBox* box = getDebugScenarioTextBox(screen, i);
		if (i < scenarioMenu.mCount) {
			box->setString(scenarioMenu.mEntries[i].mName);
			box->show();
		} else {
			box->setString("");
			box->hide();
		}
	}

	menu->unk28 = scenarioMenu.mCount;
	menu->unk2C = 0;
}
#endif

TMenuDirector::TMenuDirector()
    : unk18(0)
    , unk1C(nullptr)
    , unk28(0)
{
	f32 sync = SMSGetVSyncTimesPerSec();
	unk30    = 0;
	unk34    = sync;
	unk3C    = nullptr;
	unk40    = nullptr;
	unk44    = nullptr;
	unk48    = 0;
	unk4C    = 0;
	unk50    = false;
}

TMenuDirector::~TMenuDirector()
{
	unk2C->offFlag(0x1);
	JKRMemArchive* arc = (JKRMemArchive*)JKRFileLoader::getVolume("title");
	if (arc)
		arc->unmountFixed();
}

void* TMenuDirector::setupThreadFunc(void* param_1)
{
	// BUG: return missing
	((TMenuDirector*)param_1)->rsetup();
}

extern OSThread gSetupThread;
extern u8* gpSetupThreadStack;

void TMenuDirector::setup(JDrama::TDisplay* param_1, TMarioGamePad* param_2)
{
	unk14         = new JDrama::TDStageGroup(param_1);
	unk2C         = param_2;
	unk2C->mFlags = 1;
	OSCreateThread(&gSetupThread, &setupThreadFunc, this,
	               gpSetupThreadStack + 0x10000, 0x10000, 0x11, 0);
	OSResumeThread(&gSetupThread);
}

int TMenuDirector::rsetup()
{
	void* arcBlob      = SMSLoadArchive("/data/title.arc", nullptr, 0, nullptr);
	JKRMemArchive* arc = new JKRMemArchive;
	arc->mountFixed(arcBlob, MBF_0);

	JDrama::TViewObjPtrListT<JDrama::TViewObj>* rootViewObjs
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("root View Objs");
	unk10 = rootViewObjs;

	JDrama::TViewObjPtrListT<JDrama::TViewObj>* group2d
	    = new JDrama::TViewObjPtrListT<JDrama::TViewObj>("Group 2D");
	rootViewObjs->getChildren().push_back(group2d);

	unk3C = new J2DSetScreen("title.blo", arc);

	if (!unk3C)
		return 1;

	group2d->getChildren().push_back(new TMenuBase(unk3C));

	unk20 = unk3C->search('lisA');
	unk20->hide();
	J2DPane* statPane = unk3C->search('stat');
	if (!statPane)
		return 1;
	if (statPane->mInfoTag != 0x12)
		return 1;

	unk24 = new TFlashPane(statPane);
	group2d->getChildren().push_back(unk24);
	unk24->hide();

	unk40 = new TMenuPlane(unk2C, unk3C->search('lisB'), 33, 64);
	group2d->getChildren().push_back(unk40);

	unk40->hide();

	unk44 = new TMenuPlane(unk2C, unk3C->search('lisC'), 33, 64);
	group2d->getChildren().push_back(unk44);

	unk44->hide();

	unk38 = (J2DTextBox*)unk3C->search('stag');

	if (!unk38)
		return 1;

	unk38->hide();

	unk1C = JKRGetResource("/title/marisun_stage.bmg");
#if SMS_DEBUG_STAGE_SELECT_ENGLISH
	for (int i = 0; i < 19; ++i) {
		char acStack_40[22];
		snprintf(acStack_40, 22, "%s", sDebugStageSelectLines[i]);

		getDebugStageTextBox(unk3C, i)->setString(acStack_40);
	}
#else
	if (unk1C) {
		for (int i = 0; i < 19; ++i) {
			const char* message = SMSGetMessageData(unk1C, i);
			char acStack_40[22];
			if (message)
				snprintf(acStack_40, 22, "%02d %s", i, message);
			else if (i == 17 || i == 18)
				snprintf(acStack_40, 22, "show movie %d", i == 17 ? 1 : 2);
			else
				snprintf(acStack_40, 22, "%02d No Data            ", i);

			getDebugStageTextBox(unk3C, i)->setString(acStack_40);
		}
	}
#endif

#if SMS_DEBUG_STAGE_SELECT_PROPER_LEVELS_ONLY
	for (int i = SMS_DEBUG_STAGE_SELECT_PROPER_LEVEL_COUNT; i < 19; ++i) {
		J2DTextBox* box = getDebugStageTextBox(unk3C, i);
		box->setString("");
		box->hide();
	}
	unk40->unk28 = SMS_DEBUG_STAGE_SELECT_PROPER_LEVEL_COUNT;
#endif

	for (int i = 0; i < 20; ++i) {
		int code            = i < 9 ? 'st_1' + i : 'st_a' + i - 9;
		J2DTextBox* textBox = (J2DTextBox*)unk3C->search(code);
		SMSMakeTextBuffer(textBox, 22);
	}

	JDrama::TDStageDisp* stageDisp = new JDrama::TDStageDisp;
	unk14->getChildren().push_back(stageDisp);

	JDrama::TRect rect(0, 0, SMSGetTitleRenderWidth(),
	                   SMSGetTitleRenderHeight());
	stageDisp->getEfbCtrlDisp()->TEfbCtrl::setSrcRect(rect);

	JDrama::TOrthoProj* camera
	    = new JDrama::TOrthoProj(0.0f, 16.0f, 600.0f, 464.0f);
	group2d->getChildren().push_back(camera);

	JDrama::TScreen* screen = new JDrama::TScreen(rect, "Screen 2D");
	stageDisp->getUnk14()->getChildren().push_back(screen);
	screen->assignCamera(camera);
	screen->assignViewObj(group2d);

	unk40->show();

	return 0;
}

int TMenuDirector::direct()
{
	if (!unk50) {
		if (!OSIsThreadTerminated(&gSetupThread))
			return 0;
		void* res;
		OSJoinThread(&gSetupThread, &res);
		gpApplication.mFader->startFadeinT(0.25f);
		if (!TFlagManager::getInstance()->getBool(0x30007)) {
			TFlagManager::getInstance()->setBool(true, 0x30007);
			gpMSound->loadWave(MS_WAVE_DEFAULT);
		}
		unk50 = true;
	}

	u32 uVar13 = TApplication::APP_STATE_DEFAULT;

	JDrama::TDirector::direct();

	switch (unk18) {
	case 0:
		if (unk40->checkFlag(0x1)) {
			if (!(unk2C->getButton() & JUTGamePad::X)) {
				TFlagManager::getInstance()->firstStart();
				for (u8 i = 0; i < 30; ++i)
					TFlagManager::getInstance()->setShineFlag(i);
				for (u32 i = 0x10366; i < 0x103B4; ++i)
					TFlagManager::getInstance()->setBool(true, i);
				TFlagManager::getInstance()->saveSuccess();
			}

			unk40->fade();

			unk38->setString(unk40->unk30[unk40->unk2C]->getStringPtr());

#if SMS_DEBUG_STAGE_SELECT_ENGLISH
			if (unk40->unk2C == 6) {
				for (int i = 0; i < 10; ++i) {
					int code = i + 'st_1';
					if (i == 9)
						code = 'st_a';
					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);
					if (i < 6)
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_BEACH, i);
					else
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_HOTEL, i - 6);
				}

				snprintf(((J2DTextBox*)unk3C->search('st_f'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_BEACH, 6);
				snprintf(((J2DTextBox*)unk3C->search('st_g'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_BEACH, 7);
				snprintf(((J2DTextBox*)unk3C->search('st_h'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_HOTEL, 4);
				snprintf(((J2DTextBox*)unk3C->search('st_i'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_CASINO, 0);
				snprintf(((J2DTextBox*)unk3C->search('st_j'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_CASINO, 1);
				snprintf(((J2DTextBox*)unk3C->search('st_k'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_BOSS);
			} else if (unk40->unk2C == 5) {
				for (int i = 0; i < 10; ++i) {
					int code = i + 'st_1';
					if (i == 9)
						code = 'st_a';
					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);
					if (i < 4)
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_BEACHSIDE, i);
					else
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_PINNA_PARK,
						         i - 4);
				}

				snprintf(((J2DTextBox*)unk3C->search('st_b'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_PINNA_PARK, 6);
				snprintf(((J2DTextBox*)unk3C->search('st_c'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_PINNA_PARK, 7);
				snprintf(((J2DTextBox*)unk3C->search('st_d'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_BEACHSIDE, 4);
				snprintf(((J2DTextBox*)unk3C->search('st_h'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_BOSS_NUM, 0);
				snprintf(((J2DTextBox*)unk3C->search('st_i'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_BOSS_NUM, 1);
				snprintf(((J2DTextBox*)unk3C->search('st_j'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_DEMO, 0);
				snprintf(((J2DTextBox*)unk3C->search('st_k'))->getStringPtr(),
				         22, SMS_DEBUG_MENU_DEMO, 1);
			} else if (unk40->unk2C == 8) {
				for (int i = 0; i < 10; ++i) {
					int code = i + 'st_1';
					if (i == 9)
						code = 'st_a';
					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);
					if (i < 8)
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_NOKI, i);
					if (i == 8)
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_UNDERSEA);
					if (i == 9)
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_BOSS);
				}
			} else if (unk40->unk2C == 0x11 || unk40->unk2C == 0x12) {
				for (int i = 0; i < 20; ++i) {
					int code;
					if (i < 9)
						code = i + 'st_1';
					else
						code = i - 9 + 'st_a';

					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);

					int movie = i;

					if (unk40->unk2C == 0x12)
						movie += 20;

					const char* movieName
					    = TMovieDirector::getStreamMovieName(movie);
					if (movieName) {
						snprintf(box->getStringPtr(), 22, "%s", movieName);
						char* it = strrchr(box->getStringPtr(), '.');
						if (it)
							*it = '\0';
					} else {
						snprintf(box->getStringPtr(), 22, "%02d not found",
						         movie);
					}
				}
			} else {
				for (int i = 0; i < 20; ++i) {
					int code;
					if (i < 9)
						code = i + 'st_1';
					else
						code = i - 9 + 'st_a';

					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);

					if (i < 10)
						snprintf(box->getStringPtr(), 22,
						         SMS_DEBUG_MENU_SCENE, i, i);
					else
						snprintf(box->getStringPtr(), 22, "%02d EX %d", i,
						         i - 10);
				}
			}
#else
			if (unk40->unk2C == 6) {
				for (int i = 0; i < 10; ++i) {
					int code = i + 'st_1';
					if (i == 9)
						code = 'st_a';
					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);
					if (i < 6)
						snprintf(box->getStringPtr(), 22, "ビーチ %d", i);
					else
						snprintf(box->getStringPtr(), 22, "ホテル %d", i - 6);
				}

				snprintf(((J2DTextBox*)unk3C->search('st_f'))->getStringPtr(),
				         22, "ビーチ 6");
				snprintf(((J2DTextBox*)unk3C->search('st_g'))->getStringPtr(),
				         22, "ビーチ 7");
				snprintf(((J2DTextBox*)unk3C->search('st_h'))->getStringPtr(),
				         22, "ホテル 4");
				snprintf(((J2DTextBox*)unk3C->search('st_i'))->getStringPtr(),
				         22, "カジノ 0");
				snprintf(((J2DTextBox*)unk3C->search('st_j'))->getStringPtr(),
				         22, "カジノ 1");
				snprintf(((J2DTextBox*)unk3C->search('st_k'))->getStringPtr(),
				         22, "ボス");
			} else if (unk40->unk2C == 5) {
				for (int i = 0; i < 10; ++i) {
					int code = i + 'st_1';
					if (i == 9)
						code = 'st_a';
					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);
					if (i < 4)
						snprintf(box->getStringPtr(), 22, "ビーチサイド %d", i);
					else
						snprintf(box->getStringPtr(), 22, "ピンナパーコ %d",
						         i - 4);
				}

				snprintf(((J2DTextBox*)unk3C->search('st_b'))->getStringPtr(),
				         22, "ピンナパーコ 6");
				snprintf(((J2DTextBox*)unk3C->search('st_c'))->getStringPtr(),
				         22, "ピンナパーコ 7");
				snprintf(((J2DTextBox*)unk3C->search('st_d'))->getStringPtr(),
				         22, "ビーチサイド 4");
				snprintf(((J2DTextBox*)unk3C->search('st_h'))->getStringPtr(),
				         22, "ボス 0");
				snprintf(((J2DTextBox*)unk3C->search('st_i'))->getStringPtr(),
				         22, "ボス 1");
				snprintf(((J2DTextBox*)unk3C->search('st_j'))->getStringPtr(),
				         22, "デモ 0");
				snprintf(((J2DTextBox*)unk3C->search('st_k'))->getStringPtr(),
				         22, "デモ 1");
			} else if (unk40->unk2C == 8) {
				for (int i = 0; i < 10; ++i) {
					int code = i + 'st_1';
					if (i == 9)
						code = 'st_a';
					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);
					if (i < 8)
						snprintf(box->getStringPtr(), 22, "マーレ %d", i);
					if (i == 8)
						snprintf(box->getStringPtr(), 22, "カイテイ");
					if (i == 9)
						snprintf(box->getStringPtr(), 22, "ボス");
				}
			} else if (unk40->unk2C == 0x11 || unk40->unk2C == 0x12) {
				for (int i = 0; i < 20; ++i) {
					int code;
					if (i < 9)
						code = i + 'st_1';
					else
						code = i - 9 + 'st_a';

					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);

					int movie = i;

					if (unk40->unk2C == 0x12)
						movie += 20;

					const char* movieName
					    = TMovieDirector::getStreamMovieName(movie);
					if (movieName) {
						snprintf(box->getStringPtr(), 22, "%s", movieName);
						char* it = strrchr(box->getStringPtr(), '.');
						if (it)
							*it = '\0';
					} else {
						snprintf(box->getStringPtr(), 22, "%02d not found",
						         movie);
					}
				}
			} else {
				for (int i = 0; i < 20; ++i) {
					int code;
					if (i < 9)
						code = i + 'st_1';
					else
						code = i - 9 + 'st_a';

					J2DTextBox* box = (J2DTextBox*)unk3C->search(code);

					if (i < 10)
						snprintf(box->getStringPtr(), 22, "%02d シーン %d", i,
						         i);
					else
						snprintf(box->getStringPtr(), 22, "%02d EX %d", i,
						         i - 10);
				}
			}
#endif

#if SMS_DEBUG_STAGE_SELECT_PROPER_LEVELS_ONLY
			applyDebugScenarioMenu(unk3C, unk44, unk40->unk2C);
#endif

			unk38->show();
			unk44->show();
			unk18 = 1;
		}
		break;

	case 1:
		if (unk44->checkFlag(0x1)) {
			setFixedStageValue();
			unk18 = 2;
			gpApplication.mFader->startFadeoutT(0.25f);
			TGameSequence nextArea;
			nextArea.set(unk48, unk4C, 0);
			gpApplication.setNextArea(nextArea);
		} else if (unk44->checkFlag(0x2)) {
			unk18 = 0;
			unk40->unfade();
			unk38->hide();
			unk44->hide();
		}
		break;

	case 2:
		if (gpApplication.mFader->isFullyFadedOut()
		    && gpMSound->checkWaveOnAram(MS_WAVE_DEFAULT)) {
			if (unk40->unk2C == 0x11 || unk40->unk2C == 0x12)
				uVar13 = TApplication::APP_STATE_MOVIE;
			else
				uVar13 = TApplication::APP_STATE_GAMEPLAY;
		}
		break;

	case 3:
		if (gpApplication.mFader->isFullyFadedOut())
			uVar13 = TApplication::APP_STATE_QUIT;
		break;
	}

	return uVar13;
}

void TMenuDirector::setFixedStageValue()
{
#if SMS_DEBUG_STAGE_SELECT_PROPER_LEVELS_ONLY
	const TDebugScenarioMenu& scenarioMenu
	    = sDebugScenarioMenus[unk40->unk2C];
	const TDebugScenarioEntry& entry = scenarioMenu.mEntries[unk44->unk2C];
	unk48                           = entry.mStage;
	unk4C                           = entry.mScenario;
	return;
#endif

	unk48 = unk40->unk2C;
	unk4C = unk44->unk2C;
	int local_30[]
	    = { 0, 0x14, 0x1c, 0x1e, 0x20, 0x22, 0x28, 0, 0x2a, 0x2c, 0x2e };

	if ((unk48 == 0x11) || (unk48 == 0x12)) {
		int movie = unk4C;
		if (unk48 == 0x12)
			movie += 0x14;
		gpApplication.mMovie = movie;

		unk48 = 0xf;
		unk4C = 0;
	} else {
		if (unk48 >= 7)
			unk48 += 1;

		if (unk48 == 3) {
			if (unk4C == 0x13) {
				unk48 = 0x3b;
				unk4C = 0;
			} else if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			}
		} else if (unk48 == 9) {
			if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			} else if (unk4C == 8) {
				unk48 = 0x10;
				unk4C = 0;
			} else if (unk4C == 9) {
				unk48 = 0x39;
				unk4C = 0;
			}
		} else if (unk48 == 6) {
			if (unk4C >= 0x13) {
				unk48 = 0x38;
				unk4C = 0;
			} else if (unk4C >= 0x11) {
				unk48 = 0xe;
				unk4C = unk4C - 0x11;
			} else if (unk4C >= 0x10) {
				unk48 = 7;
				unk4C = 4;
			} else if (unk4C >= 0xe) {
				unk48 = 6;
				unk4C = unk4C - 8;
			} else if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			} else if (unk4C >= 6) {
				unk48 = 7;
				unk4C = unk4C - 6;
			}
		} else if (unk48 == 5) {
			if (unk4C == 0x12) {
				unk48 = 0xd;
				unk4C = 9;
			} else if (unk4C == 0x13) {
				unk48 = 0xd;
				unk4C = 10;
			} else if (unk4C >= 4 && unk4C <= 0xb) {
				unk48 = 0xd;
				unk4C = unk4C - 4;
			} else if (unk4C == 0xc) {
				unk48 = 5;
				unk4C = 4;
			} else if (unk4C == 0x11) {
				unk48 = 0x3a;
				unk4C = 1;
			} else if (unk4C == 0x10) {
				unk48 = 0x3a;
				unk4C = 0;
			} else if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			}
		} else if (unk48 == 10) {
			if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			} else if (unk4C == 7) {
				unk48 = 0x3c;
				unk4C = 0;
			}
		} else if (unk48 == 2) {
			if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			} else if (unk4C == 8) {
				unk48 = 0x37;
				unk4C = 0;
			}
		} else if (unk48 != 0) {
			if (unk4C >= 10) {
				unk48 = unk4C - 10 + local_30[unk48];
				unk4C = 0;
			}
		}
	}
}
