#ifndef PLAYER_YOSHIINLINE_HPP
#define PLAYER_YOSHIINLINE_HPP

#include <Player/Yoshi.hpp>

#pragma dont_inline on
BOOL TYoshi::onYoshi() { return (u8)mState == MOUNTED ? 1 : 0; }
#pragma dont_inline off

#endif
