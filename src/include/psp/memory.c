/*=========================================================

	memory.c

	malloc()/calloc()/realloc()/memalign()/free()もどき。

=========================================================*/
#include "memory.h"

#ifdef MEMORY_DEBUG
#include <stdio.h>
#endif

/*=========================================================
	ローカルマクロ
=========================================================*/
#define MEMORY_POWER_OF_TWO( x ) ( ! ( ( x ) & ( ( x ) - 1 ) ) )

/*=========================================================
	ローカル型宣言
=========================================================*/
struct memory_header {
	SceUID  blockId;
	SceSize size;
};

/*=========================================================
	ローカル関数
=========================================================*/
static inline SceSize memory_real_size( SceSize size );

/*=========================================================
	ローカル変数
=========================================================*/
#ifdef MEMORY_DEBUG
static int st_alloc_count;
#endif

/*=========================================================
	関数
=========================================================*/
void *memoryAlloc( SceSize size )
{
	return memoryAllocEx( "cg_malloc", MEMORY_USER, 0, size, PSP_SMEM_Low, NULL );
}

void *memoryCalloc( SceSize nelem, SceSize size )
{
	void *memblock;
	
	/* どちらかが0の場合、1バイト確保 */
	if( nelem == 0 || size == 0 ){
		nelem = 1;
		size  = 1;
	}
	
	/*
		サイズチェック
		
		nelem * sizeがSceSize型の最大値を超えた(オーバーフローした)場合、
		この式は偽となりNULLを返す。
	*/
	if( ( nelem * size ) / size != nelem ) return NULL;
	
	memblock = memoryAlloc( nelem * size );
	if( memblock ) memset( memblock, 0, nelem * size );
	
	return memblock;
}

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
	new_real_size = memory_real_size( new_size );
	if( src_real_size == new_real_size ) return src_memblock;
	
	new_memblock = memoryAlloc( new_real_size );
	if( ! new_memblock ) return NULL;
	
	memcpy( new_memblock, src_memblock, ( src_real_size > new_real_size ? new_real_size : src_real_size ) );
	memoryFree( src_memblock );
	
	return new_memblock;
}

void *memoryReallocf( void *src_memblock, SceSize new_size )
{
	void *new_memblock = memoryRealloc( src_memblock, new_size );
	
	if( src_memblock && new_size && ! new_memblock ) memoryFree( src_memblock );
	return new_memblock;
}

void *memoryAlign( unsigned int align, SceSize size )
{
	return memoryAllocEx( "cg_memalign", MEMORY_USER, align, size, PSP_SMEM_Low, NULL );
}

int memoryFree( void *memblock )
{
	struct memory_header *header;
#ifdef MEMORY_DEBUG
	int debug_ret;
#endif
	
	if( ! memblock ) return 0;
	header = (struct memory_header *)( (uintptr_t)memblock - sizeof( struct memory_header ) );

#ifdef MEMORY_DEBUG
	debug_ret = sceKernelFreePartitionMemory( header->blockId );
	if( debug_ret < 0 ){
		printf( "MemoryFree Error (%x): BlockID: %x, HeapAddr %p\n", debug_ret, header->blockId, memblock );
	} else{
		st_alloc_count--;
		printf( "MemoryFree: BlockID: %x, HeapAddr %p\n", header->blockId, memblock );
	}
	return debug_ret;
#else
	return sceKernelFreePartitionMemory( header->blockId );
#endif
}

SceSize memoryUsableSize( const void *memblock )
{
	struct memory_header *header;
	
	if( ! memblock ) return 0;
	header = (struct memory_header *)( (uintptr_t)memblock - sizeof( struct memory_header ) );
	
	return header->size;
}

void *memoryAllocEx( const char *name, MemoryPartition partition, unsigned int align, SceSize size, int type, void *addr )
{
	struct memory_header header;
	void *memblock;
	SceSize real_size;
	
	/* 0バイト確保しようとした場合は、最小値で確保 */
	if( ! size ) size = 1;
	
	real_size = memory_real_size( size ); /* MEMORY_PAGE_SIZE区切りの実際に確保された値に補正 */
	
	header.blockId = sceKernelAllocPartitionMemory( partition, name, type, sizeof( struct memory_header ) + real_size + align, addr );
	if( header.blockId < 0 ) return NULL;
	
	header.size = real_size; 
	
	memblock = (void *)( (uintptr_t)(sceKernelGetBlockHeadAddr( header.blockId )) + sizeof( struct memory_header ) );
	if( align ){
		if( ! MEMORY_POWER_OF_TWO( align ) ){
			sceKernelFreePartitionMemory( header.blockId );
			return NULL;
		}
		memblock = (void *)MEMORY_ALIGN_ADDR( align, memblock );
	}
	
	memcpy( (void *)( (uintptr_t)memblock - sizeof( struct memory_header ) ), (void *)&header, sizeof( struct memory_header ) );
	
#ifdef MEMORY_DEBUG
	printf( "MemoryAlloc %s: BlockID: %x, HeadAddr: %p, Size: %u\n", name, header.blockId, memblock, header.size );
	st_alloc_count++;
#endif
	
	return memblock;
}

static inline SceSize memory_real_size( SceSize size )
{
	return MEMORY_PAGE_SIZE * (unsigned int)ceil( (double)size / (double)MEMORY_PAGE_SIZE );
}

#ifdef MEMORY_DEBUG
unsigned int memoryGetAllocCount( void )
{
	return st_alloc_count;
}
#endif
