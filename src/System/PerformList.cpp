#include <System/PerformList.hpp>
#include <System/MarDirector.hpp>
#include <JSystem/JDrama/JDRNameRefGen.hpp>
#include <JSystem/JSupport/JSURandomInputStream.hpp>

void TPerformList::perform(u32 param_1, JDrama::TGraphics* param_2)
{
	for (JGadget::TSingleLinkList<TPerformLink, 0>::iterator it
	     = getChildren().begin();
	     it != getChildren().end(); ++it) {
		u32 maskedFlags = param_1 & it->unk8;
		it->unk4->testPerform(maskedFlags, param_2);
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
