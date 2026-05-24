#include <System/MarNameRefGen.hpp>
#include <string.h>
#include <Enemy/BathtubKiller.hpp>
#include <Enemy/BossGesso.hpp>
#include <Enemy/CoasterKiller.hpp>
#include <Enemy/DemoBossHanachan.hpp>
#include <Enemy/Enemy.hpp>
#include <Enemy/EnemyManager.hpp>
#include <Enemy/Hinokuri2.hpp>
#include <Enemy/SleepBossHanachan.hpp>
// Rogue includes for static init (matches original sinit block)
#include <MSound/MSoundBGM.hpp>
#include <MSound/MSSetSound.hpp>
#include <M3DUtil/InfectiousStrings.hpp>

// Forward declarations for classes whose headers don't exist yet.
// Sizes are pinned to match what the original asm passed to operator new.
class TBEelTears : public TSpineEnemy {
public:
	TBEelTears(const char* name) : TSpineEnemy(name) {}
	virtual ~TBEelTears() {}
};

class TBEelTearsManager : public JDrama::TNameRef {
public:
	TBEelTearsManager(const char*);
	char _stub[0xc4];
};

class TBathtubPeach : public JDrama::TNameRef {
public:
	TBathtubPeach(const char*);
	char _stub[0x16c];
};

class TBathtubPeachManager : public JDrama::TNameRef {
public:
	TBathtubPeachManager(const char*);
	char _stub[0x4c];
};

class TBossEel : public JDrama::TNameRef {
public:
	TBossEel(const char*);
	char _stub[0x218];
};

class TBossEelManager : public TEnemyManager {
public:
	TBossEelManager(const char* name) : TEnemyManager(name) {}
};

class TBossHanachan : public JDrama::TNameRef {
public:
	TBossHanachan(const char*);
	char _stub[0x1bc];
};

class TBossHanachanManager : public JDrama::TNameRef {
public:
	TBossHanachanManager(const char*);
	char _stub[0x5c];
};

class TBossManta : public JDrama::TNameRef {
public:
	TBossManta(const char*);
	char _stub[0x1a0];
};

class TBossMantaManager : public JDrama::TNameRef {
public:
	TBossMantaManager(const char*);
	char _stub[0x90];
};

class TBossPakkun : public JDrama::TNameRef {
public:
	TBossPakkun(const char*);
	char _stub[0x1c8];
};

class TBossPakkunManager : public JDrama::TNameRef {
public:
	TBossPakkunManager(const char*, int);
	char _stub[0x50];
};

class TBossTelesa : public JDrama::TNameRef {
public:
	TBossTelesa(const char*);
	char _stub[0x384];
};

class TBossTelesaManager : public JDrama::TNameRef {
public:
	TBossTelesaManager(const char*);
	char _stub[0x4c];
};

class TBossWanwan : public JDrama::TNameRef {
public:
	TBossWanwan(const char*);
	char _stub[0x1b0];
};

class TBossWanwanManager : public JDrama::TNameRef {
public:
	TBossWanwanManager(const char*);
	char _stub[0x4c];
};

class TBubbleManager : public JDrama::TNameRef {
public:
	TBubbleManager(const char*);
	char _stub[0x58];
};

class TEMario : public JDrama::TNameRef {
public:
	TEMario(const char*);
	char _stub[0x15c];
};

class TEMarioManager : public JDrama::TNameRef {
public:
	TEMarioManager(const char*);
	char _stub[0x4c];
};

class TKoopa : public JDrama::TNameRef {
public:
	TKoopa(const char*);
	char _stub[0x1b4];
};

class TKoopaJr : public JDrama::TNameRef {
public:
	TKoopaJr(const char*);
	char _stub[0x168];
};

class TKoopaJrManager : public JDrama::TNameRef {
public:
	TKoopaJrManager(const char*);
	char _stub[0x4c];
};

class TKoopaJrSubmarine : public JDrama::TNameRef {
public:
	TKoopaJrSubmarine(const char*);
	char _stub[0x1a4];
};

class TKoopaJrSubmarineManager : public JDrama::TNameRef {
public:
	TKoopaJrSubmarineManager(const char*);
	char _stub[0x4c];
};

class TKoopaManager : public JDrama::TNameRef {
public:
	TKoopaManager(const char*);
	char _stub[0x4c];
};

class TLimitKoopa : public JDrama::TNameRef {
public:
	TLimitKoopa(const char*);
	char _stub[0x1c0];
};

class TLimitKoopaJr : public JDrama::TNameRef {
public:
	TLimitKoopaJr(const char*);
	char _stub[0x174];
};

class TLimitKoopaJrManager : public JDrama::TNameRef {
public:
	TLimitKoopaJrManager(const char*);
	char _stub[0x4c];
};

class TLimitKoopaManager : public JDrama::TNameRef {
public:
	TLimitKoopaManager(const char*);
	char _stub[0x4c];
};

class TOilBall : public JDrama::TNameRef {
public:
	TOilBall(const char*);
	char _stub[0x168];
};

class TTinKoopa : public JDrama::TNameRef {
public:
	TTinKoopa(const char*);
	char _stub[0x1f4];
};

class TTinKoopaManager : public JDrama::TNameRef {
public:
	TTinKoopaManager(const char*);
	char _stub[0x4c];
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
		return new TBossPakkunManager("ボスパックン軽マネージャ", 0);
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
