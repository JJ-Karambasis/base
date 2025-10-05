#ifndef BASE_INL_H
#define BASE_INL_H

#ifdef __cplusplus

#include <initializer_list>

function inline v2 operator+(v2 A, v2 B) {
	return V2_Add_V2(A, B);
}

function inline v2 operator-(v2 A, v2 B) {
	return V2_Sub_V2(A, B);
}

function inline v2 operator*(v2 A, f32 B) {
	return V2_Mul_S(A, B);
}

function inline v2 operator/(v2 A, f32 B) {
	return V2_Div_S(A, B);
}

function inline v2& operator+=(v2& A, v2 B) {
	A = V2_Add_V2(A, B);
	return A;
}

function inline v2& operator*=(v2& A, f32 B) {
	A = V2_Mul_S(A, B);
	return A;
}

function inline bool operator!=(v2i A, v2i B) {
	return A.x != B.x || A.y != B.y;
}

function inline v3 operator+(v3 A, v3 B) {
	v3 Result = V3_Add_V3(A, B);
	return Result;
}

function inline v3& operator+=(v3& A, v3 B) {
	A = V3_Add_V3(A, B);
	return A;
}

function inline v3 operator*(v3 A, f32 B) {
	v3 Result = V3_Mul_S(A, B);
	return Result;
}

function inline v3& operator*=(v3& A, f32 B) {
	A = V3_Mul_S(A, B);
	return A;
}

function inline quat operator*(quat A, quat B) {
	quat Result = Quat_Mul_Quat(A, B);
	return Result;
}

function inline m4 operator*(const m4_affine& A, const m4& B) {
	m4 Result = M4_Affine_Mul_M4(&A, &B);
	return Result;
}

template <typename type>
struct span {
	const type* Ptr = NULL;
	size_t 		Count = 0;

	span() = default;
	inline span(std::initializer_list<type> List) {
		Ptr = List.begin();
		Count = List.size();
	}

	inline span(const type* _Ptr, size_t _Count) : Ptr(_Ptr), Count(_Count) {}
};

template <typename type>
struct pool_handle {
	pool_id ID;
};

template<typename type>
struct pool_t : public pool {
	
};

template<typename type>
function inline void Pool_Init(pool_t<type>* Pool) {
	Pool_Init(Pool, sizeof(type));
}

template<typename type>
function inline pool_handle<type> Pool_Allocate(pool_t<type>* Pool) {
	pool_handle<type> Result = {
		.ID = Pool_Allocate((pool*)Pool)
	};
	return Result;
}

template<typename type>
function inline void Pool_Free(pool_t<type>* Pool, pool_handle<type> Handle) {
	Pool_Free((pool*)Pool, Handle.ID);
}

template<typename type>
function inline type* Pool_Get(pool_t<type>* Pool, pool_handle<type> Handle) {
	return (type*)Pool_Get((pool*)Pool, Handle.ID);
}

#endif

#endif