#ifndef ENEMY_WIRE_BINDER_HPP
#define ENEMY_WIRE_BINDER_HPP

#include <JSystem/JGeometry.hpp>
#include <Strategic/Binder.hpp>
#include <Strategic/LiveActor.hpp>

class TWireBinder : public TBinder {
public:
	bool init(const JGeometry::TVec3<f32>&);
	bool reset(const JGeometry::TVec3<f32>&);
	void bind(TLiveActor*);
	const JGeometry::TVec3<f32>& getDirDirect() const { return mDir; }
#ifdef WIREBINDER_GETDIR_OUT_OF_LINE
	const JGeometry::TVec3<f32>& getDir() const;
#else
	const JGeometry::TVec3<f32>& getDir() const { return getDirDirect(); }
#endif
	JGeometry::TVec3<f32> getDirAtPos(const JGeometry::TVec3<f32>&, f32) const;
	void getPoint(JGeometry::TVec3<f32>*, f32) const;
	void getPoint(JGeometry::TVec3<f32>*, const JGeometry::TVec3<f32>&) const;
	bool isEndWire(const JGeometry::TVec3<float>&, float) const;

	static bool isOnWire(const JGeometry::TVec3<f32>&);

	~TWireBinder();

private:
	/* 0x04 */ s32 mWireNumber;
	/* 0x08 */ JGeometry::TVec3<f32> mDir;
};

#endif
