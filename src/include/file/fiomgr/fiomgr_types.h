/* fiomgr_types.h                 */
/* 内部で使用。外から参照しない。 */

#include "../fiomgr.h"

#include <stdbool.h>
#include "memory/memory.h"
#include "util/strutil.h"
#include "cgerrs.h"

#define FIOMGR_CACHE_SIZE 1024

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

int __fiomgr_open( struct fiomgr_params **params, const char *path, int flags, int perm );
inline void __fiomgr_cache_flush( struct fiomgr_params *params );
inline void __fiomgr_cache_clear( struct fiomgr_params *params );
inline void __fiomgr_cache_load( struct fiomgr_params *params );
