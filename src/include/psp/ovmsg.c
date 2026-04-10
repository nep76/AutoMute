/*=========================================================
	
	ovmsg.c
	
	オーバーレイメッセージ。
	エラー処理が手抜き。
	
=========================================================*/
#include "ovmsg.h"

#include <stdio.h>
#include "psp/memory.h"
#include "psp/gb.h"
#include "cgerrs.h"

/*=========================================================
	型宣言
=========================================================*/
struct ovmsg_params {
	SceUID semaId, selfThreadId;
	unsigned int x, y, w, h;
	unsigned int fgcolor;
	unsigned int bgcolor;
	char *str;
	
	bool running;
	
	time_t displayStart;
	time_t displaySec;
};

enum ovmsg_status {
	OVMSG_NONE = 0,
	OVMSG_INIT,
	OVMSG_READY,
	OVMSG_WORK,
	OVMSG_SHUTDOWN,
	OVMSG_INTERRUPT,
};

/*=========================================================
	ローカル関数
=========================================================*/
static enum ovmsg_status ovmsg_get_status( struct ovmsg_params *params );
static bool ovmsg_set_status( struct ovmsg_params *params, enum ovmsg_status old_stat, enum ovmsg_status new_stat );

/*=========================================================
	ローカル変数
=========================================================*/
static struct ovmsg_params *st_params;
static enum ovmsg_status   st_stat;

/*=========================================================
	関数
=========================================================*/
int ovmsgInit( void )
{
	st_stat = OVMSG_INIT;
	
	st_params = (struct ovmsg_params *)memoryAllocEx( "OverlayMessageParams", MEMORY_USER, 0, sizeof( struct ovmsg_params ) + OVMSG_LENGTH, PSP_SMEM_High, NULL );
	if( ! st_params ) return  CG_ERROR_NOT_ENOUGH_MEMORY;
	
	st_params->semaId = sceKernelCreateSema( "CgOverlayMessage", 0, 1, 1, 0 );
	st_params->selfThreadId = 0;
	st_params->x = 10;
	st_params->y = 250;
	st_params->w = 0;
	st_params->h = 0;
	st_params->fgcolor = 0xffeeeeee;
	st_params->bgcolor = 0x88000000;
	st_params->str = (char *)( (uintptr_t)st_params + sizeof( struct ovmsg_params ) );
	
	st_params->running = true;
	
	st_params->displayStart = 0;
	st_params->displaySec   = 0;
	
	return 0;
}

int ovmsgThreadMain( SceSize arglen, void *argp )
{
	st_params->selfThreadId = sceKernelGetThreadId();
	
	ovmsg_set_status( st_params, OVMSG_INIT, OVMSG_WORK );
	
	for( ;; ){
		ovmsg_set_status( st_params, OVMSG_WORK, OVMSG_READY );
		sceKernelSleepThread();
		if( ! st_params->running ){
			ovmsg_set_status( st_params, OVMSG_READY, OVMSG_SHUTDOWN );
			break;
		} else{
			ovmsg_set_status( st_params, OVMSG_READY, OVMSG_WORK );
		}
		
		sceKernelLibcTime( &(st_params->displayStart) );
		
		st_params->w = strlen( st_params->str ) + 1;
		st_params->h = 2;
		
		st_params->displaySec = ( st_params->w - 1 ) / 7;
		if( st_params->displaySec < 3 ) st_params->displaySec = 3;
		
		st_params->w = gbOffsetChar( st_params->w );
		st_params->h = gbOffsetLine( st_params->h );
		
		while( ovmsg_get_status( st_params ) != OVMSG_INTERRUPT && ( sceKernelLibcTime( NULL ) - st_params->displayStart < st_params->displaySec ) ){
			if( gbBind() == 0 ){
				gbUse8BitFont( GB_FONT_PSPSYM );
				gbSetOpt( GB_AUTOFLUSH | GB_ALPHABLEND );
				gbPrepare();
				gbFillRectRel( st_params->x, st_params->y, st_params->w, st_params->h, st_params->bgcolor );
				gbPrint( st_params->x + ( gbOffsetChar( 1 ) >> 1 ), st_params->y + ( gbOffsetLine( 1 ) >> 1 ), st_params->fgcolor, GB_TRANSPARENT, st_params->str );
			}
			sceDisplayWaitVblankStart();
			sceKernelDelayThread( 1000 );
		}
		ovmsg_set_status( st_params, OVMSG_INTERRUPT, OVMSG_WORK );
	}
	
	ovmsg_set_status( st_params, OVMSG_SHUTDOWN, OVMSG_NONE );
	
	sceKernelDeleteSema( st_params->semaId );
	memoryFree( st_params );
	st_params = NULL;
	
	return sceKernelExitDeleteThread( 0 );
}

void ovmsgPrintIntrStart( void )
{
	ovmsg_set_status( st_params, OVMSG_WORK, OVMSG_INTERRUPT );
}

bool ovmsgPrintf( const char *format, ... )
{
	va_list ap;
	
	if( ovmsg_get_status( st_params ) != OVMSG_READY ) return false;
	
	va_start( ap, format );
	vsnprintf( st_params->str, OVMSG_LENGTH, format, ap );
	va_end( ap );
	sceKernelWakeupThread( st_params->selfThreadId );
	return true;
}

bool ovmsgVprintf( const char *format, va_list ap )
{
	if( ovmsg_get_status( st_params ) != OVMSG_READY ) return false;
	
	vsnprintf( st_params->str, OVMSG_LENGTH, format, ap );
	sceKernelWakeupThread( st_params->selfThreadId );
	return true;
}

bool ovmsgIsRunning( void )
{
	if( st_params ){
		if( ovmsg_get_status( st_params ) != OVMSG_NONE ){
			return true;
		}
	}
	return false;
}

void ovmsgWaitForReady( void )
{
	enum ovmsg_status stat;
	
	stat = ovmsg_get_status( st_params );
	
	if( stat == OVMSG_NONE ) return;
	
	while( stat != OVMSG_READY && stat != OVMSG_SHUTDOWN ){
		sceKernelDelayThread( 5000 );
		stat = ovmsg_get_status( st_params );
	}
	
	if( stat == OVMSG_READY ){
		SceKernelThreadInfo thinfo;
		
		thinfo.size = sizeof( SceKernelThreadInfo );
		sceKernelReferThreadStatus( st_params->selfThreadId, &thinfo );
		
		while( ! ( thinfo.status & PSP_THREAD_WAITING ) ){
			sceKernelDelayThread( 5000 );
			
			thinfo.size = sizeof( SceKernelThreadInfo );
			if( sceKernelReferThreadStatus( st_params->selfThreadId, &thinfo ) != 0 ){
				memoryFree( st_params );
				sceKernelTerminateDeleteThread( st_params->selfThreadId );
				break;
			}
		}
	}
}

void ovmsgShutdownStart( void )
{
	if( ovmsg_get_status( st_params ) != OVMSG_NONE ) return;
	
	ovmsgWaitForReady();
	
	st_params->running = false;
	sceKernelWakeupThread( st_params->selfThreadId );
}

static enum ovmsg_status ovmsg_get_status( struct ovmsg_params *params )
{
	enum ovmsg_status ret;
	
	if( ! params ) return OVMSG_NONE;
	
	sceKernelWaitSema( params->semaId, 1, 0 );
	ret = st_stat;
	sceKernelSignalSema( params->semaId, 1 );
	
	return ret;
}

static bool ovmsg_set_status( struct ovmsg_params *params, enum ovmsg_status old_stat, enum ovmsg_status new_stat )
{
	bool ret;
	
	if( ! params ) return false;
	
	sceKernelWaitSema( params->semaId, 1, 0 );
	if( st_stat == old_stat ){
		st_stat = new_stat;
		ret = true;
	} else{
		ret = false;
	}
	sceKernelSignalSema( params->semaId, 1 );
	
	return ret;
}
