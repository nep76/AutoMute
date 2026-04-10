/*
	memsce.c
*/
#include "memsce.h"

void *memsceMallocEx( SceSize align, SceUID partitionid, const char *name, int type, SceSize size, void *addr )
{
	struct heap_header hhead;
	void *heap_start;
	
	hhead.block_id = sceKernelAllocPartitionMemory( partitionid, name, type, sizeof( struct heap_header ) + size + align, addr );
	if( hhead.block_id < 0 ) return NULL;
	
	/* MemSceRealloc()のために確保されたサイズを記録 */
	hhead.size = size;
	
	/* 先頭アドレスを取得 */
	heap_start = sceKernelGetBlockHeadAddr( hhead.block_id );
	heap_start = (void *)MEMSCE_ALIGNOFFSET( (unsigned int)heap_start + sizeof( struct heap_header ), align );
	
	/* 管理情報分余分に確保しているので、それを書き込む */
	memcpy( (void *)((unsigned int)heap_start - sizeof( struct heap_header )), &hhead, sizeof( struct heap_header ) );
	
	return heap_start;
}

int memsceFree( void *heap_start )
{
	struct heap_header *hhead;
	if( ! heap_start ) return -1;
	
	/* 指定アドレスから管理情報分だけ戻って管理情報取得 */
	hhead = (struct heap_header *)((unsigned int)heap_start - sizeof( struct heap_header ));
	
	return sceKernelFreePartitionMemory( hhead->block_id );
}

void *memsceMalloc( SceSize size )
{
	return memsceMallocEx( 16, MEMSCE_PART_USER, "memsceMalloc", PSP_SMEM_Low, size, 0 );
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
	new_heap_start = memsceMalloc( resize );
	if( src_hhead->size > resize ){
		memcpy( new_heap_start, src_heap_start, resize );
	} else{
		memcpy( new_heap_start, src_heap_start, src_hhead->size );
	}
	
	memsceFree( src_heap_start );
	
	return new_heap_start;
}

void *memsceMemalign( SceSize align, SceSize size )
{
	/* バウンダリ境界が2のべき乗かどうか調べる */
	if( align <= 0 || ! MEMSCE_POWER_OF_TWO( align ) ) return 0;
	
	return memsceMallocEx( align, MEMSCE_PART_USER, "memsceMemalign", PSP_SMEM_Low, size, 0 );
}
