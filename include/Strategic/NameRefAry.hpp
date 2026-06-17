#ifndef STRATEGIC_NAME_REF_ARY_HPP
#define STRATEGIC_NAME_REF_ARY_HPP

#include <JSystem/JDrama/JDRNameRef.hpp>
#include <JSystem/JGadget/std-vector.hpp>
#include <dolphin/os.h>

template <class T, class U = JDrama::TNameRef>
class TNameRefAryT : public U, public JGadget::TVector<T> {
public:
	TNameRefAryT(const char* name = "<NameRefAryT>")
	    : U(name)
	{
	}

	// fabricated
	JGadget::TVector<T>& getChildren() { return *this; }

	virtual void load(JSUMemoryInputStream& stream)
	{
		U::load(stream);
		u32 local_44 = stream.readU32();
		getChildren().resize(local_44);

		for (int i = 0; i < local_44; ++i) {
			JSUMemoryInputStream stream2;
			JDrama::TNameRef::getType(stream, stream2);
			getChildren()[i].load(stream2);
		}
	}

	virtual void loadAfter()
	{
		U::loadAfter();
		for (T* it = getChildren().begin(); it != getChildren().end(); ++it)
			it->loadAfter();
	}

	virtual JDrama::TNameRef* searchF(u16 key, const char* name)
	{
		if (JDrama::TNameRef* candidate = JDrama::TNameRef::searchF(key, name))
			return candidate;

		for (T* it = getChildren().begin(); it != getChildren().end(); ++it) {
			u32 a    = (u32)it;
			bool bad = (a < 0x80003100u || a >= 0x81800000u || (a & 3));
			if (!bad) {
				u32 vt = *(u32*)a;
				bad = (vt < 0x80003100u || vt >= 0x81800000u || (vt & 3));
			}
			if (bad) {
				OSReport((char*)"BADARY %s elem=%p\n", this->getName(),
				         (void*)it);
				break;
			}
			if (JDrama::TNameRef* candidate = it->searchF(key, name))
				return candidate;
		}

		return nullptr;
	}
};

#endif
