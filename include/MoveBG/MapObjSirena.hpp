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

#endif
