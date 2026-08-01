#ifndef JDR_VIEW_OBJ_HPP
#define JDR_VIEW_OBJ_HPP

#include <JSystem/JDrama/JDRFlag.hpp>
#include <JSystem/JDrama/JDRGraphics.hpp>
#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JGadget/std-list.hpp>

namespace JDrama {

class TViewObj : public TNameRef {
public:
#ifdef JDRAMA_NO_INLINE_VIEWOBJ_CTOR
	TViewObj(const char* name = "<TViewObj>");
#else
	TViewObj(const char* name = "<TViewObj>")
	    : TNameRef(name)
	    , unkC(0)
	{
	}
#endif

#ifdef JDRAMA_VIEWOBJ_DTOR_DECL_ONLY
	virtual ~TViewObj();
#endif

	void testPerform(u32, TGraphics*);

	virtual void perform(u32, TGraphics*) = 0;

public:
	/* 0xC */ TFlagT<u16> unkC;
};

#ifdef JDRAMA_NO_INLINE_VIEWOBJ_CTOR
TViewObj::TViewObj(const char* name)
    : TNameRef(name)
    , unkC(0)
{
}
#endif

}; // namespace JDrama

#endif
