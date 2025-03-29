#ifdef C11_ATOMICS
#include "c11_atomics.c"
#else
#error "Not Implemented!"
#endif

function inline b32 Atomic_Load_B32(atomic_b32* Atomic) {
	return (b32)Atomic_Load_S32(&Atomic->Internal);
}

function inline void Atomic_Store_B32(atomic_b32* Atomic, b32 Value) {
	Atomic_Store_S32(&Atomic->Internal, Value);
}