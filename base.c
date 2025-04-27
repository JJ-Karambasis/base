#define STB_SPRINTF_STATIC
#define STB_SPRINTF_IMPLEMENTATION
#include <third_party/stb/stb_sprintf.h>

#define XXH_STATIC_LINKING_ONLY
#define XXH_IMPLEMENTATION
#define XXH_PRIVATE_API
#include <third_party/xxHash/xxhash.h>

#include "atomic/atomic.c"

global base* G_Base;

void OS_Base_Init(base* Base);

function void Base_Set(base* Base) {
	G_Base = Base;
}

function base* Base_Get() {
	return G_Base;
}

function allocator* Default_Allocator_Get() {
	return &Base_Get()->DefaultAllocator;
}

function ALLOCATOR_ALLOCATE_MEMORY_DEFINE(Default_Allocate_Memory) {
	void* Result = malloc(Size);
	if (Result && ClearFlag == CLEAR_FLAG_YES) Memory_Clear(Result, Size);
	return Result;
}

function ALLOCATOR_FREE_MEMORY_DEFINE(Default_Free_Memory) {
	if (Memory) {
		free(Memory);
	}
}	

global allocator_vtable Default_Allocator_VTable = {
	.AllocateMemoryFunc = Default_Allocate_Memory,
	.FreeMemoryFunc = Default_Free_Memory
};

function ALLOCATOR_ALLOCATE_MEMORY_DEFINE(Arena_Allocate_Memory);
function ALLOCATOR_FREE_MEMORY_DEFINE(Arena_Free_Memory);
global allocator_vtable Arena_VTable = {
	.AllocateMemoryFunc = Arena_Allocate_Memory,
	.FreeMemoryFunc = Arena_Free_Memory
};


function base* Base_Init() {
	local base Base;
	Base.DefaultAllocator.VTable = &Default_Allocator_VTable;
	Base.ArenaVTable = &Arena_VTable;

	Base_Set(&Base);
	OS_Base_Init(&Base);

	return &Base;
}


function inline b32 Equal_Zero_Approx_F32(f32 Value, f32 Epsilon) {
    return Abs(Value) <= Epsilon;
}

function inline b32 Equal_Zero_Eps_F32(f32 Value) {
    return Equal_Zero_Approx_F32(Value, FLT_EPSILON);
}

function inline b32 Equal_Zero_Eps_Sq_F32(f32 SqValue) {
    return Equal_Zero_Approx_F32(SqValue, Sq(FLT_EPSILON));
}

function inline f32 Safe_Ratio(s32 x, s32 y) {
	Assert(y != 0);
	return (f32)x / (f32)y;
}

function inline f32 Sqrt_F32(f32 Value) {
	return sqrtf(Value);
}

function inline f32 Cos_F32(f32 Value) {
	return cosf(Value);
}

function inline f32 Sin_F32(f32 Value) {
	return sinf(Value);
}

function inline f32 Tan_F32(f32 Value) {
	return tanf(Value);
}

function inline size_t Align(size_t Value, size_t Alignment) {
    size_t Remainder = Value % Alignment;
    return Remainder ? Value + (Alignment-Remainder) : Value;
}

function u32 Ceil_Pow2_U32(u32 V) {
    V--;
    V |= V >> 1;
    V |= V >> 2;
    V |= V >> 4;
    V |= V >> 8;
    V |= V >> 16;
    V++;
    return V;
}

function u64 Ceil_Pow2_U64(u64 V) {
    V--;
    V |= V >> 1;
    V |= V >> 2;
    V |= V >> 4;
    V |= V >> 8;
    V |= V >> 16;
    V |= V >> 32;
    V++;
    return V;
}

function inline u32 Ceil_U32(f32 Value) {
	return (u32)ceilf(Value);
}

function inline f32 Ceil_F32(f32 Value) {
	return ceilf(Value);
}

function inline f32 Floor_F32(f32 Value) {
	return floorf(Value);
}

function inline f32 FMod_F32(f32 X, f32 Y) {
	return fmodf(X, Y);
}

function inline f32 SNorm(s16 Value) {
    return Clamp(-1.0f, (f32)Value / (f32)((1 << 15) - 1), 1.0f);
}

function inline s16 SNorm_S16(f32 Value) {
    s16 Result = (s16)(Clamp(-1.0f, Value, 1.0f) * (f32)(1 << 15));
    return Result;
}

function inline s8 UNorm_To_SNorm(u8 UNorm) {
	return (s8)((s32)UNorm - 128);
}

function inline f32 UNorm_To_SNorm_F32(u8 UNorm) {
	return ((f32)UNorm / 128.0f) - 1.0f;
}

function inline s32 F32_To_S32_Bits(f32 V) {
	s32 Result = *(s32 *)&V;
	return Result;
}

function inline f32 S32_To_F32_Bits(s32 V) {
	f32 Result = *(f32 *)&V;
	return Result;
}

function inline b32 Is_Alpha(char Char) {
	return isalpha(Char);
}

function inline b32 Is_Newline(char Char) {
	return Char == '\n';
}

function inline b32 Is_Whitespace(char Char) {
	return isspace(Char);
}

function inline char To_Lower(char Char) {
	return (char)tolower(Char);
}

function inline char To_Upper(char Char) {
	return (char)toupper(Char);
}

function inline b32 Is_Finite(f32 V) {
	return isfinite(V);
}

function inline b32 Is_Nan(f32 V) {
	return isnan(V);
}

function v2i V2i(s32 x, s32 y);
function v2 V2(f32 x, f32 y);

function s64 Pack_S64(v2i Data) {
	s64 PackedX = ((s64)Data.x) & 0xFFFFFFFFLL;
    s64 PackedY = ((s64)Data.y) & 0xFFFFFFFFLL;
	s64 Result = PackedX | (PackedY << 32);
	return Result;
}

function v2i Unpack_S64(s64 PackedData) {
	s32 X = (s32)PackedData;
	s32 Y = (s32)(PackedData >> 32);
	return V2i(X, Y);
}

function s64 Pack_F32_To_S64(v2 Data) {
	s32 X = F32_To_S32_Bits(Data.x);
	s32 Y = F32_To_S32_Bits(Data.y);
	return Pack_S64(V2i(X, Y));
}

function v2 Unpack_S64_To_F32(s64 PackedData) {
	v2i Data = Unpack_S64(PackedData);
	f32 X = S32_To_F32_Bits(Data.x);
	f32 Y = S32_To_F32_Bits(Data.y);
	return V2(X, Y);
}

function inline v2 V2_F64(const f64* V) {
	v2 Result = { (f32)V[0], (f32)V[1] };
	return Result;
}

function inline v2 V2(f32 x, f32 y) {
	v2 Result = { x, y };
	return Result;
}

function inline v2 V2_Zero() {
	v2 Result = V2(0, 0);
	return Result;
}

function inline v2 V2_Negate(v2 V) {
	v2 Result = V2(-V.x, -V.y);
	return Result;
}

function inline v2 V2_All(f32 V) {
	v2 Result = V2(V, V);
	return Result;
}

function inline v2 V2_Add_V2(v2 a, v2 b) {
	v2 Result = { a.x + b.x, a.y + b.y };
	return Result;
}

function inline v2 V2_Sub_V2(v2 a, v2 b) {
	v2 Result = { a.x - b.x, a.y - b.y };
	return Result;
}

function inline v2 V2_Mul_S(v2 A, f32 B) {
	v2 Result = V2(A.x * B, A.y * B);
	return Result;
}

function inline v2 V2_Mul_V2(v2 A, v2 B) {
	v2 Result = V2(A.x * B.x, A.y * B.y);
	return Result;
}

function inline v2 V2_Min(v2 A, v2 B) {
	v2 Result = V2(Min(A.x, B.x), Min(A.y, B.y));
	return Result;
}

function inline v2 V2_Max(v2 A, v2 B) {
	v2 Result = V2(Max(A.x, B.x), Max(A.y, B.y));
	return Result;
}

function inline v2 V2_Clamp(v2 Min, v2 V, v2 Max) {
	v2 Result = V2_Max(Min, V2_Min(V, Max));
	return Result;
}

function inline v2 V2_Saturate(v2 V) {
	v2 Result = V2_Clamp(V2_Zero(), V, V2_All(1.0f));
	return Result;
}

function inline size_t V2_Largest_Index(v2 v) {
	return v.Data[0] > v.Data[1] ? 0 : 1;
}

function inline f32 V2_Dot(v2 A, v2 B) {
	f32 Result = A.x * B.x + A.y * B.y;
	return Result;
}

function inline f32 V2_Sq_Mag(v2 V) {
	f32 Result = V2_Dot(V, V);
	return Result;
}

function inline f32 V2_Mag(v2 V) {
	f32 Result = Sqrt_F32(V2_Sq_Mag(V));
	return Result;
}

function inline v2 V2_Norm(v2 V) {
	f32 SqLength = V2_Sq_Mag(V);
	if (Equal_Zero_Eps_Sq_F32(SqLength)) {
		return V2_Zero();
	}

	f32 InvLength = 1.0f / Sqrt_F32(SqLength);
	return V2_Mul_S(V, InvLength);
}

function inline v2 V2_Inverse(v2 V) {
	v2 Result = V2(V.x == 0.0f ? 0 : 1.0f / V.x, 
				   V.y == 0.0f ? 0 : 1.0f / V.y);
	return Result;
}

function inline v2 V2_From_V2i(v2i V) {
	v2 Result = V2((f32)V.x, (f32)V.y);
	return Result;
}

function v2 Circle_To_Square_Mapping(v2 Circle) {
	f32 u = Circle.x;
	f32 v = Circle.y;

	f32 uSq = u * u;
    f32 vSq = v * v;
    f32 twosqrt2 = 2.0f * Sqrt_F32(2.0f);
    f32 subtermx = 2.0f + uSq - vSq;
    f32 subtermy = 2.0f - uSq + vSq;
    f32 termx1 = subtermx + u * twosqrt2;
    f32 termx2 = subtermx - u * twosqrt2;
    f32 termy1 = subtermy + v * twosqrt2;
    f32 termy2 = subtermy - v * twosqrt2;

	if (Equal_Zero_Eps_F32(termx2)) termx2 = 0.0f;
	if (Equal_Zero_Eps_F32(termy2)) termy2 = 0.0f;

	v2 Result = V2(0.5f * Sqrt_F32(termx1) - 0.5f * Sqrt_F32(termx2),
				   0.5f * Sqrt_F32(termy1) - 0.5f * Sqrt_F32(termy2));
	return Result;
}

function v2 Square_To_Circle_Mapping(v2 Square) {
	f32 x = Square.x;
	f32 y = Square.y;
	v2 Result = V2(x * Sqrt_F32(1.0f - y*y / 2.0f), 
				   y * Sqrt_F32(1.0f - x*x / 2.0f));
	return Result;
}

function inline v2i V2i(s32 x, s32 y) {
	v2i Result = { x, y };
	return Result;
}

function inline v2i V2i_Zero() {
	v2i Result = V2i(0, 0);
	return Result;
}

function inline v2i V2i_Sub_V2i(v2i a, v2i b) {
	v2i Result = { a.x - b.x, a.y - b.y };
	return Result;
}

function inline v2i V2i_Div_S32(v2i a, s32 b) {
	v2i Result = { a.x / b, a.y / b };
	return Result;
}

function inline b32 V2i_Equal(v2i a, v2i b) {
	b32 Result = a.x == b.x && a.y == b.y;
	return Result;
}

function inline v3 V3_F64(const f64* V) {
	v3 Result = { (f32)V[0], (f32)V[1], (f32)V[2] };
	return Result;
}

function inline v3 V3(f32 x, f32 y, f32 z) {
	v3 Result = { x, y, z };
	return Result;
}

function inline v3 V3_From_V2(v2 V) {
	v3 Result = { V.x, V.y, 0 };
	return Result;
}

function inline v3 V3_Zero() {
	v3 Result = V3(0, 0, 0);
	return Result;
}

function inline v3 V3_All(f32 v) {
	v3 Result = V3(v, v, v);
	return Result;
}

function inline v3 V3_Add_V3(v3 a, v3 b) {
	v3 Result = V3(a.x + b.x, a.y + b.y, a.z + b.z);
	return Result;
}

function inline v3 V3_Sub_V3(v3 a, v3 b) {
	v3 Result = V3(a.x - b.x, a.y - b.y, a.z - b.z);
	return Result;
}

function inline v3 V3_Mul_S(v3 v, f32 s) {
	v3 Result = V3(v.x * s, v.y * s, v.z * s);
	return Result;
}

function inline v3 V3_Mul_V3(v3 a, v3 b) {
	v3 Result = V3(a.x * b.x, a.y * b.y, a.z * b.z);
	return Result;
}

function inline f32 V3_Dot(v3 a, v3 b) {
	f32 Result = a.x * b.x + a.y * b.y + a.z * b.z;
	return Result;
}

function inline size_t V3_Largest_Index(v3 v) {
	size_t LargestIndex = V2_Largest_Index(v.xy);
	return v.Data[LargestIndex] > v.Data[2] ? LargestIndex : 2;
}

function inline f32 V3_Largest(v3 v) {
	return v.Data[V3_Largest_Index(v)];
}

function inline f32 V3_Sq_Mag(v3 v) {
	f32 Result = V3_Dot(v, v);
	return Result;
}

function inline f32 V3_Mag(v3 v) {
	f32 Result = Sqrt_F32(V3_Sq_Mag(v));
	return Result;
}

function v3 V3_Norm(v3 v) {
	f32 SqLength = V3_Sq_Mag(v);
	if (Equal_Zero_Eps_Sq_F32(SqLength)) {
		return V3_Zero();
	}

	f32 InvLength = 1.0f / Sqrt_F32(SqLength);
	return V3_Mul_S(v, InvLength);
}

function inline v3 V3_Negate(v3 v) {
	v3 Result = { -v.x, -v.y, -v.z };
	return Result;
}

function inline v3 V3_Cross(v3 A, v3 B) {
	v3 Result = V3(A.y*B.z - A.z*B.y, A.z*B.x - A.x*B.z, A.x*B.y - A.y*B.x);
	return Result;
}

function inline v3 V3_Lerp(v3 A, f32 t, v3 B) {
	v3 Result = V3_Add_V3(V3_Mul_S(A, 1.0f - t), V3_Mul_S(B, t));
	return Result;
}

function inline b32 V3_Is_Nan(v3 V) {
	return Is_Nan(V.x) || Is_Nan(V.y) || Is_Nan(V.z);
}

function inline b32 V3_Is_Close(v3 A, v3 B, f32 ToleranceSq) {
	b32 Result = V3_Sq_Mag(V3_Sub_V3(B, A)) <= ToleranceSq;
	return Result;
}

function inline b32 V3_Is_Zero(v3 V, f32 Epsilon) {
	b32 Result = V3_Sq_Mag(V) <= Epsilon;
	return Result;
}

function v3 V3_Get_Perp(v3 Direction) {
	v3  Z = V3_Norm(Direction);
	v3  Up = V3(0, 1, 0);

	f32 Diff = 1 - Abs(V3_Dot(Z, Up));
	if(Equal_Zero_Eps_F32(Diff))
		Up = V3(0, 0, 1);

	v3 Result = V3_Negate(V3_Norm(V3_Cross(Z, Up)));
	return Result;
}

global const v3 G_XAxis    = { 1.0f, 0.0f, 0.0f };
global const v3 G_YAxis    = { 0.0f, 1.0f, 0.0f };
global const v3 G_ZAxis    = { 0.0f, 0.0f, 1.0f };
global const v3 G_UpVector = { 0.0f, 0.0f, 1.0f };

function inline v4 V4(f32 x, f32 y, f32 z, f32 w) {
	v4 Result = { x, y, z, w };
	return Result;
}

function inline v4 V4_All(f32 v) {
	v4 Result = { v, v, v, v };
	return Result;
}

function inline v4 V4_Zero() {
	v4 Result = { 0, 0, 0, 0 };
	return Result;
}

function inline v4 V4_From_V3(v3 xyz, f32 w) {
	v4 Result = V4(xyz.x, xyz.y, xyz.z, w);
	return Result;
}

function inline f32 V4_Dot(v4 a, v4 b) {
	f32 Result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
	return Result;
}

function inline quat Quat(f32 x, f32 y, f32 z, f32 w) {
	quat Result = { x, y, z, w };
	return Result;
}

function inline quat Quat_Identity() {
	quat Result = Quat(0, 0, 0, 1);
	return Result;
}

function inline quat Quat_From_V3_S(v3 v, f32 s) {
	quat Result = { v.x, v.y, v.z, s };
	return Result;
}

function quat Quat_From_Euler(v3 Euler) {
	f32 cx = Cos_F32(Euler.x * 0.5f);
    f32 sx = Sin_F32(Euler.x * 0.5f);
    f32 cy = Cos_F32(Euler.y * 0.5f);
    f32 sy = Sin_F32(Euler.y * 0.5f);
    f32 cz = Cos_F32(Euler.z * 0.5f);
    f32 sz = Sin_F32(Euler.z * 0.5f);

	quat q;
    q.w = cx * cy * cz + sx * sy * sz;
    q.x = sx * cy * cz - cx * sy * sz;
    q.y = cx * sy * cz + sx * cy * sz;
    q.z = cx * cy * sz - sx * sy * cz;
	return q;
}

function quat Quat_Axis_Angle(v3 Axis, f32 Angle) {
	f32 s = Sin_F32(Angle * 0.5f);
	f32 c = Cos_F32(Angle * 0.5f);
	return Quat_From_V3_S(V3_Mul_S(Axis, s), c);
}

function inline quat Quat_RotX(f32 Pitch) {
	f32 PitchHalf = Pitch * 0.5f;
	return Quat(Sin_F32(PitchHalf), 0.0f, 0.0f, Cos_F32(PitchHalf));
}

function inline quat Quat_RotY(f32 Yaw) {
	f32 YawHalf = Yaw * 0.5f;
	return Quat(0.0f, Sin_F32(YawHalf), 0.0f, Cos_F32(YawHalf));
}

function inline quat Quat_RotZ(f32 Roll) {
	f32 RollHalf = Roll * 0.5f;
	return Quat(0.0f, 0.0f, Sin_F32(RollHalf), Cos_F32(RollHalf));
}

function quat Quat_Add_Quat(quat A, quat B) {
	quat Result = Quat(A.x + B.x, A.y + B.y, A.z + B.z, A.w + B.w);
	return Result;
}

function quat Quat_Mul_S(quat A, f32 B) {
	quat Result = Quat(A.x * B, A.y * B, A.z * B, A.w * B);
	return Result;
}

function quat Quat_Mul_Quat(quat a, quat b) {
	v3 v = V3_Add_V3(V3_Cross(a.v, b.v), V3_Add_V3(V3_Mul_S(a.v, b.s), V3_Mul_S(b.v, a.s)));
	f32 s = a.s * b.s - V3_Dot(a.v, b.v);
	return Quat_From_V3_S(v, s);
}

function inline f32 Quat_Dot(quat A, quat B) {
	f32 Result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;;
	return Result;
}

function inline f32 Quat_Sq_Mag(quat V) {
	f32 Result = Quat_Dot(V, V);
	return Result;
}

function inline f32 Quat_Mag(quat V) {
	f32 Result = Sqrt_F32(Quat_Sq_Mag(V));
	return Result;
}

function quat Quat_Norm(quat V) {
	f32 SqLength = Quat_Sq_Mag(V);
	if (Equal_Zero_Eps_Sq_F32(SqLength)) {
		return Quat_Identity();
	}

	f32 InvLength = 1.0f / Sqrt_F32(SqLength);
	return Quat_Mul_S(V, InvLength);
}

function inline quat Quat_Lerp(quat A, f32 t, quat B) {
	quat Result = Quat_Norm(Quat_Add_Quat(Quat_Mul_S(A, 1.0f - t), Quat_Mul_S(B, t)));
	return Result;
}

function inline b32 Quat_Is_Nan(quat Q) {
	return Is_Nan(Q.x) || Is_Nan(Q.y) || Is_Nan(Q.z) || Is_Nan(Q.w);
}

function m3 M3_From_M4(const m4* M) {
	m3 Result = {
		.x = M->x,
		.y = M->y,
		.z = M->z
	};
	return Result;
}

function m3 M3_Identity() {
	m3 Result = {
		1, 0, 0, 
		0, 1, 0, 
		0, 0, 1
	};
	return Result;
}

function m3 M3_XYZ(v3 x, v3 y, v3 z) {
	m3  Result = {
		.x = x,
		.y = y,
		.z = z
	};
	return Result;
}

function m3 M3_From_Quat(quat q) {
	f32 qxqy = q.x * q.y;
	f32 qwqz = q.w * q.z;
	f32 qxqz = q.x * q.z;
	f32 qwqy = q.w * q.y;
	f32 qyqz = q.y * q.z;
	f32 qwqx = q.w * q.x;
        	         
	f32 qxqx = q.x * q.x;
	f32 qyqy = q.y * q.y;
	f32 qzqz = q.z * q.z;

	m3 Result = {
		.x = V3(1 - 2*(qyqy + qzqz), 2*(qxqy + qwqz),     2*(qxqz - qwqy)),
		.y = V3(2*(qxqy - qwqz),     1 - 2*(qxqx + qzqz), 2*(qyqz + qwqx)),
		.z = V3(2*(qxqz + qwqy),     2*(qyqz - qwqx),     1 - 2*(qxqx + qyqy))
	};
	return Result;
}

function m3 M3_Axis_Angle(v3 Axis, f32 Angle) {
	quat Quat = Quat_Axis_Angle(Axis, Angle);
	return M3_From_Quat(Quat);
}

function m3 M3_Transpose(const m3* M) {
	m3 Result = {
		.m00 = M->m00, 
		.m01 = M->m10, 
		.m02 = M->m20, 

		.m10 = M->m01,
		.m11 = M->m11,
		.m12 = M->m21,

		.m20 = M->m02,
		.m21 = M->m12,
		.m22 = M->m22
	};

	return Result;
}

function m3 M3_Inverse(const m3* M) {
	f32 xSq = V3_Sq_Mag(M->x);
	f32 ySq = V3_Sq_Mag(M->y);
	f32 zSq = V3_Sq_Mag(M->z);

	f32 sx = Equal_Zero_Eps_Sq_F32(xSq) ? 0.0f : 1.0f/xSq;
	f32 sy = Equal_Zero_Eps_Sq_F32(ySq) ? 0.0f : 1.0f/ySq;
	f32 sz = Equal_Zero_Eps_Sq_F32(zSq) ? 0.0f : 1.0f/zSq;

	v3 NewX = V3_Mul_S(M->x, sx);
	v3 NewY = V3_Mul_S(M->y, sy);
	v3 NewZ = V3_Mul_S(M->z, sz);

	m3 Result = {
		.Data = {
			NewX.x, NewY.x, NewZ.x,
			NewX.y, NewY.y, NewZ.y,
			NewX.z, NewY.z, NewZ.z
		}
	};
	return Result;
}

function m3 M3_Mul_S(const m3* M, f32 S) {
	
	m3 Result = {
		.Data = {
			M->m00 * S, M->m01 * S, M->m02 * S,
			M->m10 * S, M->m11 * S, M->m12 * S,
			M->m20 * S, M->m21 * S, M->m22 * S
		}
	};
	return Result;
}

function m3 M3_Mul_M3(const m3* A, const m3* B) {
	m3 BTransposed = M3_Transpose(B);

	m3 Result;
	Result.Data[0] = V3_Dot(A->Rows[0], BTransposed.Rows[0]);
	Result.Data[1] = V3_Dot(A->Rows[0], BTransposed.Rows[1]);
	Result.Data[2] = V3_Dot(A->Rows[0], BTransposed.Rows[2]);

	Result.Data[3] = V3_Dot(A->Rows[1], BTransposed.Rows[0]);
	Result.Data[4] = V3_Dot(A->Rows[1], BTransposed.Rows[1]);
	Result.Data[5] = V3_Dot(A->Rows[1], BTransposed.Rows[2]);

	Result.Data[6] = V3_Dot(A->Rows[2], BTransposed.Rows[0]);
	Result.Data[7] = V3_Dot(A->Rows[2], BTransposed.Rows[1]);
	Result.Data[8] = V3_Dot(A->Rows[2], BTransposed.Rows[2]);

	return Result;
}

function inline v3 M3_Diagonal(const m3* M) {
	v3 Result = V3(M->m00, M->m11, M->m22);
	return Result;
}

function inline m3 M3_Scale(v3 S) {
	m3 Result = {
		.Data = {
			S.x, 0, 0, 
			0, S.y, 0, 
			0, 0, S.z
		}
	};
	return Result;
}

function m3 M3_Basis(v3 Direction) {
	v3 Z = V3_Norm(Direction);
	v3 Up = V3(0, 1, 0);

	f32 Diff = 1 - Abs(V3_Dot(Z, Up));
	if(Equal_Zero_Eps_F32(Diff))
		Up = V3(0, 0, 1);

	v3 X = V3_Negate(V3_Norm(V3_Cross(Z, Up)));
	v3 Y = V3_Cross(Z, X);
	return M3_XYZ(X, Y, Z);
}


function v3 V3_Mul_M3(v3 A, const m3* B) {
	m3 BTransposed = M3_Transpose(B);

	v3 Result;
	Result.x = V3_Dot(A, BTransposed.Rows[0]);
	Result.y = V3_Dot(A, BTransposed.Rows[1]);
	Result.z = V3_Dot(A, BTransposed.Rows[2]);
	return Result;
}

function m4 M4_F32(const f32* Data) {
	m4 Result;
	Memory_Copy(Result.Data, Data, sizeof(m4));
	return Result;
}

function m4 M4_Identity() {
	m4 Result = {
		1, 0, 0, 0, 
		0, 1, 0, 0, 
		0, 0, 1, 0, 
		0, 0, 0, 1
	};
	return Result;
}

function m4 M4_Transform(v3 x, v3 y, v3 z, v3 t) {
	m4 Result = M4_Identity();
	Result.x = x;
	Result.y = y;
	Result.z = z;
	Result.t = t;
	return Result;
}

function m4 M4_Transform_Scale(v3 x, v3 y, v3 z, v3 t, v3 s) {
	x = V3_Mul_S(x, s.x);
	y = V3_Mul_S(y, s.y);
	z = V3_Mul_S(z, s.z);
	return M4_Transform(x, y, z, t);
}

function inline m4 M4_Transform_Scale_Quat(v3 t, v3 s, quat Q) {
	m3 Orientation = M3_From_Quat(Q);
	return M4_Transform_Scale(Orientation.x, Orientation.y, Orientation.z, s, t);
}

function m4 M4_From_M3(const m3* M) {
	return M4_Transform(M->x, M->y, M->z, V3_Zero());
}

function m4 M4_Inv_Transform(v3 x, v3 y, v3 z, v3 t) {
	v3 tInv;
	tInv.x = -V3_Dot(x, t);
	tInv.y = -V3_Dot(y, t);
	tInv.z = -V3_Dot(z, t);

	m4 Result = {
		{
			x.x, y.x, z.x, 0, 
			x.y, y.y, z.y, 0, 
			x.z, y.z, z.z, 0, 
			tInv.x, tInv.y, tInv.z, 1
		}
	};
	return Result;
}

function m4 M4_Look_At(v3 Position, v3 Target) {
	v3 Direction = V3_Norm(V3_Sub_V3(Position, Target));
	v3 Up = G_ZAxis;
	
	//If the direction and up vector are very close, the cross products
	//will become invalid. We need to change the up vector to another 
	//direction
	f32 Diff = 1.0f - Abs(V3_Dot(Direction, Up));
	if (Equal_Zero_Eps_F32(Diff)) {
		Up = G_YAxis;
	}

	v3 X = V3_Negate(V3_Norm(V3_Cross(Direction, Up)));
	v3 Y = V3_Cross(Direction, X);
	return M4_Inv_Transform(X, Y, Direction, Position);
}

function m4 M4_Transpose(const m4* M) {
	m4 Result;
	Result.m00 = M->m00;
	Result.m01 = M->m10;
	Result.m02 = M->m20;
	Result.m03 = M->m30;

	Result.m10 = M->m01;
	Result.m11 = M->m11;
	Result.m12 = M->m21;
	Result.m13 = M->m31;

	Result.m20 = M->m02;
	Result.m21 = M->m12;
	Result.m22 = M->m22;
	Result.m23 = M->m32;

	Result.m30 = M->m03;
	Result.m31 = M->m13;
	Result.m32 = M->m23;
	Result.m33 = M->m33;

	return Result;
}

function m4 M4_Mul_M4(const m4* A, const m4* B) {
	m4 BTransposed = M4_Transpose(B);

	m4 Result;
	Result.Data[0] = V4_Dot(A->Rows[0], BTransposed.Rows[0]);
	Result.Data[1] = V4_Dot(A->Rows[0], BTransposed.Rows[1]);
	Result.Data[2] = V4_Dot(A->Rows[0], BTransposed.Rows[2]);
	Result.Data[3] = V4_Dot(A->Rows[0], BTransposed.Rows[3]);

	Result.Data[4] = V4_Dot(A->Rows[1], BTransposed.Rows[0]);
	Result.Data[5] = V4_Dot(A->Rows[1], BTransposed.Rows[1]);
	Result.Data[6] = V4_Dot(A->Rows[1], BTransposed.Rows[2]);
	Result.Data[7] = V4_Dot(A->Rows[1], BTransposed.Rows[3]);

	Result.Data[8]  = V4_Dot(A->Rows[2], BTransposed.Rows[0]);
	Result.Data[9]  = V4_Dot(A->Rows[2], BTransposed.Rows[1]);
	Result.Data[10] = V4_Dot(A->Rows[2], BTransposed.Rows[2]);
	Result.Data[11] = V4_Dot(A->Rows[2], BTransposed.Rows[3]);

	Result.Data[12] = V4_Dot(A->Rows[3], BTransposed.Rows[0]);
	Result.Data[13] = V4_Dot(A->Rows[3], BTransposed.Rows[1]);
	Result.Data[14] = V4_Dot(A->Rows[3], BTransposed.Rows[2]);
	Result.Data[15] = V4_Dot(A->Rows[3], BTransposed.Rows[3]);

	return Result;
}

function m4 M4_Perspective(f32 FOV, f32 AspectRatio, f32 ZNear, f32 ZFar) {
	f32 t = 1.0f/Tan_F32(FOV*0.5f);
	f32 n = ZNear;
	f32 f = ZFar;

	f32 a = t/AspectRatio;
	f32 b = t;
	f32 c = f/(n-f);
	f32 d = (n*f)/(n-f);
	f32 e = -1;

	f32 Data[] = {
		a, 0, 0, 0, 
		0, b, 0, 0, 
		0, 0, c, e, 
		0, 0, d, 0
	};
	return M4_F32(Data);
}

function m4_affine M4_Affine_F32(const f32* Matrix) {
	m4_affine Result;
	Memory_Copy(&Result, Matrix, sizeof(m4_affine));
	return Result;
}

function m4_affine M4_Affine_F64(const f64* Matrix) {
	m4_affine Result = {
		.Data = {
			(f32)Matrix[0], (f32)Matrix[1],  (f32)Matrix[2],
			(f32)Matrix[3], (f32)Matrix[4],  (f32)Matrix[5],
			(f32)Matrix[6], (f32)Matrix[7],  (f32)Matrix[8],
			(f32)Matrix[9], (f32)Matrix[10], (f32)Matrix[11],
		}
	};
	return Result;
}

function inline m4_affine M4_Affine_Transform_No_Scale(v3 t, const m3* M) {
	m4_affine Result;
	Result.M = *M;
	Result.t = t;
	return Result;
}

function m4_affine M4_Affine_Transform_Quat_No_Scale(v3 T, quat Q) {
	m3 M = M3_From_Quat(Q);
	return M4_Affine_Transform_No_Scale(T, &M);
}

function m4_affine M4_Affine_Identity() {
	f32 Data[] = {
		1, 0, 0, 
		0, 1, 0, 
		0, 0, 1, 
		0, 0, 0
	};
	return M4_Affine_F32(Data);
}

function m4_affine_transposed M4_Affine_Transpose(const m4_affine* M) {
	m4_affine_transposed Result = {
		.m00 = M->m00, 
		.m01 = M->m10, 
		.m02 = M->m20, 
		.m03 = M->m30,

		.m10 = M->m01,
		.m11 = M->m11,
		.m12 = M->m21,
		.m13 = M->m31,

		.m20 = M->m02,
		.m21 = M->m12,
		.m22 = M->m22,
		.m23 = M->m32
	};

	return Result;
}


function v3 V4_Mul_M4_Affine(v4 A, const m4_affine* B) {
	m4_affine_transposed BTransposed = M4_Affine_Transpose(B);

	v3 Result;
	Result.x = V4_Dot(A, BTransposed.Rows[0]);
	Result.y = V4_Dot(A, BTransposed.Rows[1]);
	Result.z = V4_Dot(A, BTransposed.Rows[2]);
	return Result;
}

function m4_affine M4_Affine_Mul_M4_Affine(const m4_affine* A, const m4_affine* B) {
	m4_affine_transposed BTransposed = M4_Affine_Transpose(B);

	m4_affine Result = {
		.m00 = V3_Dot(A->Rows[0], BTransposed.Rows[0].xyz),
		.m01 = V3_Dot(A->Rows[0], BTransposed.Rows[1].xyz),
		.m02 = V3_Dot(A->Rows[0], BTransposed.Rows[2].xyz),

		.m10 = V3_Dot(A->Rows[1], BTransposed.Rows[0].xyz),
		.m11 = V3_Dot(A->Rows[1], BTransposed.Rows[1].xyz),
		.m12 = V3_Dot(A->Rows[1], BTransposed.Rows[2].xyz),

		.m20 = V3_Dot(A->Rows[2], BTransposed.Rows[0].xyz),
		.m21 = V3_Dot(A->Rows[2], BTransposed.Rows[1].xyz),
		.m22 = V3_Dot(A->Rows[2], BTransposed.Rows[2].xyz),

		.m30 = V3_Dot(A->Rows[3], BTransposed.Rows[0].xyz) + BTransposed.Rows[0].w,
		.m31 = V3_Dot(A->Rows[3], BTransposed.Rows[1].xyz) + BTransposed.Rows[1].w,
		.m32 = V3_Dot(A->Rows[3], BTransposed.Rows[2].xyz) + BTransposed.Rows[2].w,
	};

	return Result;
}

function m4_affine M4_Affine_Inverse_No_Scale(const m4_affine* M) {
	f32 tx = -V3_Dot(M->t, M->x);
	f32 ty = -V3_Dot(M->t, M->y);
	f32 tz = -V3_Dot(M->t, M->z);

	f32 Data[] = {
		M->m00, M->m10, M->m20,
		M->m01, M->m11, M->m21,
		M->m02, M->m12, M->m22,
		tx,     ty,     tz
	};

	return M4_Affine_F32(Data);
}

function m4_affine M4_Affine_Inverse_Transform_No_Scale(v3 T, const m3* M) {
	f32 tx = -V3_Dot(T, M->x);
	f32 ty = -V3_Dot(T, M->y);
	f32 tz = -V3_Dot(T, M->z);

	f32 Data[] = {
		M->m00, M->m10, M->m20,
		M->m01, M->m11, M->m21,
		M->m02, M->m12, M->m22,
		tx,     ty,     tz
	};

	return M4_Affine_F32(Data);
}

function m4_affine M4_Affine_Inverse_Transform_Quat_No_Scale(v3 T, quat Q) {
	m3 Orientation = M3_From_Quat(Q);
	return M4_Affine_Inverse_Transform_No_Scale(T, &Orientation);
}

function inline rect2 Rect2(v2 p0, v2 p1) {
	rect2 Result = { p0, p1 };
	return Result;
}

function inline rect2 Rect2_Offset(rect2 Rect, v2 Offset) {
	rect2 Result = Rect2(V2_Add_V2(Rect.p0, Offset), V2_Add_V2(Rect.p1, Offset));
	return Result;
}

function inline b32 Rect2_Contains_V2(rect2 Rect, v2 V) {
	return (Rect.p0.x <= V.x && Rect.p0.y <= V.y) && 
		   (Rect.p1.x >= V.x && Rect.p1.y >= V.y);
}

function inline v2 Rect2_Size(rect2 Rect) {
	return V2_Sub_V2(Rect.p1, Rect.p0);
}

// Convert rgb floats ([0-1],[0-1],[0-1]) to hsv floats ([0-1],[0-1],[0-1]), from Foley & van Dam p592
// Optimized http://lolengine.net/blog/2013/01/13/fast-rgb-to-hsv
function v3 RGB_To_HSV(v3 rgb) {
    f32 K = 0.f;
    if (rgb.g < rgb.b) {
        const f32 tmp = rgb.g; rgb.g = rgb.b; rgb.b = tmp;
        K = -1.f;
    }
    
	if (rgb.r < rgb.g) {
        const f32 tmp = rgb.r; rgb.r = rgb.g; rgb.g = tmp;
        K = -2.f / 6.f - K;
    }

    const f32 chroma = rgb.r - (rgb.g < rgb.b ? rgb.g : rgb.b);
    
	v3 hsv;
	hsv.h = Abs(K + (rgb.g - rgb.b) / (6.f * chroma + 1e-20f));
    hsv.s = chroma / (rgb.r + 1e-20f);
    hsv.v = rgb.r;
	return hsv;
}

// Convert hsv floats ([0-1],[0-1],[0-1]) to rgb floats ([0-1],[0-1],[0-1]), from Foley & van Dam p593
// also http://en.wikipedia.org/wiki/HSL_and_HSV
function v3 HSV_To_RGB(v3 hsv) {
    if (hsv.s == 0.0f) {
        // gray
		v3 Result = V3_All(hsv.v);
        return Result;
    }

    hsv.h = FMod_F32(hsv.h, 1.0f) / (60.0f/360.0f);
    s32 i = (s32)hsv.h;
    f32 f = hsv.h - (f32)i;
    f32 p = hsv.v * (1.0f - hsv.s);
    f32 q = hsv.v * (1.0f - hsv.s * f);
    f32 t = hsv.v * (1.0f - hsv.s * (1.0f - f));

	v3 rgb;
    switch (i) {
		case 0:  rgb.r = hsv.v; rgb.g = t; 	   rgb.b = p; break;
		case 1:  rgb.r = q; 	rgb.g = hsv.v; rgb.b = p; break;
		case 2:  rgb.r = p; 	rgb.g = hsv.v; rgb.b = t; break;
		case 3:  rgb.r = p; 	rgb.g = q; 	   rgb.b = hsv.v; break;
		case 4:  rgb.r = t; 	rgb.g = p; 	   rgb.b = hsv.v; break;
		default: rgb.r = hsv.v; rgb.g = p; 	   rgb.b = q; break;
    }

	return rgb;
}

function v3 Get_Triangle_Centroid(v3 P0, v3 P1, v3 P2) {
	local const f32 Denominator = 1.0f / 3.0f;
	v3 Result = V3_Mul_S(V3_Add_V3(V3_Add_V3(P0, P1), P2), Denominator);
	return Result;
}

function memory_reserve Make_Memory_Reserve(size_t Size) {
	memory_reserve Result = { 0 };

	Size = Align(Size, OS_Page_Size());
	Result.BaseAddress = (u8*)OS_Reserve_Memory(Size);
	if (!Result.BaseAddress) {
		return Result;
	}

	Result.ReserveSize = Size;
	Result.CommitSize  = 0;
	return Result;
}

function inline void Delete_Memory_Reserve(memory_reserve* Reserve) {
	if (Reserve && Reserve->BaseAddress != NULL) {
		OS_Release_Memory(Reserve->BaseAddress, Reserve->ReserveSize);
	}
}

function memory_reserve Subdivide_Memory_Reserve(memory_reserve* Reserve, size_t Offset, size_t Size) {
	size_t PageSize = OS_Page_Size();
	
	Assert((Offset % PageSize) == 0);
	Assert((Size % PageSize) == 0);

	size_t CommitSize = (size_t)Max((intptr_t)Reserve->CommitSize-(intptr_t)Offset, 0);
	memory_reserve Result = {
		.BaseAddress = Reserve->BaseAddress+Offset,
		.ReserveSize = Size,
		.CommitSize  = CommitSize
	};

	return Result;
}

function inline b32 Reserve_Is_Valid(memory_reserve* Reserve) {
	return (Reserve->BaseAddress != NULL && Reserve->ReserveSize != 0);
}

function void* Commit_New_Size(memory_reserve* Reserve, size_t NewSize) {
	//Get the proper offset to the commit address and find the new commit size.
	//Committed size must align on a page boundary	
	size_t PageSize = OS_Page_Size();

	u8* BaseCommit = Reserve->BaseAddress + Reserve->CommitSize;
	size_t NewCommitSize = Align(NewSize, PageSize);
	size_t DeltaSize = NewCommitSize - Reserve->CommitSize;

	if (NewCommitSize > Reserve->ReserveSize) {
		//todo: Diagnostic and error logging
		return NULL;
	}

	Assert(NewCommitSize <= Reserve->ReserveSize);
	Assert((NewCommitSize % PageSize) == 0);
	Assert((DeltaSize % PageSize) == 0);

	void* Result = BaseCommit;
	if (DeltaSize > 0) {
		//Commit the new memory pages
		Result = OS_Commit_Memory(BaseCommit, DeltaSize);
		if (!Result) {
			//todo: Diagnostic and error logging
			return NULL;
		}
	}

	Reserve->CommitSize = NewCommitSize;
	return Result;
}

function void Decommit_New_Size(memory_reserve* Reserve, size_t NewSize) {
	if (!Reserve->CommitSize) {
		//If we have not allocated any memory yet, make sure that the marker is valid and return
		Assert(NewSize == 0);
		return;
	}

	size_t PageSize = OS_Page_Size();

	size_t NewCommitSize = Align(NewSize, PageSize);
	Assert(Reserve->CommitSize >= NewCommitSize);
	size_t DeltaSize = Reserve->CommitSize - NewCommitSize;

	Assert((NewCommitSize % PageSize) == 0);
	Assert((DeltaSize % PageSize) == 0);

	if (DeltaSize) {
		//If we have more memory than needed, decommit the pages
		u8* BaseCommit = Reserve->BaseAddress + NewCommitSize;
		OS_Decommit_Memory(BaseCommit, DeltaSize);
		Reserve->CommitSize = NewCommitSize;
	}
}

function inline b32 Commit_All(memory_reserve* Reserve) {
	b32 Result = false;
	if (Reserve && Reserve->BaseAddress) {
		Result = Commit_New_Size(Reserve, Reserve->ReserveSize) != NULL;
	}
	return Result;
}

function inline buffer Make_Buffer(void* Data, size_t Size) {
	buffer Result = { (u8*)Data, Size };
	return Result;
}

function inline buffer Buffer_From_String(string String) {
	buffer Result = { (u8*)String.Ptr, String.Size };
	return Result;
}

function inline buffer Buffer_Empty() {
	return Make_Buffer(NULL, 0);
}

function inline buffer Buffer_Alloc(allocator* Allocator, size_t Size) {
	u8* Ptr = (u8*)Allocator_Allocate_Memory(Allocator, Size);
	return Make_Buffer(Ptr, Size);
}

function inline b32 Buffer_Is_Empty(buffer Buffer) {
	return !Buffer.Size || !Buffer.Ptr;
}

function arena* Arena_Create_With_Size(size_t ReserveSize) {
	base* Base = Base_Get();

	memory_reserve MemoryReserve = Make_Memory_Reserve(ReserveSize + sizeof(arena));
	if (!MemoryReserve.BaseAddress) {
		//todo: Diagnostic and error logging
		return NULL;
	}

	arena* Arena = (arena*)Commit_New_Size(&MemoryReserve, sizeof(arena));
	
	Arena->Base.VTable = Base->ArenaVTable;
	Arena->Reserve 	   = MemoryReserve;
	Arena->Used        = sizeof(arena);

	return Arena;
}

function arena* Arena_Create() {
	arena* Result = Arena_Create_With_Size(GB(1));
	return Result;
}

function void Arena_Delete(arena* Arena) {
	if (Arena->Reserve.BaseAddress) {
		Delete_Memory_Reserve(&Arena->Reserve);
	}
}

function void* Arena_Push_Aligned_No_Clear(arena* Arena, size_t Size, size_t Alignment) {
	Assert(Is_Pow2(Alignment));
	
	memory_reserve* Reserve = &Arena->Reserve;

	size_t NewUsed = Align_Pow2(Arena->Used, Alignment);
	if (NewUsed + Size > Reserve->CommitSize) {
		if (!Commit_New_Size(Reserve, NewUsed + Size)) {
			//todo: Diagnostic and error logging
			return NULL;
		}
	}

	Arena->Used = NewUsed;
	void* Memory = Reserve->BaseAddress + Arena->Used;
	Arena->Used += Size;

	return Memory;
}

function void* Arena_Push_Aligned(arena* Arena, size_t Size, size_t Alignment) {
	void* Result = Arena_Push_Aligned_No_Clear(Arena, Size, Alignment);
	if (Result) Memory_Clear(Result, Size);
	return Result;
}

function void* Arena_Push_No_Clear(arena* Arena, size_t Size) {
	void* Result = Arena_Push_Aligned_No_Clear(Arena, Size, DEFAULT_ALIGNMENT);
	return Result;
}

function void* Arena_Push(arena* Arena, size_t Size) {
	void* Result = Arena_Push_Aligned(Arena, Size, DEFAULT_ALIGNMENT);
	return Result;
}

#define Arena_Push_Struct(arena, type) (type*)Arena_Push(arena, sizeof(type))
#define Arena_Push_Array(arena, count, type) (type*)Arena_Push(arena, sizeof(type)*(count))

#define Arena_Push_Struct_No_Clear(arena, type) (type*)Arena_Push_No_Clear(arena, sizeof(type))
#define Arena_Push_Array_No_Clear(arena, count, type) (type*)Arena_Push_No_Clear(arena, sizeof(type)*(count))

function inline arena_marker Arena_Get_Marker(arena* Arena) {
	arena_marker Result = {
		.Arena = Arena,
		.Used  = Arena->Used
	};
	return Result;
}

function inline void Arena_Set_Marker(arena* Arena, arena_marker Marker) {
	Assert(Arena == Marker.Arena);
	Arena->Used = Marker.Used;
}

function inline void Arena_Clear(arena* Arena) {
	arena_marker Marker = { .Arena = Arena, .Used = sizeof(arena) };
	Arena_Set_Marker(Arena, Marker);
}

function ALLOCATOR_ALLOCATE_MEMORY_DEFINE(Arena_Allocate_Memory) {
	arena* Arena = (arena*)Allocator;
	void* Result = Arena_Push_No_Clear(Arena, Size);
	if (Result && ClearFlag == CLEAR_FLAG_YES) {
		Memory_Clear(Result, Size);
	}
	return Result;
}

function ALLOCATOR_FREE_MEMORY_DEFINE(Arena_Free_Memory) {
	//Noop
}

function random32_xor_shift Random32_XOrShift_Init();
function thread_context* Thread_Context_Get() {
	thread_context* ThreadContext = (thread_context*)OS_TLS_Get(G_Base->ThreadContextTLS);
	if (!ThreadContext) {
		ThreadContext = Allocator_Allocate_Struct(Default_Allocator_Get(), thread_context);
		OS_TLS_Set(G_Base->ThreadContextTLS, ThreadContext);
		ThreadContext->Random32 = Random32_XOrShift_Init();
	}
	return ThreadContext;
}

function arena* Scratch_Get() {
	thread_context* ThreadContext = Thread_Context_Get();
	Assert(ThreadContext->ScratchIndex < MAX_SCRATCH_COUNT);

	size_t ScratchIndex = ThreadContext->ScratchIndex++;
	if (!ThreadContext->ScratchArenas[ScratchIndex]) {
		ThreadContext->ScratchArenas[ScratchIndex] = Arena_Create();
	}

	ThreadContext->ScratchMarkers[ScratchIndex] = Arena_Get_Marker(ThreadContext->ScratchArenas[ScratchIndex]);
	return ThreadContext->ScratchArenas[ScratchIndex];
}

function void Scratch_Release() {
	thread_context* ThreadContext = Thread_Context_Get();
	Assert(ThreadContext->ScratchIndex != 0);
	size_t ScratchIndex = --ThreadContext->ScratchIndex;
	Arena_Set_Marker(ThreadContext->ScratchArenas[ScratchIndex], ThreadContext->ScratchMarkers[ScratchIndex]);
}

function random32_xor_shift Random32_XOrShift_Init() {
	random32_xor_shift Result = { 0 };
	OS_Get_Entropy(&Result.State, sizeof(u32));
	return Result;
}

function inline u32 Random32_XOrShift(random32_xor_shift* Random) {
	u32 x = Random->State;

	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

	Random->State = x;

	return x;
}

function inline f32 Random32_XOrShift_UNorm(random32_xor_shift* Random) {
	f32 Result = (f32)Random32_XOrShift(Random) / (f32)UINT32_MAX;
	return Result;
}

function inline f32 Random32_XOrShift_SNorm(random32_xor_shift* Random) {
	f32 Result = -1.0f + 2.0f * Random32_XOrShift_UNorm(Random);
	return Result;
}

function inline s32 Random32_XOrShift_Range(random32_xor_shift* Random, s32 Min, s32 Max) {
	s32 Result = Random32_XOrShift(Random) % (Max - Min + 1) + Min;
	return Result;
}

function inline u32 Random32() {
	random32_xor_shift* Random = &Thread_Context_Get()->Random32;
	return Random32_XOrShift(Random);
}

function inline s32 Random32_Range(s32 Min, s32 Max) {
	random32_xor_shift* Random = &Thread_Context_Get()->Random32;
	return Random32_XOrShift_Range(Random, Min, Max);
}

#define Array_Implement(type, name) \
function type##_array name##_Array_Init(type* Ptr, size_t Count) { \
	type##_array Result = { \
	.Count = Count, \
	.Ptr = Ptr \
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
}

Array_Implement(char, Char);
Array_Implement(u32, U32);
Array_Implement(v3, V3);
Array_Implement(string, String);

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

#define Dynamic_Array_Implement_Type(type, name) Dynamic_Array_Implement(type, type, name)
#define Dynamic_Array_Implement_Ptr(type, name) Dynamic_Array_Implement(type, type*, name)

Dynamic_Array_Implement_Type(char, Char);
Dynamic_Array_Implement_Type(string, String);

Dynamic_Array_Implement(char_ptr, char*, Char_Ptr);

global const u8 G_ClassUTF8[32] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

function u32 UTF8_Read(const char* Str, u32* OutLength) {
    u32 Result = 0xFFFFFFFF;
    
    u8 Byte = (u8)Str[0];
    u8 ByteClass = G_ClassUTF8[Byte >> 3];
    
    local const u32 BITMASK_3 = 0x00000007;
    local const u32 BITMASK_4 = 0x0000000f;
    local const u32 BITMASK_5 = 0x0000001f;
    local const u32 BITMASK_6 = 0x0000003f;
    
    switch(ByteClass)
    {
        case 1:
        {
            Result = Byte;
        } break;
        
        case 2:
        {
            u8 NextByte = (u8)Str[1];
            if(G_ClassUTF8[NextByte >> 3] == 0)
            {
                Result = (Byte & BITMASK_5) << 6;
                Result |= (NextByte & BITMASK_6);
            }
        } break;
        
        case 3:
        {
            u8 NextBytes[2] = {(u8)Str[1], (u8)Str[2]};
            if(G_ClassUTF8[NextBytes[0] >> 3] == 0 &&
                G_ClassUTF8[NextBytes[1] >> 3] == 0)
            {
                Result = (Byte & BITMASK_4) << 12;
                Result |= ((NextBytes[0] & BITMASK_6) << 6);
                Result |= (NextBytes[1] & BITMASK_6);
            }
        } break;
        
        case 4:
        {
            u8 NextBytes[3] = {(u8)Str[1], (u8)Str[2], (u8)Str[3]};
            if(G_ClassUTF8[NextBytes[0] >> 3] == 0 &&
                G_ClassUTF8[NextBytes[1] >> 3] == 0 &&
                G_ClassUTF8[NextBytes[2] >> 3] == 0)
            {
                Result = (Byte & BITMASK_3) << 18;
                Result |= ((NextBytes[0] & BITMASK_6) << 12);
                Result |= ((NextBytes[1] & BITMASK_6) << 6);
                Result |= (NextBytes[2] & BITMASK_6);
            }
        } break;
    }
    
    if(OutLength) *OutLength = ByteClass;
    return Result;
}

function u32 UTF8_Write(char* Str, u32 Codepoint) {
    local const u32 BIT_8 = 0x00000080;
    local const u32 BITMASK_2 = 0x00000003;
    local const u32 BITMASK_3 = 0x00000007;
    local const u32 BITMASK_4 = 0x0000000f;
    local const u32 BITMASK_5 = 0x0000001f;
    local const u32 BITMASK_6 = 0x0000003f;
    
    u32 Result = 0;
    if (Codepoint <= 0x7F)
    {
        Str[0] = (char)Codepoint;
        Result = 1;
    }
    else if (Codepoint <= 0x7FF)
    {
        Str[0] = (char)((BITMASK_2 << 6) | ((Codepoint >> 6) & BITMASK_5));
        Str[1] = (char)(BIT_8 | (Codepoint & BITMASK_6));
        Result = 2;
    }
    else if (Codepoint <= 0xFFFF)
    {
        Str[0] = (char)((BITMASK_3 << 5) | ((Codepoint >> 12) & BITMASK_4));
        Str[1] = (char)(BIT_8 | ((Codepoint >> 6) & BITMASK_6));
        Str[2] = (char)(BIT_8 | ( Codepoint       & BITMASK_6));
        Result = 3;
    }
    else if (Codepoint <= 0x10FFFF)
    {
        Str[0] = (char)((BITMASK_4 << 3) | ((Codepoint >> 18) & BITMASK_3));
        Str[1] = (char)(BIT_8 | ((Codepoint >> 12) & BITMASK_6));
        Str[2] = (char)(BIT_8 | ((Codepoint >>  6) & BITMASK_6));
        Str[3] = (char)(BIT_8 | ( Codepoint        & BITMASK_6));
        Result = 4;
    }
    else
    {
        Str[0] = '?';
        Result = 1;
    }
    
    return Result;
}

function u32 UTF16_Read(const wchar_t* Str, u32* OutLength) {
    u32 Offset = 1;
    u32 Result = *Str;
    if (0xD800 <= Str[0] && Str[0] < 0xDC00 && 0xDC00 <= Str[1] && Str[1] < 0xE000)
    {
        Result = (u32)(((Str[0] - 0xD800) << 10) | (Str[1] - 0xDC00));
        Offset = 2;
    }
    if(OutLength) *OutLength = Offset;
    return Result;
}

function u32 UTF16_Write(wchar_t* Str, u32 Codepoint) {
    local const u32 BITMASK_10 = 0x000003ff;
    
    u32 Result = 0;
    if(Codepoint == 0xFFFFFFFF)
    {
        Str[0] = (wchar_t)'?';
        Result = 1;
    }
    else if(Codepoint < 0x10000)
    {
        Str[0] = (wchar_t)Codepoint;
        Result = 1;
    }
    else
    {
        Codepoint -= 0x10000;
        Str[0] = (wchar_t)(0xD800 + (Codepoint >> 10));
        Str[1] = (wchar_t)(0xDC00 + (Codepoint & BITMASK_10));
        Result = 2;
    }
    
    return Result;
}

#define STRING_INVALID_INDEX ((size_t)-1)

function inline size_t String_Length(const char* Ptr) {
	return strlen(Ptr);
}

function inline string String_Empty() {
	string Result = { 0 };
	return Result;
}

function inline string Make_String(const char* Ptr, size_t Size) {
	string Result = {
		.Ptr = Ptr,
		.Size = Size
	};
	return Result;
}

function string String_Null_Term(const char* Ptr) {
	return Make_String(Ptr, String_Length(Ptr));
}

#define String_Lit(str) Make_String(str, sizeof(str)-1)
#define String_Expand(str) { str, sizeof(str)-1 }


function string String_FormatV(allocator* Allocator, const char* Format, va_list Args) {
	char TmpBuffer[1];
	int TotalLength = stbsp_vsnprintf(TmpBuffer, 1, Format, Args);
	char* Buffer = Allocator_Allocate_Array(Allocator, TotalLength + 1, char);
	stbsp_vsnprintf(Buffer, TotalLength + 1, Format, Args);
	return Make_String(Buffer, TotalLength);
}

function string String_Format(allocator* Allocator, const char* Format, ...) {
	va_list List;
	va_start(List, Format);
	string Result = String_FormatV(Allocator, Format, List);
	va_end(List);
	return Result;
}

function inline string String_From_Buffer(buffer Buffer) {
	return Make_String((const char*)Buffer.Ptr, Buffer.Size);
}

function string String_Copy(allocator* Allocator, string Str) {
	char* Ptr = Allocator_Allocate_Array(Allocator, Str.Size+1, char);
	Memory_Copy(Ptr, Str.Ptr, Str.Size * sizeof(char));
	Ptr[Str.Size] = 0;
	return Make_String(Ptr, Str.Size);
}

function string String_Substr(string Str, size_t FirstIndex, size_t LastIndex) {
	Assert(LastIndex != STRING_INVALID_INDEX && LastIndex != STRING_INVALID_INDEX);
	if (FirstIndex == LastIndex) return String_Empty();

	LastIndex = Min(LastIndex, Str.Size);

	Assert(LastIndex > FirstIndex);

	const char* At = Str.Ptr + FirstIndex;
	return Make_String(At, LastIndex - FirstIndex);
}

function string_array String_Split(allocator* Allocator, string String, string Substr) {
	Assert(String.Size && Substr.Size);
	dynamic_string_array Result = Dynamic_String_Array_Init(Allocator);
	if (Substr.Size > String.Size) {
		Dynamic_String_Array_Add(&Result, String);
		return String_Array_Init(Result.Ptr, Result.Count);
	}

	size_t StartIndex = 0;
	size_t StringIndex = 0;
	while (StringIndex < String.Size) {
		size_t SubstringIndex = 0;
		size_t TotalStringIndex = StringIndex+SubstringIndex;
		while (SubstringIndex < Substr.Size && TotalStringIndex < String.Size) {
			if (String.Ptr[TotalStringIndex] == Substr.Ptr[SubstringIndex]) {
				SubstringIndex++;
				TotalStringIndex++;
			} else {
				break;
			}
		}

		if (SubstringIndex == Substr.Size) {
			Dynamic_String_Array_Add(&Result, String_Substr(String, StartIndex, StringIndex));
			StartIndex = StringIndex+Substr.Size;
		}

		StringIndex++;
	}

	if (StartIndex < StringIndex) {
		Dynamic_String_Array_Add(&Result, String_Substr(String, StartIndex, StringIndex));
	}

	return String_Array_Init(Result.Ptr, Result.Count);
}

function b32 String_Is_Empty(string String) {
	return !String.Ptr || !String.Size;
}

function b32 String_Is_Null_Term(string String) {
	return String.Ptr[String.Size] == 0;
}

function b32 String_Equals(string StringA, string StringB) {
	if (StringA.Size != StringB.Size) return false;

	for (size_t i = 0; i < StringA.Size; i++) {
		if (StringA.Ptr[i] != StringB.Ptr[i]) {
			return false;
		}
	}

	return true;
}

function size_t String_Find_First_Char(string String, char Character) {
	for (size_t Index = 0; Index < String.Size; Index++) {
		if (String.Ptr[Index] == Character) {
			return Index;
		}
	}

	return STRING_INVALID_INDEX;
}

function size_t String_Find_Last_Char(string String, char Character) {
	for (size_t Index = String.Size; Index != 0; Index--) {
		size_t ArrayIndex = Index - 1;
		if (String.Ptr[ArrayIndex] == Character) {
			return ArrayIndex;
		}
	}

	return STRING_INVALID_INDEX;
}

function string String_Combine(allocator* Allocator, string* Strings, size_t Count) {
	size_t TotalSize = 0;
	for (size_t i = 0; i < Count; i++) {
		TotalSize += Strings[i].Size;
	}
	char* Ptr = Allocator_Allocate_Array(Allocator, TotalSize + 1, char);
	char* At = Ptr;

	for (size_t i = 0; i < Count; i++) {
		Memory_Copy(At, Strings[i].Ptr, Strings[i].Size);
		At += Strings[i].Size;		
	}
	Assert(((size_t)(At - Ptr)) == TotalSize);
	Ptr[TotalSize] = 0;
	return Make_String(Ptr, TotalSize);
}


function string String_Concat(allocator* Allocator, string StringA, string StringB) {
	string Strings[] = { StringA, StringB };
	string String = String_Combine(Allocator, Strings, Array_Count(Strings));
	return String;
}

function size_t String_Get_Last_Directory_Slash_Index(string Directory) {
	size_t DoubleSlashPathIndex = String_Find_Last_Char(Directory, '\\');
	size_t SlashPathIndex = String_Find_Last_Char(Directory, '/');

	size_t PathIndex = STRING_INVALID_INDEX;
	if (DoubleSlashPathIndex != STRING_INVALID_INDEX && SlashPathIndex != STRING_INVALID_INDEX) {
		PathIndex = Max(SlashPathIndex, DoubleSlashPathIndex);
	} else if (DoubleSlashPathIndex != STRING_INVALID_INDEX) {
		PathIndex = DoubleSlashPathIndex;
	} else if (SlashPathIndex != STRING_INVALID_INDEX) {
		PathIndex = SlashPathIndex;
	}

	return PathIndex;
}

function string String_Get_Directory_Path(string Path) {
	size_t PathIndex = String_Get_Last_Directory_Slash_Index(Path);
	if (PathIndex == STRING_INVALID_INDEX) return Path;
	return String_Substr(Path, 0, PathIndex + 1);
}

function string String_Directory_Concat(allocator* Allocator, string StringA, string StringB) {
	size_t StringCount = 0;
	string Strings[3] = { 0 };
	if (!StringA.Size) { return StringB; }
	if (!StringB.Size) { return StringA; }
	
	if (StringA.Ptr[StringA.Size - 1] != '\\' && StringA.Ptr[StringA.Size - 1] != '/') {
		Strings[0] = StringA;
		Strings[1] = String_Lit("/");
		Strings[2] = StringB;
		StringCount = 3;
	} else {
		Strings[0] = StringA;
		Strings[1] = StringB;
		StringCount = 2;
	}

	string String = String_Combine(Allocator, Strings, StringCount);
	return String;
}

function inline b32 String_Starts_With_Char(string String, char Character) {
	if (!String.Size) return false;
	return String.Ptr[0] == Character;
}

function inline b32 String_Ends_With_Char(string String, char Character) {
	if (!String.Size) return false;
	return String.Ptr[String.Size - 1] == Character;
}

function string String_Get_Filename_Ext(string Filename) {
	size_t DotIndex = String_Find_Last_Char(Filename, '.');
	if (DotIndex == STRING_INVALID_INDEX) return Filename;
	string Result = String_Substr(Filename, DotIndex + 1, Filename.Size);
	return Result;
}

function string String_Get_Filename_Without_Ext(string Filename) {
	size_t DotIndex = String_Find_Last_Char(Filename, '.');
	size_t PathIndex = String_Get_Last_Directory_Slash_Index(Filename);

	size_t FirstIndex = 0;
	size_t LastIndex = Filename.Size;

	if (PathIndex != STRING_INVALID_INDEX) {
		FirstIndex = PathIndex + 1;
	}

	if (DotIndex != STRING_INVALID_INDEX) {
		if (DotIndex > FirstIndex) {
			LastIndex = DotIndex;
		}
	}

	string Result = String_Substr(Filename, FirstIndex, LastIndex);
	return Result;
}

function string String_Upper_Case(allocator* Allocator, string String) {
	char* Buffer = Allocator_Allocate_Array(Allocator, String.Size + 1, char);
	for (size_t i = 0; i < String.Size; i++) {
		Buffer[i] = To_Upper(String.Ptr[i]);
	}
	Buffer[String.Size] = 0;
	return Make_String(Buffer, String.Size);
}

function string String_Lower_Case(allocator* Allocator, string String) {
	char* Buffer = Allocator_Allocate_Array(Allocator, String.Size + 1, char);
	for (size_t i = 0; i < String.Size; i++) {
		Buffer[i] = To_Lower(String.Ptr[i]);
	}
	Buffer[String.Size] = 0;
	return Make_String(Buffer, String.Size);
}

function string String_Pascal_Case(allocator* Allocator, string String) {
	char* Buffer = Allocator_Allocate_Array(Allocator, String.Size + 1, char);
	
	b32 LastAlpha = false;
	for (size_t i = 0; i < String.Size; i++) {
		Buffer[i] = String.Ptr[i];
		b32 IsAlpha = Is_Alpha(Buffer[i]);

		if (IsAlpha && !LastAlpha) {
			Buffer[i] = To_Upper(Buffer[i]);
		}

		if (IsAlpha && LastAlpha) {
			Buffer[i] = To_Lower(Buffer[i]);
		}

		LastAlpha = IsAlpha;
	}

	Buffer[String.Size] = 0;

	return Make_String(Buffer, String.Size);
}

function string String_Trim_Whitespace(string String) {
	size_t StartOffset = 0;
	for (size_t i = 0; i < String.Size; i++) {
		if (Is_Whitespace(String.Ptr[i])) {
			StartOffset++;
		} else {
			break;
		}
	}

	if (StartOffset == String.Size) return String_Empty();

	size_t EndOffset = String.Size;
	for (size_t i = String.Size; i > 0; i--) {
		if (Is_Whitespace(String.Ptr[i - 1])) {
			EndOffset--;
		} else {
			break;
		}
	}

	Assert(EndOffset > StartOffset);
	return Make_String(String.Ptr + StartOffset, EndOffset - StartOffset);
}

function b32 String_Is_Whitespace(string String) {
	for (size_t i = 0; i < String.Size; i++) {
		if (!Is_Whitespace(String.Ptr[i])) {
			return false;
		}
	}
	return true;
}

function string String_Insert_Text(allocator* Allocator, string String, string TextToInsert, size_t Cursor) {
	Assert(Cursor <= String.Size);
	if (Cursor == 0) {
		return String_Concat(Allocator, TextToInsert, String);
	} else if (Cursor == String.Size) {
		return String_Concat(Allocator, String, TextToInsert);
	} else {
		string Strings[] = {
			String_Substr(String, 0, Cursor),
			TextToInsert, 
			String_Substr(String, Cursor, String.Size)
		};
		return String_Combine(Allocator, Strings, Array_Count(Strings));
	}
}

function string String_Remove(allocator* Allocator, string String, size_t Cursor, size_t Size) {
	if (String_Is_Empty(String)) {
		return String;
	}

	Assert(Cursor < String.Size);
	Assert((Cursor + Size) <= String.Size);

	if (Cursor == 0) {
		return String_Substr(String, Cursor + Size, String.Size);
	} else if (Cursor + Size == String.Size) {
		return String_Substr(String, 0, Cursor);
	} else {
		string Strings[] = {
			String_Substr(String, 0, Cursor),
			String_Substr(String, Cursor+Size, String.Size)
		};
		return String_Combine(Allocator, Strings, Array_Count(Strings));
	}

}

function b32 String_Ends_With(string String, string Substr) {
	Assert(String.Size && Substr.Size);
	if (Substr.Size > String.Size) return false;

	const char* StrEnd = String.Ptr + (String.Size-1);
	const char* SubstrEnd = Substr.Ptr + (Substr.Size-1);

	while (Substr.Size) {
		if (*SubstrEnd != *StrEnd) {
			return false;
		}
		
		SubstrEnd--;
		StrEnd--;
		Substr.Size--;
	}

	return true;
}

function b32 String_Contains(string String, string Substr) {
	Assert(String.Size && Substr.Size);
	if (Substr.Size > String.Size) return false;

	size_t StringIndex = 0;
	while (StringIndex < String.Size) {
		size_t SubstringIndex = 0;
		size_t TotalStringIndex = StringIndex+SubstringIndex;
		while (SubstringIndex < Substr.Size && TotalStringIndex < String.Size) {
			if (String.Ptr[TotalStringIndex] == Substr.Ptr[SubstringIndex]) {
				SubstringIndex++;
				TotalStringIndex++;
			} else {
				break;
			}
		}

		if (SubstringIndex == Substr.Size) {
			return true;
		}

		StringIndex++;
	}

	return false;
}

function size_t String_Find_First(string String, string Substr) {
	Assert(String.Size && Substr.Size);
	if (Substr.Size > String.Size) return STRING_INVALID_INDEX;

	size_t StringIndex = 0;
	while (StringIndex < String.Size) {
		size_t SubstringIndex = 0;
		size_t TotalStringIndex = StringIndex+SubstringIndex;
		while (SubstringIndex < Substr.Size && TotalStringIndex < String.Size) {
			if (String.Ptr[TotalStringIndex] == Substr.Ptr[SubstringIndex]) {
				SubstringIndex++;
				TotalStringIndex++;
			} else {
				break;
			}
		}

		if (SubstringIndex == Substr.Size) {
			return StringIndex;
		}

		StringIndex++;
	}

	return STRING_INVALID_INDEX;
}

function string String_From_WString(allocator* Allocator, wstring WString) {
	arena* Scratch = Scratch_Get();

    const wchar_t* WStrAt = WString.Ptr;
    const wchar_t* WStrEnd = WStrAt+WString.Size;

    char* StrStart = (char*)Arena_Push(Scratch, (WString.Size*4)+1);
    char* StrEnd = StrStart + WString.Size*4;
    char* StrAt = StrStart;

    for(;;) {
        Assert(StrAt <= StrEnd);
        if(WStrAt >= WStrEnd) {
            Assert(WStrAt == WStrEnd);
            break;
        }

        u32 Length;
        u32 Codepoint = UTF16_Read(WStrAt, &Length);
        WStrAt += Length;
        StrAt += UTF8_Write(StrAt, Codepoint);
    }

    *StrAt = 0;
	string Result = String_Copy(Allocator, Make_String(StrStart, (size_t)(StrAt - StrStart)));
	Scratch_Release();
	return Result;
}

function b32 Try_Parse_Bool(string String, b32* OutBool) {
	b32 EqualsTrue = String_Equals(String, String_Lit("true"));
	if (EqualsTrue) {
		if (OutBool) *OutBool = true;
		return true;
	}

	b32 EqualsFalse = String_Equals(String, String_Lit("false"));
	if (EqualsFalse) {
		if (OutBool) *OutBool = false;
		return true;
	}

	return false;
}

function b32 Try_Parse_Number(string String, f64* OutNumber) {
	char* OutPtr;
	f64 Numeric = strtod(String.Ptr, &OutPtr);
	if ((size_t)(OutPtr - String.Ptr) == String.Size) {
		if (OutNumber) *OutNumber = Numeric;
		return true;
	}
	return false;
}

function b32 Try_Parse_Integer(string String, s64* OutNumber) {
	char* OutPtr;
	s64 Numeric = strtol(String.Ptr, &OutPtr, 10);
	if ((size_t)(OutPtr - String.Ptr) == String.Size) {
		if (OutNumber) *OutNumber = Numeric;
		return true;
	}
	return false;
}

function size_t WString_Length(const wchar_t* Ptr) {
	return wcslen(Ptr);
}

function wstring Make_WString(const wchar_t* Ptr, size_t Size) {
	wstring Result = {
		.Ptr = Ptr,
		.Size = Size
	};
	return Result;
}

function wstring WString_Null_Term(const wchar_t* Ptr) {
	return Make_WString(Ptr, wcslen(Ptr));
}

function wstring WString_Copy(allocator* Allocator, wstring WStr) {
	wchar_t* Ptr = Allocator_Allocate_Array(Allocator, WStr.Size+1, wchar_t);
	Memory_Copy(Ptr, WStr.Ptr, WStr.Size * sizeof(wchar_t));
	Ptr[WStr.Size] = 0;
	return Make_WString(Ptr, WStr.Size);
}


function wstring WString_From_String(allocator* Allocator, string String) {
    arena* Scratch = Scratch_Get();

    const char* StrAt = String.Ptr;
    const char* StrEnd = StrAt+String.Size;

    wchar_t* WStrStart = Arena_Push_Array(Scratch, (String.Size+1), wchar_t);
    wchar_t* WStrEnd = WStrStart + String.Size;
    wchar_t* WStrAt = WStrStart;

    for(;;) {
        Assert(WStrAt <= WStrEnd);
        if(StrAt >= StrEnd) {
            Assert(StrAt == StrEnd);
            break;
        }

        u32 Length;
        u32 Codepoint = UTF8_Read(StrAt, &Length);
        StrAt += Length;
        WStrAt += UTF16_Write(WStrAt, Codepoint);
    }

    *WStrAt = 0; 
    wstring Result = WString_Copy(Allocator, Make_WString(WStrStart, (size_t)(WStrAt-WStrStart)));
	Scratch_Release();
	return Result;
}

function sstream_reader SStream_Reader_Begin(string Content) {
	sstream_reader Reader = { 0 };
	Reader.Start = Content.Ptr;
	Reader.At = Content.Ptr;
	Reader.End = Content.Ptr + Content.Size;
	Reader.LineIndex   = 1;
	Reader.ColumnIndex = 1;

	return Reader;
}

function inline b32 SStream_Reader_Is_Valid(sstream_reader* Reader) {
	return Reader->At < Reader->End;
}

function inline size_t SStream_Reader_Get_Index(sstream_reader* Reader) {
	return ((intptr_t)Reader->At - (intptr_t)Reader->Start);
}

function sstream_char SStream_Reader_Peek_Char(sstream_reader* Reader) {
	Assert(Reader->At < Reader->End);
	sstream_char Result = {
		.Char = *Reader->At,
		.Index = SStream_Reader_Get_Index(Reader),
		.LineIndex = Reader->LineIndex,
		.ColumnIndex = Reader->ColumnIndex
	};

	return Result;
}

function sstream_char SStream_Reader_Consume_Char(sstream_reader* Reader) {
	sstream_char Result = SStream_Reader_Peek_Char(Reader);
	switch (Result.Char) {
		case '\t': {
			Reader->ColumnIndex += 4;
		} break;

		default: {
			if (Is_Newline(Result.Char)) {
				Reader->LineIndex++;
				Reader->ColumnIndex = 1;
			} else {
				Reader->ColumnIndex++;
			}
		} break;
	}
	Reader->At++;
	return Result;
}

function void SStream_Reader_Eat_Whitespace(sstream_reader* Reader) {
	while(SStream_Reader_Is_Valid(Reader)) {
		sstream_char Result = SStream_Reader_Peek_Char(Reader);
		if (!Is_Whitespace(Result.Char)) {
			return;
		}

		switch (Result.Char) {
			case '\t': {
				Reader->ColumnIndex += 4;
			} break;

			default: {
				if (Is_Newline(Result.Char)) {
					Reader->LineIndex++;
					Reader->ColumnIndex = 1;
				} else {
					Reader->ColumnIndex++;
				}
			} break;
		}

		Reader->At++;
	}
}

function void SStream_Reader_Eat_Whitespace_Without_Line(sstream_reader* Reader) {
	while(SStream_Reader_Is_Valid(Reader)) {
		sstream_char Result = SStream_Reader_Peek_Char(Reader);
		if (!Is_Whitespace(Result.Char)) {
			return;
		}

		switch (Result.Char) {
			case '\t': {
				Reader->ColumnIndex += 4;
			} break;

			default: {
				if (Is_Newline(Result.Char)) {
					return;
				} else {
					Reader->ColumnIndex++;
				}
			} break;
		}

		Reader->At++;
	}
}


function string SStream_Reader_Consume_Token(sstream_reader* Reader) {
	SStream_Reader_Eat_Whitespace(Reader);

	const char* Ptr = Reader->At;
	while (SStream_Reader_Is_Valid(Reader)) {
		sstream_char Result = SStream_Reader_Peek_Char(Reader);
		if (Is_Whitespace(Result.Char)) {
			break;
		}

		SStream_Reader_Consume_Char(Reader);
	}

	size_t Size = (size_t)(Reader->At - Ptr);
	Ptr = Size == 0 ? NULL : Ptr;

	return Make_String(Ptr, Size);
}

function string SStream_Reader_Peek_Line(sstream_reader* Reader) {
	const char* Ptr = Reader->At;
	const char* At = Reader->At;
	while (At < Reader->End) {
		if (*At == '\n') {
			break;
		}
		At++;
	}

	size_t Size = (size_t)(At - Ptr);
	Ptr = Size == 0 ? NULL : Ptr;
	return Make_String(Ptr, Size);
}

function string SStream_Reader_Consume_Line(sstream_reader* Reader) {
	const char* Ptr = Reader->At;
	while (SStream_Reader_Is_Valid(Reader)) {
		sstream_char Result = SStream_Reader_Consume_Char(Reader);
		if (Result.Char == '\n') {
			break;
		}
	}

	size_t Size = (size_t)(Reader->At - Ptr);
	Ptr = Size == 0 ? NULL : Ptr;
	return Make_String(Ptr, Size);
}

function void SStream_Reader_Skip_Line(sstream_reader* Reader) {
	while (SStream_Reader_Is_Valid(Reader)) {
		sstream_char Result = SStream_Reader_Consume_Char(Reader);
		if (Result.Char == '\n') {
			break;
		}
	}
}

function inline sstream_writer Begin_Stream_Writer(allocator* Allocator) {
	sstream_writer Writer = { Allocator };
	return Writer;
}

function void SStream_Writer_Add_Front(sstream_writer* Writer, string Entry) {
	sstream_writer_node* Node = Allocator_Allocate_Struct(Writer->Allocator, sstream_writer_node);
	Node->Entry = Entry;
	SLL_Push_Front(Writer->First, Node);
	if (!Writer->Last) {
		Writer->Last = Writer->First;
	}

	Writer->NodeCount++;
	Writer->TotalCharCount += Entry.Size;
}

function void SStream_Writer_Add(sstream_writer* Writer, string Entry) {
	sstream_writer_node* Node = Allocator_Allocate_Struct(Writer->Allocator, sstream_writer_node);
	Node->Entry = Entry;
	SLL_Push_Back(Writer->First, Writer->Last, Node);

	Writer->NodeCount++;
	Writer->TotalCharCount += Entry.Size;
}

function void SStream_Writer_Add_Format(sstream_writer* Writer, const char* Format, ...) {
	va_list List;
	va_start(List, Format);
	string String = String_FormatV(Writer->Allocator, Format, List);
	va_end(List);

	SStream_Writer_Add(Writer, String);
}

function string SStream_Writer_Join(sstream_writer* Writer, allocator* Allocator, string JoinChars) {
	if (!Writer->NodeCount) return String_Empty();
	
	size_t TotalStringSize = Writer->TotalCharCount + ((Writer->NodeCount-1)*JoinChars.Size);
	char* Buffer = Allocator_Allocate_Array(Allocator, TotalStringSize + 1, char);
	char* At = Buffer;

	for (sstream_writer_node* Node = Writer->First; Node; Node = Node->Next) {
		Memory_Copy(At, Node->Entry.Ptr, Node->Entry.Size * sizeof(char));
		At += Node->Entry.Size;
		if (Node != Writer->Last) {
			Memory_Copy(At, JoinChars.Ptr, JoinChars.Size * sizeof(char));
			At += JoinChars.Size;
		}
	}

	Assert((size_t)(At - Buffer) == TotalStringSize);
	Buffer[TotalStringSize] = 0;
	return Make_String(Buffer, TotalStringSize);
}

#define HASH_INVALID_SLOT (u32)-1

function hashmap Hashmap_Init_With_Size(allocator* Allocator, size_t ValueSize, size_t KeySize, u32 ItemCapacity, key_hash_func* HashFunc, key_comp_func* CompareFunc) {
	hashmap Result = { Allocator };
	Result.ItemCapacity = ItemCapacity;
	Result.KeySize = KeySize;
	Result.ValueSize = ValueSize;

	Result.HashFunc = HashFunc;
	Result.CompareFunc = CompareFunc;

	size_t TotalKeySize = KeySize * ItemCapacity;
	size_t TotalValueSize = ValueSize * ItemCapacity;
	size_t TotalSlotSize = sizeof(u32) * ItemCapacity;

	Result.Keys = Allocator_Allocate_Memory(Result.Allocator, TotalKeySize + TotalValueSize + TotalSlotSize);
	Result.Values = Offset_Pointer(Result.Keys, TotalKeySize);
	Result.ItemSlots = (u32*)Offset_Pointer(Result.Values, TotalValueSize);

	Result.ItemCount = 0;

	return Result;
}

function inline hashmap Hashmap_Init(allocator* Allocator, size_t ItemSize, size_t KeySize, key_hash_func* HashFunc, key_comp_func* CompareFunc) {
	return Hashmap_Init_With_Size(Allocator, ItemSize, KeySize, 256, HashFunc, CompareFunc);
}

function inline b32 Hashmap_Is_Valid(hashmap* Hashmap) {
	return Hashmap->Keys != NULL;
}

function void Hashmap_Clear(hashmap* Hashmap) {
	Assert(Hashmap_Is_Valid(Hashmap));

	Memory_Clear(Hashmap->Slots, Hashmap->SlotCapacity * sizeof(hash_slot));

	for(u32 SlotIndex = 0; SlotIndex < Hashmap->SlotCapacity; SlotIndex++) {
		Hashmap->Slots[SlotIndex].ItemIndex = HASH_INVALID_SLOT;
    }

	Memory_Clear(Hashmap->Keys, Hashmap->KeySize * Hashmap->ItemCount);
	Memory_Clear(Hashmap->Values, Hashmap->ValueSize * Hashmap->ItemCount);
	for (u32 i = 0; i < Hashmap->ItemCount; i++) {
		Hashmap->ItemSlots[i] = HASH_INVALID_SLOT;
	}

    Hashmap->ItemCount = 0;
}

function u32 Hashmap_Find_Slot(hashmap* Hashmap, void* Key, u32 Hash) {
	if (Hashmap->SlotCapacity == 0 || !Hashmap->Slots) return HASH_INVALID_SLOT;

	u32 SlotMask = Hashmap->SlotCapacity - 1;
	u32 BaseSlot = (Hash & SlotMask);
	u32 BaseCount = Hashmap->Slots[BaseSlot].BaseCount;
	u32 Slot = BaseSlot;

	while (BaseCount > 0) {
		if (Hashmap->Slots[Slot].ItemIndex != HASH_INVALID_SLOT) {
			u32 SlotHash = Hashmap->Slots[Slot].Hash;
			u32 SlotBase = (SlotHash & SlotMask);
			if (SlotBase == BaseSlot) {
				Assert(BaseCount > 0);
				BaseCount--;

				if (SlotHash == Hash) {
					void* KeyComp = Offset_Pointer(Hashmap->Keys, Hashmap->Slots[Slot].ItemIndex * Hashmap->KeySize);
					if (Hashmap->CompareFunc(Key, KeyComp)) {
						return Slot;
					}
				}
			}
		}

		Slot = (Slot + 1) & SlotMask;
	}

	return HASH_INVALID_SLOT;
}

function void Hashmap_Expand_Slots(hashmap* Hashmap, u32 NewCapacity) {
	NewCapacity = Ceil_Pow2_U32(NewCapacity);
	u32 SlotMask = NewCapacity - 1;

	hash_slot* NewSlots = Allocator_Allocate_Array(Hashmap->Allocator, NewCapacity, hash_slot);
	for (u32 i = 0; i < NewCapacity; i++) {
		NewSlots[i].ItemIndex = HASH_INVALID_SLOT;
	}

	for (u32 i = 0; i < Hashmap->SlotCapacity; i++) {
		if (Hashmap->Slots[i].ItemIndex != HASH_INVALID_SLOT) {
			u32 Hash = Hashmap->Slots[i].ItemIndex;
			u32 BaseSlot = (Hash & SlotMask);
			u32 Slot = BaseSlot;
			while (NewSlots[Slot].ItemIndex != HASH_INVALID_SLOT) {
				Slot = (Slot + 1) & SlotMask;
			}
			NewSlots[Slot].Hash = Hash;
			u32 ItemIndex = Hashmap->Slots[i].ItemIndex;
			NewSlots[Slot].ItemIndex = ItemIndex;
			Hashmap->ItemSlots[ItemIndex] = Slot;
			NewSlots[BaseSlot].BaseCount++;
		}
	}

	if (Hashmap->Slots) {
		Allocator_Free_Memory(Hashmap->Allocator, Hashmap->Slots);
	}
	Hashmap->Slots = NewSlots;
	Hashmap->SlotCapacity = NewCapacity;
}

function void Hashmap_Add_By_Hash(hashmap* Hashmap, void* Key, void* Value, u32 Hash) {
	Assert(Hashmap_Is_Valid(Hashmap));
	Assert(Hashmap_Find_Slot(Hashmap, Key, Hash) == HASH_INVALID_SLOT);

	if (Hashmap->ItemCount >= (Hashmap->SlotCapacity - (Hashmap->SlotCapacity / 3))) {
		u32 NewSlotCapacity = Hashmap->SlotCapacity ? Hashmap->SlotCapacity*2 : 128;
		Hashmap_Expand_Slots(Hashmap, NewSlotCapacity);
	}

	u32 SlotMask = Hashmap->SlotCapacity - 1;
	u32 BaseSlot = (Hash & SlotMask);

	//Linear probing. This is not great for preventing collision
    u32 BaseCount = Hashmap->Slots[BaseSlot].BaseCount;
    u32 Slot = BaseSlot;
    u32 FirstFree = Slot;
    while (BaseCount) 
    {
        if (Hashmap->Slots[Slot].ItemIndex == HASH_INVALID_SLOT && Hashmap->Slots[FirstFree].ItemIndex != HASH_INVALID_SLOT) FirstFree = Slot;
        u32 SlotHash = Hashmap->Slots[Slot].Hash;
        u32 SlotBase = (SlotHash & SlotMask);
        if (SlotBase == BaseSlot) 
            --BaseCount;
        Slot = (Slot + 1) & SlotMask;
    }
    
    Slot = FirstFree;
    while (Hashmap->Slots[Slot].ItemIndex != HASH_INVALID_SLOT) 
        Slot = (Slot + 1) & SlotMask;

	Assert(Hashmap->ItemCount < Hashmap->ItemCapacity);
	if (Hashmap->ItemCount == Hashmap->ItemCapacity) {
		size_t NewItemCapacity = Hashmap->ItemCapacity * 2;
		size_t TotalKeySize = Hashmap->KeySize * NewItemCapacity;
		size_t TotalValueSize = Hashmap->ValueSize * NewItemCapacity;
		size_t TotalSlotSize = sizeof(u32) * NewItemCapacity;

		void* Keys = Allocator_Allocate_Memory(Hashmap->Allocator, TotalKeySize + TotalValueSize + TotalSlotSize);
		void* Values = Offset_Pointer(Hashmap->Keys, TotalKeySize);
		u32* ItemSlots = (u32*)Offset_Pointer(Hashmap->Values, TotalValueSize);

		Memory_Copy(Keys, Hashmap->Keys, Hashmap->ItemCapacity * Hashmap->KeySize);
		Memory_Copy(Values, Hashmap->Values, Hashmap->ItemCapacity * Hashmap->ValueSize);
		Memory_Copy(ItemSlots, Hashmap->ItemSlots, Hashmap->ItemCapacity * sizeof(u32));

		Allocator_Free_Memory(Hashmap->Allocator, Hashmap->Keys);

		Hashmap->ItemCapacity = (u32)NewItemCapacity;
		Hashmap->Keys = Keys;
		Hashmap->Values = Values;
		Hashmap->ItemSlots = ItemSlots;
	}

	u32 Index = Hashmap->ItemCount++;

	Hashmap->Slots[Slot].Hash = Hash;
	Hashmap->Slots[Slot].ItemIndex = Index;
	Hashmap->Slots[BaseSlot].BaseCount++;

	Memory_Copy(Offset_Pointer(Hashmap->Keys, (Index*Hashmap->KeySize)), Key, Hashmap->KeySize);
	Memory_Copy(Offset_Pointer(Hashmap->Values, (Index*Hashmap->ValueSize)), Value, Hashmap->ValueSize);
	Hashmap->ItemSlots[Index] = Slot;
}

function inline u32 Hashmap_Hash(hashmap* Hashmap, void* Key) {
	return Hashmap->HashFunc(Key);
}

function void Hashmap_Add(hashmap* Hashmap, void* Key, void* Value) {
	u32 Hash = Hashmap_Hash(Hashmap, Key); 
	Hashmap_Add_By_Hash(Hashmap, Key, Value, Hash);
}

function inline b32 Hashmap_Find_By_Hash(hashmap* Hashmap, void* Key, void* Value, u32 Hash) {
	Assert(Hashmap_Is_Valid(Hashmap));
	u32 Slot = Hashmap_Find_Slot(Hashmap, Key, Hash);
	if (Slot == HASH_INVALID_SLOT) return false;
	u32 ItemIndex = Hashmap->Slots[Slot].ItemIndex;
	Memory_Copy(Value, Offset_Pointer(Hashmap->Values, (ItemIndex * Hashmap->ValueSize)), Hashmap->ValueSize);
	return true;
}

function inline b32 Hashmap_Find(hashmap* Hashmap, void* Key, void* Value) {
	u32 Hash = Hashmap_Hash(Hashmap, Key);
	b32 Result = Hashmap_Find_By_Hash(Hashmap, Key, Value, Hash);
	return Result;
}

function inline b32 Hashmap_Get_Value(hashmap* Hashmap, size_t Index, void* Value) {
	if (Index >= Hashmap->ItemCount) return false;
	Memory_Copy(Value, Offset_Pointer(Hashmap->Values, (Index * Hashmap->ValueSize)), Hashmap->ValueSize);
	return true;
}

function inline b32 Hashmap_Get_Key(hashmap* Hashmap, size_t Index, void* Key) {
	if (Index >= Hashmap->ItemCount) return false;
	Memory_Copy(Key, Offset_Pointer(Hashmap->Keys, (Index * Hashmap->KeySize)), Hashmap->KeySize);
	return true;
}

function inline b32 Hashmap_Get_Key_Value(hashmap* Hashmap, size_t Index, void* Key, void* Value) {
	if (Index >= Hashmap->ItemCount) return false;
	Memory_Copy(Key, Offset_Pointer(Hashmap->Keys, (Index * Hashmap->KeySize)), Hashmap->KeySize);
	Memory_Copy(Value, Offset_Pointer(Hashmap->Values, (Index * Hashmap->ValueSize)), Hashmap->ValueSize);
	return true;
}

function inline slot_id Slot_Null() {
	slot_id Result = { .ID = 0 };
	return Result;
}

function void Slot_Map_Clear(slot_map* SlotMap) {
	SlotMap->FreeCount = SlotMap->Capacity;
	SlotMap->Count = 0;
	for (size_t i = 0; i < SlotMap->Capacity; i++) {
		SlotMap->Slots[i] = 1;
		SlotMap->FreeIndices[i] = (u32)i;
	}
}

function slot_map Slot_Map_Init(allocator* Allocator, size_t Capacity) {
	slot_map Result = { 0 };
	Result.Slots = Allocator_Allocate_Array(Allocator, 2 * Capacity, u32);
	Result.FreeIndices = Result.Slots + Capacity;
	Result.Capacity = Capacity;
	Slot_Map_Clear(&Result);
	return Result;
}

function slot_id Slot_Map_Allocate(slot_map* SlotMap) {
	Assert(SlotMap->FreeCount);
	u32 FreeIndex = SlotMap->FreeIndices[--SlotMap->FreeCount];
	
	//Set the slot as allocated (last bit)
	Set_Bit(SlotMap->Slots[FreeIndex], 31);
	
	SlotMap->Count++;
	slot_id Result = { .Index = FreeIndex, .Slot = SlotMap->Slots[FreeIndex] };
	return Result;
}

function void Slot_Map_Free(slot_map* SlotMap, slot_id ID) {
	Assert(ID.ID);
	if (!ID.ID) {
		return;
	}
	
	u32 Index = ID.Index;
	Assert(Index < SlotMap->Capacity);

	if (SlotMap->Slots[Index] == ID.Slot) {
		SlotMap->Slots[Index]++;
		Clear_Bit(SlotMap->Slots[Index], 31);

		if (!SlotMap->Slots[Index]) {
			SlotMap->Slots[Index] = 1;
		}

		SlotMap->FreeIndices[SlotMap->FreeCount++] = Index;
		SlotMap->Count--;
	}
}

function inline b32 Slot_Map_Is_Allocated(slot_map* SlotMap, slot_id SlotID) {
	if (!SlotID.ID) return false;
	u32 Index = SlotID.Index;
	Assert(Index < SlotMap->Capacity);
	return SlotMap->Slots[Index] == SlotID.Slot;
}

function inline b32 Slot_Map_Is_Allocated_Index(slot_map* SlotMap, size_t Index) {
	Assert(Index < SlotMap->Capacity);
	u32 Slot = SlotMap->Slots[Index];
	return Read_Bit(Slot, 31) != 0;
}

function inline slot_id Slot_Map_Get_ID(slot_map* SlotMap, size_t Index) {
	Assert(Index < SlotMap->Capacity);
	slot_id Result = { .Index = (u32)Index, .Slot = SlotMap->Slots[Index] };
	return Result;
}


#define Binary_Heap_Parent(i) ((i - 1) / 2)
// to get index of left child of node at index i 
#define Binary_Heap_Left(i) (2 * i + 1)
// to get index of right child of node at index i 
#define Binary_Heap_Right(i) (2 * i + 2)

function b32 Binary_Heap_Is_Empty(binary_heap* Heap) {
	b32 Result = Heap->Count == 0;
	return Result;
}

function binary_heap Binary_Heap_Init(void* Ptr, size_t Capacity, size_t ItemSize, binary_heap_compare_func* CompareFunc) {
	binary_heap Result = {
		.CompareFunc = CompareFunc,
		.Capacity = Capacity,
		.ItemSize = ItemSize,
		.Ptr = (u8*)Ptr
	};
	return Result; 
}

function binary_heap Binary_Heap_Alloc(arena* Arena, size_t Capacity, size_t ItemSize, binary_heap_compare_func* CompareFunc) {
	void* Result = Arena_Push(Arena, Capacity * ItemSize);
	return Binary_Heap_Init(Result, Capacity, ItemSize, CompareFunc);
}

function void* Binary_Heap_Get_Data(binary_heap* Heap, size_t Index) {	
	Assert(Index < Heap->Count);
	void* Ptr = Heap->Ptr + Heap->ItemSize*Index;
	return Ptr;
}

function void Binary_Heap_Swap(binary_heap* Heap, size_t Index0, size_t Index1) {
	arena* Scratch = Scratch_Get();

	void* Tmp = Arena_Push(Scratch, Heap->ItemSize);
	void* Data1 = Binary_Heap_Get_Data(Heap, Index0);
	Memory_Copy(Tmp, Data1, Heap->ItemSize);
	void* Data2 = Binary_Heap_Get_Data(Heap, Index1);
	Memory_Copy(Data1, Data2, Heap->ItemSize);
	Memory_Copy(Data2, Tmp, Heap->ItemSize);

	Scratch_Release();
}

function void Binary_Heap_Maxify(binary_heap* Heap) {
	size_t Index = 0;
	size_t Largest = Index;
	do {
		size_t Left = Binary_Heap_Left(Index);
		size_t Right = Binary_Heap_Right(Index);

		size_t Count = Heap->Count;

		binary_heap_compare_func* CompareFunc = Heap->CompareFunc;
		if (Left < Count && CompareFunc(Binary_Heap_Get_Data(Heap, Left),
										Binary_Heap_Get_Data(Heap, Index))) {
			Largest = Left;
		}

		if (Right < Count && CompareFunc(Binary_Heap_Get_Data(Heap, Right),
										 Binary_Heap_Get_Data(Heap, Largest))) {
			Largest = Right;
		}

		if (Largest != Index) {
			Binary_Heap_Swap(Heap, Index, Largest);
			Index = Largest;
		}
		
	} while (Largest != Index);
}

function void Binary_Heap_Push(binary_heap* Heap, void* Data) {
	Assert(Heap->Count < Heap->Capacity);
	size_t Index = Heap->Count++;
	Memory_Copy(Binary_Heap_Get_Data(Heap, Index), 
				Data, Heap->ItemSize);
	binary_heap_compare_func* CompareFunc = Heap->CompareFunc;

	//If the parent of the binary heap is smaller, we need to fixup the structure
	while (Index != 0 && !CompareFunc(Binary_Heap_Get_Data(Heap, Binary_Heap_Parent(Index)),
									  Binary_Heap_Get_Data(Heap, Index))) {
		Binary_Heap_Swap(Heap, Index, Binary_Heap_Parent(Index));
		Index = Binary_Heap_Parent(Index);
	}
}

function void* Binary_Heap_Pop(binary_heap* Heap) {
	Assert(Heap->Count);
	if (Heap->Count == 1) {
		Heap->Count--;
		return Heap->Ptr;
	}

	Binary_Heap_Swap(Heap, 0, Heap->Count-1);
	void* Result = Binary_Heap_Get_Data(Heap, Heap->Count-1);
	Heap->Count--;
	Binary_Heap_Maxify(Heap);
	return Result;
}

function async_stack_index16_key Async_Stack_Index16_Make_Key(u16 Index, u16 Key) {
    async_stack_index16_key Result;
    Result.Index = Index;
    Result.Key = Key;
    return Result;
}

function void Async_Stack_Index16_Init_Raw(async_stack_index16* StackIndex, u16* IndicesPtr, u16 Capacity) {
    /*Make sure atomic is properly aligned*/
    Assert((((size_t)&StackIndex->Head) % 4) == 0);
    StackIndex->NextIndices = IndicesPtr;
    StackIndex->Capacity = Capacity;
    async_stack_index16_key* HeadKey = (async_stack_index16_key*)&StackIndex->Head; 
    HeadKey->Index = ASYNC_STACK_INDEX16_INVALID;
    HeadKey->Key = 0;
}

function void Async_Stack_Index16_Push_Sync(async_stack_index16* StackIndex, u16 Index) {
    async_stack_index16_key* CurrentTop = (async_stack_index16_key*)&StackIndex->Head;
    u16 Current = CurrentTop->Index;
    StackIndex->NextIndices[Index] = Current;
    CurrentTop->Index = Index;
}

function void Async_Stack_Index16_Push(async_stack_index16* StackIndex, u16 Index) {
    for(;;) {
        async_stack_index16_key CurrentTop = { Atomic_Load_U32(&StackIndex->Head)};
        u16 Current = CurrentTop.Index;
        StackIndex->NextIndices[Index] = Current;
        async_stack_index16_key NewTop = Async_Stack_Index16_Make_Key(Index, CurrentTop.Key+1); /*Increment key to avoid ABA problem*/
        /*Add job index to the freelist atomically*/
        if(Atomic_Compare_Exchange_U32(&StackIndex->Head, CurrentTop.ID, NewTop.ID) == CurrentTop.ID) {
            return;
        }
    }
}

function u16 Async_Stack_Index16_Pop(async_stack_index16* StackIndex) {
    for(;;) {
        async_stack_index16_key CurrentTop = { Atomic_Load_U32(&StackIndex->Head)};

        u16 Index = CurrentTop.Index;
        if(Index == ASYNC_STACK_INDEX16_INVALID) {
            /*No more jobs avaiable*/
            return ASYNC_STACK_INDEX16_INVALID;
        }
        Assert(Index < StackIndex->Capacity); /*Overflow*/
        u16 Next = StackIndex->NextIndices[Index];
        async_stack_index16_key NewTop = Async_Stack_Index16_Make_Key(Next, CurrentTop.Key+1); /*Increment key to avoid ABA problem*/
        /*Atomically update the job freelist*/
        if(Atomic_Compare_Exchange_U32(&StackIndex->Head, CurrentTop.ID, NewTop.ID) == CurrentTop.ID) {
            return Index;
        }
    }
}

function async_stack_index32_key Async_Stack_Index32_Make_Key(u32 Index, u32 Key) {
    async_stack_index32_key Result;
    Result.Index = Index;
    Result.Key = Key;
    return Result;
}

function void Async_Stack_Index32_Init_Raw(async_stack_index32* StackIndex, u32* IndicesPtr, u32 Capacity) {
    /*Make sure atomic is properly aligned*/
    Assert((((size_t)&StackIndex->Head) % 8) == 0);
    StackIndex->NextIndices = IndicesPtr;
    StackIndex->Capacity = Capacity;
    async_stack_index32_key* HeadKey = (async_stack_index32_key*)&StackIndex->Head; 
    HeadKey->Index = ASYNC_STACK_INDEX32_INVALID;
    HeadKey->Key = 0;
}

function void Async_Stack_Index32_Push_Sync(async_stack_index32* StackIndex, u32 Index) {
    async_stack_index32_key* CurrentTop = (async_stack_index32_key*)&StackIndex->Head;
    u32 Current = CurrentTop->Index;
    StackIndex->NextIndices[Index] = Current;
    CurrentTop->Index = Index;
}

function void Async_Stack_Index32_Push(async_stack_index32* StackIndex, u32 Index) {
    for(;;) {
        async_stack_index32_key CurrentTop = { Atomic_Load_U64(&StackIndex->Head)};
        u32 Current = CurrentTop.Index;
        StackIndex->NextIndices[Index] = Current;
        async_stack_index32_key NewTop = Async_Stack_Index32_Make_Key(Index, CurrentTop.Key+1); /*Increment key to avoid ABA problem*/
        /*Add job index to the freelist atomically*/
        if(Atomic_Compare_Exchange_U64(&StackIndex->Head, CurrentTop.ID, NewTop.ID) == CurrentTop.ID) {
            return;
        }
    }
}

function u32 Async_Stack_Index32_Pop(async_stack_index32* StackIndex) {
    for(;;) {
        async_stack_index32_key CurrentTop = { Atomic_Load_U64(&StackIndex->Head)};

        u32 Index = CurrentTop.Index;
        if(Index == ASYNC_STACK_INDEX32_INVALID) {
            /*No more jobs avaiable*/
            return ASYNC_STACK_INDEX32_INVALID;
        }
        Assert(Index < StackIndex->Capacity); /*Overflow*/
        u32 Next = StackIndex->NextIndices[Index];
        async_stack_index32_key NewTop = Async_Stack_Index32_Make_Key(Next, CurrentTop.Key+1); /*Increment key to avoid ABA problem*/
        /*Atomically update the job freelist*/
        if(Atomic_Compare_Exchange_U64(&StackIndex->Head, CurrentTop.ID, NewTop.ID) == CurrentTop.ID) {
            return Index;
        }
    }
}

function void Debug_Log(const char* Format, ...) {
	arena* Scratch = Scratch_Get();

	va_list List;
	va_start(List, Format);
	string String = String_FormatV((allocator *)Scratch, Format, List);
	va_end(List);

#ifdef OS_WIN32
	wstring WString = WString_From_String((allocator *)Scratch, String);
	OutputDebugStringW(WString.Ptr);

	if(WString.Size) OutputDebugStringA("\n");
#endif

	if(String.Size) printf("%.*s\n", (int)String.Size, String.Ptr);

	Scratch_Release();
}

//Hash functions
global const XXH32_hash_t InitialSeed = 1234567890;

function inline u32 U32_Hash_U64(u64 Value) {
	u32 Result = (u32)XXH32(&Value, sizeof(u64), InitialSeed);
	return Result;
}

function inline u32 U32_Hash_String(string String) {
	return (u32)XXH32(String.Ptr, String.Size, InitialSeed);
}

function inline u64 U64_Hash_String_With_Seed(string String, u64 Seed) {
	return (u64)XXH64(String.Ptr, String.Size, (XXH64_hash_t)Seed);
}

function inline u32 U32_Hash_Bytes(void* Data, size_t Size) {
	return (u32)XXH32(Data, Size, InitialSeed);
}

function inline u32 U32_Hash_Bytes_With_Seed(void* Data, size_t Size, u32 Seed) {
	return (u32)XXH32(Data, Size, Seed);
}

function inline u64 U64_Hash_Bytes(void* Data, size_t Size) {
	return (u64)XXH64(Data, Size, InitialSeed);
}

function inline u32 U32_Hash_Ptr(void* Ptr) {
	size_t Value = (size_t)Ptr;
	return U32_Hash_Bytes(&Value, sizeof(size_t));
}

function inline u32 U32_Hash_Ptr_With_Seed(void* Ptr, u32 Seed) {
	size_t Value = (size_t)Ptr;
	return U32_Hash_Bytes_With_Seed(&Value, sizeof(size_t), Seed);
}

function inline u32 Hash_String(void* A) {
	string* StringA = (string*)A;
	return U32_Hash_String(*StringA);
}

function inline b32 Compare_String(void* A, void* B) {
	string* StringA = (string*)A;
	string* StringB = (string*)B;
	return String_Equals(*StringA, *StringB);
}

#include "akon.c"
#include "job.c"