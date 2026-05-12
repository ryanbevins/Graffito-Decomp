#include <Animal/AnimalBase.hpp>
#include <Animal/Realoid.hpp>

class TButterfly : public TAnimalBase {
public:
	TButterfly(u32 type, const char* name);
	virtual BOOL receiveMessage(THitActor*, u32);
	virtual void init();
	virtual void load(JSUMemoryInputStream&);
};

class TButterfloid : public TRealoid {
public:
	TButterfloid(int, const char*);
	virtual TRealoidActor* createRealoidActor(MActor*);
	virtual void load(JSUMemoryInputStream&);
	virtual void init(class TLiveManager*);
};

class TButterfloidManager {
public:
	TButterfloidManager(const char*);
	~TButterfloidManager();
	void createModelData();
};

TButterfloidManager::TButterfloidManager(const char* name) { (void)name; }
TButterfloidManager::~TButterfloidManager() { }
void TButterfloidManager::createModelData() { }

TButterfloid::TButterfloid(int count, const char* name)
    : TRealoid(name)
{
	(void)count;
}

TRealoidActor* TButterfloid::createRealoidActor(MActor* actor)
{
	(void)actor;
	return nullptr;
}

void TButterfloid::load(JSUMemoryInputStream& stream) { (void)stream; }
void TButterfloid::init(TLiveManager* mgr) { (void)mgr; }

TButterfly::TButterfly(u32 type, const char* name)
    : TAnimalBase(type, name)
{
}

BOOL TButterfly::receiveMessage(THitActor* sender, u32 msg)
{
	(void)sender;
	(void)msg;
	return 0;
}

void TButterfly::init() { }
void TButterfly::load(JSUMemoryInputStream& stream) { (void)stream; }
