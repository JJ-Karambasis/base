#ifndef BASE_H
#define BASE_H

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include <wchar.h>

#ifdef OS_WIN32
#include <windows.h>
#define export_function __declspec(dllexport)
#else
#define export_function
#endif

#define function static 
#define global static
#define local static
#define Array_Count(array) (sizeof(array)/sizeof((array)[0]))

#define Memory_Copy(dst, src, size) memcpy(dst, src, size)
#define Memory_Clear(dst, size) memset(dst, 0, size)
#define Zero_Struct(dst) Memory_Clear(&(dst), sizeof(dst))

#define Radians_Const 0.0174533f
#define To_Radians(degrees) ((degrees)*Radians_Const)
#define Abs(v) (((v) < 0) ? -(v) : (v))
#define Sq(v) ((v)*(v))

#define Offset_Of(type, variable) offsetof(type, variable)

#define Sign_Of(v) ((v) < 0 ? -1 : 1)

#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Clamp(min, v, max) Min(Max(min, v), max)
#define Saturate(v) Clamp(0, v, 1)

#define Is_Pow2(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))
#define Align_Pow2(x, a) (((x) + (a)-1) & ~((a)-1))

#define KB(x) (x*1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)

#define Offset_Pointer(ptr, offset) (void*)(((u8*)ptr) + (intptr_t)offset)

#define DEFAULT_ALIGNMENT 16

#define PI 3.14159265359f
#define TAU 6.28318530717958647692f

#ifdef DEBUG_BUILD
#include <assert.h>
#define Assert(c) assert(c)
#else
#define Assert(c)
#endif

#define Invalid_Default_Case default: { Assert(false); } break
#define Invalid_Else else Assert(false)
#define Not_Implemented Assert(false)

#define Static_Assert(c) static_assert((c), "(" #c ") failed")

//Generic link list macros
#define SLL_Pop_Front_N(First, Next) ((First) = (First)->Next)
#define SLL_Push_Front_N(First, Node, Next) (Node->Next = First, First = Node)

#define Read_Bit(S,N) ((S) & (0x1ul << (N)))
#define Set_Bit(value, bit_index) ((value) |= (1 << (bit_index)))
#define Clear_Bit(value, bit_index) ((value) &= ~(1 << (bit_index))) 

#define SLL_Push_Back_NP(First, Last, Node, Next, Prev) (!First ? (First = Last = Node) : (Last->Next = Node, Last = Node))

#define SLL_Push_Back(First, Last, Node) SLL_Push_Back_NP(First, Last, Node, Next, Prev)
#define SLL_Push_Front(First, Node) SLL_Push_Front_N(First, Node, Next)
#define SLL_Pop_Front(First) SLL_Pop_Front_N(First, Next)

#define DLL_Remove_Front_NP(First, Last, Next, Prev) \
do { \
	if(First == Last) \
		First = Last = NULL; \
			else { \
				First = First->Next; \
					First->Prev = NULL; \
	} \
} while(0)

#define DLL_Remove_Back_NP(First, Last, Next, Prev) \
do { \
	if(First == Last) \
		First = Last = NULL; \
			else { \
				Last = Last->Prev; \
					Last->Next = NULL; \
	} \
} while(0)

#define DLL_Push_Front_NP(First, Last, Node, Next, Prev) (!(First) ? ((First) = (Last) = (Node)) : ((Node)->Next = (First), (First)->Prev = (Node), (First) = (Node)))
#define DLL_Push_Back_NP(First, Last, Node, Next, Prev) (!(First) ? ((First) = (Last) = (Node)) : ((Node)->Prev = (Last), (Last)->Next = (Node), (Last) = (Node)))
#define DLL_Remove_NP(First, Last, Node, Next, Prev) \
do { \
	if(First == Node) { \
		First = First->Next; \
			if(First) First->Prev = NULL; \
	} \
		if(Last == Node) { \
			Last = Last->Prev; \
				if(Last) Last->Next = NULL; \
		} \
			if(Node->Prev) Node->Prev->Next = Node->Next; \
				if(Node->Next) Node->Next->Prev = Node->Prev; \
					Node->Prev = NULL; \
						Node->Next = NULL; \
} while(0)

#define DLL_Insert_After_NP(First, Last, Target, Node, Next, Prev) \
do { \
	if(Target->Next) { \
		Target->Next->Prev = Node; \
			Node->Next = Target->Next; \
	} \
		else { \
			Assert(Target == Last, "Invalid"); \
				Last = Node; \
	} \
		Node->Prev = Target; \
			Target->Next = Node; \
} while(0)

#define DLL_Insert_Prev_NP(First, Last, Target, Node, Next, Prev) \
do { \
	if(Target->Prev) { \
		Target->Prev->Next = Node; \
			Node->Prev = Target->Prev; \
	} \
		else { \
			Assert(Target == First, "Invalid"); \
				First = Node; \
	} \
		Node->Next = Target; \
			Target->Prev = Node; \
} while(0)

#define DLL_Remove_Back(First, Last) DLL_Remove_Back_NP(First, Last, Next, Prev)
#define DLL_Remove_Front(First, Last) DLL_Remove_Front_NP(First, Last, Next, Prev)
#define DLL_Push_Front(First, Last, Node) DLL_Push_Front_NP(First, Last, Node, Next, Prev)
#define DLL_Push_Back(First, Last, Node) DLL_Push_Back_NP(First, Last, Node, Next, Prev)
#define DLL_Remove(First, Last, Node) DLL_Remove_NP(First, Last, Node, Next, Prev)
#define DLL_Push_Front_Only(First, Node) DLL_Push_Front_Only_NP(First, Node, Next, Prev)

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef s8 	 	 b8;
typedef s16 	 b16;
typedef s32 	 b32;
typedef s64 	 b64;
typedef float    f32;
typedef double   f64;

#include "atomic/atomic.h"

typedef struct {
	union {
		f32 Data[2];
		struct {
			f32 x, y;
		};
	};
} v2;

typedef struct {
	s32 x, y;
} v2i;

typedef struct {
	union {
		f32 Data[3];
		struct {
			f32 x, y, z;
		};
		struct {
			f32 r, g, b;
		};
		struct {
			f32 h, s, v;
		};
		struct {
			v2 xy;
			f32 __unused0__;
		};
	};
} v3;

typedef struct {
	union {
		f32 Data[4];
		struct {
			f32 x, y, z, w;
		};
		v3 xyz;
	};
} v4;

typedef struct {
	union {
		f32 Data[4];
		v4  V;
		struct { f32 x, y, z, w; };
		struct { v3 v; f32 s; };
	};
} quat;

typedef struct {
	union {
		f32 Data[12];
		v3  Rows[3];
		struct {
			v3 x;
			v3 y;
			v3 z;
		};
		struct {
			f32 m00, m01, m02;
			f32 m10, m11, m12;
			f32 m20, m21, m22;
		};
	};
} m3;

typedef struct {
	union {
		f32 Data[16];
		v4  Rows[4];
		struct {
			v3 x; f32 __unused0__;
			v3 y; f32 __unused1__;
			v3 z; f32 __unused2__;
			v3 t; f32 __unused3__;
		};
		struct {
			f32 m00, m01, m02, m03;
			f32 m10, m11, m12, m13;
			f32 m20, m21, m22, m23;
			f32 m30, m31, m32, m33;
		};
	};
} m4;

typedef struct {
	union {
		f32 Data[12];
		v3  Rows[4];
		m3  M;
		struct {
			v3 x;
			v3 y;
			v3 z;
			v3 t;
		};
		struct {
			f32 m00, m01, m02;
			f32 m10, m11, m12;
			f32 m20, m21, m22;
			f32 m30, m31, m32;
		};
	};
} m4_affine;

typedef struct {
	union {
		f32 Data[12];
		v4  Rows[3];
		struct {
			f32 m00, m01, m02, m03;
			f32 m10, m11, m12, m13;
			f32 m20, m21, m22, m23;
		};
	};

} m4_affine_transposed;

typedef struct {
	v2 p0;
	v2 p1;
} rect2;

typedef struct {
	u8*    BaseAddress;
	size_t ReserveSize;
	size_t CommitSize;
} memory_reserve;

typedef struct {
	u8*    Ptr;
	size_t Size;
} buffer;

typedef enum {
	CLEAR_FLAG_NO,
	CLEAR_FLAG_YES
} clear_flag;

typedef struct allocator allocator;
#define ALLOCATOR_ALLOCATE_MEMORY_DEFINE(name) void* name(allocator* Allocator, size_t Size, clear_flag ClearFlag)
#define ALLOCATOR_FREE_MEMORY_DEFINE(name) void name(allocator* Allocator, void* Memory)

typedef ALLOCATOR_ALLOCATE_MEMORY_DEFINE(allocator_allocate_memory_func);
typedef ALLOCATOR_FREE_MEMORY_DEFINE(allocator_free_memory_func);

typedef struct {
	allocator_allocate_memory_func* AllocateMemoryFunc;
	allocator_free_memory_func* 	FreeMemoryFunc;
} allocator_vtable;

struct allocator {
	allocator_vtable* VTable;
};

#define Allocator_Allocate_Memory(allocator, size) (allocator)->VTable->AllocateMemoryFunc(allocator, size, CLEAR_FLAG_YES)
#define Allocator_Free_Memory(allocator, memory) (allocator)->VTable->FreeMemoryFunc(allocator, memory)

#define Allocator_Allocate_Struct(allocator, type) (type *)Allocator_Allocate_Memory(allocator, sizeof(type))
#define Allocator_Allocate_Array(allocator, count, type) (type *)Allocator_Allocate_Memory(allocator, sizeof(type)*(count))

typedef struct {
	allocator 	   Base;
	memory_reserve Reserve;
	size_t    	   Used;
} arena;

typedef struct {
	arena* Arena;
	size_t Used;
} arena_marker;

#define MAX_SCRATCH_COUNT 64
typedef struct {
	size_t 		 ScratchIndex;
	arena* 		 ScratchArenas[MAX_SCRATCH_COUNT];
	arena_marker ScratchMarkers[MAX_SCRATCH_COUNT];
} thread_context;

typedef struct {
	const char* Ptr;
	size_t Size;
} string;

typedef struct string_entry string_entry;
struct string_entry {
	string String;
	string_entry* Next;
};

typedef struct {
	string_entry* First;
	string_entry* Last;
	size_t Count;
} string_list;

typedef struct {
	const wchar_t* Ptr;
	size_t Size;
} wstring;

typedef struct {
	const char* Start;
	const char* End;
	const char* At;
	size_t 		LineIndex;
	size_t 		ColumnIndex;
} sstream_reader;

typedef struct sstream_writer_node sstream_writer_node;

struct sstream_writer_node {
	string Entry;
	sstream_writer_node* Next;
};

typedef struct {
	allocator*           Allocator;
	sstream_writer_node* First;
	sstream_writer_node* Last;
	size_t 				 NodeCount;
	size_t 				 TotalCharCount;
} sstream_writer;

typedef struct {
	char   Char;
	size_t Index;
	size_t LineIndex;
	size_t ColumnIndex;
} sstream_char;

typedef struct {
	u32 Hash;
	u32 ItemIndex;
	u32 BaseCount;
} hash_slot;

#define KEY_HASH_FUNC(name) u32 name(void* Key)
typedef KEY_HASH_FUNC(key_hash_func);

#define KEY_COMP_FUNC(name) b32 name(void* KeyA, void* KeyB)
typedef KEY_COMP_FUNC(key_comp_func);

typedef struct {
	allocator* Allocator;
	hash_slot* Slots;
	u32 	   SlotCapacity;
	void*      Keys;
	void*      Values;
	u32*       ItemSlots;
	size_t 	   KeySize;
	size_t 	   ValueSize;
	u32 	   ItemCapacity;
	u32 	   ItemCount;

	key_hash_func* HashFunc;
	key_comp_func* CompareFunc;
} hashmap;

#define Array_Define(type) \
typedef struct { \
size_t Count; \
type*  Ptr; \
} type##_array;

Array_Define(char);
Array_Define(u32);
Array_Define(v3);
Array_Define(string);

#define Dynamic_Array_Define(type, name) \
typedef struct { \
allocator* Allocator; \
type* Ptr; \
size_t Count; \
size_t Capacity; \
} dynamic_##name##_array

#define Dynamic_Array_Define_Type(type) Dynamic_Array_Define(type, type)
#define Dynamic_Array_Define_Ptr(type) Dynamic_Array_Define(type*, type)

Dynamic_Array_Define_Type(char);
Dynamic_Array_Define_Type(string);

Dynamic_Array_Define(char*, char_ptr);

typedef struct {
	union {
		u64 ID;
		struct {
			u32 Index;
			u32 Slot;
		};
	};
} slot_id;

typedef struct {
	u32*   Slots;
    u32*   FreeIndices;
    size_t FreeCount;
    size_t Capacity;
    size_t Count;
} slot_map;

#define BINARY_HEAP_COMPARE_FUNC(name) b32 name(void* A, void* B)
typedef BINARY_HEAP_COMPARE_FUNC(binary_heap_compare_func);

typedef struct {
	binary_heap_compare_func* CompareFunc;
	size_t Capacity;
	size_t Count;
	size_t ItemSize;
	u8*    Ptr;
} binary_heap;

#include "os/os_base.h"

typedef struct {
	os_tls*   ThreadContextTLS;
	allocator DefaultAllocator;
	os_base*  OSBase;
	allocator_vtable* ArenaVTable;
} base;

#include "akon.h"

#endif