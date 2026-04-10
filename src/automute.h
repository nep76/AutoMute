/*=========================================================
	
	AutoMute v2
	
=========================================================*/
#ifndef AUTOMUTE_H
#define AUTOMUTE_H

#include <pspkernel.h>
#include <pspctrl.h>
#include <psphprm.h>
#include <stdbool.h>

/*=========================================================
	マクロ
=========================================================*/
#define AM_VERSION  "2.0.1"
#define AM_ENABLED  "Enabled"
#define AM_DISABLED "Disabled"

#define AM_INI_PATH_DEFAULT           "ms0:/seplugins/"
#define AM_INI_FILENAME               "automute.ini"
#define AM_INI_DEFAULT_STARTUP        AM_STARTUP_AUTO
#define AM_INI_DEFAULT_TOGGLE_BUTTONS ( PSP_CTRL_LTRIGGER | PSP_CTRL_RTRIGGER | PSP_CTRL_NOTE )
#define AM_INI_DEFAULT_PREFERENCE     ( AM_PREF_STATUS_NOTIFICATION )

#define AM_POWER_CALLBACK_SLOT 9
#define AM_PATH_MAX            255

#ifdef AM_FIRST_INCLUDE
#define GLOBAL
#define INIT( x ) = x
#else
#define GLOBAL extern
#define INIT( x )
#endif

/*=========================================================
	グローバル変数
=========================================================*/
GLOBAL bool gRunning INIT( true );

#endif
