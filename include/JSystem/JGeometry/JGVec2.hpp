#ifndef JG_VEC2_HPP
#define JG_VEC2_HPP

#include <JSystem/JGeometry/JGUtil.hpp>

namespace JGeometry {

template <typename T> struct TVec2 {
	TVec2() { }
	TVec2(T x, T y) { set(x, y); }

	TVec2(const TVec2& other)
	{
		// NOTE: POD-cast to force integer move pattern (lwz/stw) instead
		// of float pattern (lfs/stfs); same trick as TVec3<f32>.
		struct _POD { T x, y; };
		*(_POD*)this = *(const _POD*)&other;
	}

	TVec2& operator=(const TVec2& other)
	{
		struct _POD { T x, y; };
		*(_POD*)this = *(const _POD*)&other;
		return *this;
	}

	void set(T x, T y)
	{
		this->x = x;
		this->y = y;
	}

	void set(const TVec2<T>& other)
	{
		x = other.x;
		y = other.y;
	}

	void zero() { x = y = 0.0f; }

	// === arithmetic stuff ===

	void setMin(const TVec2<T>& min)
	{
		if (x >= min.x)
			x = min.x;
		if (y >= min.y)
			y = min.y;
	}

	void setMax(const TVec2<T>& max)
	{
		if (x <= max.x)
			x = max.x;
		if (y <= max.y)
			y = max.y;
	}

	void add(const TVec2<T>& other)
	{
		x += other.x;
		y += other.y;
	}

#ifdef JGEOMETRY_SELECTSHINE2_OWNER_HELPERS
	void sub(const TVec2<T>& other);
#else
	void sub(const TVec2<T>& other)
	{
		x -= other.x;
		y -= other.y;
	}
#endif

	void scale(f32 scale)
	{
		x *= scale;
		y *= scale;
	}

	// fabricated
	void rotate(f32 angle)
	{
		f32 cosTheta = cosf(angle);
		f32 sinTheta = sinf(angle);
		set(x * cosTheta - y * sinTheta, x * sinTheta + y * cosTheta);
	}

	bool isAbove(const TVec2<T>& other) const
	{
		return (x >= other.x) && (y >= other.y) ? true : false;
	}

	T dot(const TVec2<T>& other) const { return x * other.x + y * other.y; }

	// === length stuff ===

	T squared() const { return dot(*this); }

	T length()
	{
		T sqr = squared();
		return TUtil<T>::sqrt(sqr);
	}

	// === normalize stuff lifted from JGVec3.hpp ===

	// fabricated
	void setLength(f32 length)
	{
		f32 lsq = squared();
		if (lsq <= TUtil<f32>::epsilon()) {
			zero();
			return;
		}

		scale(length * JGeometry::TUtil<f32>::inv_sqrt(lsq));
	}

	// fabricated
	void normalize() { setLength(1.0f); }

	T x;
	T y;
};

} // namespace JGeometry

#endif
