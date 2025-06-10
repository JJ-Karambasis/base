#include "c11_atomics.c"

export_function b32 Atomic_Load_B32(atomic_b32* Atomic) {
	return (b32)Atomic_Load_S32(&Atomic->Internal);
}

export_function void Atomic_Store_B32(atomic_b32* Atomic, b32 Value) {
	Atomic_Store_S32(&Atomic->Internal, Value);
}

export_function b32 Atomic_Compare_Exchange_B32(atomic_b32* Atomic, b32 OldValue, b32 NewValue) {
	return (b32)Atomic_Compare_Exchange_S32(&Atomic->Internal, OldValue, NewValue);
}

export_function void* Atomic_Load_Ptr(atomic_ptr* Atomic) {
#ifdef USE_32_BIT
	return (void*)(size_t)Atomic_Load_U32(&Atomic->Internal);
#else
	return (void*)(size_t)Atomic_Load_U64(&Atomic->Internal);
#endif
}

export_function void Atomic_Store_Ptr(atomic_ptr* Atomic, void* Ptr) {
#ifdef USE_32_BIT
	Atomic_Store_U32(&Atomic->Internal, (u32)(size_t)Ptr);
#else
	Atomic_Store_U64(&Atomic->Internal, (u64)(size_t)Ptr);
#endif
}

export_function void* Atomic_Compare_Exchange_Ptr(atomic_ptr* Atomic, void* OldPtr, void* NewPtr) {
#ifdef USE_32_BIT
	return (void*)(size_t)Atomic_Compare_Exchange_U32(&Atomic->Internal, (u32)(size_t)OldPtr, (u32)(size_t)NewPtr);
#else
	return (void*)(size_t)Atomic_Compare_Exchange_U64(&Atomic->Internal, (u64)(size_t)OldPtr, (u64)(size_t)NewPtr);
#endif
}