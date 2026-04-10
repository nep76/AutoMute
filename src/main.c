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
static SceUID st_wdpw_thid;
static SceUID st_pwcb_thid;
static int    st_power_info = 0;

static bool   st_show_status_message = true;

/*=============================================*/

void amNoticef( char *format, ... )
{
	char msg[NOTICE_MESSAGE_BUFFER];
	va_list ap;
	
	if( ! st_show_status_message ) return;
	
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

int amWatchdogPower( SceSize arglen, void *argp )
{
	st_pwcb_thid = sceKernelCreateCallback( "AutoMute PowerCallback", amPowerCb, NULL );
	if( st_pwcb_thid && scePowerRegisterCallback( AM_POWER_CALLBACK_SLOT, st_pwcb_thid ) == 0 ){
		sceKernelSleepThreadCB();
	}
	
	return sceKernelExitDeleteThread( 0 );
}

int amPowerCb( int unk, int pwinfo, void *argp )
{
	st_power_info = pwinfo;
	return 0;
}

int amMain( SceSize arglen, void *argp )
{
	SceCtrlData pad;
	CtrlpadParams ctrl;
	
	bool activate = false;
	struct am_params params;
	
	memset( &params, 0, sizeof( struct am_params ) );
	
	params.main.autoDetect         = AM_INIDEF_MAIN_AUTODETECT;
	params.main.toggleButtons      = AM_INIDEF_MAIN_TOGGLEBUTTONS;
	params.main.showMuteWarning    = AM_INIDEF_MAIN_SHOWMUTEWARNING;
	
	params.resume.autoDetect       = AM_INIDEF_RESUME_AUTODETECT;
	params.resume.allowAutoDisable = AM_INIDEF_RESUME_ALLOWAUTODISABLE;
	
	{
		IniUID ini;
		ini = inimgrNew();
		if( ini > 0 ){
			char buttons[128];
			if( inimgrLoad( ini, "ms0:/seplugins/automute.ini" ) != 0 ){
				inimgrSetBool( ini, "Main", "AutoDetect", AM_INIDEF_MAIN_AUTODETECT );
				inimgrSetString( ini, "Main", "ToggleButtons", AM_INIDEF_MAIN_TOGGLEBUTTONSNAME );
				inimgrSetBool( ini, "Main", "ShowStatusMessage", AM_INIDEF_MAIN_SHOWSTATUSMESSAGE );
				inimgrSetBool( ini, "Main", "ShowMuteWarning", AM_INIDEF_MAIN_SHOWMUTEWARNING );
				
				inimgrSetBool( ini, "Resume", "AutoDetect", AM_INIDEF_RESUME_AUTODETECT );
				inimgrSetBool( ini, "Resume", "AllowAutoDisable", AM_INIDEF_RESUME_ALLOWAUTODISABLE );
				
				inimgrSave( ini, "ms0:/seplugins/automute.ini" );
			}
			
			params.main.autoDetect = inimgrGetBool( ini, "Main", "AutoDetect", AM_INIDEF_MAIN_AUTODETECT );
			
			inimgrGetString( ini, "Main", "ToggleButtons", AM_INIDEF_MAIN_TOGGLEBUTTONSNAME, buttons, sizeof( buttons ) );
			params.main.toggleButtons = ctrlpadUtilStringToButtons( buttons );
			
			st_show_status_message      = inimgrGetBool( ini, "Main", "ShowStatusMessage", AM_INIDEF_MAIN_SHOWSTATUSMESSAGE );
			params.main.showMuteWarning = inimgrGetBool( ini, "Main", "ShowMuteWarning",   AM_INIDEF_MAIN_SHOWMUTEWARNING );
			
			params.resume.autoDetect       = inimgrGetBool( ini, "Resume", "AutoDetect",       AM_INIDEF_RESUME_AUTODETECT );
			params.resume.allowAutoDisable = inimgrGetBool( ini, "Resume", "AllowAutoDisable", AM_INIDEF_RESUME_ALLOWAUTODISABLE );
			
			inimgrDestroy( ini );
		}
	}
	if( params.resume.autoDetect ){
		if( st_wdpw_thid > 0 ) sceKernelStartThread( st_wdpw_thid, 0, NULL );
	} else{
		if( st_wdpw_thid > 0 ) sceKernelDeleteThread( st_wdpw_thid );
	}
	
	ctrlpadInit( &ctrl );
	ctrlpadSetRepeatButtons( &ctrl,
		PSP_CTRL_CIRCLE   | PSP_CTRL_CROSS    | PSP_CTRL_SQUARE | PSP_CTRL_TRIANGLE |
		PSP_CTRL_UP       | PSP_CTRL_RIGHT    | PSP_CTRL_DOWN   | PSP_CTRL_LEFT     |
		PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_SELECT | PSP_CTRL_START    |
		PSP_CTRL_NOTE     | PSP_CTRL_SCREEN   | PSP_CTRL_VOLUP  | PSP_CTRL_VOLDOWN  |
		CTRLPAD_CTRL_ANALOG_UP   | CTRLPAD_CTRL_ANALOG_RIGHT |
		CTRLPAD_CTRL_ANALOG_DOWN | CTRLPAD_CTRL_ANALOG_LEFT
	);
	ctrlpadPref( &ctrl, 0, 0, 0 ); /* リピートしない */
	
	/* 表示スレッドを待つ */
	while( noticeGetStat() != NS_READY ) sceKernelDelayThread( 10000 );
	
	if( params.main.autoDetect ){
		if( sceHprmIsHeadphoneExist() ){
			activate = true;
			amNoticef( AM_ENABLE_BOOT, AM_VERSION );
		} else{
			amNoticef( AM_DISABLE_BOOT, AM_VERSION );
		}
	}
	
	while( st_running ){
		sceKernelDelayThread( 10000 );
		
		pad.Buttons = ctrlpadGetData( &ctrl, &pad, 40 );
		
		if( params.resume.autoDetect && ( st_power_info & PSP_POWER_CB_RESUME_COMPLETE ) ){
			/*
				リジューム完了直後はまだグラフィックが初期化されていない場合があるようで、
				即描画すると何も表示されない場合がある。
				
				初期化のためにVsyncを二回待つ。
			*/
			sceDisplayWaitVblankStart();
			sceDisplayWaitVblankStart();
			
			if( sceHprmIsHeadphoneExist() ){
				if( ! activate ){
					activate = true;
					
					amNoticef( AM_ENABLE_RESUME, AM_VERSION );
				}
			} else if( params.resume.allowAutoDisable ){
				if( activate ){
					activate = false;
					amNoticef( AM_DISABLE_RESUME, AM_VERSION );
				}
			}
			
			/* 電源状態をリセット */
			st_power_info = 0;
		} else if( ( pad.Buttons & (~( ~0 ^ params.main.toggleButtons )) ) == params.main.toggleButtons ){
			if( activate ){
				if( st_mute ) amMuteDisable();
				activate = false;
				amNoticef( AM_MAIN_DISABLED, AM_VERSION );
			} else{
				activate = true;
				amNoticef( AM_MAIN_ENABLED, AM_VERSION );
			}
		} 
		
		if( ! activate ) continue;
		
		if( ! sceHprmIsHeadphoneExist() ){
			/*
				リジューム時に音が復活するので、
				Enabled状態でヘッドホンが抜けている場合は、
				毎回ミュート化を試みる。
			*/
			amMuteEnable();
			if( params.main.showMuteWarning ) amNoticef( AM_MUTE, AM_VERSION );
		} else if( sceHprmIsHeadphoneExist() && st_mute ){
			noticePrintAbort();
			amMuteDisable();
		}
	}
	
	scePowerUnregisterCallback( AM_POWER_CALLBACK_SLOT );
	sceKernelDeleteThread( st_pwcb_thid );
	
	noticeThreadExit();
	sceKernelDeleteThread( st_disp_thid );
	
	return sceKernelExitDeleteThread( 0 );
}

int module_start( SceSize arglen, void *argp )
{
	st_main_thid = sceKernelCreateThread( "AutoMute Main",   amMain,       32, 0xC00, 0, 0 );
	st_disp_thid = sceKernelCreateThread( "AutoMute Notice", noticeThread, 16, 0x400, 0, 0 );
	if( st_main_thid > 0 && st_disp_thid > 0 ){
		sceKernelStartThread( st_main_thid, arglen, argp );
		sceKernelStartThread( st_disp_thid, arglen, argp );
	}
	
	st_wdpw_thid = sceKernelCreateThread( "AutoMute Watchdog Power", amWatchdogPower, 24, 0x300, 0, 0 );
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	/* 呼び出されていない */
	
	st_running = false;
	
	return 0;
}
