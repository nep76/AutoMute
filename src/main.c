/*
	main.c
*/

#include "main.h"

PSP_MODULE_INFO( "AutoMute", PSP_MODULE_KERNEL, 0, 0 );

/*-----------------------------------------------
	ローカル関数プロトタイプ
-----------------------------------------------*/
static void                 am_proc_config( struct am_params *params );
static int                  am_power_callback( int unk, int pwinfo, void *argp );
static enum am_startup_mode am_get_startup_mode( char *modestr );

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

static struct am_params st_params;

/*=============================================*/

bool amIsMuted( void )
{
	return st_mute;
}

void amMuteEnable( void )
{
	unsigned int k1 = pspSdkSetK1( 0 );
	sceSysregAudioIoDisable();
	pspSdkSetK1( k1 );
		
	st_mute = true;
}

void amMuteDisable( void )
{
	unsigned int k1 = pspSdkSetK1( 0 );
	sceSysregAudioIoEnable();
	pspSdkSetK1( k1 );
	
	st_mute = false;
}

void amNoticef( char *format, ... )
{
	char msg[NOTICE_MESSAGE_BUFFER];
	va_list ap;
	
	if( ! st_params.main.showStatusMessage || ! format || ! *format ) return;
	
	va_start( ap, format );
	vsnprintf( msg, NOTICE_MESSAGE_BUFFER, format, ap );
	va_end( ap );
	
	noticePrint( msg );
}

int amWatchdogPower( SceSize arglen, void *argp )
{
	st_pwcb_thid = sceKernelCreateCallback( "AutoMute PowerCallback", am_power_callback, NULL );
	if( st_pwcb_thid && scePowerRegisterCallback( AM_POWER_CALLBACK_SLOT, st_pwcb_thid ) == 0 ){
		sceKernelSleepThreadCB();
	}
	
	return sceKernelExitDeleteThread( 0 );
}

int amMain( SceSize arglen, void *argp )
{
	SceCtrlData pad;
	CtrlpadParams ctrl;
	
	bool activate  = false;
	unsigned int buttons;
	
	am_proc_config( &st_params );
	
	if( st_params.resume.autoDetect ){
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
	
	switch( st_params.main.startup ){
		case AM_STARTUP_OFF:
			amNoticef( AM_MSG_STARTUP_OFF, AM_VERSION );
			break;
		case AM_STARTUP_ON:
			activate = true;
			amNoticef( AM_MSG_STARTUP_ON, AM_VERSION );
			break;
		case AM_STARTUP_AUTO:
			if( sceHprmIsHeadphoneExist() ){
				activate = true;
				amNoticef( AM_MSG_STARTUP_AUTO_DETECT, AM_VERSION );
			} else{
				amNoticef( AM_MSG_STARTUP_AUTO_NOTDETECT, AM_VERSION );
			}
			break;
	}
	
	while( st_running ){
		sceKernelDelayThread( 10000 );
		
		buttons = ctrlpadGetData( &ctrl, &pad, 40 );
		
		if( ( st_power_info & PSP_POWER_CB_RESUME_COMPLETE ) && st_params.resume.autoDetect ){
			noticePrintAbort();
			
			/*
				リジューム完了直後はまだグラフィックが初期化されていない場合があるようで、
				即描画すると何も表示されない場合がある。
				
				初期化のためにVsyncを二回待つ。
			*/
			sceDisplayWaitVblankStart();
			sceDisplayWaitVblankStart();
			
			if( pad.Buttons & PSP_CTRL_REMOTE ){
				if( ! activate ){
					activate = true;
					amNoticef( AM_MSG_RESUME_DETECT, AM_VERSION );
				}
			} else if( st_params.resume.allowAutoDisable ){
				if( activate ){
					activate = false;
					amNoticef( AM_MSG_RESUME_NOTDETECT, AM_VERSION );
				}
			}
			
			/* 電源状態をリセット */
			st_power_info = 0;
		} else if( ( buttons & (~( ~0 ^ st_params.main.toggleButtons )) ) == st_params.main.toggleButtons ){
			if( activate ){
				activate = false;
				amNoticef( AM_MSG_DISABLED, AM_VERSION );
			} else{
				activate = true;
				amNoticef( AM_MSG_ENABLED, AM_VERSION );
			}
		} 
		
		if( ! activate ){
			if( amIsMuted() ){
				amMuteDisable();
			}
		} else{
			if( ! ( pad.Buttons & PSP_CTRL_REMOTE ) ){
				/*
					リジューム時に音が復活するので、
					Enabled状態でヘッドホンが抜けている場合は、
					毎回ミュート化を試みる。
				*/
				amMuteEnable();
				if( st_params.main.showMuteWarning ) amNoticef( AM_MSG_MUTE, AM_VERSION );
			} else if( ( pad.Buttons & PSP_CTRL_REMOTE ) && st_mute ){
				sceKernelDelayThread( 200000 );
				noticePrintAbort();
				amMuteDisable();
			}
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

static int am_power_callback( int unk, int pwinfo, void *argp )
{
	st_power_info = pwinfo;
	return 0;
}

static void am_proc_config( struct am_params *params )
{
	IniUID ini;
	
	params->main.startup            = am_get_startup_mode( AM_INIDEF_MAIN_STARTUP );
	params->main.toggleButtons      = AM_INIDEF_MAIN_TOGGLEBUTTONS;
	params->main.showStatusMessage  = AM_INIDEF_MAIN_SHOWSTATUSMESSAGE;
	params->main.showMuteWarning    = AM_INIDEF_MAIN_SHOWMUTEWARNING;
	
	params->resume.autoDetect       = AM_INIDEF_RESUME_AUTODETECT;
	params->resume.allowAutoDisable = AM_INIDEF_RESUME_ALLOWAUTODISABLE;
	
	ini = inimgrNew();
	if( ini > 0 ){
		char buf[128];
		if( inimgrLoad( ini, AM_INI_FILE_FULLPATH ) != 0 ){
			inimgrSetString( ini, "Main", "Startup", AM_INIDEF_MAIN_STARTUP );
			inimgrSetString( ini, "Main", "ToggleButtons", ctrlpadUtilButtonsToString( AM_INIDEF_MAIN_TOGGLEBUTTONS, buf, sizeof( buf ) ) );
			inimgrSetBool( ini, "Main", "ShowStatusMessage", AM_INIDEF_MAIN_SHOWSTATUSMESSAGE );
			inimgrSetBool( ini, "Main", "ShowMuteWarning", AM_INIDEF_MAIN_SHOWMUTEWARNING );
			
			inimgrSetBool( ini, "Resume", "AutoDetect", AM_INIDEF_RESUME_AUTODETECT );
			inimgrSetBool( ini, "Resume", "AllowAutoDisable", AM_INIDEF_RESUME_ALLOWAUTODISABLE );
			
			inimgrSave( ini, AM_INI_FILE_FULLPATH );
		}
		
		inimgrGetString( ini, "Main", "Startup", AM_INIDEF_MAIN_STARTUP, buf, sizeof( buf ) );
		params->main.startup = am_get_startup_mode( buf );
		
		inimgrGetString( ini, "Main", "ToggleButtons", ctrlpadUtilButtonsToString( AM_INIDEF_MAIN_TOGGLEBUTTONS, buf, sizeof( buf ) ), buf, sizeof( buf ) );
		params->main.toggleButtons = ctrlpadUtilStringToButtons( buf );
		
		params->main.showStatusMessage = inimgrGetBool( ini, "Main", "ShowStatusMessage", AM_INIDEF_MAIN_SHOWSTATUSMESSAGE );
		params->main.showMuteWarning   = inimgrGetBool( ini, "Main", "ShowMuteWarning",   AM_INIDEF_MAIN_SHOWMUTEWARNING );
		
		params->resume.autoDetect       = inimgrGetBool( ini, "Resume", "AutoDetect", AM_INIDEF_RESUME_AUTODETECT );
		params->resume.allowAutoDisable = inimgrGetBool( ini, "Resume", "AllowAutoDisable", AM_INIDEF_RESUME_ALLOWAUTODISABLE );
		
		inimgrDestroy( ini );
	}
}

static enum am_startup_mode am_get_startup_mode( char *modestr )
{
	strutilToUpper( modestr );
	
	if( strcmp( modestr, "ON" ) == 0 ){
		return AM_STARTUP_ON;
	} else if( strcmp( modestr, "AUTO" ) == 0 ){
		return AM_STARTUP_AUTO;
	} else{
		return AM_STARTUP_OFF;
	}
}
