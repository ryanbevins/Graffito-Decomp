#include <System/PerformList.hpp>
#include <System/MarDirector.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JSupport/JSURandomInputStream.hpp>
#include <dolphin/os.h>
#include <string.h>

static bool isPerformProbeName(const char* name)
{
	if (!name)
		return false;

	return strcmp(name, "ZBufferCatch") == 0
	       || strcmp(name, "<ZBufferCatch>") == 0
	       || strcmp(name, "AlphaCatch") == 0
	       || strcmp(name, "<AlphaCatch>") == 0
	       || strcmp(name, "J3DSysFlag") == 0
	       || strcmp(name, "<J3DSysFlag>") == 0
	       || strcmp(name, "SMSDrawInit") == 0
	       || strcmp(name, "<SMSDrawInit>") == 0
	       || strcmp(name, "J3DSysSetViewMtx") == 0
	       || strcmp(name, "<J3DSysSetViewMtx>") == 0;
}

void TPerformList::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	static s32 sPerformProbeLogCount;
	for (JGadget::TSingleLinkList<TPerformLink, 0>::iterator it
	     = getChildren().begin();
	     it != getChildren().end(); ++it) {
		u32 maskedFlags = param_1 & it->unk8;
		JDrama::TViewObj* obj = it->unk4;

		if (sPerformProbeLogCount < 512
		    && ((maskedFlags & 0x8) || isPerformProbeName(getName())
		        || isPerformProbeName(obj ? obj->getName() : 0))) {
			OSReport((char*)"PFLIST_PERFORM list=%s obj=%p objName=%s in=%08x filter=%08x masked=%08x\n",
			         getName(), obj, obj ? obj->getName() : "<null>", param_1,
			         it->unk8, maskedFlags);
			++sPerformProbeLogCount;
		}

		obj->testPerform(maskedFlags, param_2);
	}
}

void TPerformList::load(JSUMemoryInputStream& stream)
{
	JDrama::TViewObj::load(stream);

	JSURandomInputStream& randomStream = stream;
	while (randomStream.getLength() - randomStream.getPosition() > 0) {
		char acStack_6c[80];
		stream.readString(acStack_6c, 80);

		JDrama::TViewObj* obj
		    = JDrama::TNameRefGen::search<JDrama::TViewObj>(acStack_6c);
		u32 value;
		stream.read(&value, 4);
		u32 uVar5 = value;
		if (value & 1)
			uVar5 |= 0x3000;
		if (!obj || isPerformProbeName(acStack_6c)
		    || isPerformProbeName(obj->getName())) {
			OSReport((char*)"PFLIST_LOAD list=%s ref=%s obj=%p objName=%s flags=%08x\n",
			         getName(), acStack_6c, obj,
			         obj ? obj->getName() : "<null>", uVar5);
		}
		if (obj)
			getChildren().Push_back(new TPerformLink(obj, uVar5));
	}
}

void TPerformList::push_back(const char* param_1, u32 param_2)
{
	JDrama::TViewObj* obj
	    = JDrama::TNameRefGen::search<JDrama::TViewObj>(param_1);

	getChildren().Push_back(new TPerformLink(obj, param_2));
}

void TPerformList::push_back(JDrama::TViewObj* param_1, u32 param_2)
{
	getChildren().Push_back(new TPerformLink(param_1, param_2));
}
