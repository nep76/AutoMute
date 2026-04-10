/*
	グローバルヘッダー
*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include <pspkernel.h>
#include <stdbool.h>
#include <stdarg.h>
#include "error.h"

#define AM_VERSION "AutoMute v1.0.0"

#ifdef GLOBAL_VARIABLES_DEFINE
#define GLOBAL
#define INIT_VALUE( x ) = ( x )
#else
#define GLOBAL extern
#define INIT_VALUE( x )
#endif

#define ARRAY_NUM( x )  sizeof( x ) / sizeof( x[0] )

/* グローバル変数 */

#undef GLOBAL
#undef VALUE

#endif
