#ifndef MOVE_BG_MAP_OBJ_SIRENA_HPP
#define MOVE_BG_MAP_OBJ_SIRENA_HPP

#include <MoveBG/MapObjBase.hpp>
#include <Strategic/HitActor.hpp>

class TMultiBtk;

class TWarpAreaActor : public THitActor {
public:
	TWarpAreaActor(const char*);

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

public:
	/* 0x68 */ s16 unk68;
	/* 0x6A */ s16 unk6A;
};

class TSirenaCasinoRoof : public TMapObjBase {
public:
	TSirenaCasinoRoof(const char* name)
	    : TMapObjBase(name)
	    , unk138(nullptr)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual u32 getSDLModelFlag() const;
	virtual void initMapObj();

public:
	/* 0x138 */ TMultiBtk* unk138;
};

class TSirenabossWall : public TMapObjBase {
public:
	TSirenabossWall(const char* name)
	    : TMapObjBase(name)
	    , unk138(nullptr)
	{
	}

	virtual void perform(u32, JDrama::TGraphics*);
	virtual void drawObject(JDrama::TGraphics*);
	virtual u32 getSDLModelFlag() const;
	virtual void initMapObj();

public:
	/* 0x138 */ TMultiBtk* unk138;
};

class TMapCollisionWarp;

class TSakuCasino : public TMapObjBase {
public:
	TSakuCasino(const char*);

	virtual void loadAfter();
	virtual void calcRootMatrix();
	virtual void initMapObj();

public:
	/* 0x138 */ TMapCollisionWarp* unk138;
	/* 0x13C */ u8 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ THitActor* unk144;
};

class TDonchou : public TMapObjBase {
public:
	TDonchou(const char*);

	virtual void loadAfter();
	virtual void calcRootMatrix();
	virtual void initMapObj();
	virtual u32 touchWater(THitActor*);

public:
	/* 0x138 */ TMapCollisionWarp* unk138;
	/* 0x13C */ u8 unk13C;
	/* 0x140 */ f32 unk140;
	/* 0x144 */ THitActor* unk144;
	/* 0x148 */ THitActor* unk148;
	/* 0x14C */ s32 unk14C;
};

#endif
