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
static int        st_bg_width;

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
	
	if( st_semaid < 0 ) notice_error( CG_ERROR_FAILED_TO_CREATE_SEMA );
	
	st_stat = NS_READY;
	
	blitSetOptions( BO_ALPHABLEND );
	
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
			/*
				他のスレッドを停止させず、フレームバッファを自分で制御しない場合、
				表示バッファと描画バッファがめまぐるしく切り替わっているため、
				表示を試みる度にblitInit()を呼んでディスプレイ情報を再取得する。
			*/
			sceKernelWaitSema( st_semaid, 1, 0 );
			if( blitInit() == 0 ){
				blitFillBox( (unsigned int)(blitOffsetChar( 1 ) >> 1), 255 - (unsigned int)( blitMeasureLine( 1 ) >> 1 ) , st_bg_width, blitMeasureLine( 2 ), 0x88000000 );
				blitString( blitOffsetChar( 1 ), 255, 0xffeeeeee, BLIT_TRANSPARENT, st_string );
				
				sceKernelDcacheWritebackAll();
				sceDisplayWaitVblankStart();
			}
			sceKernelSignalSema( st_semaid, 1 );
			
			/*
				表示バッファに直接描画するのだから、
				待ち時間がなくても常に最前面に表示されることに気がついた。
				
				それでもディレイを入れておかないと、メッセージ表示状態でゲーム終了を選んだときに、
				システムがスレッドを殺す隙がないので、Vsync間隔以下のディレイを入れる。
			*/
			sceKernelDelayThread( 1000 );
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
	st_errcode      = CG_ERROR_OK;
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
	st_bg_width = blitMeasureChar( strlen( st_string ) + 1 );
	sceKernelLibcTime( &st_start_time );
	sceKernelSignalSema( st_semaid, 1 );
}
