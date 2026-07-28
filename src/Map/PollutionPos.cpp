#include <Map/PollutionPos.hpp>
#include <Map/PollutionLayer.hpp>

// rogue includes needed for matching sinit & bss
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

int TPollutionPos::getEdgeDegree(int x, int y) const
{
	bool inArea;
	if (x < 0 || mWidth <= x || y < 0 || mHeight <= y)
		inArea = false;
	else
		inArea = true;

	if (!inArea)
		return 0;

	int degree = 0;
	for (int yOffset = -1; yOffset <= 1; ++yOffset) {
		for (int xOffset = -1; xOffset <= 1; ++xOffset) {
			if (xOffset != 0 || yOffset != 0) {
				if (getDepth(x + xOffset, y + yOffset) == 0xffU)
					++degree;
			}
		}
	}
	return degree;
}

f32 TPollutionPos::getDepthWorld(int x, int y) const
{
	if (getDepth(x, y) < 0xff) {
		f32 verticalScale  = mVerticalScale;
		f32 verticalOffset = mVerticalOffset;
		f32 d = getDepth(x, y) * verticalScale;
		return d + verticalOffset;
	} else {
		return -9999.0f;
	}
}

bool TPollutionPos::isSame(int x, int y, f32 param_3) const
{
	bool inArea;
	if (x < 0 || mWidth <= x || y < 0 || mHeight <= y)
		inArea = false;
	else
		inArea = true;

	if (!inArea)
		return false;

	int depth = getDepth(x, y);
	if (depth < 0xff) {
		int layerDepth = mOwner->unk48;
		int worldDepth = worldToDepth(param_3);
		if (depth - layerDepth <= worldDepth
		    && worldDepth <= depth + layerDepth)
			return 1;
	}
	return false;
}

bool TPollutionPos::isProhibit(int x, int y) const
{
	if (x < 0 || mWidth <= x || y < 0 || mHeight <= y) {
		return 1;
	} else {
		if (getDepth(x, y) < 0xff) {
			return 0;
		} else {
			return 1;
		}
	}
}

int TPollutionPos::worldToDepth(f32 v) const
{
	v -= mVerticalOffset;
	v *= 0.025f;
	return v;
}

int TPollutionPos::worldToTexSize(f32 v) const
{
	return v * mInverseVerticalScale;
}

void TPollutionPos::init(TPollutionLayer* param_1, f32 param_2, f32 param_3,
                         u8* param_4, int param_5, int param_6)
{
	mOwner                = param_1;
	mMap                  = param_4;
	mVerticalOffset       = param_2;
	mVerticalScale        = param_3;
	mInverseVerticalScale = 1.0f / mVerticalScale;
	unk8                  = param_5;
	unkC                  = param_6;
	mWidth                = 1 << unk8;
	mHeight               = 1 << unkC;
}

TPollutionPos::TPollutionPos()
{
	mWidth                = 0;
	mHeight               = 0;
	unk8                  = 0;
	unkC                  = 0;
	mVerticalOffset       = 0.0f;
	mVerticalScale        = 0.0f;
	mInverseVerticalScale = 0.0f;
	mMap                  = nullptr;
	mOwner                = nullptr;
}
