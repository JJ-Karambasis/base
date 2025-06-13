#ifndef ATOMIC_H
#define ATOMIC_H

#ifdef __cplusplus
#include "cpp_atomics.h"
#else
#include "c11_atomics.h"
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

export_function u16 Atomic_Load_U16(atomic_u16* Atomic);
export_function void Atomic_Store_U16(atomic_u16* Atomic, u16 Value);
export_function u16 Atomic_Increment_U16(atomic_u16* Atomic);
export_function u16 Atomic_Decrement_U16(atomic_u16* Atomic);
export_function u16 Atomic_Compare_Exchange_U16(atomic_u16* Atomic, u16 OldValue, u16 NewValue);
export_function s32 Atomic_Load_S32(atomic_s32* Atomic);
export_function void Atomic_Store_S32(atomic_s32* Atomic, s32 Value);
export_function s32 Atomic_Compare_Exchange_S32(atomic_s32* Atomic, s32 OldValue, s32 NewValue);
export_function u32 Atomic_Load_U32(atomic_u32* Atomic);
export_function void Atomic_Store_U32(atomic_u32* Atomic, u32 Value);
export_function u32 Atomic_Increment_U32(atomic_u32* Atomic);
export_function u32 Atomic_Decrement_U32(atomic_u32* Atomic);
export_function u32 Atomic_Compare_Exchange_U32(atomic_u32* Atomic, u32 OldValue, u32 NewValue);
export_function u64 Atomic_Load_U64(atomic_u64* Atomic);
export_function void Atomic_Store_U64(atomic_u64* Atomic, u64 Value);
export_function u64 Atomic_Compare_Exchange_U64(atomic_u64* Atomic, u64 OldValue, u64 NewValue);
export_function b32 Atomic_Load_B32(atomic_b32* Atomic);
export_function void Atomic_Store_B32(atomic_b32* Atomic, b32 Value);
export_function b32 Atomic_Compare_Exchange_B32(atomic_b32* Atomic, b32 OldValue, b32 NewValue);
export_function void* Atomic_Load_Ptr(atomic_ptr* Atomic);
export_function void Atomic_Store_Ptr(atomic_ptr* Atomic, void* Ptr);
export_function void* Atomic_Compare_Exchange_Ptr(atomic_ptr* Atomic, void* OldPtr, void* NewPtr);

#endif