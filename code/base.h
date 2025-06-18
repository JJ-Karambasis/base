#ifndef BASE_H
#define BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include <wchar.h>

#ifdef _WIN32
// Windows (both 32-bit and 64-bit)
#define OS_WIN32

#ifndef _WIN64
#define USE_32_BIT
#endif

#endif

#if defined(__clang__)
#define COMPILER_CLANG
#elif defined(_MSC_VER)
#define COMPILER_MSVC
#endif

#ifdef OS_WIN32
#include <windows.h>
#ifdef __cplusplus
#define export_function extern "C" __declspec(dllexport)
#else
#define export_function __declspec(dllexport)
#endif
#else
#define export_function
#endif

#if defined(COMPILER_MSVC)
# if defined(__SANITIZE_ADDRESS__)
#  define ASAN_ENABLED
#  define NO_ASAN __declspec(no_sanitize_address)
# else
#  define NO_ASAN
# endif
#elif defined(COMPILER_CLANG)
# if defined(__has_feature)
#  if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#   define ASAN_ENABLED
#  endif
# endif
# define NO_ASAN __attribute__((no_sanitize("address")))
#else
# define NO_ASAN
#endif

#define Stringify_(x) #x
#define Stringify(x) Stringify_(x)

#ifdef ASAN_ENABLED
#pragma comment(lib, "clang_rt.asan_dynamic-x86_64.lib")
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
# define Asan_Poison_Memory_Region(addr, size)   __asan_poison_memory_region((addr), (size))
# define Asan_Unpoison_Memory_Region(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
# define Asan_Poison_Memory_Region(addr, size)   ((void)(addr), (void)(size))
# define Asan_Unpoison_Memory_Region(addr, size) ((void)(addr), (void)(size))
#endif

#define function static 
#define global static
#define local static
#define Array_Count(array) (sizeof(array)/sizeof((array)[0]))

#define PTR_SIZE sizeof(size_t)

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
#define Static_Assert(c) static_assert((c), "(" #c ") failed")
#else
#define Assert(c)

#ifdef __cplusplus
#define Static_Assert(c) static_assert((c), "(" #c ") failed")
#else
#define Static_Assert(c) _Static_assert((c), "(" #c ") failed")
#endif

#endif

#define Invalid_Default_Case default: { Assert(false); } break
#define Invalid_Else else Assert(false)
#define Not_Implemented Assert(false)

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

export_function b32 Equal_Zero_Approx_F32(f32 Value, f32 Epsilon);
export_function b32 Equal_Zero_Eps_F32(f32 Value);
export_function b32 Equal_Zero_Eps_Sq_F32(f32 SqValue);
export_function f32 Safe_Ratio(s32 x, s32 y);
export_function f32 Sqrt_F32(f32 Value);
export_function f32 Cos_F32(f32 Value);
export_function f32 Sin_F32(f32 Value);
export_function f32 Tan_F32(f32 Value);
export_function f32 ATan2_F32(f32 a, f32 b);
export_function size_t Align(size_t Value, size_t Alignment);
export_function u32 Ceil_Pow2_U32(u32 V);
export_function u64 Ceil_Pow2_U64(u64 V);
export_function u32 Ceil_U32(f32 Value);
export_function f32 Ceil_F32(f32 Value);
export_function f32 Floor_F32(f32 Value);
export_function f32 FMod_F32(f32 X, f32 Y);
export_function f32 SNorm(s16 Value);
export_function s16 SNorm_S16(f32 Value);
export_function s8 UNorm_To_SNorm(u8 UNorm);
export_function f32 UNorm_To_SNorm_F32(u8 UNorm);
export_function s32 F32_To_S32_Bits(f32 V);
export_function f32 S32_To_F32_Bits(s32 V);
export_function b32 Is_Alpha(char Char);
export_function b32 Is_Newline(char Char);
export_function b32 Is_Whitespace(char Char);
export_function char To_Lower(char Char);
export_function char To_Upper(char Char);
export_function b32 Is_Finite(f32 V);
export_function b32 Is_Nan(f32 V);

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


export_function s64 Pack_S64(v2i Data);
export_function v2i Unpack_S64(s64 PackedData);
export_function s64 Pack_F32_To_S64(v2 Data);
export_function v2 Unpack_S64_To_F32(s64 PackedData);
export_function v2 V2_F64(const f64* V);
export_function v2 V2(f32 x, f32 y);
export_function v2 V2_Zero();
export_function v2 V2_Negate(v2 V);
export_function v2 V2_All(f32 V);
export_function v2 V2_Add_V2(v2 a, v2 b);
export_function v2 V2_Sub_V2(v2 a, v2 b);
export_function v2 V2_Mul_S(v2 A, f32 B);
export_function v2 V2_Mul_V2(v2 A, v2 B);
export_function v2 V2_Min(v2 A, v2 B);
export_function v2 V2_Max(v2 A, v2 B);
export_function v2 V2_Clamp(v2 Min, v2 V, v2 Max);
export_function v2 V2_Saturate(v2 V);
export_function size_t V2_Largest_Index(v2 v);
export_function f32 V2_Dot(v2 A, v2 B);
export_function f32 V2_Sq_Mag(v2 V);
export_function f32 V2_Mag(v2 V);
export_function v2 V2_Norm(v2 V);
export_function v2 V2_Inverse(v2 V);
export_function v2 V2_From_V2i(v2i V);
export_function v2 Circle_To_Square_Mapping(v2 Circle);
export_function v2 Square_To_Circle_Mapping(v2 Square);
export_function v2i V2i(s32 x, s32 y);
export_function v2i V2i_Zero();
export_function v2i V2i_Add_V2i(v2i a, v2i b);
export_function v2i V2i_Sub_V2i(v2i a, v2i b);
export_function v2i V2i_Div_S32(v2i a, s32 b);
export_function b32 V2i_Equal(v2i a, v2i b);

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
	s32 x, y, z;
} v3i;

export_function v3 V3_F64(const f64* V);
export_function v3 V3(f32 x, f32 y, f32 z);
export_function v3 V3_From_V2(v2 V);
export_function v3 V3_Zero();
export_function v3 V3_All(f32 v);
export_function v3 V3_Add_V3(v3 a, v3 b);
export_function v3 V3_Sub_V3(v3 a, v3 b);
export_function v3 V3_Mul_S(v3 v, f32 s);
export_function v3 V3_Mul_V3(v3 a, v3 b);
export_function f32 V3_Dot(v3 a, v3 b);
export_function size_t V3_Largest_Index(v3 v);
export_function f32 V3_Largest(v3 v);
export_function f32 V3_Sq_Mag(v3 v);
export_function f32 V3_Mag(v3 v);
export_function v3 V3_Norm(v3 v);
export_function v3 V3_Negate(v3 v);
export_function v3 V3_Cross(v3 A, v3 B);
export_function v3 V3_Lerp(v3 A, f32 t, v3 B);
export_function b32 V3_Is_Nan(v3 V);
export_function b32 V3_Is_Close(v3 A, v3 B, f32 ToleranceSq);
export_function b32 V3_Is_Zero(v3 V, f32 Epsilon);
export_function v3 V3_Get_Perp(v3 Direction);
export_function v3i V3i(s32 x, s32 y, s32 z);

global const v3 G_XAxis    = { 1.0f, 0.0f, 0.0f };
global const v3 G_YAxis    = { 0.0f, 1.0f, 0.0f };
global const v3 G_ZAxis    = { 0.0f, 0.0f, 1.0f };
global const v3 G_UpVector = { 0.0f, 0.0f, 1.0f };

typedef struct {
	union {
		f32 Data[4];
		struct {
			f32 x, y, z, w;
		};
		v3 xyz;
		struct {
			f32 __unused0__;
			v3 yzw;
		};
	};
} v4;

typedef struct {
	union {
		s32 Data[4];
		struct {
			s32 x, y, z, w;
		};
	};
} v4i;

typedef struct {
	union {
		u32 Data[4];
		struct {
			u32 x, y, z, w;
		};
	};
} v4u;

export_function v4 V4(f32 x, f32 y, f32 z, f32 w);
export_function v4 V4_All(f32 v);
export_function v4 V4_Zero();
export_function v4 V4_From_V3(v3 xyz, f32 w);
export_function f32 V4_Dot(v4 a, v4 b);

typedef struct {
	union {
		f32 Data[4];
		v4  V;
		struct { f32 x, y, z, w; };
		struct { v3 v; f32 s; };
	};
} quat;

export_function quat Quat(f32 x, f32 y, f32 z, f32 w);
export_function quat Quat_Identity();
export_function quat Quat_From_V3_S(v3 v, f32 s);
export_function quat Quat_From_Euler(v3 Euler);
export_function v3 	 Euler_From_Quat(quat q);
export_function quat Quat_Axis_Angle(v3 Axis, f32 Angle);
export_function quat Quat_RotX(f32 Pitch);
export_function quat Quat_RotY(f32 Yaw);
export_function quat Quat_RotZ(f32 Roll);
export_function quat Quat_Add_Quat(quat A, quat B);
export_function quat Quat_Mul_S(quat A, f32 B);
export_function quat Quat_Mul_Quat(quat a, quat b);
export_function f32 Quat_Dot(quat A, quat B);
export_function f32 Quat_Sq_Mag(quat V);
export_function f32 Quat_Mag(quat V);
export_function quat Quat_Norm(quat V);
export_function quat Quat_Lerp(quat A, f32 t, quat B);
export_function b32 Quat_Is_Nan(quat Q);

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

typedef struct m4 m4;
export_function m3 M3_From_M4(const m4* M);
export_function m3 M3_Identity();
export_function m3 M3_XYZ(v3 x, v3 y, v3 z);
export_function m3 M3_From_Quat(quat q);
export_function m3 M3_Axis_Angle(v3 Axis, f32 Angle);
export_function m3 M3_Transpose(const m3* M);
export_function m3 M3_Inverse(const m3* M);
export_function m3 M3_Mul_S(const m3* M, f32 S);
export_function m3 M3_Mul_M3(const m3* A, const m3* B);
export_function v3 M3_Diagonal(const m3* M);
export_function m3 M3_Scale(v3 S);
export_function m3 M3_Basis(v3 Direction);
export_function v3 V3_Mul_M3(v3 A, const m3* B);

struct m4 {
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
};

export_function m4 M4_F32(const f32* Data);
export_function m4 M4_Identity();
export_function m4 M4_Transform(v3 x, v3 y, v3 z, v3 t);
export_function m4 M4_Transform_Scale(v3 x, v3 y, v3 z, v3 t, v3 s);
export_function m4 M4_Transform_Scale_Quat(v3 t, v3 s, quat Q);
export_function m4 M4_From_M3(const m3* M);
export_function m4 M4_Inv_Transform(v3 x, v3 y, v3 z, v3 t);
export_function m4 M4_Look_At(v3 Position, v3 Target);
export_function m4 M4_Transpose(const m4* M);
export_function m4 M4_Mul_M4(const m4* A, const m4* B);
export_function m4 M4_Perspective(f32 FOV, f32 AspectRatio, f32 ZNear, f32 ZFar);

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

export_function m4_affine M4_Affine_F32(const f32* Matrix);
export_function m4_affine M4_Affine_F64(const f64* Matrix);
export_function m4_affine M4_Affine_Transform_No_Scale(v3 t, const m3* M);
export_function m4_affine M4_Affine_Transform_Quat_No_Scale(v3 T, quat Q);
export_function m4_affine M4_Affine_Identity();
export_function m4_affine_transposed M4_Affine_Transpose(const m4_affine* M);
export_function v3 V4_Mul_M4_Affine(v4 A, const m4_affine* B);
export_function m4_affine M4_Affine_Mul_M4_Affine(const m4_affine* A, const m4_affine* B);
export_function m4_affine M4_Affine_Inverse_No_Scale(const m4_affine* M);
export_function m4_affine M4_Affine_Inverse_Transform_No_Scale(v3 T, const m3* M);
export_function m4_affine M4_Affine_Inverse_Transform_Quat_No_Scale(v3 T, quat Q);

typedef struct {
	v2 p0;
	v2 p1;
} rect2;

export_function rect2 Rect2(v2 p0, v2 p1);
export_function rect2 Rect2_Offset(rect2 Rect, v2 Offset);
export_function b32 Rect2_Contains_V2(rect2 Rect, v2 V);
export_function v2 Rect2_Size(rect2 Rect);

export_function v3 RGB_To_HSV(v3 rgb);
export_function v3 HSV_To_RGB(v3 hsv);
export_function v3 Get_Triangle_Centroid(v3 P0, v3 P1, v3 P2);

typedef struct {
	u8*    BaseAddress;
	size_t ReserveSize;
	size_t CommitSize;
} memory_reserve;

export_function memory_reserve Make_Memory_Reserve(size_t Size);
export_function void Delete_Memory_Reserve(memory_reserve* Reserve);
export_function memory_reserve Subdivide_Memory_Reserve(memory_reserve* Reserve, size_t Offset, size_t Size);
export_function inline b32 Reserve_Is_Valid(memory_reserve* Reserve);
export_function void* Commit_New_Size(memory_reserve* Reserve, size_t NewSize);
export_function void Decommit_New_Size(memory_reserve* Reserve, size_t NewSize);
export_function b32 Commit_All(memory_reserve* Reserve);

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

export_function allocator* Default_Allocator_Get();

#define Allocator_Allocate_Memory(allocator, size) (allocator)->VTable->AllocateMemoryFunc(allocator, size, CLEAR_FLAG_YES)
#define Allocator_Free_Memory(allocator, memory) (allocator)->VTable->FreeMemoryFunc(allocator, memory)

#define Allocator_Allocate_Struct(allocator, type) (type *)Allocator_Allocate_Memory(allocator, sizeof(type))
#define Allocator_Allocate_Array(allocator, count, type) (type *)Allocator_Allocate_Memory(allocator, sizeof(type)*(count))

typedef struct arena_block arena_block;
struct arena_block {
	u8*    Memory;
	size_t Used;
	size_t Size;
	arena_block* Next;
};

typedef enum {
	ARENA_TYPE_VIRTUAL,
	ARENA_TYPE_ALLOCATOR
} arena_type;

typedef struct {
	allocator  Base;
	arena_type Type;
	union {
		struct {
			memory_reserve Reserve;
			size_t Used;
		};

		struct {
			allocator*   Allocator;
			arena_block* FirstBlock;
			arena_block* LastBlock;
			arena_block* CurrentBlock;
		};
	};
} arena;

typedef struct {
	arena* 		 Arena;
	arena_block* Block;
	size_t 		 Used;
} arena_marker;

export_function arena* Arena_Create_With_Size(size_t ReserveSize);
export_function arena* Arena_Create();
export_function arena* Arena_Create_With_Allocator(allocator* Allocator);
export_function void Arena_Delete(arena* Arena);
export_function void* Arena_Push_Aligned_No_Clear(arena* Arena, size_t Size, size_t Alignment);
export_function void* Arena_Push_Aligned(arena* Arena, size_t Size, size_t Alignment);
export_function void* Arena_Push_No_Clear(arena* Arena, size_t Size);
export_function void* Arena_Push(arena* Arena, size_t Size);
export_function arena_marker Arena_Get_Marker(arena* Arena);
export_function void Arena_Set_Marker(arena* Arena, arena_marker Marker);
export_function void Arena_Clear(arena* Arena);

#define Arena_Push_Struct(arena, type) (type *)Arena_Push(arena, sizeof(type))
#define Arena_Push_Array(arena, count, type) (type *)Arena_Push(arena, sizeof(type)*(count))
#define Arena_Push_Struct_No_Clear(arena, type) (type *)Arena_Push_No_Clear(arena, sizeof(type))
#define Arena_Push_Array_No_Clear(arena, count, type) (type *)Arena_Push_No_Clear(arena, sizeof(type)*(count))

typedef struct heap heap;
export_function heap* Heap_Create();
export_function void  Heap_Delete(heap* Heap);
export_function void* Heap_Alloc_Aligned_No_Clear(heap* Heap, size_t Size, size_t Alignment);
export_function void* Heap_Alloc_Aligned(heap* Heap, size_t Size, size_t Alignment);
export_function void* Heap_Alloc_No_Clear(heap* Heap, size_t Size);
export_function void* Heap_Alloc(heap* Heap, size_t Size);
export_function void  Heap_Free(heap* Heap, void* Memory);
export_function void  Heap_Clear(heap* Heap);

#define Array_Define(type) \
typedef struct { \
type*  Ptr; \
size_t Count; \
} type##_array;

#define Dynamic_Array_Define(type, name) \
typedef struct { \
allocator* Allocator; \
type* Ptr; \
size_t Count; \
size_t Capacity; \
} dynamic_##name##_array

#define Array_Implement(type, name) \
function type##_array name##_Array_Init(type* Ptr, size_t Count) { \
type##_array Result = { \
.Ptr = Ptr, \
.Count = Count \
}; \
return Result; \
} \
function type##_array name##_Array_Alloc(allocator* Allocator, size_t Count) { \
type* Ptr = Allocator_Allocate_Array(Allocator, Count, type); \
return name##_Array_Init(Ptr, Count); \
} \
function type##_array name##_Array_Copy(allocator* Allocator, type* Ptr, size_t Count) { \
type##_array Result = name##_Array_Alloc(Allocator, Count); \
Memory_Copy(Result.Ptr, Ptr, Count * sizeof(type)); \
return Result; \
} \
function type##_array name##_Array_Sub(type##_array Array, size_t StartIndex, size_t EndIndex) { \
Assert(StartIndex <= EndIndex); \
Assert(EndIndex <= Array.Count); \
type##_array Result = name##_Array_Init(Array.Ptr + StartIndex, EndIndex-StartIndex); \
return Result; \
} \
function type##_array name##_Array_Empty() { \
type##_array Result = { 0 }; \
return Result; \
} \

#define Dynamic_Array_Implement(container_name, type, name) \
function void Dynamic_##name##_Array_Reserve(dynamic_##container_name##_array* Array, size_t NewCapacity) { \
allocator* Allocator = Array->Allocator; \
type* NewPtr = Allocator_Allocate_Array(Allocator, NewCapacity, type); \
\
if (Array->Ptr) { \
size_t MinCapacity = Min(Array->Capacity, NewCapacity); \
Memory_Copy(NewPtr, Array->Ptr, MinCapacity * sizeof(type)); \
Allocator_Free_Memory(Allocator, Array->Ptr); \
} \
\
if (Array->Count > NewCapacity) { \
Array->Count = NewCapacity; \
} \
\
Array->Capacity = NewCapacity; \
Array->Ptr = NewPtr; \
} \
function dynamic_##container_name##_array Dynamic_##name##_Array_Init_With_Size(allocator* Allocator, size_t InitialCapacity) { \
dynamic_##container_name##_array Result = { \
.Allocator = Allocator \
}; \
\
Dynamic_##name##_Array_Reserve(&Result, InitialCapacity); \
return Result; \
} \
function dynamic_##container_name##_array Dynamic_##name##_Array_Init(allocator* Allocator) { \
return Dynamic_##name##_Array_Init_With_Size(Allocator, 32); \
} \
function void Dynamic_##name##_Array_Add(dynamic_##container_name##_array* Array, type Entry) { \
if (Array->Count == Array->Capacity) { \
Dynamic_##name##_Array_Reserve(Array, Array->Capacity*2); \
} \
Array->Ptr[Array->Count++] = Entry; \
} \
function void Dynamic_##name##_Array_Add_Range(dynamic_##container_name##_array* Array, type* Ptr, size_t Count) { \
if (Array->Count+Count >= Array->Capacity) { \
Dynamic_##name##_Array_Reserve(Array, Max(Array->Capacity*2, Array->Count+Count)); \
} \
Memory_Copy(Array->Ptr + Array->Count, Ptr, Count * sizeof(type)); \
Array->Count += Count; \
} \
function void Dynamic_##name##_Array_Clear(dynamic_##container_name##_array* Array) { \
Array->Count = 0; \
}

#define Dynamic_Array_Define_Type(type) Dynamic_Array_Define(type, type)
#define Dynamic_Array_Define_Ptr(type) Dynamic_Array_Define(type*, type)

#define Dynamic_Array_Implement_Type(type, name) Dynamic_Array_Implement(type, type, name)
#define Dynamic_Array_Implement_Ptr(type, name) Dynamic_Array_Implement(type, type*, name)

Array_Define(char);
Array_Define(u32);

Dynamic_Array_Define_Type(char);
Dynamic_Array_Define(char*, char_ptr);

typedef struct {
	u32 State;
} random32_xor_shift;

#define MAX_SCRATCH_COUNT 64
typedef struct {
	random32_xor_shift Random32;
	size_t 		 	   ScratchIndex;
	arena* 		 	   ScratchArenas[MAX_SCRATCH_COUNT];
	arena_marker 	   ScratchMarkers[MAX_SCRATCH_COUNT];
} thread_context;

export_function thread_context* Thread_Context_Get();
export_function arena* Scratch_Get();
export_function void Scratch_Release();

#ifdef DEBUG_BUILD
export_function void Thread_Context_Validate_();
#define Thread_Context_Validate Thread_Context_Validate_
#else
#define Thread_Context_Validate
#endif


typedef struct {
	u8*    Ptr;
	size_t Size;
} buffer;

typedef struct {
	const char* Ptr;
	size_t Size;
} string;

export_function buffer Make_Buffer(void* Data, size_t Size);
export_function buffer Buffer_From_String(string String);
export_function buffer Buffer_Empty();
export_function buffer Buffer_Alloc(allocator* Allocator, size_t Size);
export_function b32 Buffer_Is_Empty(buffer Buffer);

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

Array_Define(string);
Dynamic_Array_Define_Type(string);

#define STRING_INVALID_INDEX ((size_t)-1)
export_function size_t String_Length(const char* Ptr);
export_function string String_Empty();
export_function string Make_String(const char* Ptr, size_t Size);
export_function string String_Null_Term(const char* Ptr);
export_function string String_FormatV(allocator* Allocator, const char* Format, va_list Args);
export_function string String_Format(allocator* Allocator, const char* Format, ...);
export_function string String_From_Buffer(buffer Buffer);
export_function string String_From_Bool(b32 Bool);
export_function string String_From_F64(allocator* Allocator, f64 Number);
export_function string String_Copy(allocator* Allocator, string Str);
export_function string String_Substr(string Str, size_t FirstIndex, size_t LastIndex);
export_function string_array String_Split(allocator* Allocator, string String, string Substr);
export_function b32 String_Is_Empty(string String);
export_function b32 String_Is_Null_Term(string String);
export_function b32 String_Equals(string StringA, string StringB);
export_function size_t String_Find_First_Char(string String, char Character);
export_function size_t String_Find_Last_Char(string String, char Character);
export_function string String_Combine(allocator* Allocator, string* Strings, size_t Count);
export_function string String_Concat(allocator* Allocator, string StringA, string StringB);
export_function size_t String_Get_Last_Directory_Slash_Index(string Directory);
export_function string String_Get_Directory_Path(string Path);
export_function string String_Directory_Concat(allocator* Allocator, string StringA, string StringB);
export_function b32 String_Starts_With_Char(string String, char Character);
export_function b32 String_Ends_With_Char(string String, char Character);
export_function string String_Get_Filename_Ext(string Filename);
export_function string String_Get_Filename_Without_Ext(string Filename);
export_function string String_Upper_Case(allocator* Allocator, string String);
export_function string String_Lower_Case(allocator* Allocator, string String);
export_function string String_Pascal_Case(allocator* Allocator, string String);
export_function string String_Trim_Whitespace(string String);
export_function b32 String_Is_Whitespace(string String);
export_function string String_Insert_Text(allocator* Allocator, string String, string TextToInsert, size_t Cursor);
export_function string String_Remove(allocator* Allocator, string String, size_t Cursor, size_t Size);
export_function b32 String_Ends_With(string String, string Substr);
export_function b32 String_Contains(string String, string Substr);
export_function size_t String_Find_First(string String, string Substr);
export_function string String_From_WString(allocator* Allocator, wstring WString);
export_function b32 Try_Parse_Bool(string String, b32* OutBool);
export_function b32 Try_Parse_F64(string String, f64* OutNumber);
export_function b32 Try_Parse_S64(string String, s64* OutNumber);
export_function size_t WString_Length(const wchar_t* Ptr);
export_function wstring Make_WString(const wchar_t* Ptr, size_t Size);
export_function wstring WString_Null_Term(const wchar_t* Ptr);
export_function wstring WString_Copy(allocator* Allocator, wstring WStr);
export_function wstring WString_From_String(allocator* Allocator, string String);

#define String_Lit(str) Make_String(str, sizeof(str)-1)
#define String_Expand(str) { str, sizeof(str)-1 }

typedef struct {
	const char* Start;
	const char* End;
	const char* At;
	size_t 		LineIndex;
	size_t 		ColumnIndex;
} sstream_reader;

typedef struct {
	char   Char;
	size_t Index;
	size_t LineIndex;
	size_t ColumnIndex;
} sstream_char;

export_function sstream_reader SStream_Reader_Begin(string Content);
export_function b32 SStream_Reader_Is_Valid(sstream_reader* Reader);
export_function size_t SStream_Reader_Get_Index(sstream_reader* Reader);
export_function sstream_char SStream_Reader_Peek_Char(sstream_reader* Reader);
export_function sstream_char SStream_Reader_Consume_Char(sstream_reader* Reader);
export_function void SStream_Reader_Eat_Whitespace(sstream_reader* Reader);
export_function void SStream_Reader_Eat_Whitespace_Without_Line(sstream_reader* Reader);
export_function string SStream_Reader_Consume_Token(sstream_reader* Reader);
export_function string SStream_Reader_Peek_Line(sstream_reader* Reader);
export_function string SStream_Reader_Consume_Line(sstream_reader* Reader);
export_function void SStream_Reader_Skip_Line(sstream_reader* Reader);

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

export_function sstream_writer Begin_Stream_Writer(allocator* Allocator);
export_function void SStream_Writer_Add_Front(sstream_writer* Writer, string Entry);
export_function void SStream_Writer_Add(sstream_writer* Writer, string Entry);
export_function void SStream_Writer_Add_Format(sstream_writer* Writer, const char* Format, ...);
export_function string SStream_Writer_Join(sstream_writer* Writer, allocator* Allocator, string JoinChars);

function inline void SStream_Writer_Add_Char(sstream_writer* Writer, char Char) {
	SStream_Writer_Add(Writer, Make_String(&Char, 1));
}

function inline void SStream_Writer_Add_Newline(sstream_writer* Writer) {
	SStream_Writer_Add_Char(Writer, '\n');
}

function inline void SStream_Writer_Add_Tab(sstream_writer* Writer) {
	SStream_Writer_Add_Char(Writer, '\t');
}

function inline void SStream_Writer_Add_Space(sstream_writer* Writer) {
	SStream_Writer_Add_Char(Writer, ' ');
}

typedef struct {
	const u8* Start;
	const u8* End;
	const u8* At;
} bstream_reader;

export_function bstream_reader BStream_Reader_Begin(buffer Buffer);
export_function b32 BStream_Reader_Is_Valid(bstream_reader* Reader);
export_function const void* BStream_Reader_Size(bstream_reader* Reader, size_t Size);
export_function u32 BStream_Reader_U32(bstream_reader* Reader);
#define BStream_Reader_Struct(reader, type) *(const type*)BStream_Reader_Size(reader, sizeof(type))

#define HASH_INVALID_SLOT (u32)-1
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

export_function hashmap Hashmap_Init_With_Size(allocator* Allocator, size_t ValueSize, size_t KeySize, u32 ItemCapacity, key_hash_func* HashFunc, key_comp_func* CompareFunc);
export_function hashmap Hashmap_Init(allocator* Allocator, size_t ItemSize, size_t KeySize, key_hash_func* HashFunc, key_comp_func* CompareFunc);
export_function void Hashmap_Delete(hashmap* Hashmap);
export_function b32 Hashmap_Is_Valid(hashmap* Hashmap);
export_function void Hashmap_Clear(hashmap* Hashmap);
export_function u32 Hashmap_Find_Slot(hashmap* Hashmap, void* Key, u32 Hash);
export_function void Hashmap_Add_By_Hash(hashmap* Hashmap, void* Key, void* Value, u32 Hash);
export_function u32 Hashmap_Hash(hashmap* Hashmap, void* Key);
export_function void Hashmap_Add(hashmap* Hashmap, void* Key, void* Value);
export_function b32 Hashmap_Find_By_Hash(hashmap* Hashmap, void* Key, void* Value, u32 Hash);
export_function b32 Hashmap_Find(hashmap* Hashmap, void* Key, void* Value);
export_function b32 Hashmap_Get_Value(hashmap* Hashmap, size_t Index, void* Value);
export_function b32 Hashmap_Get_Key(hashmap* Hashmap, size_t Index, void* Key);
export_function b32 Hashmap_Get_Key_Value(hashmap* Hashmap, size_t Index, void* Key, void* Value);

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
	allocator* Allocator;
	u32*   	   Slots;
	u32*   	   FreeIndices;
	size_t 	   FreeCount;
	size_t 	   Capacity;
	size_t 	   Count;
} slot_map;

function inline slot_id Slot_Null() {
	slot_id Result = { .ID = 0 };
	return Result;
}

function inline b32 Slot_Is_Null(slot_id SlotID) {
	return SlotID.ID == 0;
}

export_function void Slot_Map_Clear(slot_map* SlotMap);
export_function slot_map Slot_Map_Init(allocator* Allocator, size_t Capacity);
export_function void Slot_Map_Delete(slot_map* SlotMap);
export_function slot_id Slot_Map_Allocate(slot_map* SlotMap);
export_function void Slot_Map_Free(slot_map* SlotMap, slot_id ID);
export_function b32 Slot_Map_Is_Allocated(slot_map* SlotMap, slot_id SlotID);
export_function b32 Slot_Map_Is_Allocated_Index(slot_map* SlotMap, size_t Index);
export_function slot_id Slot_Map_Get_ID(slot_map* SlotMap, size_t Index);

#define BINARY_HEAP_COMPARE_FUNC(name) b32 name(void* A, void* B)
typedef BINARY_HEAP_COMPARE_FUNC(binary_heap_compare_func);

typedef struct {
	binary_heap_compare_func* CompareFunc;
	size_t Capacity;
	size_t Count;
	size_t ItemSize;
	u8*    Ptr;
} binary_heap;

export_function b32 Binary_Heap_Is_Empty(binary_heap* Heap);
export_function binary_heap Binary_Heap_Init(void* Ptr, size_t Capacity, size_t ItemSize, binary_heap_compare_func* CompareFunc);
export_function binary_heap Binary_Heap_Alloc(arena* Arena, size_t Capacity, size_t ItemSize, binary_heap_compare_func* CompareFunc);
export_function void  Binary_Heap_Push(binary_heap* Heap, void* Data);
export_function void* Binary_Heap_Pop(binary_heap* Heap);

#define ASYNC_STACK_INDEX16_INVALID ((u16)-1) 

typedef union {
	u32 ID;
	struct {
		u16 Index;
		u16 Key;
	};
} async_stack_index16_key;

typedef struct {
	u16*  	   NextIndices;
	atomic_u32 Head;
	u16   	   Capacity;
} async_stack_index16;

export_function void Async_Stack_Index16_Init_Raw(async_stack_index16* StackIndex, u16* IndicesPtr, u16 Capacity);
export_function void Async_Stack_Index16_Push_Sync(async_stack_index16* StackIndex, u16 Index);
export_function void Async_Stack_Index16_Push(async_stack_index16* StackIndex, u16 Index);
export_function u16  Async_Stack_Index16_Pop(async_stack_index16* StackIndex);

#define ASYNC_STACK_INDEX32_INVALID ((u32)-1) 

typedef union {
	u64 ID;
	struct {
		u32 Index;
		u32 Key;
	};
} async_stack_index32_key;

typedef struct {
	u32*  	   NextIndices;
	atomic_u64 Head;
	u32   	   Capacity;
} async_stack_index32;

export_function void Async_Stack_Index32_Init_Raw(async_stack_index32* StackIndex, u32* IndicesPtr, u32 Capacity);
export_function void Async_Stack_Index32_Push_Sync(async_stack_index32* StackIndex, u32 Index);
export_function void Async_Stack_Index32_Push(async_stack_index32* StackIndex, u32 Index);
export_function u32  Async_Stack_Index32_Pop(async_stack_index32* StackIndex);

#include "os/os_base.h"

export_function void Debug_Log(const char* Format, ...);
export_function u32 U32_Hash_U64(u64 Value);
export_function u32 U32_Hash_String(string String);
export_function u64 U64_Hash_String_With_Seed(string String, u64 Seed);
export_function u32 U32_Hash_Bytes(void* Data, size_t Size);
export_function u32 U32_Hash_Bytes_With_Seed(void* Data, size_t Size, u32 Seed);
export_function u64 U64_Hash_Bytes(void* Data, size_t Size);
export_function u32 U32_Hash_Ptr(void* Ptr);
export_function u32 U32_Hash_Ptr_With_Seed(void* Ptr, u32 Seed);
export_function u32 Hash_String(void* A);
export_function b32 Compare_String(void* A, void* B);
export_function buffer Read_Entire_File(allocator* Allocator, string Path);
export_function b32 Write_Entire_File(string Path, buffer Data);
#define Read_Entire_File_Str(allocator, path) String_From_Buffer(Read_Entire_File(allocator, path))
#define Write_Entire_File_Str(path, content) Write_Entire_File(path, Buffer_From_String(content))

typedef struct {
	os_tls*   ThreadContextTLS;
	allocator DefaultAllocator;
	os_base*  OSBase;
	allocator_vtable* ArenaVTable;
	allocator_vtable* HeapVTable;
} base;

export_function void Base_Set(base* Base);
export_function base* Base_Get();
export_function base* Base_Init();

#include "akon.h"
#include "job.h"
#include "profiler.h"

#ifdef __cplusplus
}
#endif

#endif