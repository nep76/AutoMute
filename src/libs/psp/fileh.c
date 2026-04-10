/*
	fileh.c
*/

#include "fileh.h"

static void fileh_seek( enum fileh_seek_mode mode, struct fileh_params *params, void *offset, FilehWhence whence, void *result )
{
	int seek_whence = 0;
	
	switch( whence ){
		case FW_SEEK_SET:
			seek_whence = PSP_SEEK_SET;
			break;
		case FW_SEEK_CUR:
			seek_whence = PSP_SEEK_CUR;
			break;
		case FW_SEEK_END:
			seek_whence = PSP_SEEK_END;
			break;
	}
	
	if( mode == FSM_32BITS ){
		*((int *)result) = sceIoLseek32( params->fd, *((int *)offset), seek_whence );
	} else if( mode == FSM_64BITS ){
		*((long long *)result) = sceIoLseek( params->fd, *((SceOff *)offset), seek_whence );
	}
	
	if( result < 0 ){
		params->lError = FILEH_ERROR_SEEK_FAILED;
		params->sError = *((int *)result);
	}
}

FilehUID filehOpen( const char *path, int mode, int perm )
{
	struct fileh_params *params;
	
	params = (struct fileh_params *)memsceMalloc( sizeof( struct fileh_params ) );
	if( ! params ) return 0;
	
	params->path   = (char *)memsceMalloc( strlen( path ) + 1 );
	if( ! params->path ){
		memsceFree( params );
		return 0;
	}
	strcpy( params->path, path );
	params->fd     = 0;
	params->lError = 0;
	params->sError = 0;
	
	if( ! ( mode & PSP_O_CREAT ) && ! filehUpdateStat( (FilehUID)params ) ) return (FilehUID)params;
	
	params->fd = sceIoOpen( path, mode, perm );
	if( params->fd < 0 ){
		params->lError = FILEH_ERROR_OPEN_FAILED;
		params->sError = params->fd;
	}
	
	return (FilehUID)params;
}

void filehClose( FilehUID uid )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	sceIoClose( params->fd );
	filehDestroy( uid );
}

void filehDestroy( FilehUID uid )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	memsceFree( params->path );
	memsceFree( params );
}

int filehRead( FilehUID uid, void *data, size_t size )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	int readsize = sceIoRead( params->fd, data, size );
	
	if( readsize < 0 ){
		params->lError = FILEH_ERROR_READ_FAILED;
		params->sError = readsize;
	}
	
	return readsize;
}

int filehReadln( FilehUID uid, void *data, size_t size )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	int readsize = 0;
	int offset   = 0;
	
	if( size <= 0 ) return 0;
	
	size--;
	
	while( size-- ){
		readsize = sceIoRead( params->fd, &(((char *)data)[offset]), 1 );
		if( readsize <= 0 ){
			((char *)data)[offset] = '\0';
			break;
		} else if( ((char *)data)[offset] == '\n' ){
			if( ((char *)data)[offset - 1] == '\r' ){
				((char *)data)[offset - 1] = '\0';
			} else{
				((char *)data)[offset] = '\0';
			}
			break;
		}
		offset++;
	}
	
	if( readsize < 0 ){
		params->lError = FILEH_ERROR_READ_FAILED;
		params->sError = readsize;
	}
	
	return offset;
}

void filehWrite( FilehUID uid, void *data, size_t size )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	int ret = sceIoWrite( params->fd, data, size );
	
	if( ret < 0 ){
		params->lError = FILEH_ERROR_WRITE_FAILED;
		params->sError = ret;
	}
}

void filehWritef( FilehUID uid, size_t bufsize, char *format, ... )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	char *buf = (char *)memsceMalloc( bufsize );
	va_list ap;
	
	if( ! buf ){
		params->lError = FILEH_ERROR_NOT_ENOUGH_MEMORY;
		params->sError = 0;
		return;
	}
	
	va_start( ap, format );
	vsnprintf( buf, bufsize, format, ap );
	va_end( ap );
	
	filehWrite( uid, buf, strlen( buf ) );
}

int filehSeek32( FilehUID uid, int offset, FilehWhence whence )
{
	int pos;
	
	fileh_seek( FSM_32BITS, (struct fileh_params *)uid, &offset, whence, &pos );
	
	return pos;
}

long long filehSeek64( FilehUID uid, long long offset, FilehWhence whence )
{
	long long pos;
	
	fileh_seek( FSM_32BITS, (struct fileh_params *)uid, &offset, whence, &pos );
	
	return pos;
}

int filehTell32( FilehUID uid )
{
	return filehSeek32( uid, 0, FW_SEEK_CUR );
}

long long filehTell64( FilehUID uid )
{
	return filehSeek64( uid, 0, FW_SEEK_CUR );
}

bool filehEof( FilehUID uid )
{
	SceIoStat *stat;
	filehUpdateStat( uid );
	
	stat = filehGetStat( uid );
	
	if( filehTell64( uid ) >= stat->st_size ){
		return true;
	} else{
		return false;
	}
}

bool filehUpdateStat( FilehUID uid )
{
	struct fileh_params *params = (struct fileh_params *)uid;
	int ret;
	memset( &(params->stat), 0, sizeof( SceIoStat ) );
	
	ret = sceIoGetstat( params->path, &(params->stat) );
	if( ret < 0 ){
		params->lError = FILEH_ERROR_GETSTAT_FAILED;
		params->sError = ret;
		return false;
	} else if( ! FIO_S_ISREG( params->stat.st_mode ) ){
		params->lError = FILEH_ERROR_PATH_IS_NOT_REGFILE;
		params->sError = 0;
		return false;
	}
	
	return true;
}

SceIoStat *filehGetStat( FilehUID uid )
{
	return &(((struct fileh_params *)uid)->stat);
}

int filehGetLastError( FilehUID uid )
{
	return ((struct fileh_params *)uid)->lError;
}

int filehGetLastSystemError( FilehUID uid )
{
	return ((struct fileh_params *)uid)->sError;
}

