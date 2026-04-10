/* fiomgrOpen.c */

#include "fiomgr_types.h"

FiomgrHandle fiomgrOpen( const char *path, int flags, int perm )
{
	struct fiomgr_params *params;
	int ret;
	
	if( ( ret = __fiomgr_open( &params, path, flags, perm ) ) < 0 ) return ret;
	
	return (FiomgrHandle)params;
}

int __fiomgr_open( struct fiomgr_params **params, const char *path, int flags, int perm )
{
	*params = (struct fiomgr_params *)memoryExalloc( "FiomgrParams", MEMORY_USER, 0, sizeof( struct fiomgr_params ) + sizeof( struct fiomgr_cache_params ), flags & FH_O_ALLOC_HIGH ? PSP_SMEM_High : PSP_SMEM_Low, NULL );
	if( ! *params ) return CG_ERROR_NOT_ENOUGH_MEMORY;
	
	(*params)->fd = sceIoOpen( path, flags & 0x0000FFFF, perm );
	
	if( (*params)->fd < 0 ){
		int error = (*params)->fd;
		memoryFree( *params );
		return error;
	}
	
	(*params)->largeFile = flags & FH_O_LARGEFILE ? true : false;
	(*params)->eof       = false;
	(*params)->cache.memory[0] = '\0';
	(*params)->cache.length    = 0;
	(*params)->cache.position  = 0;
	(*params)->cache.lastState = FIOMGR_CACHE_LAST_READ;
	
	return 0;
}

inline void __fiomgr_cache_flush( struct fiomgr_params *params )
{
	sceIoWrite( params->fd, (const void *)params->cache.memory, params->cache.length );
}

inline void __fiomgr_cache_clear( struct fiomgr_params *params )
{
	params->cache.memory[0] = '\0';
	params->cache.length    = 0;
	params->cache.position  = 0;
}

inline void __fiomgr_cache_load( struct fiomgr_params *params )
{
	params->cache.length = sceIoRead( params->fd, params->cache.memory, FIOMGR_CACHE_SIZE - 1 );
	if( params->cache.length < FIOMGR_CACHE_SIZE - 1 ) params->eof = true;
	
	(params->cache.memory)[params->cache.length] = '\0';
}
