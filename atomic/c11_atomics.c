function inline u32 Atomic_Load_S32(atomic_s32* Atomic) {
	return atomic_load(&Atomic->Internal);
}

function inline void Atomic_Store_S32(atomic_s32* Atomic, s32 Value) {
	atomic_store(&Atomic->Internal, Value);
}

function inline u32 Atomic_Load_U32(atomic_u32* Atomic) {
	return atomic_load(&Atomic->Internal);
}

function inline void Atomic_Store_U32(atomic_u32* Atomic, u32 Value) {
	atomic_store(&Atomic->Internal, Value);
}

function inline u64 Atomic_Load_U64(atomic_u64* Atomic) {
	return atomic_load(&Atomic->Internal);
}

function inline void Atomic_Store_U64(atomic_u64* Atomic, u64 Value) {
	atomic_store(&Atomic->Internal, Value);
}