/* fiomgrClose.c */

#include "fiomgr_types.h"

int fiomgrClose( FiomgrHandle fh )
{
	int ret;
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	
	if( ( params->cache.lastState != FIOMGR_CACHE_LAST_READ ) && params->cache.length ){
		__fiomgr_cache_flush( params );
	}
	
	ret = sceIoClose( params->fd );
	
	if( ! ( ret < 0 ) ) memoryFree( params );
	
	return ret;
}
