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
};

#endif

#endif