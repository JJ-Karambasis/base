#ifndef C11_ATOMICS_H
#define C11_ATOMICS_H

#include <stdatomic.h>

typedef struct {
	atomic_uint_least16_t Internal;
} atomic_u16;

typedef struct {
	atomic_int_least32_t Internal;  
} atomic_s32;

typedef struct {
	atomic_uint_least32_t Internal;  
} atomic_u32;

typedef struct {
	atomic_uint_least64_t Internal;  
} atomic_u64;

#endif