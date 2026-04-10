/* memoryRealloc.c */

#include "memory_types.h"

void *memoryRealloc( void *src_memblock, SceSize new_size )
{
	void *new_memblock;
	SceSize src_real_size, new_real_size;
	
	if( ! src_memblock && ! new_size ){
		return NULL;
	} else if( ! src_memblock ){
		return memoryAlloc( new_size );
	} else if( ! new_size ){
		memoryFree( src_memblock );
		return NULL;
	}
	
	src_real_size = memoryUsableSize( src_memblock );
	new_real_size = MEMORY_REAL_SIZE( new_size );
	if( src_real_size == new_real_size ) return src_memblock;
	
	new_memblock = memoryAlloc( new_real_size );
	if( ! new_memblock ) return NULL;
	
	memcpy( new_memblock, src_memblock, ( src_real_size > new_real_size ? new_real_size : src_real_size ) );
	memoryFree( src_memblock );
	
	return new_memblock;
}
