#ifndef STACK_PADDING_H
#define STACK_PADDING_H

// Reserve otherwise unused stack space to reproduce the retail MWCC frame.
// These explicit matching pads can be removed when the original local/inline
// structure is recovered. No initialization or runtime memory access is needed.
#define PAD_STACK(bytes)                                                      \
	do {                                                                      \
		unsigned char stackPadding[(bytes)];                                   \
		(void)stackPadding;                                                   \
	} while (0)

// A bare declaration avoids changing MWCC's inline cost for small helpers.
#define PAD_STACK_ARRAY(bytes) unsigned char stackPadding[(bytes)]

#ifdef __cplusplus
// Inlining the reservation places it with compiler-generated temporaries.
template <int bytes> inline void reserveStackTemporary()
{
	unsigned char stackPadding[bytes];
	(void)stackPadding;
}
#define PAD_STACK_TEMP(bytes) reserveStackTemporary<(bytes)>()
#endif

#endif
