/*=========================================================
	
	ammain.c
	
=========================================================*/
#define AM_FIRST_INCLUDE
#include "automute.h"
#undef AM_FIRST_INCLUDE

#include <pspimpose_driver.h>
#include <pspdisplay.h>
#include <psppower.h>
#include "ammain.h"
#include "ovmsg.h"
#include "pad/padutil.h"
#include "file/inimgr.h"
#include "util/hook.h"
#include "sysclib.h"
#include "pspapi.h"

PSP_MODULE_INFO( "AutoMute", PSP_MODULE_KERNEL, 2, 0 );

/*=========================================================
	マクロ
=========================================================*/

/*=========================================================
	型宣言
=========================================================*/
enum am_startup {
	AM_STARTUP_OFF = 0,
	AM_STARTUP_ON,
	AM_STARTUP_AUTO
};

enum am_preference {
	AM_PREF_STATUS_NOTIFICATION       = 0x00000001,
	AM_PREF_MUTE_NOTIFICATION         = 0x00000002,
	AM_PREF_AUTO_ENABLE_WHILE_PLUGGED = 0x00000004,
	AM_PREF_RESUME_HANDLER            = 0x00000008,
	AM_PREF_AUTO_DISABLE_WHEN_RESUME  = 0x00000010,
};
	
struct am_params {
	enum am_startup startup;
	PadutilButtons  toggleButtons;
	unsigned int    preference;
};

enum am_internal_status {
	AM_ACTIVATE                       = 0x00000001,
	AM_WAIT_FOR_RELEASE_TOGGLE_BUTTON = 0x00000002,
	AM_HEADPHONES_EXIST               = 0x00000004,
	AM_MUTE_DISABLED_BEFORE_AUTOMUTE  = 0x00000008,
	AM_HEADPHONES_PLUGGED_IN          = 0x00000010,
	AM_HEADPHONES_PLUGGED_OUT         = 0x00000020,
};

typedef int (*am_impose_set_param)( SceImposeParam, int );
typedef int (*am_impose_get_param)( SceImposeParam );

/*=========================================================
	ローカル関数
=========================================================*/
static int  am_main( SceSize size, void *argp );
static void am_ini( struct am_params *params, const char *path );
static bool am_notificf( const char *format, ... );
static void am_notificf_abort( void );
static void am_enable( unsigned int *status );
static void am_disable( unsigned int *status );
static bool am_is_muted( void );
static void am_mute( unsigned int *status );
static void am_unmute( unsigned int *status );

static int am_power_callback( int unk, int pwinfo, void *argp );

/*=========================================================
	ローカル変数
=========================================================*/
static int st_power_status = 0;
static am_impose_get_param st_impose_get_param;
static am_impose_set_param st_impose_set_param;

/*=========================================================
	関数
=========================================================*/
/*-----------------------------------------------
	メインループ
-----------------------------------------------*/
static int am_main( SceSize size, void *argp )
{
	struct am_params params;
	SceUID power_cb_thid;
	
	SceCtrlData pad;
	unsigned int hk;
	PadutilButtons buttons;
	
	unsigned int status = 0;
	
	/* 設定ファイルのロード(なければ作成) */
	am_ini( &params, (const char *)argp );
	
	/* 通知が有効な場合は画面描画用スレッドを生成し待機させる */
	if( params.preference & AM_PREF_STATUS_NOTIFICATION ){
		SceUID thid;
		ovmsgInit();
		
		thid = sceKernelCreateThread( "AutoMuteNotification", ovmsgThreadMain, 16, 0x200, 0, 0 );
		if( thid > 0 ) sceKernelStartThread( thid, 0, NULL );
		ovmsgWaitForReady();
	}
	
	/* リジューム監視が有効な場合はコールバックスレッドをセット */
	if( params.preference & AM_PREF_RESUME_HANDLER ){
		power_cb_thid = sceKernelCreateCallback( "AutoMuteResumeCheck", am_power_callback, NULL );
		if( power_cb_thid ) scePowerRegisterCallback( AM_POWER_CALLBACK_SLOT, power_cb_thid );
	}
	
	/* PopsLoader経由で起動された場合は、PopsManagerの起動を待つ */
	if( sceKernelFindModuleByName( "popsloader_trademark" ) != NULL ){
		while( ! sceKernelFindModuleByName( "scePops_Manager" ) ) sceKernelDelayThread( 50000 );
	}
	
	/* scePops_ManagerがFW4.01のImposeドライバNIDを取り込んでいたら旧バージョン。もっと良い判定方法はないものか？ */
	if( hookFindImportAddr( "scePops_Manager", "sceImpose_driver", 0x6F502C0A ) ){
		st_impose_get_param = sceImposeGetParam401;
		st_impose_set_param = sceImposeSetParam401;
	} else{
		st_impose_get_param = sceImposeGetParam;
		st_impose_set_param = sceImposeSetParam;
	}
	
	/* 気休めリンク待ち */
	sceKernelDelayThread( 1000000 );
	
	switch( params.startup ){
		case AM_STARTUP_AUTO:
			if( sceHprmIsHeadphoneExist() ){
				status = ( AM_ACTIVATE | AM_HEADPHONES_EXIST );
				am_notificf( "AutoMute %s detected headphones.", AM_VERSION );
			}
			break;
		case AM_STARTUP_ON:
			status = ( AM_ACTIVATE | ( sceHprmIsHeadphoneExist() ? AM_HEADPHONES_EXIST : 0 ) );
			am_notificf( "AutoMute %s %s.", AM_VERSION, AM_ENABLED );
			break;
		case AM_STARTUP_OFF:
			break;
	}
	
	buttons = 0;
	while( gRunning ){
		sceKernelDelayThreadCB( 10000 );
		
		/* ボタンコード取得 */
		sceCtrlPeekBufferPositive( &pad, 1 );
		sceHprmPeekCurrentKey( &hk );
		buttons = padutilSetPad( pad.Buttons | padutilGetAnalogStickDirection( pad.Lx, pad.Ly, 70 ) ) | padutilSetHprm( hk );
		
		/* フラグをリセット */
		status          &= ~( AM_HEADPHONES_PLUGGED_IN | AM_HEADPHONES_PLUGGED_OUT );
		st_power_status &= ( PSP_POWER_CB_RESUMING | PSP_POWER_CB_RESUME_COMPLETE );
		if( buttons & PSP_CTRL_REMOTE ){
			if( ! ( status & AM_HEADPHONES_EXIST ) ) status |= AM_HEADPHONES_PLUGGED_IN;
			status |= AM_HEADPHONES_EXIST;
		} else{
			if( ( status & AM_HEADPHONES_EXIST ) ) status |= AM_HEADPHONES_PLUGGED_OUT;
			status &= ~AM_HEADPHONES_EXIST;
		}
			
		if( st_power_status ){
			if( st_power_status & PSP_POWER_CB_RESUME_COMPLETE ){
				st_power_status = 0;
				if( ! ( status & AM_ACTIVATE ) && ( status & AM_HEADPHONES_EXIST ) ){
					am_enable( &status );
				} else if( ( params.preference & AM_PREF_AUTO_DISABLE_WHEN_RESUME ) && ( status & AM_ACTIVATE ) && ! ( status & AM_HEADPHONES_EXIST ) ){
					am_disable( &status );
					status |= AM_HEADPHONES_PLUGGED_OUT; /* プラグが抜かれたことにする */
				}
			} else{
				continue;
			}
		} else if( ( buttons & params.toggleButtons ) == params.toggleButtons ){
			if( ! ( status & AM_WAIT_FOR_RELEASE_TOGGLE_BUTTON ) ){
				status |= AM_WAIT_FOR_RELEASE_TOGGLE_BUTTON;
				if( ! ( status & AM_ACTIVATE ) ){
					am_enable( &status );
				} else{
					am_disable( &status );
					if( ! ( status & AM_HEADPHONES_EXIST ) ) status |= AM_HEADPHONES_PLUGGED_OUT; /* プラグが抜かれたことにする */
				}
			}
		} else if( status & AM_WAIT_FOR_RELEASE_TOGGLE_BUTTON ){
			status &= ~AM_WAIT_FOR_RELEASE_TOGGLE_BUTTON;
		}
		
		if( status & AM_ACTIVATE ){
			if( ! ( status & AM_HEADPHONES_EXIST ) ){
				am_mute( &status );
				if( params.preference & AM_PREF_MUTE_NOTIFICATION ) am_notificf( "Sound is currently muted by AutoMute %s", AM_VERSION );
			} else if( status & AM_HEADPHONES_PLUGGED_IN ){
				/* ヘッドホンが検出されてから、実際に出力がヘッドホンに切り替わるまでタイムラグがある */
				sceKernelDelayThread( 200000 );
				am_unmute( &status );
				if( params.preference & AM_PREF_MUTE_NOTIFICATION ) am_notificf_abort();
			}
		} else{
			if( status & AM_HEADPHONES_EXIST ){
				if( params.preference & AM_PREF_AUTO_ENABLE_WHILE_PLUGGED ) am_enable( &status );
			} else if( status & AM_HEADPHONES_PLUGGED_OUT ){
				am_unmute( &status );
			}
		}
	}
	
	/* コールバックスレッドを抹消 */
	if( params.preference & AM_PREF_RESUME_HANDLER ){
		scePowerUnregisterCallback( AM_POWER_CALLBACK_SLOT );
	}
	
	/* 画面描画用スレッドを終了 */
	if( ovmsgIsRunning() ) ovmsgShutdownStart();
	
	return sceKernelExitDeleteThread( 0 );
}

static void am_enable( unsigned int *status )
{
	if( ! ( *status & AM_ACTIVATE ) ){
		*status |= AM_ACTIVATE;
		am_notificf( "AutoMute %s %s.", AM_VERSION, AM_ENABLED );
	}
}

static void am_disable( unsigned int *status )
{
	if( *status & AM_ACTIVATE ){
		*status &= ~AM_ACTIVATE;
		am_notificf( "AutoMute %s %s.", AM_VERSION, AM_DISABLED );
	}
}

static bool am_is_muted( void )
{
	return st_impose_get_param( PSP_IMPOSE_MUTE ) ? true : false;
}

static void am_mute( unsigned int *status )
{
	if( ! am_is_muted() ){
		st_impose_set_param( PSP_IMPOSE_MUTE, 1 );
		*status |= AM_MUTE_DISABLED_BEFORE_AUTOMUTE;
	}
}

static void am_unmute( unsigned int *status )
{
	if( *status & AM_MUTE_DISABLED_BEFORE_AUTOMUTE ){
		st_impose_set_param( PSP_IMPOSE_MUTE, 0 );
	}
	*status &= ~AM_MUTE_DISABLED_BEFORE_AUTOMUTE;
}

static void am_ini( struct am_params *params, const char *path )
{
	IniUID ini;
	char inipath[AM_PATH_MAX];
	PadutilButtonName cvbtn[] = { PADUTIL_BUTTON_NAMES };
	
	/* 設定ファイルのパスを決定 */
	{
		char *delim;
		
		strutilCopy(
			inipath,
			( ( ! path || path[0] == '\0' || strncasecmp( path, "flash", 5 ) == 0 || ! strrchr( path, '/' ) ) ? AM_INI_PATH_DEFAULT : path ),
			sizeof( inipath ) - strlen( AM_INI_FILENAME )
		);
		delim = strrchr( inipath, '/' );
		strcpy( delim + 1, AM_INI_FILENAME );
	}
	
	ini = inimgrNew();
	if( ini < 0 ){
		/* INIファイルの読み込み失敗 */
		return;
	} else{
		char str[255];
		if( inimgrLoad( ini, inipath, NULL, 0, 0, 0 ) == 0 ){
			bool pref;
			
			/* 読み込みに成功したので設定値を初期化 */
			params->startup       = AM_STARTUP_AUTO;
			params->toggleButtons = 0;
			params->preference    = 0;
			
			if( inimgrGetString( ini, "Main", "Startup", str, sizeof( str ) ) > 0 ){
				if     ( strcasecmp( str, "Auto" ) == 0 ){ params->startup = AM_STARTUP_AUTO; }
				else if( strcasecmp( str, "On"   ) == 0 ){ params->startup = AM_STARTUP_ON;   }
				else                                      { params->startup = AM_STARTUP_OFF;  }
			}
			
			if( inimgrGetString( ini, "Main", "ToggleButtons",  str, sizeof( str ) ) > 0 ){
				params->toggleButtons = padutilGetButtonCodeByNames( cvbtn, str, "+", PADUTIL_OPT_IGNORE_SP );
			}
			
			if( inimgrGetBool( ini, "Main",   "StatusNotification",     &pref ) && pref ) params->preference |= AM_PREF_STATUS_NOTIFICATION;
			if( inimgrGetBool( ini, "Main",   "MuteNotification",       &pref ) && pref ) params->preference |= AM_PREF_MUTE_NOTIFICATION;
			if( inimgrGetBool( ini, "Main",   "AutoEnableWhilePlugged", &pref ) && pref ) params->preference |= AM_PREF_AUTO_ENABLE_WHILE_PLUGGED;
			if( inimgrGetBool( ini, "Resume", "AutoDetect",             &pref ) && pref ) params->preference |= AM_PREF_RESUME_HANDLER;
			if( inimgrGetBool( ini, "Resume", "AllowAutoDisable",       &pref ) && pref ) params->preference |= AM_PREF_AUTO_DISABLE_WHEN_RESUME;
		} else{
			/* iniが読めなかったのでデフォルト値をセット */
			params->startup       = AM_INI_DEFAULT_STARTUP;
			params->toggleButtons = AM_INI_DEFAULT_TOGGLE_BUTTONS;
			params->preference    = AM_INI_DEFAULT_PREFERENCE;
			
			inimgrSetString(
				ini, "Main", "Startup",
				params->startup == AM_STARTUP_AUTO ? "Auto" :
				params->startup == AM_STARTUP_ON   ? "On"   :
				"Off"
			);
			inimgrSetString( ini, "Main", "ToggleButtons", padutilGetButtonNamesByCode( cvbtn, params->toggleButtons, "+", PADUTIL_OPT_TOKEN_SP, str, sizeof( str ) ) );
			
			inimgrSetBool( ini, "Main",   "StatusNotification",     params->preference & AM_PREF_STATUS_NOTIFICATION       ? true : false );
			inimgrSetBool( ini, "Main",   "MuteNotification",       params->preference & AM_PREF_MUTE_NOTIFICATION         ? true : false );
			inimgrSetBool( ini, "Main",   "AutoEnableWhilePlugged", params->preference & AM_PREF_AUTO_ENABLE_WHILE_PLUGGED ? true : false );
			inimgrSetBool( ini, "Resume", "AutoDetect",             params->preference & AM_PREF_RESUME_HANDLER            ? true : false );
			inimgrSetBool( ini, "Resume", "AllowAutoDisable",       params->preference & AM_PREF_AUTO_DISABLE_WHEN_RESUME  ? true : false );
			inimgrSave( ini, inipath, NULL, 0, 0, 0 );
		}
		inimgrDestroy( ini );
	}
}

static bool am_notificf( const char *format, ... )
{
	va_list ap;
	bool ret;
	
	va_start( ap, format );
	
	ovmsgPrintIntrStart();
	ovmsgWaitForReady();
	ret = ovmsgVprintf( format, ap );
	
	va_end( ap );
	
	return ret;
}

static void am_notificf_abort( void )
{
	ovmsgPrintIntrStart();
	ovmsgWaitForReady();
}

/*-----------------------------------------------
	コールバック
-----------------------------------------------*/
static int am_power_callback( int unk, int pwinfo, void *argp )
{
	st_power_status = pwinfo;
	return 0;
}

/*-----------------------------------------------
	エントリポイント
-----------------------------------------------*/
int module_start( SceSize arglen, void *argp )
{
	SceUID thid = sceKernelCreateThread( "AutoMute", am_main, 32, 0xC00, 0, 0 );
	if( thid > 0 ) sceKernelStartThread( thid, arglen, argp );
	
	return 0;
}

int module_stop( SceSize arglen, void *argp )
{
	gRunning = false;
	return 0;
}
