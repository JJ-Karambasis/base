#ifndef BASE_INL_H
#define BASE_INL_H

#ifdef __cplusplus

#include <initializer_list>
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

#endif

#endif