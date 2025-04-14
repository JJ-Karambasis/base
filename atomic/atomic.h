#ifndef ATOMIC_H
#define ATOMIC_H

#ifdef C11_ATOMICS
#include "c11_atomics.h"
#else
#error "Not Implemented!"
#endif

typedef struct {
	atomic_s32 Internal;
} atomic_b32;

typedef struct {
#ifdef USE_32_BIT
	atomic_u32 Internal;
#else
	atomic_u64 Internal;
#endif
} atomic_ptr;

Static_Assert(sizeof(atomic_s32) == 4);
Static_Assert(sizeof(atomic_u32) == 4);
Static_Assert(sizeof(atomic_u64) == 8);
Static_Assert(sizeof(atomic_b32) == 4);
Static_Assert(sizeof(atomic_ptr) == PTR_SIZE);

#endif