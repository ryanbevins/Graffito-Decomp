#include <Map/PollutionObj.hpp>
#include <Map/PollutionLayer.hpp>
#include <Map/Map.hpp>
#include <Map/MapEventSink.hpp>
#include <JSystem/J3D/J3DGraphAnimator/J3DJoint.hpp>
#include <types.h>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

static inline bool is_near(f32 a, f32 b)
{
	if (a - 30.0f <= b && b <= a + 30.0f)
		return true;

	return false;
}

static inline bool is_near(f32 a, f32 b, f32 c, f32 d)
{
	if (is_near(a, b) && is_near(b, c) && is_near(c, d))
		return true;

	return false;
}

int TPollutionLayer::getTexPosT(f32 z) const
{
	return unk5C.worldToTexSize(z - unk40);
}

u8 TPollutionObj::getDepthFromMap(int x, int y)
{
	const TBGCheckData* checkData;
	f32 scale = unk34->unk5C.mVerticalScale;
	f32 scaledX = x * scale;
	f32 scaledZ = y * scale;
	f32 baseX = unk34->unk38;
	f32 baseZ = unk34->unk40;
	f32 worldX = baseX + scaledX;
	f32 worldZ = baseZ + scaledZ;

	f32 minX = worldX - 5.0f;
	f32 maxX = worldX + scale + 5.0f;
	f32 minZ = worldZ - 5.0f;
	f32 maxZ = worldZ + scale + 5.0f;

	f32 ground0 = gpMap->checkGround(minX, 9999999.0f, minZ, &checkData);
	f32 ground1 = gpMap->checkGround(maxX, 9999999.0f, minZ, &checkData);
	f32 ground2 = gpMap->checkGround(minX, 9999999.0f, maxZ, &checkData);
	f32 ground3 = gpMap->checkGround(maxX, 9999999.0f, maxZ, &checkData);

	if (is_near(ground0, ground2, ground1, ground3)) {
		f32 halfScale = 0.5f * scale;
		f32 ground = gpMap->checkGround(worldX + halfScale, 9999999.0f,
		                                worldZ + halfScale, &checkData);
		return unk34->unk5C.worldToDepth(ground);
	}

	return 0xff;
}

void TPollutionObj::updateDepthMap()
{
	for (int y = unk28; y < unk2C; ++y)
		for (int x = unk20; x < unk24; ++x)
			unk34->unk5C.setDepth(x, y, getDepthFromMap(x, y));
}

bool TPollutionObj::isCleaned() const
{
	if (unk30 < TMapEventSink::mCleanedDegree)
		return true;
	return false;
}

void TPollutionObj::initAreaInfo(TPollutionLayer* layer)
{
	unk34          = layer;
	const Vec& min = mJoint->getMin();
	const Vec& max = mJoint->getMax();

	unk20 = unk34->getTexPosS(min.x);
	unk24 = unk34->getTexPosS(max.x);
	unk28 = unk34->getTexPosT(min.z);
	unk2C = unk34->getTexPosT(max.z);

	if (unk20 < 0)
		unk24 = 0;
	if (unk28 < 0)
		unk2C = 0;
	if (unk24 > unk34->unk5C.mWidth)
		unk24 = unk34->unk5C.mWidth;
	if (unk2C > unk34->unk5C.mHeight)
		unk2C = unk34->unk5C.mHeight;

	for (int i = 0; i < mChildrenNum; ++i)
		((TPollutionObj*)mChildren[i])->initAreaInfo(layer);
}

TPollutionObj::TPollutionObj()
    : unk20(0)
    , unk24(0)
    , unk28(0)
    , unk2C(0)
    , unk30(0)
    , unk34(nullptr)
{
}
