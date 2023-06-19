#pragma once
#include <ntifs.h>
#include <memory>

namespace krnlib
{
const ULONG kPoolTag = 0x14451100;

template<class T>
inline T* Allocate(size_t count) {
	return (T*)ExAllocatePoolWithTag(PagedPool, sizeof(T) * count, kPoolTag);
}

inline void Deallocate(void* mem) {
	ExFreePoolWithTag(mem, kPoolTag);
}

template<class T, class... ArgsT>
inline T* Construct(T* dest_mem, ArgsT&&... args) {
	return ::new(std::_Voidify_iter(dest_mem)) T(std::forward<ArgsT>(args)...);
}

template<class T>
inline void Destroy(T* ptr) {
	ptr->~T();
}

template<class T, class... ArgsT>
inline T* New(ArgsT... args) {
	return Construct(Allocate<T>(1), std::forward<ArgsT>(args)...);
}

template<class T>
inline void Delete(T* ptr) {
	Destroy(ptr);
	Deallocate(ptr);
}
}