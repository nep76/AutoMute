/* fiomgrSeek.c */

#include "fiomgr_types.h"

SceOff fiomgrSeek( FiomgrHandle fh, SceOff offset, int whence )
{
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	SceOff pos;
	
	if( whence == FH_SEEK_CUR && offset == 0 ){
		return fiomgrTell( fh );
	}
	
	params->eof = false;
	
	if( params->cache.lastState != FIOMGR_CACHE_LAST_READ ){
		if( params->cache.length ) __fiomgr_cache_flush( params );
		params->cache.lastState = FIOMGR_CACHE_LAST_READ;
	}
	
	__fiomgr_cache_clear( params );
	
	switch( whence ){
		case FH_SEEK_SET:
			if( params->largeFile ){
				pos = sceIoLseek( params->fd, offset, FH_SEEK_SET );
			} else{
				pos = sceIoLseek32( params->fd, offset, FH_SEEK_SET );
			}
			break;
		case FH_SEEK_CUR:
			if( params->largeFile ){
				pos = sceIoLseek( params->fd, offset, FH_SEEK_CUR );
			} else{
				pos = sceIoLseek32( params->fd, offset, FH_SEEK_CUR );
			}
			break;
		case FH_SEEK_END:
			if( params->largeFile ){
				pos = sceIoLseek( params->fd, offset, FH_SEEK_END );
			} else{
				pos = sceIoLseek32( params->fd, offset, FH_SEEK_END );
			}
			break;
		default:
			pos = 0;
	}
	
	return pos;
}
