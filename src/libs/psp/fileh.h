/*
	File Handler
*/

#ifndef FILEH_H
#define FILEH_H

#include <pspkernel.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include "psp/memsce.h"

#define FILEH_ERROR_SUCCESS 0
#define FILEH_ERROR_GETSTAT_FAILED      0xe0000001
#define FILEH_ERROR_PATH_IS_NOT_REGFILE 0xe0000002
#define FILEH_ERROR_OPEN_FAILED         0xe0000003
#define FILEH_ERROR_READ_FAILED         0xe0000004
#define FILEH_ERROR_WRITE_FAILED        0xe0000005
#define FILEH_ERROR_NOT_ENOUGH_MEMORY   0xe0000006
#define FILEH_ERROR_SEEK_FAILED         0xe0000007

#ifdef __cplusplus
extern "C" {
#endif

typedef int FilehUID;

typedef enum {
	FW_SEEK_SET = PSP_SEEK_SET,
	FW_SEEK_CUR = PSP_SEEK_CUR,
	FW_SEEK_END = PSP_SEEK_END
} FilehWhence;

typedef struct {
	char *path;
	SceUID fd;
	SceIoStat stat;
	int lError;
	int sError;
} FilehParams;

enum fileh_seek_mode {
	FSM_32BITS,
	FSM_64BITS
};

FilehUID filehOpen( const char *path, int mode, int perm );
void filehClose( FilehUID uid );
void filehDestroy( FilehUID uid );
int filehRead( FilehUID uid, void *data, size_t size );
int filehReadln( FilehUID uid, void *data, size_t size );
int filehWrite( FilehUID uid, void *data, size_t size );
int filehWritef( FilehUID uid, char *buf, size_t bufsize, char *format, ... );
int filehSeek32( FilehUID uid, int offset, FilehWhence whence );
SceOff filehSeek( FilehUID uid, SceOff offset, FilehWhence whence );
int filehTell32( FilehUID uid );
SceOff filehTell( FilehUID uid );
bool filehEof( FilehUID uid );
bool filehUpdateStat( FilehUID uid );
SceIoStat *filehGetStat( FilehUID uid );
int filehGetLastError( FilehUID uid );
int filehGetLastSystemError( FilehUID uid );

#ifdef __cplusplus
}
#endif

#endif
