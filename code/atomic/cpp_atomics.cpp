export_function u16 Atomic_Load_U16(atomic_u16* Atomic) {
	return atomic_load(&Atomic->Internal);
}

export_function void Atomic_Store_U16(atomic_u16* Atomic, u16 Value) {
	atomic_store(&Atomic->Internal, Value);
}

export_function u16 Atomic_Increment_U16(atomic_u16* Atomic) {
	return atomic_fetch_add(&Atomic->Internal, 1)+1;
}

export_function u16 Atomic_Decrement_U16(atomic_u16* Atomic) {
	return atomic_fetch_sub(&Atomic->Internal, 1)-1;
}

export_function u16 Atomic_Compare_Exchange_U16(atomic_u16* Atomic, u16 OldValue, u16 NewValue) {
	atomic_compare_exchange_strong(&Atomic->Internal, &OldValue, NewValue);
	return OldValue;
}

export_function s32 Atomic_Load_S32(atomic_s32* Atomic) {
	return atomic_load(&Atomic->Internal);
}

export_function void Atomic_Store_S32(atomic_s32* Atomic, s32 Value) {
	atomic_store(&Atomic->Internal, Value);
}

export_function s32 Atomic_Compare_Exchange_S32(atomic_s32* Atomic, s32 OldValue, s32 NewValue) {
	atomic_compare_exchange_strong(&Atomic->Internal, &OldValue, NewValue);
	return OldValue;
}

export_function u32 Atomic_Load_U32(atomic_u32* Atomic) {
	return atomic_load(&Atomic->Internal);
}

export_function void Atomic_Store_U32(atomic_u32* Atomic, u32 Value) {
	atomic_store(&Atomic->Internal, Value);
}

export_function u32 Atomic_Increment_U32(atomic_u32* Atomic) {
	return atomic_fetch_add(&Atomic->Internal, 1)+1;
}

export_function u32 Atomic_Decrement_U32(atomic_u32* Atomic) {
	return atomic_fetch_sub(&Atomic->Internal, 1)-1;
}

export_function u32 Atomic_Compare_Exchange_U32(atomic_u32* Atomic, u32 OldValue, u32 NewValue) {
	atomic_compare_exchange_strong(&Atomic->Internal, &OldValue, NewValue);
	return OldValue;
}

export_function u64 Atomic_Load_U64(atomic_u64* Atomic) {
	return atomic_load(&Atomic->Internal);
}

export_function void Atomic_Store_U64(atomic_u64* Atomic, u64 Value) {
	atomic_store(&Atomic->Internal, Value);
}

export_function u64 Atomic_Compare_Exchange_U64(atomic_u64* Atomic, u64 OldValue, u64 NewValue) {
	atomic_compare_exchange_strong(&Atomic->Internal, &OldValue, NewValue);
	return OldValue;
}