// rogue includes for matching __sinit (15 JALList<T>) + JADPrm<u8> weak emit
#include <MSound/MSSetSound.hpp>
#include <MSound/MSoundBGM.hpp>

// Force weak emit of JADPrm<u8>::JADPrm(u8, const char*) in this TU.
template class JADPrm<u8>;
