#include <Player/MarioRecord.hpp>

void TMarioInputReplay::reset() { }

bool TMarioInputReplay::play(f32* outIntendedMag, s16* outIntendedYaw,
                             u32* outPressedBtns, u32* outJustPressedBtns,
                             u8* a, u8* b)
{
	(void)outIntendedMag;
	(void)outIntendedYaw;
	(void)outPressedBtns;
	(void)outJustPressedBtns;
	(void)a;
	(void)b;
	return false;
}

void TMarioInputReplay::init(u8* data) { (void)data; }

template <typename T> void TRecordValueManager<T>::reset() { }
template <typename T> bool TRecordValueManager<T>::get(T* out)
{
	(void)out;
	return false;
}

template class TRecordValueManager<u8>;
template class TRecordValueManager<u16>;
template class TRecordValueManager<s16>;
template class TRecordValueManager<f32>;
