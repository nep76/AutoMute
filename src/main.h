/*
	Auto Mute
*/

#define GLOBAL_VARIABLES_DEFINE
#include "global.h"
#undef GLOBAL_VARIABLES_DEFINE

#include <pspsdk.h>
#include <pspctrl.h>
#include <psphprm.h>
#include <psppower.h>
#include "utils/inimgr.h"
#include "psp/ctrlpad.h"
#include "notice.h"

/*-----------------------------------------------
	固定設定値
-----------------------------------------------*/
#define AM_CONF_FILE_FULLPATH  "ms0:/seplugins/automute_conf.txt"

/*
	Power Callbackを登録するスロット、0-15の数値。
	-1をセットすると、自動割り当てを行うようなことがドキュメントにはあるが、
	実際には0x80000004のエラーが返ってくる。
	
	すでにゲームがこのスロットにコールバック関数を登録している場合は、
	どうなるのかわからない。
	気休めで被りにくいように中途半端なスロットを使ってみる。
*/
#define AM_POWER_CALLBACK_SLOT 9

/*-----------------------------------------------
	メッセージ
-----------------------------------------------*/
#define AM_MAIN_ENABLED   "%s enabled."
#define AM_MAIN_DISABLED  "%s disabled."
#define AM_ENABLE_BOOT    "%s detected headphones."
#define AM_DISABLE_BOOT   ""
#define AM_ENABLE_RESUME  AM_MAIN_ENABLED
#define AM_DISABLE_RESUME AM_MAIN_DISABLED
#define AM_MUTE           "Sound is currently muted by %s."

/*-----------------------------------------------
	ボタン
-----------------------------------------------*/
#define AM_INIDEF_MAIN_AUTODETECT                          true
#define AM_INIDEF_MAIN_TOGGLEBUTTONS                       ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_NOTE )
#define AM_INIDEF_MAIN_TOGGLEBUTTONSNAME                   "LTRIGGER + RTRIGGER + NOTE"
#define AM_INIDEF_MAIN_SHOWSTATUSMESSAGE                   true
#define AM_INIDEF_MAIN_SHOWMUTEWARNING                     true
#define AM_INIDEF_RESUME_AUTODETECT                        true
#define AM_INIDEF_RESUME_ALLOWAUTODISABLE                  false

/*-----------------------------------------------
	宣言
-----------------------------------------------*/
struct am_avail_buttons {
	unsigned int button;
	char *label;
};

struct am_params {
	struct {
		bool autoDetect;
		unsigned int toggleButtons;
		bool showStatusMessage;
		bool showMuteWarning;
	} main;
	struct {
		bool autoDetect;
		bool allowAutoDisable;
	} resume;
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
void amNoticef( char *format, ... );
void amMuteEnable( void );
void amMuteDisable( void );
int amWatchdogPower( SceSize arglen, void *argp );
int amPowerCb( int unk, int pwinfo, void *argp );
int amMain( SceSize arglen, void *argp );

int module_start( SceSize arglen, void *argp );
int module_stop( SceSize arglen, void *argp );
