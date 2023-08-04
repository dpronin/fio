#include <cassert>
#include <cstdlib>

#include "memalign.h"
#include "smalloc.h"

#define PTR_ALIGN(ptr, mask)   \
	(char *)((uintptr_t)((ptr) + (mask)) & ~(mask))

struct align_footer {
	unsigned int offset;
};

void *__fio_memalign(size_t alignment, size_t size, malloc_fn fn)
{
	struct align_footer *f;
	void *ptr, *ret = NULL;

	assert(!(alignment & (alignment - 1)));

	ptr = fn(size + alignment + sizeof(*f) - 1);
	if (ptr) {
		ret = PTR_ALIGN((char*)(ptr), alignment - 1);
		f = reinterpret_cast<struct align_footer *>((char*)ret + size);
		f->offset = (uintptr_t) ret - (uintptr_t) ptr;
	}

	return ret;
}

void __fio_memfree(void *ptr, size_t size, free_fn fn)
{
	struct align_footer *f = reinterpret_cast<struct align_footer *>((char*)ptr + size);

	fn((char*)ptr - f->offset);
}
