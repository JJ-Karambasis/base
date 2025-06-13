#ifndef CPP_ATOMICS_H
#define CPP_ATOMICS_H

#undef function
#include <atomic>
#define function static

typedef struct {
	std::atomic<u16> Internal;
} atomic_u16;

typedef struct {
	std::atomic<s32> Internal;  
} atomic_s32;

typedef struct {
	std::atomic<u32>Internal;  
} atomic_u32;

typedef struct {
	std::atomic<u64> Internal;  
} atomic_u64;

#endif