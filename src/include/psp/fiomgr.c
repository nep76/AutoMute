/*=========================================================
	
	fiomgr.c
	
	ファイルI/Oマネージャ。
	
=========================================================*/
#include "fiomgr.h"

#include <stdbool.h>
#include "psp/memory.h"
#include "cgerrs.h"

/*=========================================================
	マクロ
=========================================================*/
#define FIOMGR_CACHE_SIZE 1024

/*=========================================================
	型宣言
=========================================================*/
enum fiomgr_cache_last_state {
	FIOMGR_CACHE_LAST_READ = 0,
	FIOMGR_CACHE_LAST_WRITE
};

struct fiomgr_cache_params {
	char    memory[FIOMGR_CACHE_SIZE];
	size_t length, position;
	enum fiomgr_cache_last_state lastState;
};

struct fiomgr_params {
	SceUID    fd;
	bool      largeFile;
	bool      eof;
	struct fiomgr_cache_params cache;
};

/*=========================================================
	ローカル関数
=========================================================*/
static int fiomgr_open( struct fiomgr_params **params, const char *path, int flags, int perm );
static inline void fiomgr_cache_flush( struct fiomgr_params *params );
static inline void fiomgr_cache_clear( struct fiomgr_params *params );
static inline void fiomgr_cache_load( struct fiomgr_params *params );

/*=========================================================
	関数
=========================================================*/
FiomgrHandle fiomgrOpen( const char *path, int flags, int perm )
{
	struct fiomgr_params *params;
	int ret;
	
	if( ( ret = fiomgr_open( &params, path, flags, perm ) ) < 0 ) return ret;
	
	return (FiomgrHandle)params;
}

int fiomgrClose( FiomgrHandle fh )
{
	int ret;
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	
	if( ( params->cache.lastState != FIOMGR_CACHE_LAST_READ ) && params->cache.length ){
		fiomgr_cache_flush( params );
	}
	
	ret = sceIoClose( params->fd );
	
	if( ! ( ret < 0 ) ) memoryFree( params );
	
	return ret;
}

int fiomgrRead( FiomgrHandle fh, void *data, size_t length )
{
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	
	if( ! length ) return 0;
	
	if( params->cache.lastState != FIOMGR_CACHE_LAST_READ ){
		if( params->cache.length ){
			fiomgr_cache_flush( params );
			fiomgr_cache_clear( params );
		}
		params->cache.lastState = FIOMGR_CACHE_LAST_READ;
	}
	
	if( length > params->cache.length - params->cache.position ){
		int number_of_bytes_read = params->cache.length - params->cache.position;
		int number_of_bytes_rest = length - number_of_bytes_read;
		
		if( number_of_bytes_read ) memcpy( data, params->cache.memory + params->cache.position, number_of_bytes_read );
		fiomgr_cache_clear( params );
		
		if( params->eof ){
			return number_of_bytes_read;
		} else if( number_of_bytes_rest > FIOMGR_CACHE_SIZE ){
			/* 読み込み量がキャッシュサイズを超える場合は、キャッシュせずに直接読み出す */
			int non_cached_number_of_bytes_read = sceIoRead( params->fd, data + number_of_bytes_read, number_of_bytes_rest );
			if( non_cached_number_of_bytes_read < number_of_bytes_rest ) params->eof = true;
			return number_of_bytes_read + non_cached_number_of_bytes_read;
		}
		
		fiomgr_cache_load( params );
		
		params->cache.position = length - number_of_bytes_read;
		
		if( params->cache.length < number_of_bytes_rest ){
			memcpy( data + number_of_bytes_read, params->cache.memory, params->cache.length );
			params->cache.position = params->cache.length;
		} else{
			memcpy( data + number_of_bytes_read, params->cache.memory, number_of_bytes_rest );
			params->cache.position = number_of_bytes_rest;
		}
		return number_of_bytes_read + params->cache.position;
	} else{
		memcpy( data, params->cache.memory + params->cache.position, length );
		params->cache.position += length;
		return length;
	}
}

int fiomgrReadln( FiomgrHandle fh, char *str, size_t maxlength )
{
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	char *start, *lnbrk, lncode, nextcode;
	size_t read_bytes, str_length, free_length, copy_length;
	
	if( params->cache.lastState != FIOMGR_CACHE_LAST_READ ){
		if( params->cache.length ){
			fiomgr_cache_flush( params );
			fiomgr_cache_clear( params );
		}
		params->cache.lastState = FIOMGR_CACHE_LAST_READ;
	}
	
	/* 終端文字分確保 */
	maxlength--;
	
	*str = '\0';
	str_length = 0;
	read_bytes = 0;
	do{
		start = params->cache.memory + params->cache.position;
		lnbrk = strpbrk( start, "\x0D\x0A" );
		
		free_length = maxlength - str_length;
		copy_length = lnbrk ? ( lnbrk - start ) : params->cache.length - params->cache.position;
		
		if( ! lnbrk && ! copy_length ){
			fiomgr_cache_clear( params );
			if( params->eof ) break;
			fiomgr_cache_load( params );
		} else if( copy_length > free_length ){
			memcpy( str + str_length, params->cache.memory + params->cache.position, free_length );
			str[maxlength] = '\0';
			params->cache.position += free_length;
			read_bytes += free_length;
			break;
		} else{
			memcpy( str + str_length, params->cache.memory + params->cache.position, copy_length );
			str_length             += copy_length;
			params->cache.position += copy_length;
			read_bytes             += copy_length;
			
			str[str_length] = '\0';
			
			if( lnbrk ){
				lncode   = *lnbrk;
				nextcode = 0;
				if( params->cache.length == params->cache.position ){
					fiomgr_cache_clear( params );
					if( ! params->eof ){
						fiomgr_cache_load( params );
						nextcode = *(params->cache.memory);
					}
				} else{
					nextcode = *(lnbrk + 1);
				}
				
				params->cache.position++;
				read_bytes++;
				if( lncode == '\x0D' ){
					if( nextcode == '\x0A' ){
						params->cache.position++;
						read_bytes++;
					}
				} else if( nextcode == '\x0D' ){
					params->cache.position++;
					read_bytes++;
				}
			} else{
				fiomgr_cache_clear( params );
				fiomgr_cache_load( params );
			}
		}
	} while( ! lnbrk );
	
	return read_bytes;
}

SceOff fiomgrTell( FiomgrHandle fh )
{
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	SceOff pos;
	
	if( params->largeFile ){
		pos = sceIoLseek( params->fd, 0, FH_SEEK_CUR );
	} else{
		pos = sceIoLseek32( params->fd, 0, FH_SEEK_CUR );
	}
	
	if( params->cache.length ){
		switch( params->cache.lastState ){
			case FIOMGR_CACHE_LAST_READ:
				pos -= params->cache.length - params->cache.position;
				break;
			case FIOMGR_CACHE_LAST_WRITE:
				pos += params->cache.length;
				break;
		}
	}
	
	return pos;
}

SceOff fiomgrSeek( FiomgrHandle fh, SceOff offset, int whence )
{
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	SceOff pos;
	
	if( whence == FH_SEEK_CUR && offset == 0 ){
		return fiomgrTell( fh );
	}
	
	params->eof = false;
	
	if( params->cache.lastState != FIOMGR_CACHE_LAST_READ ){
		if( params->cache.length ) fiomgr_cache_flush( params );
		params->cache.lastState = FIOMGR_CACHE_LAST_READ;
	}
	
	fiomgr_cache_clear( params );
	
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

int fiomgrWrite( FiomgrHandle fh, void *data, size_t length )
{
	struct fiomgr_params *params = (struct fiomgr_params *)fh;
	int number_of_bytes_write = 0;
	
	if( ! length ) return 0;
	
	if( params->cache.lastState != FIOMGR_CACHE_LAST_WRITE ){
		fiomgr_cache_clear( params );
		params->cache.lastState = FIOMGR_CACHE_LAST_WRITE;
	}
	
	if( ( FIOMGR_CACHE_SIZE - 1 ) - params->cache.length < length ){
		if( params->cache.length ){
			sceIoWrite( params->fd, params->cache.memory, params->cache.length );
			fiomgr_cache_clear( params );
		}
	}
	
	if( length > ( FIOMGR_CACHE_SIZE - 1 ) ){
		number_of_bytes_write = sceIoWrite( params->fd, data, length );
	} else{
		number_of_bytes_write = strutilCat( params->cache.memory, data, FIOMGR_CACHE_SIZE ) - 1;
		params->cache.length = number_of_bytes_write;
	}
	
	return number_of_bytes_write;
}

int fiomgrWriteln( FiomgrHandle fh, void *data, size_t length )
{
	int ret = fiomgrWrite( fh, data, length );
	if( ret >= 0 ) ret += fiomgrWrite( fh, "\x0D\x0A", 2 );
	return ret;
}

int fiomgrWritef( FiomgrHandle fh, char *buf, size_t buflen, const char *format, ... )
{
	va_list ap;
	
	if( ! buf ) return CG_ERROR_INVALID_ARGUMENT;
	
	va_start( ap, format );
	vsnprintf( buf, buflen, format, ap );
	va_end( ap );
	
	return fiomgrWrite( fh, buf, strlen( buf ) );
}

int fiomgrWritefln( FiomgrHandle fh, char *buf, size_t buflen, const char *format, ... )
{
	int ret;
	va_list ap;
	
	if( ! buf ) return CG_ERROR_INVALID_ARGUMENT;
	
	va_start( ap, format );
	vsnprintf( buf, buflen, format, ap );
	va_end( ap );
	
	ret = fiomgrWrite( fh, buf, strlen( buf ) );
	
	if( ret >= 0 ) ret += fiomgrWrite( fh, "\x0D\x0A", 2 );
	return ret;
}

static int fiomgr_open( struct fiomgr_params **params, const char *path, int flags, int perm )
{
	*params = (struct fiomgr_params *)memoryAllocEx( "FiomgrParams", MEMORY_USER, 0, sizeof( struct fiomgr_params ) + sizeof( struct fiomgr_cache_params ), flags & FH_O_ALLOC_HIGH ? PSP_SMEM_High : PSP_SMEM_Low, NULL );
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

static inline void fiomgr_cache_flush( struct fiomgr_params *params )
{
	sceIoWrite( params->fd, (const void *)params->cache.memory, params->cache.length );
}

static inline void fiomgr_cache_clear( struct fiomgr_params *params )
{
	params->cache.memory[0] = '\0';
	params->cache.length    = 0;
	params->cache.position  = 0;
}

static inline void fiomgr_cache_load( struct fiomgr_params *params )
{
	params->cache.length = sceIoRead( params->fd, params->cache.memory, FIOMGR_CACHE_SIZE - 1 );
	if( params->cache.length < FIOMGR_CACHE_SIZE - 1 ) params->eof = true;
	
	(params->cache.memory)[params->cache.length] = '\0';
}
