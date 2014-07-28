#ifndef __ELIB_MEMORY_H__
#define __ELIB_MEMORY_H__

#include <elib/types.h>

#define e_slice_new(type) \
	((type *)e_slice_alloc(sizeof(type)))

#define e_slice_free(type, mem) \
	e_slice_free1(sizeof(type), (mem))

ePointer e_slice_alloc(euint size);
void e_slice_free1(euint size, ePointer mem);
void e_slice_set_level(euint size, euint level);


ePointer e_heap_malloc(elong, bool);
void e_heap_free(ePointer);
ePointer e_heap_realloc(ePointer ptr, euint size);

#endif
