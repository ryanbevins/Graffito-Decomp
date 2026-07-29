#ifndef JDR_FLAG_HPP
#define JDR_FLAG_HPP

namespace JDrama {

template <class T> class TFlagT {
public:
	TFlagT(T v = T());

#if defined(JDRAMA_TFLAG_COPY_CTOR_OUT_OF_LINE)                              \
    || defined(JDRAMA_TFLAG_COPY_CTOR_DECL_ONLY)
	TFlagT(const TFlagT<T>& other);
#else
	TFlagT(const TFlagT<T>& other)
	    : mValue(other.mValue)
	{
	}
#endif

	// fabricated
	TFlagT& operator=(const TFlagT<T>& other)
	{
		set(other.mValue);
		return *this;
	}

	void set(T v)
#ifdef JDRAMA_TFLAG_SET_DECL_ONLY
	    ;
#else
	{
		mValue = v;
	}
#endif
	T get() const { return mValue; }

	void setBit(T bit, bool on)
	{
		if (on)
			mValue |= bit;
		else
			mValue &= ~bit;
	}

	bool check(T bit) const { return (mValue & bit) != 0; }
	void on(T bit) { mValue |= bit; }
	void off(T bit) { mValue &= ~bit; }

public:
	T mValue;
};

#ifdef JDRAMA_TFLAG_CTOR_DECL_ONLY
#elif defined(JDRAMA_NO_INLINE_BASE_CTORS)
#pragma dont_inline on
template <class T> TFlagT<T>::TFlagT(T v) { set(v); }
#pragma dont_inline off
#elif defined(JDRAMA_TFLAG_SET_DECL_ONLY)
template <class T>
inline TFlagT<T>::TFlagT(T v)
    : mValue(v)
{
}
#else
template <class T> inline TFlagT<T>::TFlagT(T v) { set(v); }
#endif

#ifdef JDRAMA_TFLAG_COPY_CTOR_OUT_OF_LINE
#pragma dont_inline on
template <class T>
TFlagT<T>::TFlagT(const TFlagT<T>& other)
    : mValue(other.mValue)
{
}
#pragma dont_inline off
#endif

}; // namespace JDrama

#endif
