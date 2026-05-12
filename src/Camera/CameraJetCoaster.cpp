#include <Camera/Camera.hpp>

class TCameraJetCoaster {
public:
	TCameraJetCoaster();

	/* 0x0 */ u32 unk0;
	/* 0x4 */ u32 unk4;
	/* 0x8 */ f32 unk8;
};

TCameraJetCoaster::TCameraJetCoaster()
{
	unk0 = 0;
	unk4 = 0;
	unk8 = 0.0f;
}

void CPolarSubCamera::ctrlJetCoasterCamera_() { }
