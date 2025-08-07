#include "base.c"

void operator delete(void* p) noexcept {
	Allocator_Free_Memory(Default_Allocator_Get(), p);
}

void operator delete[](void* p) noexcept {
	Allocator_Free_Memory(Default_Allocator_Get(), p);
}

void* operator new(size_t Size) noexcept(false) {
	return Allocator_Allocate_Memory(Default_Allocator_Get(), Size);
}

void* operator new[](size_t Size) noexcept(false) {
	return Allocator_Allocate_Memory(Default_Allocator_Get(), Size);
}