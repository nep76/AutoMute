/* memoryReallocf.c */

#include "memory_types.h"

void *memoryReallocf( void *src_memblock, SceSize new_size )
{
	void *new_memblock = memoryRealloc( src_memblock, new_size );
	
	if( src_memblock && new_size && ! new_memblock ) memoryFree( src_memblock );
	return new_memblock;
}
