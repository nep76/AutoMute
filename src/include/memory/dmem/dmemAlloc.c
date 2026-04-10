/* dmemAlloc.c */

#include "dmem_types.h"

static size_t dmem_need_heapsize( size_t minblock, size_t requestsize );
static struct dmem_heap *dmem_heap_new( size_t heapsize, int type );

void *dmemAlloc( DmemUID uid, size_t size )
{
	struct dmem_root *root = (struct dmem_root *)uid;
	struct dmem_heap **ptr;
	
	if( ! root->lastUse ){
		/* まだヒープが一つも無ければ最初のヒープを作成する */
		root->heapList = dmem_heap_new( dmem_need_heapsize( root->minHeapSize, size ), root->allocType );
		if( ! root->heapList ) return NULL;
		root->lastUse = root->heapList;
	} else if( root->lastUse->maxFreeSize < size + DMEM_HEADER_SIZE ){
		struct dmem_heap *heap;
		
		/* 十分な空き容量のあるヒープを探す */
		for( heap = root->heapList; heap; heap = heap->next ){
			if( heap->maxFreeSize >= size + DMEM_HEADER_SIZE ) break;
		}
		
		/* 空きが無ければ新規作成 */
		if( ! heap ){
			struct dmem_heap *last;
			
			heap = dmem_heap_new( dmem_need_heapsize( root->minHeapSize, size ), root->allocType );
			if( ! heap ) return NULL;
			
			for( last = root->lastUse; last->next; last = last->next );
			last->next = heap;
		}
		root->lastUse = heap;
	}
	
	/* 空きを見つけた、あるいは新規作成したヒープからメモリを確保し、親となるヒープのポインタをセット */
	ptr = heapAlloc( root->lastUse->uid, size + sizeof( struct dmem_heap * ) );
	*ptr = root->lastUse;
	
	/* 割り当てカウントを増加 */
	root->lastUse->count++;
	
	/* ヒープの空き容量を更新 */
	root->lastUse->maxFreeSize = heapMaxFreeSize( root->lastUse->uid );
	
	/* 親となるヒープのポインタが書かれている部分の後ろをメモリ領域として返す */
	return (void *)( (uintptr_t)ptr + sizeof( struct dmem_heap * ) );
}

static size_t dmem_need_heapsize( size_t minblock, size_t requestsize )
{
	size_t first_data_size = requestsize + DMEM_HEADER_SIZE; //HEAP_HEADER_SIZE + sizeof( struct dmem_heap );
	
	if( first_data_size > minblock ){
		return first_data_size;
	} else{
		return minblock;
	}
}

static struct dmem_heap *dmem_heap_new( size_t heapsize, int type )
{
	struct dmem_heap *newheap;
	HeapUID huid = heapExCreate( "DmemHeap", MEMORY_USER, 0, heapsize, type, NULL );
	if( ! huid ) return NULL;
	
	newheap              = heapAlloc( huid, sizeof( struct dmem_heap ) );
	newheap->uid         = huid;
	newheap->maxFreeSize = heapMaxFreeSize( huid );
	newheap->count       = 0;
	newheap->next        = NULL;
	
	return newheap;
}
