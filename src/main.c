/*
	main.c
*/

#include "main.h"

PSP_MODULE_INFO( "AutoMute", PSP_MODULE_KERNEL, 0, 0 );

/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/
void am_conf_load_togglebuttons( const char *name, const char *value, void *arg );

/*-----------------------------------------------
	ローカル変数
-----------------------------------------------*/
static bool   st_running = true;
static bool   st_mute    = false;
static SceUID st_main_thid;
static SceUID st_disp_thid;

/*=============================================*/

void amNoticef( char *format, ... )
{
	char msg[NOTICE_MESSAGE_BUFFER];
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( msg, NOTICE_MESSAGE_BUFFER, format, ap );
	va_end( ap );
	
	noticePrint( msg );
}

void amMuteEnable( void )
{
	unsigned int k1 = pspSdkGetK1();
	pspSdkSetK1( 0 );
	sceSysregAudioIoDisable();
	pspSdkSetK1( k1 );
	
	st_mute = true;
}

void amMuteDisable( void )
{
	unsigned int k1 = pspSdkGetK1();
	pspSdkSetK1( 0 );
	sceSysregAudioIoEnable();
	pspSdkSetK1( k1 );
	
	st_mute = false;
}

int amMain( SceSize arglen, void *argp )
{
	SceCtrlData pad;
	bool wait_toggle_button_release = false;
	bool activate = false;
	unsigned int toggle_buttons = AM_DEFAULT_TOGGLE_BUTTONS; 
	
	if( sceHprmIsHeadphoneExist() ){
		activate = true;
		amNoticef( AM_ENABLED_BOOT, AM_VERSION );
	} else{
		amNoticef( AM_DISABLED_BOOT, AM_VERSION );
	}
	
	{
		ConfmgrHandlerParams conf[] = {
			{ "TOGGLE_BUTTONS", CH_CALLBACK_LOAD,  cmpspbtnLoad,  &toggle_buttons },
			{ "TOGGLE_BUTTONS", CH_CALLBACK_STORE, cmpspbtnStore, &toggle_buttons }
		};
		
		cmpspbtnSetMask( PSP_CTRL_HOME | PSP_CTRL_HOLD | PSP_CTRL_WLAN_UP | PSP_CTRL_REMOTE | PSP_CTRL_DISC | PSP_CTRL_MS );
		if( ! confmgrLoad( conf, sizeof( conf ) / sizeof( conf[0] ), AM_CONF_FILE_FULLPATH ) ){
			confmgrStore( conf, sizeof( conf ) / sizeof( conf[0] ), AM_CONF_FILE_FULLPATH );
		}
	}
	
	while( st_running ){
		sceKernelDelayThread( 10000 );
		sceCtrlPeekBufferPositive( &pad, 1 );
		
		/* 指定されたボタンのフラグのみ取り出す */
		pad.Buttons &= (~( ~0 ^ toggle_buttons ));
		
		if( pad.Buttons == toggle_buttons ){
			if( ! wait_toggle_button_release ){
				if( activate ){
					if( st_mute ){
						amMuteDisable();
					}
					activate = false;
					amNoticef( AM_DISABLED, AM_VERSION );
				} else{
					activate = true;
					amNoticef( AM_ENABLED, AM_VERSION );
				}
				wait_toggle_button_release = true;
			}
		} else if( wait_toggle_button_release ){
			wait_toggle_button_release = false;
		}
		
		if( ! activate ) continue;
		
		if( ! sceHprmIsHeadphoneExist() ){
			/*
				スリープ復帰時に音が復活するので、
				Enabled状態でヘッドホンが抜けている場合は、
				毎回ミュート化を試みる。
			*/
			amMuteEnable();
			amNoticef( AM_MUTE, AM_VERSION );
		} else if( sceHprmIsHeadphoneExist() && st_mute ){
			noticePrintAbort();
			amMuteDisable();
		}
	}
	
	noticeThreadExit();
	sceKernelDeleteThread( st_disp_thid );
	
	return sceKernelExitDeleteThread( 0 );
}

int module_start( SceSize arglen, void *argp )
{
	st_main_thid = sceKernelCreateThread( "AutoMute Main", amMain, 32, 0xC00, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
	st_disp_thid = sceKernelCreateThread( "AutoMute Notice", noticeThread, 16, 0x400, PSP_THREAD_ATTR_NO_FILLSTACK, 0 );
	
	if( st_main_thid > 0 && st_disp_thid > 0 ){
		sceKernelStartThread( st_main_thid, arglen, argp );
		sceKernelStartThread( st_disp_thid, arglen, argp );
	}
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	/* 呼び出されていない */
	
	st_running = false;
	
	return 0;
}
