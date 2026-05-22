#include <Player/MarioRecord.hpp>

template <typename T> void TRecordValueManager<T>::reset()
{
	mCurrentDurationPtr = mFirstDurationPtr;
	mCurrentValuePtr    = mFirstValuePtr;
	mElapsedFrames      = 0;
	mUnk14              = 0;
}

template <typename T> bool TRecordValueManager<T>::get(T* outValue)
{
	*outValue = *mCurrentValuePtr;
	mElapsedFrames++;
	if ((u32)mElapsedFrames >= *mCurrentDurationPtr) {
		mCurrentDurationPtr++;
		mCurrentValuePtr++;
		mElapsedFrames = 0;
		return true;
	}
	return false;
}

void TMarioInputReplay::reset()
{
	mCanPlay      = 0;
	mPrevBtnMask  = 0;
	mReplayPos    = 0;
	mMag.reset();
	mYaw.reset();
	mBtnMask.reset();
	mUnk64.reset();
	mUnk80.reset();
}

bool TMarioInputReplay::play(f32* outIntendedMag, s16* outIntendedYaw,
                             u32* outPressedBtns, u32* outJustPressedBtns,
                             u8* a, u8* b)
{
	if (mCanPlay != 1)
		return false;
	if (mReplayPos >= mReplayLength)
		return false;

	mMag.get(outIntendedMag);
	mYaw.get(outIntendedYaw);

	u16 btnMask;
	mBtnMask.get(&btnMask);
	u16 just             = btnMask & ~mPrevBtnMask;
	mPrevBtnMask         = btnMask;
	((u16*)outPressedBtns)[1]     = btnMask;
	((u16*)outJustPressedBtns)[1] = just;

	mUnk64.get(a);
	mUnk80.get(b);

	mReplayPos++;
	return true;
}

void TMarioInputReplay::init(u8* iData)
{
	mUnk0         = 0;
	mCanPlay      = 0;
	mPrevBtnMask  = 0;
	mReplayPos    = 0;
	mReplayLength = *(u32*)(iData + 0x10);

	mMag.mFirstDurationPtr = (u32*)(iData + *(u32*)(iData + 0x14));
	mMag.mFirstValuePtr    = (f32*)(iData + *(u32*)(iData + 0x18));
	mMag.reset();

	mYaw.mFirstDurationPtr = (u32*)(iData + *(u32*)(iData + 0x1C));
	mYaw.mFirstValuePtr    = (s16*)(iData + *(u32*)(iData + 0x20));
	mYaw.reset();

	mBtnMask.mFirstDurationPtr = (u32*)(iData + *(u32*)(iData + 0x24));
	mBtnMask.mFirstValuePtr    = (u16*)(iData + *(u32*)(iData + 0x28));
	mBtnMask.reset();

	mUnk64.mFirstDurationPtr = (u32*)(iData + *(u32*)(iData + 0x2C));
	mUnk64.mFirstValuePtr    = (u8*)(iData + *(u32*)(iData + 0x30));
	mUnk64.reset();

	mUnk80.mFirstDurationPtr = (u32*)(iData + *(u32*)(iData + 0x34));
	mUnk80.mFirstValuePtr    = (u8*)(iData + *(u32*)(iData + 0x38));
	mUnk80.reset();
}

template class TRecordValueManager<u8>;
template class TRecordValueManager<u16>;
template class TRecordValueManager<s16>;
template class TRecordValueManager<f32>;
