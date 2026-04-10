/* heapMaxFreeSize.c */

#include "heap_types.h"

SceSize heapMaxFreeSize( HeapUID uid )
{
	SceSize maxsize, freesize;
	uintptr_t endaddr;
	struct heap_allocated_memblock_header *cur = (struct heap_allocated_memblock_header *)uid;
	struct heap_allocated_memblock_header *prev;
	
	if( ! cur->next ){
		return cur->size;
	} else{
		endaddr = (uintptr_t)cur + cur->size;
		prev = (struct heap_allocated_memblock_header *)((uintptr_t)cur + sizeof( struct heap_allocated_memblock_header ));
		cur  = cur->next;
		
		/* root‚ÆÅ‰‚Ì—v‘f‚ÌŠÔ‚Ì‹ó‚«—e—Ê‚ðŒvŽZ */
		maxsize = (uintptr_t)cur - (uintptr_t)prev;
		
		prev = cur;
		cur  = cur->next;
	}
	
	for( maxsize = 0; cur; prev = cur, cur = cur->next ){
		freesize = (uintptr_t)cur - ( (uintptr_t)prev + prev->size );
		if( maxsize < freesize ) maxsize = freesize;
	}
	
	
	freesize = endaddr - ( (uintptr_t)prev + prev->size );
	if( maxsize < freesize ) maxsize = freesize;
	
	return maxsize;
}
