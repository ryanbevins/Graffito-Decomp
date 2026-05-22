#include <Animal/Realoid.hpp>

class TFish {
public:
	void init();
};

class TFishoid : public TRealoid {
public:
	TFishoid(int, const char*);
	virtual TRealoidActor* createRealoidActor(MActor*);
	virtual void load(JSUMemoryInputStream&);
	virtual void init(class TLiveManager*);
	virtual void perform(u32, JDrama::TGraphics*);
};

class TFishoidManager {
public:
	TFishoidManager(const char*);
	~TFishoidManager();
	void createModelData();
};

TFishoidManager::TFishoidManager(const char* name) { (void)name; }
TFishoidManager::~TFishoidManager() { }
void TFishoidManager::createModelData() { }

TFishoid::TFishoid(int count, const char* name)
    : TRealoid(name)
{
	(void)count;
}

TRealoidActor* TFishoid::createRealoidActor(MActor* actor)
{
	(void)actor;
	return nullptr;
}

void TFishoid::load(JSUMemoryInputStream& stream) { (void)stream; }
void TFishoid::init(TLiveManager* mgr) { (void)mgr; }
void TFishoid::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

void TFish::init() { }

TRealoid::TRealoid(const char* name)
    : TSpineEnemy(name)
{
	mBoidLeader = nullptr;
	onLiveFlag(0x38);
}

void TRealoid::loadDefault(JSUMemoryInputStream& stream, const char* name,
                           int count)
{
	(void)stream;
	(void)name;
	(void)count;
}

TRealoidActor::TRealoidActor(MActor* actor)
    : TTakeActor("boid")
{
	(void)actor;
	mMActor = actor;
	unk74   = 0;
}

BOOL TRealoidActor::receiveMessage(THitActor* sender, u32 msg)
{
	(void)sender;
	(void)msg;
	return 0;
}

MtxPtr TRealoidActor::getTakingMtx() { return mTakingMtx; }
void TRealoidActor::init() { }
void TRealoidActor::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

void TRealoidActor::checkHitActors() { }
