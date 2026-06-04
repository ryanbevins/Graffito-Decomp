#include <System/MarNameRefGen.hpp>
#include <string.h>
#include <Enemy/BathtubKiller.hpp>
#include <Enemy/BathtubPeach.hpp>
#include <Enemy/BossEel.hpp>
#include <Enemy/BossHanachan.hpp>
#include <Enemy/BossManta.hpp>
#include <Enemy/BossPakkun.hpp>
#include <Enemy/BossGesso.hpp>
#include <Enemy/CoasterKiller.hpp>
#include <Enemy/DemoBossHanachan.hpp>
#include <Enemy/EMario.hpp>
#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/Hinokuri2.hpp>
#include <Enemy/LimitKoopa.hpp>
#include <Enemy/LimitKoopaJr.hpp>
#include <Enemy/SleepBossHanachan.hpp>
// Rogue includes for static init (matches original sinit block)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>

static const char* dummyMactorStringValue1 = "\0\0\0\0\0\0\0\0\0\0\0";
static const char* SMS_NO_MEMORY_MESSAGE   = "メモリが足りません\n";
static const char cDirtyFileName[] = "/scene/map/pollution/H_ma_rak.bti";
static const char cDirtyTexName[]  = "H_ma_rak_dummy";
static const char* MtxCalcTypeName[] = {
	"MActorMtxCalcType_Basic クラシックスケールＯＮ",
	"MActorMtxCalcType_Softimage クラシックスケールＯＦＦ",
	"MActorMtxCalcType_MotionBlend モーションブレンド",
	"MActorMtxCalcType_User ユーザー定義",
};

class TBossTelesa : public JDrama::TNameRef {
public:
	TBossTelesa(const char*);
	char _stub[0x380];
};

class TBossTelesaManager : public JDrama::TNameRef {
public:
	TBossTelesaManager(const char*);
	char _stub[0x48];
};

class TBossWanwan : public JDrama::TNameRef {
public:
	TBossWanwan(const char*);
	char _stub[0x1ac];
};

class TBossWanwanManager : public JDrama::TNameRef {
public:
	TBossWanwanManager(const char*);
	char _stub[0x48];
};

class TBubbleManager : public JDrama::TNameRef {
public:
	TBubbleManager(const char*);
	char _stub[0x54];
};

class TKoopa : public JDrama::TNameRef {
public:
	TKoopa(const char*);
	char _stub[0x1b0];
};

class TKoopaJr : public JDrama::TNameRef {
public:
	TKoopaJr(const char*);
	char _stub[0x164];
};

class TKoopaJrManager : public JDrama::TNameRef {
public:
	TKoopaJrManager(const char*);
	char _stub[0x48];
};

class TKoopaJrSubmarine : public JDrama::TNameRef {
public:
	TKoopaJrSubmarine(const char*);
	char _stub[0x1a0];
};

class TKoopaJrSubmarineManager : public JDrama::TNameRef {
public:
	TKoopaJrSubmarineManager(const char*);
	char _stub[0x48];
};

class TKoopaManager : public JDrama::TNameRef {
public:
	TKoopaManager(const char*);
	char _stub[0x48];
};

class TTinKoopa : public JDrama::TNameRef {
public:
	TTinKoopa(const char*);
	char _stub[0x1f0];
};

class TTinKoopaManager : public JDrama::TNameRef {
public:
	TTinKoopaManager(const char*);
	char _stub[0x48];
};

JDrama::TNameRef* TMarNameRefGen::getNameRef_BossEnemy(const char* name) const
{
	if (strcmp(name, "EMario") == 0)
		return new TEMario("マリオモドキ");
	if (strcmp(name, "EMarioManager") == 0)
		return new TEMarioManager("典型敵マネージャ");
	if (strcmp(name, "BossHanachan") == 0)
		return new TBossHanachan("?");
	if (strcmp(name, "BossHanachanManager") == 0)
		return new TBossHanachanManager("?");
	if (strcmp(name, "SleepBossHanachan") == 0)
		return new TSleepBossHanachan("?");
	if (strcmp(name, "SleepBossHanachanManager") == 0)
		return new TSleepBossHanachanManager("?");
	if (strcmp(name, "BossEel") == 0)
		return new TBossEel("?");
	if (strcmp(name, "BossEelManager") == 0)
		return new TBossEelManager("?");
	if (strcmp(name, "BEelTearsManager") == 0)
		return new TBEelTearsManager("めおとウナギ涙マネージャー");
	if (strcmp(name, "Koopa") == 0)
		return new TKoopa("クッパ");
	if (strcmp(name, "KoopaManager") == 0)
		return new TKoopaManager("クッパマネージャー");
	if (strcmp(name, "HinoKuri2") == 0)
		return new THinokuri2("ヒノクリ２");
	if (strcmp(name, "HinoKuri2Manager") == 0)
		return new THinokuri2Manager("ヒノクリ２マネージャ");
	if (strcmp(name, "BossGesso") == 0)
		return new TBossGesso("ボスゲッソー");
	if (strcmp(name, "BossGessoManager") == 0)
		return new TBossGessoManager("ボスゲッソーマネージャ");
	if (strcmp(name, "TinKoopa") == 0)
		return new TTinKoopa("メカクッパ");
	if (strcmp(name, "TinKoopaManager") == 0)
		return new TTinKoopaManager("メカクッパマネージャ");
	if (strcmp(name, "CoasterKillerManager") == 0)
		return new TCoasterKillerManager("コースターキラーマネージャー");
	if (strcmp(name, "CoasterKiller") == 0)
		return new TCoasterKiller("コースターキラー");
	if (strcmp(name, "KoopaJrManager") == 0)
		return new TKoopaJrManager("クッパジュニアマネージャー");
	if (strcmp(name, "KoopaJr") == 0)
		return new TKoopaJr("クッパジュニア");
	if (strcmp(name, "KoopaJrSubmarineManager") == 0)
		return new TKoopaJrSubmarineManager("クッパジュニアサブマリンマネージャー");
	if (strcmp(name, "KoopaJrSubmarine") == 0)
		return new TKoopaJrSubmarine("クッパジュニアサブマリン");
	if (strcmp(name, "LimitKoopaJrManager") == 0)
		return new TLimitKoopaJrManager("リミットクッパジュニアマネージャー");
	if (strcmp(name, "LimitKoopaJr") == 0)
		return new TLimitKoopaJr("リミットクッパジュニア");
	if (strcmp(name, "LimitKoopaManager") == 0)
		return new TLimitKoopaManager("クッパマネージャー");
	if (strcmp(name, "LimitKoopa") == 0)
		return new TLimitKoopa("クッパ");
	if (strcmp(name, "BathtubKillerManager") == 0)
		return new TBathtubKillerManager("バスタブキラーマネージャー");
	if (strcmp(name, "BathtubKiller") == 0)
		return new TBathtubKiller("バスタブキラー");
	if (strcmp(name, "BathtubPeachManager") == 0)
		return new TBathtubPeachManager("バスタブピーチマネージャー");
	if (strcmp(name, "BathtubPeach") == 0)
		return new TBathtubPeach("バスタブピーチ");
	if (strcmp(name, "BossWanwan") == 0)
		return new TBossWanwan("ボスワンワン");
	if (strcmp(name, "BossWanwanManager") == 0)
		return new TBossWanwanManager("ボスワンワンマネージャ");
	if (strcmp(name, "BossPakkun") == 0)
		return new TBossPakkun("ボスパックン改");
	if (strcmp(name, "KBossPakkun") == 0)
		return new TBossPakkun("ボスパックン軽");
	if (strcmp(name, "BossPakkunManager") == 0)
		return new TBossPakkunManager("ボスパックンマネージャー", 0);
	if (strcmp(name, "KBossPakkunManager") == 0)
		return new TBossPakkunManager("ボスパックン軽マネージャ", 1);
	if (strcmp(name, "BossTelesa") == 0)
		return new TBossTelesa("ボステレサ");
	if (strcmp(name, "BossTelesaManager") == 0)
		return new TBossTelesaManager("ボステレサマネージャー");
	if (strcmp(name, "BubbleManager") == 0)
		return new TBubbleManager("バブルマネージャー");
	if (strcmp(name, "OilBall") == 0)
		return new TOilBall("油ダマ");
	if (strcmp(name, "BossManta") == 0)
		return new TBossManta("ボスマンタ");
	if (strcmp(name, "BossMantaManager") == 0)
		return new TBossMantaManager("ボスマンタマネージャ");
	return nullptr;
}
