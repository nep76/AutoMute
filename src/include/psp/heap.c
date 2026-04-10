/*=========================================================

	heap.c

	ヒープとして使用するメモリ領域を作成する。
	大きさの分かっているものを複数確保する場合に有利。

=========================================================*/
#include "heap.h"

/*=========================================================
	マクロ
=========================================================*/
#define HEAP_ALIGN_SIZE 8

/*=========================================================
	関数
=========================================================*/
HeapUID heapCreateEx( const char *name, MemoryPartition partition, unsigned int align, SceSize size, int type, void *addr )
{
	struct heap_allocated_memblock_header *heap_root = (struct heap_allocated_memblock_header *)memoryAllocEx( name, partition, align, size + sizeof( struct heap_allocated_memblock_header ), type, addr );
	if( heap_root ){
		heap_root->next = NULL;
		heap_root->size = memoryUsableSize( (void *)heap_root ) - sizeof( struct heap_allocated_memblock_header );
	}
	
	return (HeapUID)heap_root;
}

HeapUID heapCreate( SceSize size )
{
	return (HeapUID)heapCreateEx( "cg_user_heap", MEMORY_USER, 0, size, PSP_SMEM_Low, NULL );
}

void heapDestroy( HeapUID uid )
{
	memoryFree( (void *)uid );
}

void *heapAlloc( HeapUID uid, SceSize size )
{
	struct heap_allocated_memblock_header *cur  = (struct heap_allocated_memblock_header *)uid;
	struct heap_allocated_memblock_header *prev = NULL;
	struct heap_allocated_memblock_header *heap = NULL;
	SceSize alloc_size;
	
	/* 確保するサイズが0の場合は、1が指定されたものと見なす */
	if( ! size ) size = 1;
	
	alloc_size = size + sizeof( struct heap_allocated_memblock_header ) + HEAP_ALIGN_SIZE;
	
	/* 先頭要素だけはヒープ全体の情報として特別扱い */
	if( ! cur->next ){
		if( cur->size < alloc_size ) return NULL;
		cur->next = (struct heap_allocated_memblock_header *)MEMORY_ALIGN_ADDR( HEAP_ALIGN_SIZE, ( (uintptr_t)cur + sizeof( struct heap_allocated_memblock_header ) ) );
		cur->next->size = alloc_size;
		cur->next->next = NULL;
		return (void *)( (uintptr_t)cur->next + sizeof( struct heap_allocated_memblock_header ) );
	} else{
		/* メモリが一つでも割り当てられていれば、rootと最初の要素の間に空き容量がないか検査 */
		struct heap_allocated_memblock_header *topheap = (struct heap_allocated_memblock_header *)( (uintptr_t)cur + sizeof( struct heap_allocated_memblock_header ) );
		
		if( (uintptr_t)(cur->next) - (uintptr_t)topheap >= alloc_size ){
			topheap->size = alloc_size;
			topheap->next = cur->next;
			cur->next = (struct heap_allocated_memblock_header *)MEMORY_ALIGN_ADDR( HEAP_ALIGN_SIZE, topheap );
			return (void *)( (uintptr_t)topheap + sizeof( struct heap_allocated_memblock_header ) );
		}
	}
	
	prev = cur->next;
	cur  = prev->next;
	
	/* 十分な空き容量を探して取得 */
	for( ; cur; prev = cur, cur = cur->next ){
		/*
			直前のメモリブロックの終端(prev + prev->size)と現在のメモリブロック(cur)の差から、
			未使用メモリサイズを算出し、必要な容量が空いていれば使用、そうでなければ次へ。
		*/
		if( (uintptr_t)cur - ( (uintptr_t)prev + prev->size ) >= alloc_size ){
			heap = (struct heap_allocated_memblock_header *)MEMORY_ALIGN_ADDR( HEAP_ALIGN_SIZE, ( (uintptr_t)prev + prev->size ) );
			heap->size = alloc_size;
			heap->next = prev->next;
			prev->next = heap;
			break;
		}
	}
	
	/*
		この時点で割り当てられていない場合、prevがもっとも後ろのメモリブロックを指しているので、
		ヒープメモリの終端とprevの終端の差から末尾の空きメモリをチェック。
	*/
	
	if( ! heap &&  ( ( (uintptr_t)uid + ((struct heap_allocated_memblock_header *)uid)->size ) - ( (uintptr_t)prev + prev->size ) >= alloc_size ) ){
		heap = (struct heap_allocated_memblock_header *)MEMORY_ALIGN_ADDR( HEAP_ALIGN_SIZE, ( (uintptr_t)prev + prev->size ) );
		heap->size = alloc_size;
		heap->next = prev->next;
		prev->next = heap;
	}
	
	return heap ? (void *)( (uintptr_t)heap + sizeof( struct heap_allocated_memblock_header ) ) : NULL;
}

void *heapCalloc( HeapUID uid, SceSize size )
{
	void *heap = heapAlloc( uid, size );
	
	if( heap ) memset( heap, 0, size );
	
	return heap;
}

int heapFree( HeapUID uid, void *ptr )
{
	struct heap_allocated_memblock_header *root;
	struct heap_allocated_memblock_header *cur;
	struct heap_allocated_memblock_header *prev;
	struct heap_allocated_memblock_header *heap;
	
	if( ! ptr ) return CG_ERROR_OK;
	
	root = (struct heap_allocated_memblock_header *)uid;
	cur  = NULL;
	prev = NULL;
	heap = (struct heap_allocated_memblock_header *)( (uintptr_t)ptr - sizeof( struct heap_allocated_memblock_header ) );
	
	/* 先頭要素だけはヒープ全体の情報として特別扱い */
	if( ! root->next ){
		/* 先頭の次がない場合は、メモリは何も割り当てられていない */
		return CG_ERROR_INVALID_ARGUMENT;
	} else{
		cur = root->next;
	}
	
	for( ; cur != heap; prev = cur, cur = cur->next ){
		/* curがNULLになるまで回った場合、そのアドレスは使用されていない */
		if( ! cur ) return CG_ERROR_INVALID_ARGUMENT;
	}
	
	/*
		対象メモリブロックの前のメモリブロックが、対象メモリブロックの次を指すことで、
		対象メモリブロックは未使用として扱われる。
	*/
	if( prev ){
		prev->next = cur->next;
	} else{
		/* prevが無い場合は、ヒープの先頭なので、rootのリンク先を変更 */
		root->next = cur->next;
	}
	
	return CG_ERROR_OK;
}

SceSize heapUsableSize( HeapUID uid )
{
	return ((struct heap_allocated_memblock_header *)uid)->size;
}

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
		
		/* rootと最初の要素の間の空き容量を計算 */
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

SceSize heapTotalFreeSize( HeapUID uid )
{
	SceSize totalsize = 0;
	struct heap_allocated_memblock_header *cur = (struct heap_allocated_memblock_header *)uid;
	
	if( ! cur->next ){
		return cur->size;
	} else{
		totalsize = cur->size;
		cur = cur->next;
	}
	
	for( ; cur; cur = cur->next ){
		totalsize -= cur->size;
	}
	
	return totalsize;
}
