/*=========================================================
	
	dmem.c
	
	動的メモリ管理。
	ヒープを数珠上に繋いで、物理メモリが許す限りのメモリを動的に確保する。
	自動化している都合上オーバーヘッドが大きいので、
	サイズが分かっている場合は、heap.hやmemory.hで直接確保する方がよい。
	
	伸び縮みするリンクリストなどで有効。
	
=========================================================*/
#include "dmem.h"

/*=========================================================
	マクロ
=========================================================*/
#define DMEM_DEFAULT_MINBLOCK 1024
#define DMEM_HEADER_SIZE ( HEAP_HEADER_SIZE + sizeof( struct dmem_heap ) )

/*=========================================================
	型宣言
=========================================================*/
struct dmem_heap {
	HeapUID uid;
	size_t maxFreeSize;
	size_t count;
	struct dmem_heap *next;
};

struct dmem_root {
	size_t minHeapSize;
	int    allocType;
	struct dmem_heap *heapList;
	struct dmem_heap *lastUse;
};

/*=========================================================
	ローカル関数
=========================================================*/
static size_t dmem_need_heapsize( size_t minblock, size_t requestsize );
static struct dmem_heap *dmem_heap_new( size_t heapsize, int type );

/*=========================================================
	関数
=========================================================*/
DmemUID dmemNew( size_t minblock, int type )
{
	struct dmem_root *root;
	
	if( type != PSP_SMEM_High ) type = PSP_SMEM_Low;
	
	root = memoryAllocEx( "DmemParams", MEMORY_USER, 0, sizeof( struct dmem_root ), type, NULL );
	if( ! root ) return 0;
	
	root->minHeapSize = minblock > 0 ? minblock : DMEM_DEFAULT_MINBLOCK;
	root->allocType   = type;
	root->heapList    = NULL;
	root->lastUse     = NULL;
	
	return (DmemUID)root;
}

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

void *dmemCalloc( DmemUID uid, size_t nelem, size_t size )
{
	void *ptr;
	
	/* オーバーフローチェック */
	if( ( nelem * size ) / size != nelem ) return NULL;
	
	ptr = dmemAlloc( uid, nelem * size );
	if( ptr ) memset( ptr, 0, nelem * size );
	return ptr;
}

int dmemFree( DmemUID uid, void *ptr )
{
	struct dmem_root *root = (struct dmem_root *)uid;
	struct dmem_heap **heap;
	
	if( ! ptr || ! root->lastUse ) return CG_ERROR_OK;
	
	heap = (struct dmem_heap **)( (uintptr_t)ptr - sizeof( struct dmem_heap * ) );
	
	if( (*heap)->count == 1 ){
		/* ヒープブロックに自分しかいなければヒープごと解放 */
		struct dmem_heap *avail_heap;
		
		if( *heap == root->heapList ){
			/* 削除対象ヒープが先頭の場合はルートのヒープエリアリストの開始位置を修正 */
			avail_heap = (*heap)->next;
			root->heapList = avail_heap;
		} else{
			/* 削除対象ヒープの前のヒープを取得 */
			for( avail_heap = root->heapList; avail_heap; avail_heap = avail_heap->next ){
				if( avail_heap->next == *heap ) break;
			}
			
			if( ! avail_heap ){
				/* 削除対象ヒープのアドレスは、渡されたDmemUIDの管轄外 */
				return CG_ERROR_INVALID_ARGUMENT;
			}
			
			/* 再リンク */
			avail_heap->next = (*heap)->next;
		}
		
		/*
			最後に使用したヒープが削除対象ヒープを指していたら、どこでもいいので生きているヒープを指す。
			空き容量を調べないのは、次回のdmemAlloc()時に計算するため。
		*/
		if( root->lastUse == *heap ) root->lastUse = avail_heap;
		
		heapDestroy( (*heap)->uid );
	} else{
		int ret;
		/* ヒープに割り当て済みカウントを減算し、メモリをヒープに返却 */
		(*heap)->count--;
		ret = heapFree( (*heap)->uid, (void *)heap );
		
		if( ret != CG_ERROR_OK ) return ret;
	}
	
	return CG_ERROR_OK;
}

void dmemDestroy( DmemUID uid )
{
	struct dmem_root *root = (struct dmem_root *)uid;
	
	if( root->heapList ){
		struct dmem_heap *prev = root->heapList;
		struct dmem_heap *cur  = prev->next;
		
		while( cur ){
			heapDestroy( prev->uid );
			prev = cur;
			cur  = cur->next;
		}
		heapDestroy( prev->uid );
	}
	
	memoryFree( root );
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
	HeapUID huid = heapCreateEx( "DmemHeap", MEMORY_USER, 0, heapsize, type, NULL );
	if( ! huid ) return NULL;
	
	newheap              = heapAlloc( huid, sizeof( struct dmem_heap ) );
	newheap->uid         = huid;
	newheap->maxFreeSize = heapMaxFreeSize( huid );
	newheap->count       = 0;
	newheap->next        = NULL;
	
	return newheap;
}
