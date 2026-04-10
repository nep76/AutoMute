/* heapUsableSize.c */

#include "heap_types.h"

SceSize heapUsableSize( HeapUID uid )
{
	return ((struct heap_allocated_memblock_header *)uid)->size;
}
