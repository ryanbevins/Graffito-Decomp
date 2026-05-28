#include <Player/MarioEffect.hpp>

// rogue includes for matching __sinit (15 JALList<T> templates)
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

void TMarioEffect::perform(u32 flags, JDrama::TGraphics* gfx)
{
	(void)flags;
	(void)gfx;
}

void TMarioEffect::init(TMario* mario) { (void)mario; }

void TMarioEffect::setJumpIntoWaterEffect() { }
void TMarioEffect::setJumpIntoWaterEffectSmall() { }
