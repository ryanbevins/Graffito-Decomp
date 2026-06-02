#ifndef SYSTEM_PERFORM_LIST_HPP
#define SYSTEM_PERFORM_LIST_HPP

#include <JSystem/JDrama/JDRViewObj.hpp>
#include <JSystem/JGadget/singlelinklist.hpp>

class TPerformLink {
public:
	TPerformLink(JDrama::TViewObj* param_1, u32 param_2)
	    : unk4(param_1)
	    , unk8(param_2)
	{
	}

public:
	/* 0x0 */ JGadget::TSingleLinkListNode unk0;
	/* 0x4 */ JDrama::TViewObj* unk4;
	/* 0x8 */ u32 unk8;
};

class TPerformList : public JDrama::TViewObj,
                     public JGadget::TSingleLinkList<TPerformLink, 0> {
public:
	TPerformList() { }
	TPerformList(const char* name)
	    : JDrama::TViewObj(name)
	{
	}

	virtual void load(JSUMemoryInputStream&);
	virtual void perform(u32, JDrama::TGraphics*);

	// Filters are &ed with the first param in perform
	void push_back(JDrama::TViewObj* object, u32 filter);
	void push_back(const char* name, u32 filter);

	void forEachPerform(JGadget::TSingleLinkList<TPerformLink, 0>::iterator b,
	                    JGadget::TSingleLinkList<TPerformLink, 0>::iterator e,
	                    JDrama::TGraphics* graphics, u32 flags)
	{
		for (JGadget::TSingleLinkList<TPerformLink, 0>::iterator it = b;
		     it != e; ++it) {
			it->unk4->testPerform(flags & it->unk8, graphics);
		}
	}

	JGadget::TSingleLinkList<TPerformLink, 0>& getChildren() { return *this; }
};

#endif
