#ifndef JG_VEC3_HPP
#define JG_VEC3_HPP

#include <dolphin/types.h>
#include <dolphin/mtx.h>
#include <JSystem/JGeometry/JGUtil.hpp>

namespace JGeometry {

template <class T> class TVec3 { };

template <> struct TVec3<s16> : public S16Vec {
public:
	TVec3() { }

	TVec3(const S16Vec& b) { set(b.x, b.y, b.z); }

	void sub(const JGeometry::TVec3<s16>&);
	void add(const JGeometry::TVec3<s16>&);

	// fabricated
	TVec3(s16 x_, s16 y_, s16 z_) { set(x_, y_, z_); }

	// I replaced this with the one in TP and it matched perfectly for me
	// I could not see it was in use anywhere, so i suspect the
	// replacement below is more correct. Yell at me to move it back if needed
	// in PR :) Or else i will remove this! -AZ template <class TY> void set(TY
	// x_, TY y_, TY z_)
	// {
	// 	x = x_;
	// 	y = y_;
	// 	z = z_;
	// }

	void set(s16 x_, s16 y_, s16 z_)
	{
		x = (s16)x_;
		y = (s16)y_;
		z = (s16)z_;
	}

	void zero() { x = y = z = 0; }
};

template <> class TVec3<f32> : public Vec {
public:
	TVec3() { }

#ifdef JGEOMETRY_ITEM_TVEC3_CTOR_SET_VEC
	TVec3(const Vec& b) { set(b); }
#else
	TVec3(const Vec& b) { set(b.x, b.y, b.z); }
#endif

	template <class T> TVec3(T x_, T y_, T z_) { set(x_, y_, z_); }

	explicit TVec3(f32 value) { setAll(value); }

#ifndef JGEOMETRY_TVEC3_IMPLICIT_COPY_CTOR
	TVec3(const TVec3& other)
	{
		// NOTE: yes, this has to use lwz/stw and not lfs/stf.
		// Checked via MarioCollision.cpp where this is not inlined
		*(Vec*)this = *(Vec*)&other;
	}
#endif

	TVec3& operator=(const TVec3& other)
	{
		// NOTE: yes, this has to use lwz/stw and not lfs/stf.
		// Checked via enemy.cpp where this is not inlined
		*(Vec*)this = *(Vec*)&other;
		return *this;
	}

	// fabricated
	operator Vec*() const { return (Vec*)&x; }
	operator const Vec*() const { return (Vec*)&x; }

	void zero() { x = y = z = 0.0f; }

#ifdef JGEOMETRY_EVENTWATCHER_TVEC3_SET_VEC_OUT_OF_LINE
	void set(const Vec& v);
#else
	void set(const Vec& v)
	{
		x = v.x;
		y = v.y;
		z = v.z;
	}
#endif
	TVec3& operator/=(f32 other)
	{
		div(other);
		return *this;
	}

	template <class TY> void set(TY x_, TY y_, TY z_)
	{
		x = x_;
		y = y_;
		z = z_;
	}

	template <class TY> void set(const TVec3<TY>& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
	}

	template <class TY> void setAll(TY value)
	{
		x = value;
		y = value;
		z = value;
	}

	// === arithmetic stuff ===

#if defined(JGEOMETRY_DRAWUTIL_OWNER_HELPERS)                                  \
    || defined(JGEOMETRY_MODELWATERMANAGER_TVEC3_OUT_OF_LINE)                  \
    || defined(JGEOMETRY_TVEC3_ADD_OUT_OF_LINE)
	void add(const TVec3& operand);
#else
	void add(const TVec3& operand)
	{
		x += operand.x;
		y += operand.y;
		z += operand.z;
	}
#endif

	void add(const TVec3& a, const TVec3& b)
	{
		x = a.x + b.x;
		y = a.y + b.y;
		z = a.z + b.z;
	}

#if defined(JGEOMETRY_DRAWUTIL_OWNER_HELPERS)                                  \
    || defined(JGEOMETRY_MODELWATERMANAGER_TVEC3_OUT_OF_LINE)                  \
    || defined(JGEOMETRY_MARIORECEIVEMSG_TVEC3_SUB_OUT_OF_LINE)                \
    || defined(JGEOMETRY_MARIOMOVE_TVEC3_SUB_OUT_OF_LINE)
	void sub(const TVec3& translate);
#else
	void sub(const TVec3& translate)
	{
		x -= translate.x;
		y -= translate.y;
		z -= translate.z;
	}
#endif

	void sub(const TVec3& fst, const TVec3& snd)
	{
		x = fst.x - snd.x;
		y = fst.y - snd.y;
		z = fst.z - snd.z;
	}

	void mul(const TVec3& b)
	{
		x *= b.x;
		y *= b.y;
		z *= b.z;
	}

	void mul(const TVec3& fst, const TVec3& snd)
	{
		x = fst.x * snd.x;
		y = fst.y * snd.y;
		z = fst.z * snd.z;
	}

#ifdef JGEOMETRY_TVEC3_DIV_OUT_OF_LINE
	void div(f32 divisor);
#else
	void div(f32 divisor) { scale(1.0f / divisor); }
#endif

	TVec3& operator+=(const TVec3& other)
	{
		add(other);
		return *this;
	}
#ifdef JGEOMETRY_TONGUE_TVEC3_MINUS_ASSIGN_OUT_OF_LINE
	TVec3& operator-=(const TVec3& other);
#else
	TVec3& operator-=(const TVec3& other)
	{
		sub(other);
		return *this;
	}
#endif
	TVec3& operator*=(const TVec3& other)
	{
		mul(other);
		return *this;
	}
#ifdef JGEOMETRY_MARIOCOLLISION_TVEC3_MUL_ASSIGN_OUT_OF_LINE
	TVec3& operator*=(f32 other);
#else
	// @fabricated
	TVec3& operator*=(f32 other)
	{
		scale(other);
		return *this;
	}
#endif

	// fabricated and fake and UB but it makes things match??
	friend const TVec3& operator-(TVec3 fst, const TVec3& snd)
	{
		fst -= snd;
		return fst;
	}

	// fabricated and fake and UB but it makes things match??
	friend const TVec3& operator+(TVec3 fst, const TVec3& snd)
	{
		fst += snd;
		return fst;
	}

#ifdef JGEOMETRY_TONGUE_TVEC3_MUL_RET_REF
	friend const TVec3& operator*(TVec3 fst, f32 snd)
	{
		fst *= snd;
		return fst;
	}
#else
	// @fabricated
	friend TVec3 operator*(TVec3 fst, f32 snd)
	{
		fst *= snd;
		return fst;
	}
#endif
	friend TVec3 operator/(TVec3 fst, f32 snd)
	{
		fst /= snd;
		return fst;
	}

#ifdef JGEOMETRY_CAMERASHAKE_TVEC3_DOT_SCALE_OUT_OF_LINE
	f32 dot(const TVec3& other) const;
#else
	f32 dot(const TVec3& other) const
	{
		return x * other.x + y * other.y + z * other.z;
	}
#endif

	// Incorrect!!!
	void cross(const TVec3& a, const TVec3& b)
	{
		f32 _x = a.y * b.z - a.z * b.y;
		f32 _y = a.z * b.x - a.x * b.z;
		f32 _z = a.x * b.y - a.y * b.x;

		x = _x;
		y = _y;
		z = _z;
	}

	// Incorrect!!!
	void cross2(const TVec3& a, const TVec3& b)
	{
		set(a.y * b.z - a.z * b.y, //
		    a.z * b.x - a.x * b.z, //
		    a.x * b.y - a.y * b.x);
	}

	void negate()
	{
		x = -x;
		y = -y;
		z = -z;
	}

#ifdef JGEOMETRY_MODELWATERMANAGER_TVEC3_OUT_OF_LINE
	void scale(f32 scale);
#else
	void scale(f32 scale)
	{
		x *= scale;
		y *= scale;
		z *= scale;
	}
#endif

#ifdef JGEOMETRY_CAMERASHAKE_TVEC3_DOT_SCALE_OUT_OF_LINE
	void scale(f32 scale, const TVec3& b);
#else
	void scale(f32 scale, const TVec3& b)
	{
		x = b.x * scale;
		y = b.y * scale;
		z = b.z * scale;
	}
#endif

#ifdef JGEOMETRY_MAPMIRROR_TVEC3_SCALEADD_OUT_OF_LINE
	void scaleAdd(f32 scale, const TVec3& b, const TVec3& c);
#else
	void scaleAdd(f32 scale, const TVec3& b, const TVec3& c)
	{
		x = b.x + c.x * scale;
		y = b.y + c.y * scale;
		z = b.z + c.z * scale;
	}
#endif

	// === length stuff ===

	f32 distance(const TVec3& other) const
	{
		f32 dx = x - other.x;
		f32 dy = y - other.y;
		f32 dz = z - other.z;
		return TUtil<f32>::sqrt(dx * dx + dy * dy + dz * dz);
	}

	f32 squared() const { return dot(*this); }

#ifdef JGEOMETRY_MARIOSPECIAL_TVEC3_LENGTH_INLINE
	f32 length() const
	{
		f32 mag = squared();
		if (mag <= 0.0f)
			return mag;

		f32 root = __frsqrte(mag);
		return 0.5f * root * (3.0f - mag * (root * root)) * mag;
	}
#else
	f32 length() const { return TUtil<f32>::sqrt(squared()); }
#endif

	// @fabricated
	f32 length2() const { return length(); }

	bool isZero() const { return squared() <= TUtil<f32>::epsilon(); }

	void setLength(f32 length) { setLength(*this, length); }

	void normalize() { setLength(*this, TUtil<f32>::one()); }

	void normalize(const TVec3& other) { setLength(other, TUtil<f32>::one()); }

	void setLength(const TVec3& v, f32 length)
	{
		f32 lsq = v.squared();
		if (lsq <= TUtil<f32>::epsilon()) {
			zero();
			return;
		}

		scale(length * JGeometry::TUtil<f32>::inv_sqrt(lsq), v);
	}

	void setMax(const TVec3& max)
	{
		if (x <= max.x)
			x = max.x;
		if (y <= max.y)
			y = max.y;
		if (z <= max.z)
			z = max.z;
	}

	void setMin(const TVec3& min)
	{
		if (x >= min.x)
			x = min.x;
		if (y >= min.y)
			y = min.y;
		if (z >= min.z)
			z = min.z;
	}

	// TODO: SMG's operator== uses epsilonEquals. Maybe this wasn't operator==
	// but a separate function? Eh, whatever.
	bool operator==(const TVec3& other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

} // namespace JGeometry

#endif
