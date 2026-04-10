/*
	memsce.c
*/
#include "memsce.h"

void *memsceMallocEx( SceSize boundary, SceUID partitionid, const char *name, int type, SceSize size, void *addr )
{
	struct heap_header hhead;
	void *heap_start;
	
	hhead.block_id = sceKernelAllocPartitionMemory( partitionid, name, type, sizeof( struct heap_header ) + size + boundary, addr );
	if( hhead.block_id < 0 ) return NULL;
	
	/* MemSceRealloc()のために確保されたサイズを記録 */
	hhead.size = size;
	
	/* メモリは利用中であるとマーク */
	hhead.is_free = 0;
	
	/* 先頭アドレスを取得 */
	heap_start = sceKernelGetBlockHeadAddr( hhead.block_id );
	heap_start = (void *)MEMSCE_ALIGNOFFSET( (unsigned int)heap_start + sizeof( struct heap_header ), boundary );
	
	/* 管理情報分余分に確保しているので、それを書き込む */
	memcpy( (void *)((unsigned int)heap_start - sizeof( struct heap_header )), &hhead, sizeof( struct heap_header ) );
	
	return heap_start;
}

int memsceFree( void *heap_start )
{
	struct heap_header *hhead;
	if( ! heap_start ) return 0;
	
	/* 指定アドレスから管理情報分だけ戻って管理情報取得 */
	hhead = (struct heap_header *)((unsigned int)heap_start - sizeof( struct heap_header ));
	if( hhead->is_free == 1 ) return -1;
	
	/* メモリが利用可能であるとマークして解放 */
	hhead->is_free = 1;
	
	return sceKernelFreePartitionMemory( hhead->block_id );
}

void *memsceMalloc( SceSize size )
{
	return memsceMallocEx( 16, PSP_MEMPART_USER_1, "memsceMalloc", PSP_SMEM_Low, size, 0 );
}

void *memsceCalloc( SceSize blk_num, SceSize size )
{
	void *mem = memsceMalloc( blk_num * size );
	if( ! mem ) return NULL;
	
	memset( mem, 0, blk_num * size );
	return mem;
}

void *memsceRealloc( void *src_heap_start, SceSize resize )
{
	struct heap_header *src_hhead;
	void *new_heap_start;
	
	src_hhead = ( struct heap_header *)((unsigned int)src_heap_start - sizeof( struct heap_header ));
	if( src_hhead->is_free ) return NULL;
	
	new_heap_start = memsceMalloc( resize );
	if( src_hhead->size > resize ){
		memcpy( new_heap_start, src_heap_start, resize );
	} else{
		memcpy( new_heap_start, src_heap_start, src_hhead->size );
	}
	
	memsceFree( src_heap_start );
	
	return new_heap_start;
}

void *memsceMemalign( SceSize boundary, SceSize size )
{
	/* バウンダリ境界が2のべき乗かどうか調べる */
	if( boundary <= 0 || ! MEMSCE_POWER_OF_TWO( boundary ) ) return 0;
	
	return memsceMallocEx( boundary, PSP_MEMPART_USER_1, "MemSceMemalign", PSP_SMEM_Low, size, 0 );
}
