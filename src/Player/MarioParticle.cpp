#include <Player/MarioMain.hpp>
#include <Player/MarioEffect.hpp>

bool TMario::askJumpIntoWaterEffectExist() const
{
	TMarioEffect* eff = (TMarioEffect*)mMarioEffect;
	if (eff->unk6C[0] == 1)
		return true;
	if (eff->unk6C[1] == 1)
		return true;
	return false;
}
