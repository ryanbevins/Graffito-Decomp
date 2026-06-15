#ifndef JDR_NAME_REF_HPP
#define JDR_NAME_REF_HPP

#include <types.h>
#include <JSystem/JSupport/JSUMemoryInputStream.hpp>
#include <JSystem/JSupport/JSUMemoryOutputStream.hpp>

namespace JDrama {

class TNameRef {
public:
	TNameRef(const char* name);

	static u16 calcKeyCode(char const*);
	static const char* getType(JSUMemoryInputStream&, JSUMemoryInputStream&);
	static TNameRef* genObject(JSUMemoryInputStream&, JSUMemoryInputStream&);

	virtual ~TNameRef();
	virtual int getType() const;
	virtual void load(JSUMemoryInputStream&);
	virtual void save(JSUMemoryOutputStream&);
	virtual void loadAfter();

	virtual TNameRef* searchF(u16 key, char const* name);
#ifdef JDRAMA_NAMEREF_SEARCH_OUT_OF_LINE
	TNameRef* search(const char* name);
#else
	TNameRef* search(const char* name)
	{
		return searchF(calcKeyCode(name), name);
	}
#endif

	// fabricated
	const char* getName() const { return mName; }

protected:
	/* 0x4 */ const char* mName;
	/* 0x8 */ u16 mKeyCode;
};

#ifdef JDRAMA_NO_INLINE_BASE_CTORS
#pragma dont_inline on
TNameRef::TNameRef(const char* name)
    : mName(name)
    , mKeyCode(calcKeyCode(name))
{
}
#pragma dont_inline off
#else
inline TNameRef::TNameRef(const char* name)
    : mName(name)
    , mKeyCode(calcKeyCode(name))
{
}
#endif

} // namespace JDrama

#endif
