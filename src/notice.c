/*
	notice.c
*/

#include "notice.h"

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static SceUID     st_self_thid;
static bool       st_running;
static SceUID     st_semaid;
static NoticeStat st_stat;
static int        st_errcode;
static int        st_auto_dispsec;
static int        st_dispsec;
static time_t     st_start_time;
static char       st_string[NOTICE_MESSAGE_BUFFER];

/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/
static void notice_init( void );
static void notice_error( int error );
static void notice_set_message( char *str );

/*=============================================*/

NoticeStat noticeGetStat( void )
{
	return st_stat;
}

void noticeSetDisplaySec( int sec )
{
	if( sec <= 0 ){
		st_auto_dispsec = true;
	} else{
		st_dispsec      = sec;
		st_auto_dispsec = false;
	}
}

void noticePrint( char *str )
{
	if( ! str || str[0] == '\0' || st_stat == NS_INIT || st_stat == NS_ERROR ) return;
	
	notice_set_message( str );
	
	if( st_stat == NS_READY ){
		 SceKernelThreadInfo thinfo;
		 
		 do{
		 	 thinfo.size = sizeof( SceKernelThreadInfo );
		 	 if( sceKernelReferThreadStatus( st_self_thid, &thinfo ) != 0 ) return;
		 } while( thinfo.status != PSP_THREAD_WAITING );
		 
		 sceKernelWakeupThread( st_self_thid );
	}
}

void noticePrintAbort( void )
{
	if( st_stat == NS_WORK ){
		st_stat = NS_READY;
	}
}

int noticeThread( SceSize arglen, void *argp )
{
	notice_init();
	
	st_self_thid = sceKernelGetThreadId();
	st_semaid    = sceKernelCreateSema( "AutoMute Semaphore", 0, 1, 1, 0 );
	
	if( st_semaid < 0 ) notice_error( AM_ERROR_FAILED_TO_CREATE_SEMA );
	
	while( st_running ){
		if( st_stat == NS_READY ){
			sceKernelSleepThread();
			
			st_stat = NS_WORK;
			
			if( st_auto_dispsec ){
				sceKernelWaitSema( st_semaid, 1, 0 );
				st_dispsec = strlen( st_string ) / 7;
				sceKernelSignalSema( st_semaid, 1 );
				
				if( st_dispsec < 3 ){
					st_dispsec = 3;
				}
			}
			sceKernelLibcTime( &st_start_time );
		}
		
		if( sceKernelLibcTime( NULL ) - st_start_time < st_dispsec ){
			
			sceKernelWaitSema( st_semaid, 1, 0 );
			blitString( 0, 255, 0xffffffff, 0xff000000, st_string );
			sceKernelSignalSema( st_semaid, 1 );
			
			sceKernelDcacheWritebackAll();
			sceDisplayWaitVblankStart();
			
			/*
				垂直同期開始を待ってからさらに13ミリ秒ほど置く。
				これによって描画タイミングを少しずらし、
				他のアプリケーションが描画した画面に上書きする。
				
				力業なので本当はもっといい方法があるかもしれません。
			*/
			sceKernelDelayThread( 13000 );
		} else{
			st_stat = NS_READY;
		}
	}
	
	sceKernelDeleteSema( st_semaid );
	
	return sceKernelExitThread( 0 );
}

void noticeThreadExit( void )
{
	st_running = false;
}

static void notice_init( void )
{
	st_self_thid    = 0;
	st_running      = true;
	st_semaid       = 0;
	st_stat         = NS_INIT;
	st_errcode      = AM_ERROR_SUCCESS;
	st_auto_dispsec = true;
	st_dispsec      = 0;
	st_start_time   = 0;
	st_string[0]    = '\0';
}

static void notice_error( int error )
{
	st_stat    = NS_ERROR;
	st_errcode = error;
	sceKernelSleepThread();
}

static void notice_set_message( char *str )
{
	sceKernelWaitSema( st_semaid, 1, 0 );
	strutilSafeCopy( st_string, str, NOTICE_MESSAGE_BUFFER );
	sceKernelLibcTime( &st_start_time );
	sceKernelSignalSema( st_semaid, 1 );
}
