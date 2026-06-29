#include <System/MarDirector.hpp>
#include <System/Application.hpp>
#include <System/MarioGamePad.hpp>
#include <System/DrawSyncManager.hpp>
#include <System/TalkCursor.hpp>
#include <System/MSoundMainSide.hpp>
#include <System/PerformList.hpp>
#include <System/StageEventInfo.hpp>
#include <Map/PollutionManager.hpp>
#include <Map/MapEventSink.hpp>
#include <Strategic/TakeActor.hpp>
#include <System/DrawSyncCallback.hpp>
#include <Camera/SunMgr.hpp>
#include <GC2D/GCConsole2.hpp>
#include <GC2D/ScrnFader.hpp>
#include <GC2D/PauseMenu2.hpp>
#include <GC2D/Guide.hpp>
#include <GC2D/CardLoad.hpp>
#include <GC2D/Talk2D2.hpp>
#include <GC2D/SunGlass.hpp>
#include <THPPlayer/THPPlayer.h>
#include <MSound/MSound.hpp>
#include <MoveBG/MapObjDolpic.hpp>
#include <JSystem/JKernel/JKRFileLoader.hpp>
#include <JSystem/JKernel/JKRMemArchive.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <dolphin/os.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

static void dummy(Vec* v)
{
	*v = (Vec) { 0.0f, 0.0f, 0.0f };
	*v = (Vec) { 1.0f, 1.0f, 1.0f };
}

class JPAEmitterManager;

class TMario : public TTakeActor, public TDrawSyncCallback {
public:
	TMario();
	void setGamePad(TMarioGamePad*);
};

extern JPAEmitterManager* gpEmitterManager4D2;
extern TMario* gpMarioOriginal;

void TMarDirector::setup2()
{
	OSReport((char*)"CODEX_SETUP2 start curr=%u:%u map=%u scenario=%u\n",
	         gpApplication.mCurrArea.getStage(),
	         gpApplication.mCurrArea.getScenario(), mMap, unk7D);
	OSReport((char*)"CODEX_SETUP2 event-table-begin\n");
	unkBC = JDrama::TNameRefGen::search<TNameRefAryT<TStageEventInfo> >(
	    "イベントテーブル");
	if (unkBC) {
		int i = 0;
		for (TStageEventInfo* it = unkBC->begin(); it != unkBC->end();
		     ++i, ++it) {
			TMapObjBase* ref
			    = JDrama::TNameRefGen::search<TMapObjBase>(it->unk14);
			if (ref) {
				ref->unk134 = (u16)i;
				it->unk28 = ref;
			}
		}
	}
	OSReport((char*)"CODEX_SETUP2 event-table-end table=%p\n", unkBC);

	OSReport((char*)"CODEX_SETUP2 mario-begin\n");
	JDrama::TNameRefGen::search<TMario>("マリオ")->setGamePad(unk18[0]);
	TMarioGamePad* gamePad = unk18[0];
	JDrama::TNameRefGen::search<CPolarSubCamera>("camera 1")->unk120 = gamePad;
	OSReport((char*)"CODEX_SETUP2 mario-end pad=%p\n", gamePad);

	OSReport((char*)"CODEX_SETUP2 ui-search-begin\n");
	unk84 = JDrama::TNameRefGen::search<TTalkCursor>("会話カーソル");

	mConsole = JDrama::TNameRefGen::search<TGCConsole2>("GCコンソール");

	mConsole->unkC = 0xB;

	unkDC = JDrama::TNameRefGen::search<TShineFader>("シャインフェーダー");

	unkDC->mRate = 120.0f;
	unkDC->setColor(JUtility::TColor(0xD2, 0xD2, 0xD2, 0xFF));

	unkE0 = JDrama::TNameRefGen::search<TSunGlass>("サングラスフェーダ");
	unk78 = JDrama::TNameRefGen::search<TGuide>("ガイド画面");
	unkAC = JDrama::TNameRefGen::search<TPauseMenu2>("ポーズメニュー");
	unkAC->unk10C = unk18[0];
	unkB0 = JDrama::TNameRefGen::search<TTalk2D2>("会話表示");
	unkB0->unk24C = (u32)unk18[0];
	unk70 = JDrama::TNameRefGen::search<TCardLoad>("データロード");
	OSReport((char*)"CODEX_SETUP2 ui-search-end cursor=%p console=%p shine=%p guide=%p pause=%p talk=%p card=%p\n",
	         unk84, mConsole, unkDC, unk78, unkAC, unkB0, unk70);

	unk70->unk38 = unk18[0];
	unk78->unkC0 = unk18[0];

	unk18[0]->mFlags = 0;
	if (mMap == 15) {
		unkAC->unkC = 0xB;
		unkB0->unkC = 0xB;
		unk18[0]->onFlag(0x20);
	} else {
		unk70->unkC = 0xB;
	}
	OSReport((char*)"CODEX_SETUP2 ui-config-end flags=%04x\n",
	         unk18[0]->mFlags);

	OSReport((char*)"CODEX_SETUP2 demo-cannon-begin\n");
	unk254 = JDrama::TNameRefGen::search<TDemoCannon>("デモ砲台");
	OSReport((char*)"CODEX_SETUP2 demo-cannon-end ptr=%p\n", unk254);

	OSReport((char*)"CODEX_SETUP2 drawsync-begin sun=%p pollution=%p mario=%p\n",
	         gpSunMgr, gpPollution, gpMarioOriginal);
	TDrawSyncManager::smInstance->setCallback(1, 0x7D, 0x7D, gpSunMgr);
	TDrawSyncManager::smInstance->setCallback(2, 0x7E, 0x91,
	                                          &gpPollution->getCounterLayer());
	TDrawSyncManager::smInstance->setCallback(3, 0x92, 0xA5,
	                                          &gpPollution->getCounterObj());
	TDrawSyncManager::smInstance->setCallback(4, 0x7C, 0x7C, gpMarioOriginal);
	OSReport((char*)"CODEX_SETUP2 drawsync-end\n");

	OSReport((char*)"CODEX_SETUP2 sound-stage-begin camera=%p msound=%p\n",
	         gpCamera, gpMSound);
	gpMSound->setCameraInfo(&gpCamera->unk124, gpCamera->unk13C,
	                        gpCamera->unk1EC, 0);

	u8 stageScenario = unk7D;
	u8 stageArea     = mMap;
	unk258           = MSStage::init(stageArea, stageScenario);
	OSReport((char*)"CODEX_SETUP2 sound-stage-end stage=%u scenario=%u ms=%p\n",
	         stageArea, stageScenario, unk258);

	OSReport((char*)"CODEX_SETUP2 perform-begin list40=%p list38=%p\n",
	         unk40, unk38);
	JDrama::TGraphics graphics;
	unk40->perform(0xffffffff, &graphics);
	OSReport((char*)"CODEX_SETUP2 perform-mid list40-done\n");
	unk38->perform(0xffffffff, &graphics);
	OSReport((char*)"CODEX_SETUP2 perform-end list38-done\n");
	OSReport((char*)"CODEX_SETUP2 gxsetdone-begin\n");
	GXSetDrawDone();
	OSReport((char*)"CODEX_SETUP2 gxsetdone-end\n");
	OSReport((char*)"CODEX_SETUP2 gxwait-begin\n");
	GXWaitDrawDone();
	OSReport((char*)"CODEX_SETUP2 gxwait-end\n");

	TMapEventSinkInPollution* sinkInPollutionEvent;

	OSReport((char*)"CODEX_SETUP2 sink-search-1-begin\n");
	sinkInPollutionEvent
	    = JDrama::TNameRefGen::search<TMapEventSinkInPollution>(
	        "イベント（地形沈む）");
	OSReport((char*)"CODEX_SETUP2 sink-search-1-end ptr=%p\n",
	         sinkInPollutionEvent);

	if (!sinkInPollutionEvent) {
		OSReport((char*)"CODEX_SETUP2 sink-search-2-begin\n");
		sinkInPollutionEvent
		    = JDrama::TNameRefGen::search<TMapEventSinkInPollution>(
		        "イベント（地形沈む再汚染）");
		OSReport((char*)"CODEX_SETUP2 sink-search-2-end ptr=%p\n",
		         sinkInPollutionEvent);
		if (!sinkInPollutionEvent) {
			OSReport((char*)"CODEX_SETUP2 sink-search-3-begin\n");
			sinkInPollutionEvent
			    = JDrama::TNameRefGen::search<TMapEventSinkInPollution>(
			        "イベント（地形沈むビアンコ）");
			OSReport((char*)"CODEX_SETUP2 sink-search-3-end ptr=%p\n",
			         sinkInPollutionEvent);
		}
	}

	if (sinkInPollutionEvent) {
		OSReport((char*)"CODEX_SETUP2 sink-init-begin ptr=%p\n",
		         sinkInPollutionEvent);
		sinkInPollutionEvent->initBuriedBuilding();
		OSReport((char*)"CODEX_SETUP2 sink-init-end\n");
	}
	OSReport((char*)"CODEX_SETUP2 end\n");
}

TMarDirector::~TMarDirector()
{
	gpMSound->exitStage();
	if (gpApplication.mCurrArea.unk0 == 15) {
		if (JKRMemArchive* arch
		    = (JKRMemArchive*)JKRFileLoader::getVolume("option"))
			arch->unmountFixed();
	}

	if (JKRMemArchive* arch
	    = (JKRMemArchive*)JKRFileLoader::getVolume("game_6"))
		arch->unmountFixed();

	if (JKRMemArchive* arch = (JKRMemArchive*)JKRFileLoader::getVolume("guide"))
		arch->unmountFixed();

	if (JKRMemArchive* arch = (JKRMemArchive*)JKRFileLoader::getVolume("yoshi"))
		arch->unmountFixed();

	if (JKRMemArchive* arch = (JKRMemArchive*)JKRFileLoader::getVolume("scene"))
		arch->unmountFixed();

	unk18[0]->offFlag(0x20);
	if (mMap == 1 || (mMap == 0 && unk7D == 0)) {
		THPPlayerStop();
		THPPlayerClose();
		THPPlayerQuit();
	}

	TDrawSyncManager::smInstance->setCallback(1, 0, 0, nullptr);
	TDrawSyncManager::smInstance->setCallback(2, 0, 0, nullptr);
	TDrawSyncManager::smInstance->setCallback(3, 0, 0, nullptr);
	TDrawSyncManager::smInstance->setCallback(4, 0, 0, nullptr);
	gpEmitterManager4D2           = nullptr;
	JDrama::TNameRefGen::instance = nullptr;
	gpMarDirector                 = nullptr;
}
