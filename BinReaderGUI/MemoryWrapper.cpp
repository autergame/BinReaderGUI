#include "MemoryWrapper.h"

void* mallocb(size_t size)
{
	void *pointer = mi_malloc(size);
	//void* pointer = malloc(size);
	myassert(pointer == nullptr)
#ifdef TRACY_ENABLE_ZONES
		TracyAlloc(pointer, size);
#endif
	return pointer;
}

void* reallocb(void *pointer, size_t size)
{
	void *newpointer = nullptr;
	//newpointer = realloc(pointer, size);
	size_t usable = mi_usable_size(pointer);
	if (usable >= size) {
		newpointer = mi_realloc(pointer, size);
	} 
	else {
		newpointer = mallocb(size);
		myassert(memcpy(newpointer, pointer, size) != newpointer)
		mi_free(pointer);
	}
	myassert(newpointer == nullptr)
	return newpointer;
}

void freeb(void *pointer)
{
#ifdef TRACY_ENABLE_ZONES
	TracyFree(pointer);
#endif
	mi_free(pointer);
	//free(pointer);
}

void* MallocbWrapper(size_t size, void *user_data) {
	IM_UNUSED(user_data); return mallocb(size);
}
void FreebWrapper(void *ptr, void *user_data) {
	IM_UNUSED(user_data); freeb(ptr);
}

void operator delete(void *p) noexcept { freeb(p); };
void operator delete[](void *p) noexcept { freeb(p); };

void operator delete(void *p, size_t n) noexcept { freeb(p); };
void operator delete[](void *p, size_t n) noexcept { freeb(p); };

void* operator new(size_t n) noexcept(false) { return mallocb(n); }
void* operator new[](size_t n) noexcept(false) { return mallocb(n); }