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
#define AM_INI_FILE_FULLPATH  "ms0:/seplugins/automute.ini"

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
#define AM_MSG_ENABLED                "%s enabled."
#define AM_MSG_DISABLED               "%s disabled."
#define AM_MSG_MUTE                   "Sound is currently muted by %s."
#define AM_MSG_STARTUP_ON             "Starting %s."
#define AM_MSG_STARTUP_OFF            ""
#define AM_MSG_STARTUP_AUTO_DETECT    "%s detected headphones."
#define AM_MSG_STARTUP_AUTO_NOTDETECT ""
#define AM_MSG_RESUME_DETECT          AM_MSG_STARTUP_AUTO_DETECT
#define AM_MSG_RESUME_NOTDETECT       AM_MSG_DISABLED

/*-----------------------------------------------
	ボタン
-----------------------------------------------*/
#define AM_INIDEF_MAIN_STARTUP            "AUTO"
#define AM_INIDEF_MAIN_TOGGLEBUTTONS      ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_NOTE )
#define AM_INIDEF_MAIN_SHOWSTATUSMESSAGE  true
#define AM_INIDEF_MAIN_SHOWMUTEWARNING    true
#define AM_INIDEF_RESUME_AUTODETECT       true
#define AM_INIDEF_RESUME_ALLOWAUTODISABLE false

/*-----------------------------------------------
	宣言
-----------------------------------------------*/
enum am_startup_mode {
	AM_STARTUP_OFF,
	AM_STARTUP_ON,
	AM_STARTUP_AUTO
};

struct am_avail_buttons {
	unsigned int button;
	char *label;
};

struct am_params {
	struct {
		enum am_startup_mode startup;
		unsigned int         toggleButtons;
		bool                 showStatusMessage;
		bool                 showMuteWarning;
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
bool amIsMuted( void );
void amMuteEnable( void );
void amMuteDisable( void );
void amNoticef( char *format, ... );
int amWatchdogPower( SceSize arglen, void *argp );
int amPowerCb( int unk, int pwinfo, void *argp );
int amMain( SceSize arglen, void *argp );

int module_start( SceSize arglen, void *argp );
int module_stop( SceSize arglen, void *argp );
