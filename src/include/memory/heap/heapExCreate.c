/* heapExCreate.c */

#include "heap_types.h"

HeapUID heapExCreate( const char *name, MemoryPartition partition, unsigned int align, SceSize size, int type, void *addr )
{
	struct heap_allocated_memblock_header *heap_root = (struct heap_allocated_memblock_header *)memoryAllocEx( name, partition, align, size + sizeof( struct heap_allocated_memblock_header ), type, addr );
	if( heap_root ){
		heap_root->next = NULL;
		heap_root->size = memoryUsableSize( (void *)heap_root ) - sizeof( struct heap_allocated_memblock_header );
	}
	
	return (HeapUID)heap_root;
}

void heapDestroy( HeapUID uid )
{
	memoryFree( (void *)uid );
}
