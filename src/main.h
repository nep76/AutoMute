/*
	Auto Mute
*/

#define GLOBAL_VARIABLES_DEFINE
#include "global.h"
#undef GLOBAL_VARIABLES_DEFINE

#include <pspsdk.h>
#include <pspctrl.h>
#include <psphprm.h>
#include "utils/confmgr.h"
#include "psp/confmgr/cmpspbtn.h"
#include "notice.h"

/*-----------------------------------------------
	固定設定値
-----------------------------------------------*/
#define AM_CONF_FILE_FULLPATH "ms0:/seplugins/automute_conf.txt"

/*-----------------------------------------------
	メッセージ
-----------------------------------------------*/
#define AM_ENABLED_BOOT  "%s detected headphones."
#define AM_DISABLED_BOOT ""
#define AM_ENABLED       "%s Enabled."
#define AM_DISABLED      "%s Disabled."
#define AM_MUTE          "Sound is currently muted by %s."

/*-----------------------------------------------
	ボタン
-----------------------------------------------*/
#define AM_DEFAULT_TOGGLE_BUTTONS ( PSP_CTRL_RTRIGGER | PSP_CTRL_LTRIGGER | PSP_CTRL_NOTE )

/*-----------------------------------------------
	宣言
-----------------------------------------------*/
struct am_avail_buttons {
	unsigned int button;
	char *label;
};

/*-----------------------------------------------
	プロトタイプの無いAudio切り替え関数
	
	どちらもpspSdkSetK1( 0 )実行後でないと動作が不安定。
	
	たまにK1レジスタをいじらなくても動くときがある。
	ほかの関数との兼ね合いもあるようで、これはたまたま？
-----------------------------------------------*/

/* 全オーディオ入出力を有効にする? */
int sceSysregAudioIoEnable( void );

/* 全オーディオ入出力を無効にする? */
int sceSysregAudioIoDisable( void );


/*-----------------------------------------------
	関数プロトタイプ
-----------------------------------------------*/
int amMain( SceSize arglen, void *argp );
void amNoticef( char *format, ... );

int module_start( SceSize arglen, void *argp );
int module_stop( SceSize arglen, void *argp );
