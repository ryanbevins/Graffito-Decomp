#include <Player/MarioMain.hpp>
#include <MSound/MSound.hpp>
#include <MSound/MSoundSE.hpp>

TMarioSoundValues::TMarioSoundValues()
{
	unk00 = 0;
	unk04 = -1;
	unk08 = 0;
	unk0C = 0;
	unk10 = 0;
	unk14 = 0;
	unk18 = 0;
	unk1C = 0;
	unk20 = 0;
	unk22 = 0;
	unk24 = 0;
	unk26 = 0;
	unk14 = 0;
	unk29 = 0;
	unk2A = 1;
	unk2B = 0;
	unk2C = 0;
}

void TMario::startSoundActor(u32 soundID)
{
	if (gpMSound->gateCheck(soundID))
		MSoundSESystem::MSoundSE::startSoundActor(soundID, (const Vec*)&mPosition,
		                                          0, nullptr, 0, 4);
}

